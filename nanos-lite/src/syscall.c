#include <common.h>
#include <am.h>
#include "syscall.h"

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;  // syscall number
  a[1] = c->GPR2;  // arg0
  a[2] = c->GPR3;  // arg1
  a[3] = c->GPR4;  // arg2

  // 调试时可打开；正式跑测试建议关掉，避免输出太多干扰
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

      if (fd == 1 || fd == 2) {  // stdout or stderr
        for (size_t i = 0; i < len; i++) {
          putch(buf[i]);
        }
        // write 成功时返回实际写入的字节数
        c->GPRx = len;
      } else {
        // 当前 Nanos-lite 只支持标准输出/错误
        c->GPRx = -1;
      }
      break;
    }

    case SYS_brk:
      // 在 Nanos-lite 当前阶段，一般不真正维护复杂堆空间，
      // 只需告诉用户层“设置成功”即可。
      // 用户层 _sbrk() 自己记录 program break。
      c->GPRx = 0;
      break;

    default:
      panic("Unhandled syscall ID = %d", a[0]);
  }
}