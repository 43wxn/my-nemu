#include <common.h>
#include <am.h>
#include "syscall.h"

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;

  printf("do_syscall: no=%ld arg0=%ld arg1=%ld arg2=%ld\n",
      a[0], a[1], a[2], a[3]);

  switch (a[0]) {
    case SYS_yield:
      c->GPRx = 0;
      yield();
      break;

    case SYS_exit:
      halt(a[1]);
      break;

    default:
      panic("Unhandled syscall ID = %d", a[0]);
  }
}