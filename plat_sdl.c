#include <SDL/SDL.h>
#include "core.h"
#include "libpicofe/fonts.h"
#include "libpicofe/menu.h"
#include "libpicofe/plat.h"
#include "plat.h"
#include "scale.h"

static SDL_Surface* screen;

struct audio_state {
	unsigned buf_w;
	unsigned max_buf_w;
	unsigned buf_r;
	size_t buf_len;
	struct audio_frame *buf;
	int freq;
	int in_sample_rate;
	int out_sample_rate;
};

struct audio_state audio;

static char msg[HUD_LEN];

static void video_clear_msg(uint16_t *dst, uint32_t h, uint32_t pitch)
{
	memset(dst + (h - 10) * pitch, 0, 10 * pitch * sizeof(uint16_t));
}

static void video_print_msg(uint16_t *dst, uint32_t h, uint32_t pitch, char *msg)
{
	basic_text_out16_nf(dst, pitch, 2, h - 10, msg);
}

static int audio_resample_nearest(struct audio_frame data) {
	static int diff = 0;
	int consumed = 0;

	if (diff < audio.out_sample_rate) {
		audio.buf[audio.buf_w++] = data;
		if (audio.buf_w >= audio.buf_len) audio.buf_w = 0;

		diff += audio.in_sample_rate;
	}

	if (diff >= audio.out_sample_rate) {
		consumed++;
		diff -= audio.out_sample_rate;
	}

	return consumed;
}

static void *fb_flip(void)
{
	SDL_Flip(screen);
	return screen->pixels;
}

void *plat_prepare_screenshot(int *w, int *h, int *bpp)
{
	if (w) *w = SCREEN_WIDTH;
	if (h) *h = SCREEN_HEIGHT;
	if (bpp) *bpp = SCREEN_BPP;

	return g_menuscreen_ptr;
}

void plat_video_menu_enter(int is_rom_loaded)
{
}

void plat_video_menu_begin(void)
{
	g_menuscreen_ptr = fb_flip();
}

void plat_video_menu_end(void)
{
	g_menuscreen_ptr = fb_flip();
}

void plat_video_menu_leave(void)
{
	SDL_LockSurface(screen);
	memset(g_menuscreen_ptr, 0, SCREEN_WIDTH * SCREEN_HEIGHT * SCREEN_BPP);
	SDL_UnlockSurface(screen);
	g_menuscreen_ptr = fb_flip();
	SDL_LockSurface(screen);
	memset(g_menuscreen_ptr, 0, SCREEN_WIDTH * SCREEN_HEIGHT * SCREEN_BPP);
	SDL_UnlockSurface(screen);
}

void plat_video_open(void)
{
}

void plat_video_set_msg(const char *new_msg)
{
	snprintf(msg, HUD_LEN, "%s", new_msg);
}

void plat_video_process(const void *data, unsigned width, unsigned height, size_t pitch) {
	SDL_LockSurface(screen);

	if (msg[0])
		video_clear_msg(screen->pixels, screen->h, screen->pitch / SCREEN_BPP);

	scale(width, height, pitch, data, screen->pixels);

	if (msg[0])
		video_print_msg(screen->pixels, screen->h, screen->pitch / SCREEN_BPP, msg);

	SDL_UnlockSurface(screen);
}

void plat_video_flip(void)
{
	g_menuscreen_ptr = fb_flip();
	msg[0] = 0;
}

void plat_video_close(void)
{
}

static void plat_sound_callback(void *unused, uint8_t *stream, int len)
{
	int16_t *p = (int16_t *)stream;
	len /= (sizeof(int16_t) * 2);

	while (audio.buf_r != audio.buf_w && len > 0) {
		*p++ = audio.buf[audio.buf_r].left;
		*p++ = audio.buf[audio.buf_r].right;
		audio.max_buf_w = audio.buf_r;

		len--;
		audio.buf_r++;

		if (audio.buf_r >= audio.buf_len) audio.buf_r = 0;
	}

	while(len > 0) {
		*p++ = 0;
		--len;
	}
}

static void plat_sound_finish(void)
{
	SDL_PauseAudio(1);
	SDL_CloseAudio();
	if (audio.buf) {
		free(audio.buf);
		audio.buf = NULL;
	}
}

static int plat_sound_init(void)
{
	if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		return -1;
	}

	SDL_AudioSpec spec, received;

	spec.freq = SAMPLE_RATE;
	spec.format = AUDIO_S16;
	spec.channels = 2;
	spec.samples = 512;
	spec.callback = plat_sound_callback;

	if (SDL_OpenAudio(&spec, &received) < 0) {
		plat_sound_finish();
		return -1;
	}

	audio.in_sample_rate = sample_rate;
	audio.out_sample_rate = received.freq;
	plat_sound_resize_buffer();

	SDL_PauseAudio(0);
	return 0;
}

float plat_sound_capacity(void)
{
	int buffered = 0;
	if (audio.buf_w != audio.buf_r) {
		buffered = audio.buf_w > audio.buf_r ?
			audio.buf_w - audio.buf_r :
			(audio.buf_w + audio.buf_len) - audio.buf_r;
	}

	return 1.0 - (float)buffered / audio.buf_len;
}

#define BATCH_SIZE 100
void plat_sound_write(const struct audio_frame *data, int frames)
{
	int consumed = 0;
	SDL_LockAudio();

	while (frames > 0) {
		int tries = 0;
		int amount = MIN(BATCH_SIZE, frames);

		while (tries < 10 && audio.buf_w == audio.max_buf_w) {
			tries++;
			SDL_UnlockAudio();

			if (!limit_frames)
				return;

			plat_sleep_ms(1);
			SDL_LockAudio();
		}

		while (amount && audio.buf_w != audio.max_buf_w) {
			consumed = audio_resample_nearest(*data);
			data += consumed;
			amount -= consumed;
			frames -= consumed;
		}
	}
	SDL_UnlockAudio();
}

void plat_sound_resize_buffer(void) {
	size_t buf_size;
	audio.buf_len = current_audio_buffer_size * audio.in_sample_rate / frame_rate;
	buf_size = audio.buf_len * sizeof(struct audio_frame);
	audio.buf = realloc(audio.buf, buf_size);

	if (!audio.buf) {
		PA_ERROR("Error initializing sound buffer\n");
		plat_sound_finish();
		return;
	}

	memset(audio.buf, 0, buf_size);
	audio.buf_w = 0;
	audio.buf_r = 0;
	audio.max_buf_w = audio.buf_len - 1;
}

void plat_sdl_event_handler(void *event_)
{
}

int plat_init(void)
{
	SDL_Init(SDL_INIT_VIDEO);
	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP * 8, SDL_SWSURFACE);

	if (screen == NULL) {
		PA_ERROR("%s, failed to set video mode\n", __func__);
		return -1;
	}

	SDL_ShowCursor(0);

	g_menuscreen_w = SCREEN_WIDTH;
	g_menuscreen_h = SCREEN_HEIGHT;
	g_menuscreen_pp = SCREEN_WIDTH;
	g_menuscreen_ptr = fb_flip();

	if (in_sdl_init(&in_sdl_platform_data, plat_sdl_event_handler)) {
		PA_ERROR("SDL input failed to init: %s\n", SDL_GetError());
		return -1;
	}
	in_probe();

	if (plat_sound_init()) {
		PA_ERROR("SDL sound failed to init: %s\n", SDL_GetError());
		return -1;
	}
	return 0;
}

void plat_finish(void)
{
	plat_sound_finish();
	SDL_Quit();
}
