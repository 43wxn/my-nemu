/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "sdb.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char expr[256];
  word_t old_val;

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);

    /* 我补充: 初始化新增字段 */
    wp_pool[i].expr[0] = '\0';
    wp_pool[i].old_val = 0;
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

/* 我补充: 从空闲链表中申请一个监视点 */
WP* new_wp() {
  if (free_ == NULL) {
    panic("No free watchpoint.");
  }

  WP *wp = free_;
  free_ = free_->next;

  wp->next = head;
  head = wp;

  wp->expr[0] = '\0';
  wp->old_val = 0;

  return wp;
}

/* 我补充: 释放一个监视点, 挂回空闲链表 */
void free_wp(WP *wp) {
  if (wp == NULL) return;

  if (head == NULL) {
    panic("free_wp(): head is NULL");
  }

  if (head == wp) {
    head = head->next;
  } else {
    WP *prev = head;
    while (prev->next != NULL && prev->next != wp) {
      prev = prev->next;
    }

    if (prev->next == NULL) {
      panic("free_wp(): watchpoint not found in active list");
    }

    prev->next = wp->next;
  }

  wp->expr[0] = '\0';
  wp->old_val = 0;

  wp->next = free_;
  free_ = wp;
}

/* 我补充: 按编号查找监视点 */
static WP* find_wp_by_no(int no) {
  WP *p = head;
  while (p != NULL) {
    if (p->NO == no) return p;
    p = p->next;
  }
  return NULL;
}

/* 我补充: 创建监视点 */
void add_watchpoint(char *e) {
  if (e == NULL) {
    printf("Usage: w EXPR\n");
    return;
  }

  bool success = true;
  word_t val = expr(e, &success);
  if (!success) {
    printf("Bad expression: %s\n", e);
    return;
  }

  WP *wp = new_wp();

  strncpy(wp->expr, e, sizeof(wp->expr) - 1);
  wp->expr[sizeof(wp->expr) - 1] = '\0';
  wp->old_val = val;

  printf("Watchpoint %d: %s = %lu (0x%lx)\n",
      wp->NO, wp->expr,
      (unsigned long)wp->old_val,
      (unsigned long)wp->old_val);
}

/* 我补充: 删除指定编号的监视点 */
void delete_watchpoint(int no) {
  WP *wp = find_wp_by_no(no);
  if (wp == NULL) {
    printf("Watchpoint %d not found\n", no);
    return;
  }

  printf("Delete watchpoint %d: %s\n", wp->NO, wp->expr);
  free_wp(wp);
}

/* 我补充: 打印所有监视点 */
void display_watchpoints() {
  WP *p = head;

  if (p == NULL) {
    printf("No watchpoints.\n");
    return;
  }

  printf("Num\tWhat\n");
  while (p != NULL) {
    printf("%d\t%s\n", p->NO, p->expr);
    p = p->next;
  }
}

/* 我补充: 每次执行后检查监视点是否变化
 * 返回 true 表示至少有一个监视点触发
 */
bool check_watchpoints() {
  WP *p = head;
  bool changed = false;

  while (p != NULL) {
    bool success = true;
    word_t new_val = expr(p->expr, &success);

    if (!success) {
      printf("Failed to evaluate watchpoint %d: %s\n", p->NO, p->expr);
      p = p->next;
      continue;
    }

    if (new_val != p->old_val) {
      printf("Watchpoint %d triggered: %s\n", p->NO, p->expr);
      printf("Old value = %lu (0x%lx)\n",
          (unsigned long)p->old_val,
          (unsigned long)p->old_val);
      printf("New value = %lu (0x%lx)\n",
          (unsigned long)new_val,
          (unsigned long)new_val);

      p->old_val = new_val;
      changed = true;
    }

    p = p->next;
  }

  return changed;
}