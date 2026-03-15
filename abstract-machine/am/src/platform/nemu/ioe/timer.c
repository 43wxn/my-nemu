#include <am.h>
#include <nemu.h>

#define RTC_US_LOW   0x00
#define RTC_US_HIGH  0x04

void __am_timer_init() {
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  uint32_t lo = inl(RTC_ADDR + RTC_US_LOW);
  uint32_t hi = inl(RTC_ADDR + RTC_US_HIGH);
  uptime->us = ((uint64_t)hi << 32) | lo;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  // NEMU/PA2 中通常不提供真实 RTC，给一个固定值即可
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 1;
  rtc->month  = 1;
  rtc->year   = 1900;
}