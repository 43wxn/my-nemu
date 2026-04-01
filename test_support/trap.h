#ifndef LOCAL_TRAP_H
#define LOCAL_TRAP_H

#include <stdio.h>

#define check(cond)                                                                  \
  do {                                                                               \
    if (!(cond)) {                                                                   \
      printf("CHECK_FAIL:%s:%d: %s\n", __FILE__, __LINE__, #cond);                \
      return 1;                                                                      \
    }                                                                                \
  } while (0)

#endif
