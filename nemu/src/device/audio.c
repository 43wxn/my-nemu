#include <common.h>
#include <device/map.h>
#include <SDL2/SDL.h>
#include <string.h>

enum {
  reg_freq,
  reg_channels,
  reg_samples,
  reg_sbuf_size,
  reg_init,
  reg_count,
  nr_reg
};

static uint8_t *sbuf = NULL;
static uint32_t *audio_base = NULL;

static SDL_AudioSpec s = {};
static uint32_t sbuf_rpos = 0;
static bool audio_inited = false;

static void audio_play_callback(void *userdata, uint8_t *stream, int len) {
  memset(stream, 0, len);

  uint32_t count = audio_base[reg_count];
  uint32_t nread = (len < count ? len : count);

  if (nread == 0) return;

  uint32_t sbuf_size = audio_base[reg_sbuf_size];

  uint32_t first = nread;
  if (sbuf_rpos + nread > sbuf_size) {
    first = sbuf_size - sbuf_rpos;
  }

  memcpy(stream, sbuf + sbuf_rpos, first);
  if (nread > first) {
    memcpy(stream + first, sbuf, nread - first);
  }

  sbuf_rpos = (sbuf_rpos + nread) % sbuf_size;
  audio_base[reg_count] -= nread;
}

static void audio_io_handler(uint32_t offset, int len, bool is_write) {
  if (!is_write) return;

  if (offset == reg_init * sizeof(uint32_t) &&
      audio_base[reg_init] != 0 && !audio_inited) {
    SDL_AudioSpec desired = {};

    desired.freq = audio_base[reg_freq];
    desired.channels = audio_base[reg_channels];
    desired.samples = audio_base[reg_samples];
    desired.format = AUDIO_S16SYS;
    desired.callback = audio_play_callback;
    desired.userdata = NULL;

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
      panic("SDL_InitSubSystem(SDL_INIT_AUDIO) failed: %s", SDL_GetError());
    }

    if (SDL_OpenAudio(&desired, &s) < 0) {
      panic("SDL_OpenAudio failed: %s", SDL_GetError());
    }

    SDL_PauseAudio(0);
    audio_inited = true;
  }
}

void init_audio() {
  uint32_t space_size = sizeof(uint32_t) * nr_reg;
  audio_base = (uint32_t *)new_space(space_size);
  memset(audio_base, 0, space_size);

#ifdef CONFIG_HAS_PORT_IO
  add_pio_map("audio", CONFIG_AUDIO_CTL_PORT, audio_base, space_size, audio_io_handler);
#else
  add_mmio_map("audio", CONFIG_AUDIO_CTL_MMIO, audio_base, space_size, audio_io_handler);
#endif

  sbuf = (uint8_t *)new_space(CONFIG_SB_SIZE);
  memset(sbuf, 0, CONFIG_SB_SIZE);
  add_mmio_map("audio-sbuf", CONFIG_SB_ADDR, sbuf, CONFIG_SB_SIZE, NULL);

  audio_base[reg_sbuf_size] = CONFIG_SB_SIZE;
  audio_base[reg_count] = 0;
}