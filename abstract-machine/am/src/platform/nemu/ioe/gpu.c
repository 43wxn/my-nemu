#include <am.h>
#include <nemu.h>
#include <string.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)
#define FB_ADDR   (MMIO_BASE + 0x1000000)

static int screen_width = 0;
static int screen_height = 0;

static inline void get_screen_size() {
  if (screen_width == 0 || screen_height == 0) {
    uint32_t info = inl(VGACTL_ADDR);
    screen_width = info >> 16;
    screen_height = info & 0xffff;
  }
}

void __am_gpu_init() {
  get_screen_size();

  // 不要在这里画测试图，否则会一直看到“init画面”
  // 如有需要可以清屏，但一般不强制要求
  // uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  // memset(fb, 0, screen_width * screen_height * sizeof(uint32_t));
  // outl(SYNC_ADDR, 1);
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  get_screen_size();

  *cfg = (AM_GPU_CONFIG_T) {
    .present = true,
    .has_accel = false,
    .width = screen_width,
    .height = screen_height,
    .vmemsz = screen_width * screen_height * sizeof(uint32_t)
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  get_screen_size();

  // 即使 pixels == NULL，也仍然可能只是要求 sync
  if (ctl->pixels != NULL) {
    uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
    uint32_t *pixels = (uint32_t *)ctl->pixels;

    int x = ctl->x;
    int y = ctl->y;
    int w = ctl->w;
    int h = ctl->h;

    // 保存原始步长，裁剪后逐行取源要用原始宽度
    int src_stride = ctl->w;

    int x0 = 0, y0 = 0;

    // 裁剪到屏幕范围内
    if (x < 0) {
      x0 = -x;
      w += x;
      x = 0;
    }
    if (y < 0) {
      y0 = -y;
      h += y;
      y = 0;
    }
    if (x + w > screen_width) {
      w = screen_width - x;
    }
    if (y + h > screen_height) {
      h = screen_height - y;
    }

    if (w > 0 && h > 0) {
      for (int j = 0; j < h; j++) {
        uint32_t *dst = fb + (y + j) * screen_width + x;
        uint32_t *src = pixels + (y0 + j) * src_stride + x0;
        memcpy(dst, src, w * sizeof(uint32_t));
      }
    }
  }

  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}