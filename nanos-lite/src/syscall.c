#include <common.h>
#include <am.h>
#include <fs.h>
#include "syscall.h"

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;  // syscall number
  a[1] = c->GPR2;  // arg0
  a[2] = c->GPR3;  // arg1
  a[3] = c->GPR4;  // arg2

  switch (a[0]) {
    case SYS_yield:
      c->GPRx = 0;
      yield();
      break;

    case SYS_exit:
      halt(a[1]);
      break;

    case SYS_open: {
      Log("SYS_open(path=%p, flags=%d, mode=%d)", (void *)a[1], a[2], a[3]);
      intptr_t ret = fs_open((const char *)a[1], a[2], a[3]);
      Log("SYS_open -> %d", ret);
      c->GPRx = ret;
      break;
    }

    case SYS_close: {
      Log("SYS_close(fd=%d)", a[1]);
      intptr_t ret = fs_close(a[1]);
      Log("SYS_close -> %d", ret);
      c->GPRx = ret;
      break;
    }

    case SYS_lseek: {
      Log("SYS_lseek(fd=%d, off=%d, whence=%d)", a[1], (off_t)a[2], a[3]);
      intptr_t ret = fs_lseek(a[1], (off_t)a[2], a[3]);
      Log("SYS_lseek -> %d", ret);
      c->GPRx = ret;
      break;
    }

    case SYS_read: {
      Log("SYS_read(fd=%d, buf=%p, len=%d)", a[1], (void *)a[2], a[3]);
      intptr_t ret = fs_read(a[1], (void *)a[2], a[3]);
      Log("SYS_read -> %d", ret);
      c->GPRx = ret;
      break;
    }

    case SYS_write: {
      Log("SYS_write(fd=%d, buf=%p, len=%d)", a[1], (void *)a[2], a[3]);
      intptr_t ret = fs_write(a[1], (const void *)a[2], a[3]);
      Log("SYS_write -> %d", ret);
      c->GPRx = ret;
      break;
    }

    case SYS_brk:
      c->GPRx = 0;
      break;

    default:
      panic("Unhandled syscall ID = %d", a[0]);
  }
}