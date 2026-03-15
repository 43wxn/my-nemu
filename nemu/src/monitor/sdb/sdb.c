/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You may use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <memory/vaddr.h>
#include "sdb.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* sdb 命令需要调用 expr() */
word_t expr(char *e, bool *success);

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

/* 添加iringbuf显示命令 */
#ifdef CONFIG_IRINGBUF
extern void iringbuf_display();
static int cmd_ir(char *args) {
  iringbuf_display();
  return 0;
}
#endif

/* si [N] - 单步执行 N 条指令, 缺省为 1 */
static int cmd_si(char *args) {
  int n = 1;
  if (args != NULL) {
    n = atoi(args);
    if (n <= 0) n = 1;
  }
  cpu_exec(n);
  return 0;
}

/* info r / info w */
static int cmd_info(char *args) {
  if (args == NULL) {
    printf("Usage: info r | info w\n");
    return 0;
  }

  if (strcmp(args, "r") == 0) {
    isa_reg_display();
    return 0;
  }

  if (strcmp(args, "w") == 0) {
    display_watchpoints();
    return 0;
  }

  printf("Unknown info subcommand '%s'\n", args);
  return 0;
}

/* p EXPR - 表达式求值 */
static int cmd_p(char *args) {
  if (args == NULL) {
    printf("Usage: p EXPR\n");
    return 0;
  }

  bool success = true;
  word_t val = expr(args, &success);

  if (success) {
    printf("%lu (0x%lx)\n", (unsigned long)val, (unsigned long)val);
  } else {
    printf("Bad expression: %s\n", args);
  }

  return 0;
}

/* x N EXPR - 扫描内存 */
static int cmd_x(char *args) {
  if (args == NULL) {
    printf("Usage: x N EXPR\n");
    return 0;
  }

  char *n_str = strtok(args, " ");
  char *expr_str = strtok(NULL, "");

  if (n_str == NULL || expr_str == NULL) {
    printf("Usage: x N EXPR\n");
    return 0;
  }

  int n = atoi(n_str);
  if (n <= 0) {
    printf("N should be positive\n");
    return 0;
  }

  bool success = true;
  vaddr_t addr = expr(expr_str, &success);
  if (!success) {
    printf("Bad expression: %s\n", expr_str);
    return 0;
  }

  for (int i = 0; i < n; i++) {
    word_t data = vaddr_read(addr + i * 4, 4);
    printf("0x%08lx: 0x%08lx\n",
        (unsigned long)(addr + i * 4),
        (unsigned long)data);
  }

  return 0;
}

/* w EXPR - 设置监视点 */
static int cmd_w(char *args) {
  if (args == NULL) {
    printf("Usage: w EXPR\n");
    return 0;
  }

  add_watchpoint(args);
  return 0;
}

/* d N - 删除监视点 */
static int cmd_d(char *args) {
  if (args == NULL) {
    printf("Usage: d N\n");
    return 0;
  }

  int no = atoi(args);
  delete_watchpoint(no);
  return 0;
}

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c",    "Continue the execution of the program", cmd_c },
  { "q",    "Exit NEMU", cmd_q },

  /* 添加的指令 */
  { "si",   "Step instruction execution: si [N]", cmd_si },
  { "info", "Print program status: info r | info w", cmd_info },
  { "p",    "Evaluate expression: p EXPR", cmd_p },
  { "x",    "Examine memory: x N EXPR", cmd_x },
  { "w",    "Set a watchpoint: w EXPR", cmd_w },
  { "d",    "Delete a watchpoint: d N", cmd_d },
  
#ifdef CONFIG_IRINGBUF
  { "ir",   "Display instruction ring buffer", cmd_ir },
#endif
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}