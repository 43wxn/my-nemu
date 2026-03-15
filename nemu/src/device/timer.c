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
#include <device/map.h>
#include <device/alarm.h>
#include <utils.h>

static uint32_t *rtc_port_base = NULL;

static void rtc_io_handler(uint32_t offset, int len, bool is_write) {
  if (!is_write) {
    uint64_t us = get_time();
    uint32_t us_low = (uint32_t)us;
    uint32_t us_high = us >> 32;
    uint32_t seconds = us / 1000000;
    uint32_t milliseconds = (us % 1000000) / 1000;
    
    // 根据不同偏移返回不同值
    switch (offset) {
      case 0:   // 0xa0000048 - 微秒低32位
        rtc_port_base[0] = us_low;
        break;
      case 4:   // 0xa000004c - 微秒高32位
        rtc_port_base[1] = us_high;
        break;
      case 8:   // 0xa0000050 - 秒
        rtc_port_base[2] = seconds;
        break;
      case 12:  // 0xa0000054 - 毫秒
        rtc_port_base[3] = milliseconds;
        break;
      case 16:  // 0xa0000058 - 备用
      case 20:  // 0xa000005c - 备用
        rtc_port_base[offset/4] = 0;
        break;
      default:
        // 其他地址返回0
        if (offset < 24) {  // 只到 0xa000005f
          rtc_port_base[offset/4] = 0;
        }
        break;
    }
  }
}

#ifndef CONFIG_TARGET_AM
static void timer_intr() {
  if (nemu_state.state == NEMU_RUNNING) {
    extern void dev_raise_intr();
    dev_raise_intr();
  }
}
#endif

void init_timer() {
  // 扩展到 24 字节，刚好到 0xa000005f（键盘开始于 0xa0000060）
  rtc_port_base = (uint32_t *)new_space(24);  // 24字节 = 6个uint32_t
  
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map ("rtc", CONFIG_RTC_PORT, rtc_port_base, 24, rtc_io_handler);
#else
  add_mmio_map("rtc", CONFIG_RTC_MMIO, rtc_port_base, 24, rtc_io_handler);
#endif
  
  // 初始化所有寄存器为0
  for (int i = 0; i < 6; i++) {
    rtc_port_base[i] = 0;
  }
  
  IFNDEF(CONFIG_TARGET_AM, add_alarm_handle(timer_intr));
}