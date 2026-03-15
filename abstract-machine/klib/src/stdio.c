#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>
#include <limits.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

static void emit_char(char **out, char *end, int *cnt, char ch) {
  if (*out < end) {
    *(*out)++ = ch;
  }
  (*cnt)++;
}

static void emit_str(char **out, char *end, int *cnt, const char *s) {
  if (s == NULL) s = "(null)";
  while (*s) {
    emit_char(out, end, cnt, *s++);
  }
}

static void emit_uint_base(char **out, char *end, int *cnt, unsigned int num,
                           unsigned int base, int uppercase) {
  char buf[32];
  int i = 0;

  if (num == 0) {
    emit_char(out, end, cnt, '0');
    return;
  }

  while (num > 0) {
    unsigned int digit = num % base;
    if (digit < 10) {
      buf[i++] = '0' + digit;
    } else {
      buf[i++] = (uppercase ? 'A' : 'a') + (digit - 10);
    }
    num /= base;
  }

  while (i > 0) {
    emit_char(out, end, cnt, buf[--i]);
  }
}

static void emit_int(char **out, char *end, int *cnt, int num) {
  unsigned int u;
  if (num < 0) {
    emit_char(out, end, cnt, '-');
    // 避免 INT_MIN 取反溢出
    u = (unsigned int)(-(num + 1)) + 1;
  } else {
    u = (unsigned int)num;
  }
  emit_uint_base(out, end, cnt, u, 10, 0);
}

static int format_to_buffer(char *out, size_t n, const char *fmt, va_list ap) {
  char *p = out;
  char *end;
  int cnt = 0;

  if (n == 0) {
    // 不写任何字符，但仍统计理论输出长度
    p = NULL;
    end = NULL;
  } else {
    end = out + n - 1;  // 留一个给 '\0'
  }

  while (*fmt) {
    if (*fmt != '%') {
      if (n != 0) emit_char(&p, end, &cnt, *fmt);
      else cnt++;
      fmt++;
      continue;
    }

    fmt++;  // 跳过 '%'

    if (*fmt == '\0') break;

    switch (*fmt) {
      case 'd':
      case 'i': {
        int v = va_arg(ap, int);
        if (n != 0) emit_int(&p, end, &cnt, v);
        else {
          char tmp[32];
          char *tp = tmp;
          char *tend = tmp + sizeof(tmp) - 1;
          emit_int(&tp, tend, &cnt, v);
        }
        break;
      }

      case 'u': {
        unsigned int v = va_arg(ap, unsigned int);
        if (n != 0) emit_uint_base(&p, end, &cnt, v, 10, 0);
        else {
          char tmp[32];
          char *tp = tmp;
          char *tend = tmp + sizeof(tmp) - 1;
          emit_uint_base(&tp, tend, &cnt, v, 10, 0);
        }
        break;
      }

      case 'x': {
        unsigned int v = va_arg(ap, unsigned int);
        if (n != 0) emit_uint_base(&p, end, &cnt, v, 16, 0);
        else {
          char tmp[32];
          char *tp = tmp;
          char *tend = tmp + sizeof(tmp) - 1;
          emit_uint_base(&tp, tend, &cnt, v, 16, 0);
        }
        break;
      }

      case 'X': {
        unsigned int v = va_arg(ap, unsigned int);
        if (n != 0) emit_uint_base(&p, end, &cnt, v, 16, 1);
        else {
          char tmp[32];
          char *tp = tmp;
          char *tend = tmp + sizeof(tmp) - 1;
          emit_uint_base(&tp, tend, &cnt, v, 16, 1);
        }
        break;
      }

      case 'p': {
        uintptr_t v = (uintptr_t)va_arg(ap, void *);
        if (n != 0) {
          emit_char(&p, end, &cnt, '0');
          emit_char(&p, end, &cnt, 'x');
          emit_uint_base(&p, end, &cnt, (unsigned int)v, 16, 0);
        } else {
          cnt += 2;
          char tmp[32];
          char *tp = tmp;
          char *tend = tmp + sizeof(tmp) - 1;
          emit_uint_base(&tp, tend, &cnt, (unsigned int)v, 16, 0);
        }
        break;
      }

      case 's': {
        const char *s = va_arg(ap, const char *);
        if (n != 0) emit_str(&p, end, &cnt, s);
        else {
          if (s == NULL) s = "(null)";
          while (*s) { cnt++; s++; }
        }
        break;
      }

      case 'c': {
        char ch = (char)va_arg(ap, int);
        if (n != 0) emit_char(&p, end, &cnt, ch);
        else cnt++;
        break;
      }

      case '%': {
        if (n != 0) emit_char(&p, end, &cnt, '%');
        else cnt++;
        break;
      }

      default: {
        // 未知格式，按原样输出
        if (n != 0) {
          emit_char(&p, end, &cnt, '%');
          emit_char(&p, end, &cnt, *fmt);
        } else {
          cnt += 2;
        }
        break;
      }
    }

    fmt++;
  }

  if (n != 0) {
    *p = '\0';
  }

  return cnt;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  va_list aq;
  va_copy(aq, ap);
  int ret = format_to_buffer(out, n, fmt, aq);
  va_end(aq);
  return ret;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  // 这里给一个很大的上限，避免无限写爆栈
  va_list aq;
  va_copy(aq, ap);
  int ret = format_to_buffer(out, (size_t)-1, fmt, aq);
  va_end(aq);
  return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vsnprintf(out, n, fmt, ap);
  va_end(ap);
  return ret;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int ret = vsprintf(out, fmt, ap);
  va_end(ap);
  return ret;
}

int printf(const char *fmt, ...) {
  // 这张 logo 很大，1024 不够；给足空间
  char buf[4096];
  va_list ap;
  va_start(ap, fmt);
  int ret = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  for (int i = 0; buf[i] != '\0'; i++) {
    putch(buf[i]);
  }
  return ret;
}

#endif