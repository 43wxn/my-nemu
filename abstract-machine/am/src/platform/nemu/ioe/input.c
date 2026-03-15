#include <am.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000

// NEMU 键盘设备地址
#define KBD_ADDR        (DEVICE_BASE + 0x0000060)  // 已经在 nemu.h 中定义

// AM 键盘码定义 (am.h 中已定义)
// AM_KEY_* 常量

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  // 从键盘设备读取数据
  uint32_t data = inl(KBD_ADDR);
  
  if (data == AM_KEY_NONE) {
    // 没有按键
    kbd->keydown = 0;
    kbd->keycode = AM_KEY_NONE;
  } else {
    // 解析按键
    // 高15位是按键码，最高位是按键状态（按下/释放）
    kbd->keydown = (data & KEYDOWN_MASK) ? 1 : 0;
    kbd->keycode = data & ~KEYDOWN_MASK;  // 去掉最高位
  }
}