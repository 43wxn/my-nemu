#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

/* 随机数生成器状态 */
static unsigned long int next = 1;

/* 简单的内存分配器（固定大小内存池） */
#define HEAP_POOL_SIZE     (128 * 1024)  // 128KB 堆大小
#define BLOCK_SIZE    16            // 最小分配单元
#define BLOCK_COUNT   (HEAP_POOL_SIZE / BLOCK_SIZE)

// 改名为 heap_pool，避免与 am.h 中的 heap 冲突
static char heap_pool[HEAP_POOL_SIZE];
static char block_used[BLOCK_COUNT];  // 0 = 空闲, 1 = 已用

/* 初始化内存分配器 */
static void init_malloc() {
  static int initialized = 0;
  if (!initialized) {
    for (int i = 0; i < BLOCK_COUNT; i++) {
      block_used[i] = 0;
    }
    initialized = 1;
  }
}

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

long labs(long x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  int sign = 1;
  
  // 跳过空白字符
  while (*nptr == ' ' || *nptr == '\t' || *nptr == '\n') {
    nptr++;
  }
  
  // 处理正负号
  if (*nptr == '-') {
    sign = -1;
    nptr++;
  } else if (*nptr == '+') {
    nptr++;
  }
  
  // 转换数字
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + (*nptr - '0');
    nptr++;
  }
  
  return sign * x;
}

long atol(const char* nptr) {
  return (long)atoi(nptr);
}

void *malloc(size_t size) {
  // On native, malloc() will be called during initializaion of C runtime.
  // Therefore do not call panic() here, else it will yield a dead recursion:
  //   panic() -> putchar() -> (glibc) -> malloc() -> panic()
#if defined(__ISA_NATIVE__) && !defined(__NATIVE_USE_KLIB__)
  return NULL;
#else
  init_malloc();
  
  // 计算需要的块数
  int needed_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
  if (needed_blocks == 0) needed_blocks = 1;
  
  // 寻找连续的空闲块
  int start = -1;
  int count = 0;
  
  for (int i = 0; i < BLOCK_COUNT; i++) {
    if (block_used[i] == 0) {
      if (start == -1) start = i;
      count++;
      if (count >= needed_blocks) {
        // 找到足够的连续空间
        for (int j = start; j < start + needed_blocks; j++) {
          block_used[j] = 1;
        }
        return (void*)(heap_pool + start * BLOCK_SIZE);
      }
    } else {
      start = -1;
      count = 0;
    }
  }
  
  // 内存不足
  return NULL;
#endif
}

void free(void *ptr) {
#if !(defined(__ISA_NATIVE__) && !defined(__NATIVE_USE_KLIB__))
  if (ptr == NULL) return;
  
  init_malloc();
  
  // 计算对应的块号
  int block_index = ((char*)ptr - heap_pool) / BLOCK_SIZE;
  int block_offset = ((char*)ptr - heap_pool) % BLOCK_SIZE;
  
  // 检查指针是否有效
  if (block_index >= 0 && block_index < BLOCK_COUNT && block_offset == 0) {
    // 释放这块内存（简单实现：只释放第一块）
    // 注意：这个简单实现假设用户free时传入的是malloc返回的原始指针
    if (block_used[block_index] == 1) {
      block_used[block_index] = 0;
    }
  }
#endif
}

void *calloc(size_t nmemb, size_t size) {
  size_t total = nmemb * size;
  void *ptr = malloc(total);
  if (ptr) {
    char *p = (char*)ptr;
    for (size_t i = 0; i < total; i++) {
      p[i] = 0;
    }
  }
  return ptr;
}

void *realloc(void *ptr, size_t size) {
  if (ptr == NULL) return malloc(size);
  if (size == 0) {
    free(ptr);
    return NULL;
  }
  
  // 分配新内存
  void *new_ptr = malloc(size);
  if (new_ptr) {
    // 复制旧数据（假设旧大小至少为BLOCK_SIZE）
    char *old = (char*)ptr;
    char *new = (char*)new_ptr;
    for (size_t i = 0; i < BLOCK_SIZE; i++) {
      new[i] = old[i];
    }
    free(ptr);
  }
  return new_ptr;
}

/* 字符串转浮点数（简单实现） */
double atof(const char* nptr) {
  double x = 0.0;
  double fraction = 0.0;
  int sign = 1;
  int fraction_digits = 0;
  
  // 跳过空白
  while (*nptr == ' ') nptr++;
  
  // 处理符号
  if (*nptr == '-') {
    sign = -1;
    nptr++;
  } else if (*nptr == '+') {
    nptr++;
  }
  
  // 整数部分
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + (*nptr - '0');
    nptr++;
  }
  
  // 小数部分
  if (*nptr == '.') {
    nptr++;
    while (*nptr >= '0' && *nptr <= '9') {
      fraction = fraction * 10 + (*nptr - '0');
      fraction_digits++;
      nptr++;
    }
  }
  
  // 合并整数和小数部分
  while (fraction_digits > 0) {
    fraction /= 10.0;
    fraction_digits--;
  }
  
  return sign * (x + fraction);
}

/* 快速退出 */
void _Exit(int status) {
  halt(status);
}

void _exit(int status) {
  halt(status);
}

#endif