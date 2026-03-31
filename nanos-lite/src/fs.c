#include <fs.h>
#include <common.h>

typedef size_t (*ReadFn)(void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn)(const void *buf, size_t offset, size_t len);

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
size_t serial_write(const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
  size_t open_offset;
} Finfo;

enum { FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB };

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin",  0, 0, invalid_read,  invalid_write, 0},
  [FD_STDOUT] = {"stdout", 0, 0, invalid_read,  serial_write,  0},
  [FD_STDERR] = {"stderr", 0, 0, invalid_read,  serial_write,  0},
#include "files.h"
};

#define NR_FILES (int)(sizeof(file_table) / sizeof(file_table[0]))

void init_fs() {
  // 这一步先不处理 /dev/fb 大小，也能先把基础文件系统跑起来
}

int fs_open(const char *pathname, int flags, int mode) {
  for (int i = 0; i < NR_FILES; i++) {
    if (strcmp(pathname, file_table[i].name) == 0) {
      file_table[i].open_offset = 0;
      return i;
    }
  }

  panic("fs_open: file not found: %s", pathname);
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

  size_t rest = f->size - f->open_offset;
  size_t real_len = len < rest ? len : rest;

  ramdisk_read(buf, f->disk_offset + f->open_offset, real_len);
  f->open_offset += real_len;
  return real_len;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  assert(fd >= 0 && fd < NR_FILES);

  Finfo *f = &file_table[fd];

  // stdout/stderr 直接走串口，不维护 open_offset
  if (fd == FD_STDOUT || fd == FD_STDERR) {
    return serial_write(buf, 0, len);
  }

  if (f->write != NULL && f->write != invalid_write) {
    size_t ret = f->write(buf, f->open_offset, len);
    f->open_offset += ret;
    return ret;
  }

  // 普通文件写回 ramdisk，不能越界
  if (f->open_offset >= f->size) {
    return 0;
  }

  size_t rest = f->size - f->open_offset;
  size_t real_len = len < rest ? len : rest;

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