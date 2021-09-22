#include <SDL/SDL.h>
#include <unistd.h>
#include "core.h"
#include "libpicofe/fonts.h"
#include "libpicofe/plat.h"
#include "menu.h"
#include "plat.h"
#include "scale.h"
#include "util.h"

static SDL_Surface* screen;

struct audio_state {
	unsigned buf_w;
	unsigned max_buf_w;
	unsigned buf_r;
	size_t buf_len;
	struct audio_frame *buf;
	int in_sample_rate;
	int out_sample_rate;
};

struct audio_state audio;

static char msg[HUD_LEN];
static unsigned msg_priority = 0;
static unsigned msg_expire = 0;

static void video_expire_msg(void)
{
	msg[0] = '\0';
	msg_priority = 0;
	msg_expire = 0;
}

static void video_update_msg(void)
{
	if (msg[0] && msg_expire < plat_get_ticks_ms())
		video_expire_msg();
}

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

	return screen->pixels;
}

int plat_dump_screen(const char *filename) {
	char imgname[MAX_PATH];
	int ret = -1;
	SDL_Surface *surface = NULL;

	snprintf(imgname, MAX_PATH, "%s.bmp", filename);

	if (g_menuscreen_ptr) {
		surface = SDL_CreateRGBSurfaceFrom(g_menubg_src_ptr,
		                                   g_menubg_src_w,
		                                   g_menubg_src_h,
		                                   16,
		                                   g_menubg_src_w * sizeof(uint16_t),
		                                   0xF800, 0x07E0, 0x001F, 0x0000);
		if (surface) {
			ret = SDL_SaveBMP(surface, imgname);
			SDL_FreeSurface(surface);
		}
	} else {
		ret = SDL_SaveBMP(screen, imgname);
	}

	return ret;
}

int plat_load_screen(const char *filename, void *buf, size_t buf_size, int *w, int *h, int *bpp) {
	int ret = -1;
	char imgname[MAX_PATH];
	SDL_Surface *imgsurface = NULL;
	SDL_Surface *surface = NULL;

	snprintf(imgname, MAX_PATH, "%s.bmp", filename);
	imgsurface = SDL_LoadBMP(imgname);
	if (!imgsurface)
		goto finish;

	surface = SDL_DisplayFormat(imgsurface);
	if (!surface)
		goto finish;

	if (surface->pitch > SCREEN_PITCH ||
	    surface->h > SCREEN_HEIGHT ||
	    surface->w == 0 ||
	    surface->h * surface->pitch > buf_size)
		goto finish;

	memcpy(buf, surface->pixels, surface->pitch * surface->h);
	*w = surface->w;
	*h = surface->h;
	*bpp = surface->pitch / surface->w;

	ret = 0;

finish:
	if (imgsurface)
		SDL_FreeSurface(imgsurface);
	if (surface)
		SDL_FreeSurface(surface);
	return ret;
}


void plat_video_menu_enter(int is_rom_loaded)
{
	SDL_LockSurface(screen);
	memcpy(g_menubg_src_ptr, screen->pixels, g_menubg_src_h * g_menubg_src_pp * sizeof(uint16_t));
	SDL_UnlockSurface(screen);
	g_menuscreen_ptr = fb_flip();
}

void plat_video_menu_begin(void)
{
	SDL_LockSurface(screen);
	menu_begin();
}

void plat_video_menu_end(void)
{
	menu_end();
	SDL_UnlockSurface(screen);
	g_menuscreen_ptr = fb_flip();
}

void plat_video_menu_leave(void)
{
	memset(g_menubg_src_ptr, 0, g_menuscreen_h * g_menuscreen_pp * sizeof(uint16_t));

	SDL_LockSurface(screen);
	memset(screen->pixels, 0, g_menuscreen_h * g_menuscreen_pp * sizeof(uint16_t));
	SDL_UnlockSurface(screen);
	fb_flip();
	SDL_LockSurface(screen);
	memset(screen->pixels, 0, g_menuscreen_h * g_menuscreen_pp * sizeof(uint16_t));
	SDL_UnlockSurface(screen);

	g_menuscreen_ptr = NULL;
}

void plat_video_open(void)
{
}

void plat_video_set_msg(const char *new_msg, unsigned priority, unsigned msec)
{
	if (!new_msg) {
		video_expire_msg();
	} else if (priority >= msg_priority) {
		snprintf(msg, HUD_LEN, "%s", new_msg);
		string_truncate(msg, HUD_LEN - 1);
		msg_priority = priority;
		msg_expire = plat_get_ticks_ms() + msec;
	}
}

void plat_video_process(const void *data, unsigned width, unsigned height, size_t pitch) {
	static int had_msg = 0;
	SDL_LockSurface(screen);

	if (had_msg) {
		video_clear_msg(screen->pixels, screen->h, screen->pitch / SCREEN_BPP);
		had_msg = 0;
	}

	scale(width, height, pitch, data, screen->pixels);

	if (msg[0]) {
		video_print_msg(screen->pixels, screen->h, screen->pitch / SCREEN_BPP, msg);
		had_msg = 1;
	}

	SDL_UnlockSurface(screen);

	video_update_msg();
}

void plat_video_flip(void)
{
	fb_flip();
}

void plat_video_close(void)
{
}

unsigned plat_cpu_ticks(void)
{
	long unsigned ticks = 0;
	long ticksps = 0;
	FILE *file = NULL;

	file = fopen("/proc/self/stat", "r");
	if (!file)
		goto finish;

	if (!fscanf(file, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu", &ticks))
		goto finish;

	ticksps = sysconf(_SC_CLK_TCK);

	if (ticksps)
		ticks = ticks * 100 / ticksps;

finish:
	if (file)
		fclose(file);

	return ticks;
}

static void plat_sound_callback(void *unused, uint8_t *stream, int len)
{
	int16_t *p = (int16_t *)stream;
	if (audio.buf_len == 0)
		return;

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
	if (audio.buf_len == 0)
		return 1.0;

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
	if (audio.buf_len == 0)
		return;

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
	SDL_LockAudio();

	audio.buf_len = frame_rate > 0
		? current_audio_buffer_size * audio.in_sample_rate / frame_rate
		: 0;

	if (audio.buf_len == 0) {
		SDL_UnlockAudio();
		return;
	}

	buf_size = audio.buf_len * sizeof(struct audio_frame);
	audio.buf = realloc(audio.buf, buf_size);

	if (!audio.buf) {
		SDL_UnlockAudio();
		PA_ERROR("Error initializing sound buffer\n");
		plat_sound_finish();
		return;
	}

	memset(audio.buf, 0, buf_size);
	audio.buf_w = 0;
	audio.buf_r = 0;
	audio.max_buf_w = audio.buf_len - 1;
	SDL_UnlockAudio();
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
	g_menuscreen_ptr = NULL;

	g_menubg_src_w = SCREEN_WIDTH;
	g_menubg_src_h = SCREEN_HEIGHT;
	g_menubg_src_pp = SCREEN_WIDTH;

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

int plat_reinit(void)
{
	audio.in_sample_rate = sample_rate;
	plat_sound_resize_buffer();
	scale_update_scaler();
	return 0;
}

void plat_finish(void)
{
	plat_sound_finish();
	SDL_FreeSurface(screen);
	screen = NULL;
	SDL_Quit();
}
