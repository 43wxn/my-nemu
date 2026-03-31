#include <fs.h>
#include <common.h>
#include <sys/stat.h>
#include <string.h>
typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
size_t serial_write(const void *buf, size_t offset, size_t len);
size_t events_read(void *buf, size_t offset, size_t len);
size_t dispinfo_read(void *buf, size_t offset, size_t len);
size_t fb_write(const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;          // 文件名
  size_t size;         // 文件大小
  size_t disk_offset;  // 文件在 ramdisk 中的偏移
  ReadFn read;         // 读函数指针
  WriteFn write;       // 写函数指针
  size_t open_offset;  // 当前打开偏移
} Finfo;

enum {
  FD_STDIN,
  FD_STDOUT,
  FD_STDERR,
  FD_EVENTS,
  FD_DISPINFO,
  FD_FB
};

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("invalid_read");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("invalid_write");
  return 0;
}

/* 所有文件的信息 */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]    = {"stdin",          0, 0, invalid_read,  invalid_write, 0},
  [FD_STDOUT]   = {"stdout",         0, 0, invalid_read,  serial_write,  0},
  [FD_STDERR]   = {"stderr",         0, 0, invalid_read,  serial_write,  0},
  [FD_EVENTS]   = {"/dev/events",    0, 0, events_read,   invalid_write, 0},
  [FD_DISPINFO] = {"/proc/dispinfo", 0, 0, dispinfo_read, invalid_write, 0},
  [FD_FB]       = {"/dev/fb",        0, 0, invalid_read,  fb_write,      0},
#include "files.h"
};

#define NR_FILES (int)(sizeof(file_table) / sizeof(file_table[0]))

void init_fs() {
  // 这一步先不用处理 /dev/fb 的 size
}

int fs_open(const char *pathname, int flags, int mode) {
  for (int i = 0; i < NR_FILES; i++) {
    if (strcmp(pathname, file_table[i].name) == 0) {
      file_table[i].open_offset = 0;
      return i;
    }
  }
  panic("fs_open: file not found = %s", pathname);
  return -1;
}

size_t fs_read(int fd, void *buf, size_t len) {
  assert(fd >= 0 && fd < NR_FILES);
  Finfo *f = &file_table[fd];

  if (f->read != NULL && f->read != invalid_read) {
    size_t ret = f->read(buf, f->open_offset, len);
    f->open_offset += ret;
    return ret;
  }

  if (f->open_offset >= f->size) {
    return 0;
  }

  size_t remain = f->size - f->open_offset;
  size_t real_len = len < remain ? len : remain;

  ramdisk_read(buf, f->disk_offset + f->open_offset, real_len);
  f->open_offset += real_len;
  return real_len;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  Log("fs_write(fd=%d, buf=%p, len=%d, off=%zu)",
      fd, buf, len, file_table[fd].open_offset);

  assert(fd >= 0 && fd < NR_FILES);
  Finfo *f = &file_table[fd];

  // 如果有专门的写函数，使用它
  if (f->write != NULL && f->write != invalid_write) {
    size_t ret = f->write(buf, f->open_offset, len);
    f->open_offset += ret;
    return ret;
  }

  // 普通文件：检查边界
  // 注意：如果写入超出文件大小，应该允许扩展吗？
  // 当前实现不允许超出，但 file-test 需要写入相同大小的数据
  if (f->open_offset >= f->size) {
    Log("fs_write: offset %zu >= size %zu, returning 0", f->open_offset, f->size);
    return 0;
  }

  size_t remain = f->size - f->open_offset;
  size_t real_len = len < remain ? len : remain;
  
  if (real_len == 0) {
    Log("fs_write: nothing to write");
    return 0;
  }

  Log("fs_write: writing %zu bytes to disk_offset %zu", real_len, f->disk_offset + f->open_offset);
  ramdisk_write(buf, f->disk_offset + f->open_offset, real_len);
  f->open_offset += real_len;
  return real_len;
}

size_t fs_lseek(int fd, off_t offset, int whence) {
  assert(fd >= 0 && fd < NR_FILES);
  Finfo *f = &file_table[fd];

  off_t new_offset = 0;
  switch (whence) {
    case SEEK_SET:
      new_offset = offset;
      break;
    case SEEK_CUR:
      new_offset = (off_t)f->open_offset + offset;
      break;
    case SEEK_END:
      new_offset = (off_t)f->size + offset;
      break;
    default:
      panic("fs_lseek: invalid whence = %d", whence);
  }

  assert(new_offset >= 0 && (size_t)new_offset <= f->size);
  f->open_offset = (size_t)new_offset;
  return f->open_offset;
}

int fs_close(int fd) {
  assert(fd >= 0 && fd < NR_FILES);
  return 0;
}
int fs_fstat(int fd, struct stat *buf) {
  assert(fd >= 0 && fd < NR_FILES);
  assert(buf != NULL);

  memset(buf, 0, sizeof(*buf));

  // 标准输入输出错误，以及设备文件先当字符设备
  if (fd == FD_STDIN || fd == FD_STDOUT || fd == FD_STDERR ||
      fd == FD_EVENTS || fd == FD_DISPINFO || fd == FD_FB) {
    buf->st_mode = S_IFCHR;
  } else {
    buf->st_mode = S_IFREG;
    buf->st_size = file_table[fd].size;
  }

  buf->st_blksize = 4096;
  return 0;
}