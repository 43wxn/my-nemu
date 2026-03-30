#include <common.h>
#include <am.h>
#include "syscall.h"

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;  // syscall number
  a[1] = c->GPR2;  // arg0
  a[2] = c->GPR3;  // arg1
  a[3] = c->GPR4;  // arg2

  // 调试时打开，平时建议注释掉
  // printf("do_syscall: no=%ld arg0=%ld arg1=%ld arg2=%ld\n",
  //     a[0], a[1], a[2], a[3]);

  switch (a[0]) {
    case SYS_yield:
      c->GPRx = 0;
      yield();
      break;

    case SYS_exit:
      halt(a[1]);
      break;

    case SYS_write: {
      int fd = a[1];
      const char *buf = (const char *)a[2];
      size_t len = a[3];

      if (fd == 1 || fd == 2) {
        for (size_t i = 0; i < len; i++) {
          putch(buf[i]);
        }
        c->GPRx = len;
      } else {
        c->GPRx = -1;
      }
      break;
    }

    case SYS_brk:
      c->GPRx = 0;
      break;

    default:
      panic("Unhandled syscall ID = %d", a[0]);
  }
}