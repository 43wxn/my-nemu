#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include "proc.h"
#include "syscall.h"

void do_syscall(Context *c);

static Context *do_event(Event e, Context *c) {
  switch (e.event) {
    case EVENT_YIELD:
      yield();
      break;

    case EVENT_SYSCALL:
      do_syscall(c);
      c->mepc += 4;   // RISC-V: 跳过 ecall
      break;

    default:
      panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}