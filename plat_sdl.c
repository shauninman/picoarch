#include <SDL/SDL.h>
#include <unistd.h>
#include <math.h>
#include "core.h"
#include "libpicofe/fonts.h"
#include "libpicofe/plat.h"
#include "menu.h"
#include "plat.h"
#include "scale.h"
#include "scaler_neon.h"
#include "util.h"

static SDL_Surface* screen;

// begin miyoo hardware scaling support
// loosely based on eggs' picogpsp gfx.c
#include <mi_sys.h>
#include <mi_gfx.h>
#define	pixelsPa	unused1
#define ALIGN4K(val)	((val+4095)&(~4095))

//
//	Get GFX_ColorFmt from SDL_Surface
//
MI_GFX_ColorFmt_e	GFX_ColorFmt(SDL_Surface *surface) {
	if (surface != NULL) {
		if (surface->format->BytesPerPixel == 2) {
			if (surface->format->Amask == 0) return E_MI_GFX_FMT_RGB565;
			if (surface->format->Amask == 0x8000) return E_MI_GFX_FMT_ARGB1555;
			if (surface->format->Amask == 0xF000) {
				if (surface->format->Bmask == 0x000F) return E_MI_GFX_FMT_ARGB4444;
				return E_MI_GFX_FMT_ABGR4444;
			}
			if (surface->format->Amask == 0x000F) {
				if (surface->format->Bmask == 0x00F0) return E_MI_GFX_FMT_RGBA4444;
				return E_MI_GFX_FMT_BGRA4444;
			}
			return E_MI_GFX_FMT_RGB565;
		}
		if (surface->format->Bmask == 0x000000FF) return E_MI_GFX_FMT_ARGB8888;
		if (surface->format->Amask == 0x000000FF) {
			if (surface->format->Bmask == 0x0000FF00) return E_MI_GFX_FMT_RGBA8888;
			return E_MI_GFX_FMT_BGRA8888;
		}
		if (surface->format->Rmask == 0x000000FF) return E_MI_GFX_FMT_ABGR8888;
	}
	return E_MI_GFX_FMT_ARGB8888;
}

//
//	GFX BlitSurface (MI_GFX ver) / in place of SDL_BlitSurface
//		with scale/bpp convert and rotate/mirror
//		rotate : 1 = 90 / 2 = 180 / 3 = 270
//		mirror : 1 = Horizontal / 2 = Vertical / 3 = Both
//		nowait : 0 = wait until done / 1 = no wait
//
void GFX_BlitSurfaceExec(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect,
			 uint32_t rotate, uint32_t mirror, uint32_t nowait) {
	if ((src != NULL)&&(dst != NULL)&&(src->pixelsPa)&&(dst->pixelsPa)) {
		MI_GFX_Surface_t Src;
		MI_GFX_Surface_t Dst;
		MI_GFX_Rect_t SrcRect;
		MI_GFX_Rect_t DstRect;
		MI_GFX_Opt_t Opt;
		MI_U16 Fence;

		memset(&Opt, 0, sizeof(Opt));
		Opt.eSrcDfbBldOp = E_MI_GFX_DFB_BLD_ONE;
		Opt.eRotate = (MI_GFX_Rotate_e)rotate;
		Opt.eMirror = (MI_GFX_Mirror_e)mirror;

		Src.phyAddr = src->pixelsPa;
		Src.u32Width = src->w;
		Src.u32Height = src->h;
		Src.u32Stride = src->pitch;
		Src.eColorFmt = GFX_ColorFmt(src);
		if (srcrect != NULL) {
			SrcRect.s32Xpos = srcrect->x;
			SrcRect.s32Ypos = srcrect->y;
			SrcRect.u32Width = srcrect->w;
			SrcRect.u32Height = srcrect->h;
		} else {
			SrcRect.s32Xpos = 0;
			SrcRect.s32Ypos = 0;
			SrcRect.u32Width = Src.u32Width;
			SrcRect.u32Height = Src.u32Height;
		}

		Dst.phyAddr = dst->pixelsPa;
		Dst.u32Width = dst->w;
		Dst.u32Height = dst->h;
		Dst.u32Stride = dst->pitch;
		Dst.eColorFmt = GFX_ColorFmt(dst);
		if (dstrect != NULL) {
			DstRect.s32Xpos = dstrect->x;
			DstRect.s32Ypos = dstrect->y;
			if ((dstrect->w==0)||(dstrect->h==0)) {
				DstRect.u32Width = SrcRect.u32Width;
				DstRect.u32Height = SrcRect.u32Height;
			} else {
				DstRect.u32Width = dstrect->w;
				DstRect.u32Height = dstrect->h;
			}
		} else {
			DstRect.s32Xpos = 0;
			DstRect.s32Ypos = 0;
			DstRect.u32Width = Dst.u32Width;
			DstRect.u32Height = Dst.u32Height;
		}

		MI_SYS_FlushInvCache(src->pixels, ALIGN4K(src->pitch * src->h));
		MI_SYS_FlushInvCache(dst->pixels, ALIGN4K(dst->pitch * dst->h));
		MI_GFX_BitBlit(&Src, &SrcRect, &Dst, &DstRect, &Opt, &Fence);
		if (!nowait) MI_GFX_WaitAllDone(FALSE, Fence);
	} else SDL_BlitSurface(src, srcrect, dst, dstrect);
}

struct GFX_Buffer {
	MI_PHY		phyAddr;
	void*		virAddr;
	int 		width;
	int 		height;
	int 		depth;
	int 		pitch;
	uint32_t 	size;
};
struct GFX_Scaler {
	scale_neon_t upscale;
	
	int src_w;
	int src_h;
	int src_p;
	
	int dst_w;
	int dst_h;
	int dst_p;
	
	int asp_x;
	int asp_y;
	int asp_w;
	int asp_h;
};
static struct GFX_Buffer buffer;
static struct GFX_Scaler scaler;
static SDL_Surface* scaled = NULL;

static uint16_t* dst_buf = NULL;
static size_t dst_buf_p = 0;
static void buffer_upscale(unsigned src_w, unsigned src_h, size_t src_p, const void* src,
			 unsigned dst_w, unsigned dst_h, size_t dst_p, void* dst) {
	
	unsigned scale = dst_w / src_w;
	if (scale==1) {
		memcpy(dst,src,dst_h*dst_p);
		return;
	}
	
	if (dst_p!=dst_buf_p) {
		if (dst_buf) free(dst_buf);
		dst_buf_p = dst_p;
		dst_buf = malloc(dst_p);
	}
	
	unsigned _src_w = src_w;
	unsigned _scale = scale;
	while (src_h) {
		const uint16_t* src_row = src;
		uint16_t* dst_row = dst_buf;
		while (src_w) {
			uint16_t s = *(src_row++);
			while (scale) {
				*(dst_row++) = s;
				scale -= 1;
			}
			scale = _scale;
			src_w -= 1;
		}
		src_w = _src_w;
		
		while (scale) {
			memcpy(dst, dst_buf, dst_p);
			dst += dst_p;
			scale -= 1;
		}
		scale = _scale;
		src_h -= 1;
		
		src += src_p;
	}
}

static void buffer_init(void) {
	buffer.width  = 1280;
	buffer.height =  960;
	buffer.depth  =   16;
	buffer.pitch  = buffer.width * SCREEN_BPP;
	buffer.size = buffer.pitch * buffer.height;

	if (MI_SYS_MMA_Alloc(NULL, ALIGN4K(buffer.size), &buffer.phyAddr)) return;
	MI_SYS_MemsetPa(buffer.phyAddr, 0, buffer.size);
	MI_SYS_Mmap(buffer.phyAddr, ALIGN4K(buffer.size), &buffer.virAddr, TRUE);
	
	PA_INFO("buffer_init()\n"); fflush(stdout);
	
	memset(&scaler, 0, sizeof(struct GFX_Scaler));
}
static void buffer_quit(void) {
	if (buffer.phyAddr) {
		MI_SYS_Munmap(buffer.virAddr, ALIGN4K(buffer.size));
		MI_SYS_MMA_Free(buffer.phyAddr);
	}
	if (scaled) {
		scaled->pixelsPa = 0;
		SDL_FreeSurface(scaled);
		scaled = NULL;
	}
	
	if (dst_buf) free(dst_buf);
}
static void buffer_clear(void) {
	PA_INFO("buffer_clear()\n"); fflush(stdout);
	MI_SYS_FlushInvCache(buffer.virAddr, ALIGN4K(buffer.size));
	MI_SYS_MemsetPa(buffer.phyAddr, 0, buffer.size);
}
static void buffer_renew_surface(int src_w, int src_h, int src_p) {
	if (scaled) {
		PA_INFO("freed %ix%i surface\n", scaled->w, scaled->h); fflush(stdout);
		scaled->pixelsPa = 0;
		SDL_FreeSurface(scaled);
		scaled = NULL;
	}
	
	scaler.src_w = src_w;
	scaler.src_h = src_h;
	scaler.src_p = src_p;
	
	// minimum
	// snes can barely keep up
	int sx = ceilf((float)SCREEN_WIDTH / src_w);
	int sy = ceilf((float)SCREEN_HEIGHT / src_h);
	int s = sx>sy ? sx : sy;
	
	while (s>1 && s*src_w*SCREEN_BPP*s*src_h>buffer.size) s -= 1;
	
	// maximum
	// snes can't keep up (so we added max_upscale)
	// int sx = 2 * SCREEN_WIDTH / src_w;
	// int sy = 2 * SCREEN_HEIGHT / src_h;
	// int s = sx<sy ? sx : sy;
	//
	// if (s>max_upscale) s = max_upscale;
	
	if (s>6) s = 6;
	switch(s) {
		case 1: scaler.upscale = scale1x_n16; break;
		case 2: scaler.upscale = scale2x_n16; break;
		case 3: scaler.upscale = scale3x_n16; break;
		case 4: scaler.upscale = scale4x_n16; break;
		case 5: scaler.upscale = scale5x_n16; break;
		case 6: scaler.upscale = scale6x_n16; break;
		// TODO: make an 8x version?
	}
	
	
	scaler.dst_w = src_w * s;
	scaler.dst_h = src_h * s;
	scaler.dst_p = scaler.dst_w * SCREEN_BPP;
	
	scaled = SDL_CreateRGBSurfaceFrom(buffer.virAddr,scaler.dst_w,scaler.dst_h,buffer.depth,scaler.dst_p,0,0,0,0);
	if (scaled != NULL) scaled->pixelsPa = buffer.phyAddr;
	PA_INFO("created %ix%i surface (%ix)\n", scaler.dst_w, scaler.dst_h, s); fflush(stdout);
	// buffer_clear();
}
static void buffer_scale(unsigned w, unsigned h, size_t pitch, const void *src) {
	bool update_asp = false;
	// static int scaler_max_upscale;
	if (w!=scaler.src_w || h!=scaler.src_h || pitch!=scaler.src_p) { //  || scaler_max_upscale!=max_upscale
		// scaler_max_upscale = max_upscale;
		buffer_renew_surface(w,h,pitch);
		update_asp = true;
	}
	
	static int scaler_mode;
	if (scaler_mode!=scale_size) {
		scaler_mode = scale_size;
		update_asp = true;
	}
	
	if (update_asp) {
		PA_INFO("update aspect ratio\n"); fflush(stdout);
		buffer_clear();
		
		if (aspect_ratio<=0) aspect_ratio = (float)scaler.src_w / scaler.src_h;
	
		if (scale_size==SCALE_SIZE_ASPECT) {
			scaler.asp_w = SCREEN_HEIGHT * aspect_ratio;
			scaler.asp_h = SCREEN_HEIGHT;
			if (scaler.asp_w>SCREEN_WIDTH) {
				scaler.asp_w = SCREEN_WIDTH;
				scaler.asp_h = SCREEN_WIDTH / aspect_ratio;
			}
		
			scaler.asp_x = (SCREEN_WIDTH - scaler.asp_w) / 2;
			scaler.asp_y = (SCREEN_HEIGHT - scaler.asp_h) / 2;
		}
	}
	
	// buffer_upscale_nn(src);
	// buffer_upscale(scaler.src_w,scaler.src_h,scaler.src_p,src,
	// 		scaler.dst_w,scaler.dst_h,scaler.dst_p,buffer.virAddr);
	scaler.upscale(src, buffer.virAddr,scaler.src_w,scaler.src_h,scaler.src_p,scaler.dst_p);
	
	if (scale_size==SCALE_SIZE_ASPECT) {
		GFX_BlitSurfaceExec(scaled, NULL, screen, &(SDL_Rect){scaler.asp_x, scaler.asp_y, scaler.asp_w, scaler.asp_h}, 0,0,0);
	}
	else {
		GFX_BlitSurfaceExec(scaled, NULL, screen, NULL, 0,0,0);
	}
	
	// just awful
	// void* dst = screen->pixels;
	// if (scale_effect==SCALE_EFFECT_SCANLINE) {
	// 	for (int i=1; i<480; i+=2) {
	// 		memset(dst+i*SCREEN_PITCH, 0, SCREEN_PITCH);
	// 	}
	// }
}

// end miyoo hardware scaling support

struct audio_state {
	unsigned buf_w;
	unsigned max_buf_w;
	unsigned buf_r;
	size_t buf_len;
	struct audio_frame *buf;
	int in_sample_rate;
	int out_sample_rate;
	int sample_rate_adj;
	int adj_out_sample_rate;
};

struct audio_state audio;

static void plat_sound_select_resampler(void);
void (*plat_sound_write)(const struct audio_frame *data, int frames);

#define DRC_MAX_ADJUSTMENT 0.003
#define DRC_ADJ_BELOW 40
#define DRC_ADJ_ABOVE 60

static char msg[HUD_LEN];
static unsigned msg_priority = 0;
static unsigned msg_expire = 0;

static bool frame_dirty = false;
static int frame_time = 1000000 / 60;

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

static int audio_resample_passthrough(struct audio_frame data) {
	audio.buf[audio.buf_w++] = data;
	if (audio.buf_w >= audio.buf_len) audio.buf_w = 0;

	return 1;
}

static int audio_resample_nearest(struct audio_frame data) {
	static int diff = 0;
	int consumed = 0;

	if (diff < audio.adj_out_sample_rate) {
		audio.buf[audio.buf_w++] = data;
		if (audio.buf_w >= audio.buf_len) audio.buf_w = 0;

		diff += audio.in_sample_rate;
	}

	if (diff >= audio.adj_out_sample_rate) {
		consumed++;
		diff -= audio.adj_out_sample_rate;
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


static SDL_Surface* clean_screen = NULL;
static const void* framebuffer; // NOTE: we don't own this
void* plat_clean_screen(void) {
	return scale_clean(framebuffer, clean_screen->pixels) ? clean_screen : NULL;
}

void plat_video_process(const void *data, unsigned width, unsigned height, size_t pitch) {
	framebuffer = data;
	
	static int had_msg = 0;
	frame_dirty = true;
	SDL_LockSurface(screen);
	
	if (had_msg) {
		video_clear_msg(screen->pixels, screen->h, screen->pitch / SCREEN_BPP);
		had_msg = 0;
	}
	
	if (scale_size==SCALE_SIZE_NONE) {
		scale(width, height, pitch, data, screen->pixels);
	}
	else {
		buffer_scale(width, height, pitch, data);
	}
	
	if (msg[0]) {
		video_print_msg(screen->pixels, screen->h, screen->pitch / SCREEN_BPP, msg);
		had_msg = 1;
	}
	
	SDL_UnlockSurface(screen);
	
	video_update_msg();
}

#include <sys/time.h>
static uint64_t plat_get_ticks_us_u64(void) {
    uint64_t ret;
    struct timeval tv;

    gettimeofday(&tv, NULL);

    ret = (uint64_t)tv.tv_sec * 1000000;
    ret += (uint64_t)tv.tv_usec;

    return ret;
}

void plat_video_flip(void)
{
	static uint64_t next_frame_time_us = 0;

	if (frame_dirty) {
		if (!enable_drc) {
			fb_flip();
			next_frame_time_us = 0;
		} else {
			uint64_t time = plat_get_ticks_us_u64();

            if ( (limit_frames) && (time < next_frame_time_us) ) {
                uint32_t delaytime = (next_frame_time_us - time - 1) / 1000 + 1;
                if (delaytime < 1000) SDL_Delay(delaytime);
                else next_frame_time_us = 0;
                time = plat_get_ticks_us_u64();
            }

			if ( (!next_frame_time_us) || (!limit_frames) ) {
				next_frame_time_us = time;
			}

			fb_flip();

			do {
				next_frame_time_us += frame_time;
			} while (next_frame_time_us < time);
		}
		frame_dirty = false;
	}
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

	spec.freq = MIN(sample_rate, MAX_SAMPLE_RATE);
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
	audio.sample_rate_adj = audio.out_sample_rate * DRC_MAX_ADJUSTMENT;
	audio.adj_out_sample_rate = audio.out_sample_rate;

	plat_sound_select_resampler();
	plat_sound_resize_buffer();

	SDL_PauseAudio(0);
	return 0;
}

int plat_sound_occupancy(void)
{
	int buffered = 0;
	if (audio.buf_len == 0)
		return 0;

	if (audio.buf_w != audio.buf_r) {
		buffered = audio.buf_w > audio.buf_r ?
			audio.buf_w - audio.buf_r :
			(audio.buf_w + audio.buf_len) - audio.buf_r;
	}

	return buffered * 100 / audio.buf_len;
}

#define BATCH_SIZE 100
void plat_sound_write_resample(const struct audio_frame *data, int frames, int (*resample)(struct audio_frame data), bool drc)
{
	int consumed = 0;
	if (audio.buf_len == 0)
		return;

	if (drc) {
		int occupancy = plat_sound_occupancy();

		if (occupancy < DRC_ADJ_BELOW) {
			audio.adj_out_sample_rate = audio.out_sample_rate + audio.sample_rate_adj;
		} else if (occupancy > DRC_ADJ_ABOVE) {
			audio.adj_out_sample_rate = audio.out_sample_rate - audio.sample_rate_adj;
		} else {
			audio.adj_out_sample_rate = audio.out_sample_rate;
		}
	}

	SDL_LockAudio();

	while (frames > 0) {
		int tries = 0;
		int amount = MIN(BATCH_SIZE, frames);

		while (tries < 10 && audio.buf_w == audio.max_buf_w) {
			tries++;
			SDL_UnlockAudio();

			if (!limit_frames)
				return;

			SDL_Delay(1);
			SDL_LockAudio();
		}

		while (amount && audio.buf_w != audio.max_buf_w) {
			consumed = resample(*data);
			data += consumed;
			amount -= consumed;
			frames -= consumed;
		}
	}
	SDL_UnlockAudio();
}

void plat_sound_write_passthrough(const struct audio_frame *data, int frames)
{
	plat_sound_write_resample(data, frames, audio_resample_passthrough, false);
}

void plat_sound_write_nearest(const struct audio_frame *data, int frames)
{
	plat_sound_write_resample(data, frames, audio_resample_nearest, false);
}

void plat_sound_write_drc(const struct audio_frame *data, int frames)
{
	plat_sound_write_resample(data, frames, audio_resample_nearest, true);
}

void plat_sound_resize_buffer(void) {
	size_t buf_size;
	SDL_LockAudio();

	audio.buf_len = frame_rate > 0
		? current_audio_buffer_size * audio.in_sample_rate / frame_rate
		: 0;

		/* Dynamic adjustment keeps buffer 50% full, need double size */
	if (enable_drc)
		audio.buf_len *= 2;

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

static void plat_sound_select_resampler(void)
{
	if (enable_drc) {
		PA_INFO("Using audio adjustment (in: %d, out: %d-%d)\n", audio.in_sample_rate, audio.out_sample_rate - audio.sample_rate_adj, audio.out_sample_rate + audio.sample_rate_adj);
		plat_sound_write = plat_sound_write_drc;
	} else if (audio.in_sample_rate == audio.out_sample_rate) {
		PA_INFO("Using passthrough resampler (in: %d, out: %d)\n", audio.in_sample_rate, audio.out_sample_rate);
		plat_sound_write = plat_sound_write_passthrough;
	} else {
		PA_INFO("Using nearest resampler (in: %d, out: %d)\n", audio.in_sample_rate, audio.out_sample_rate);
		plat_sound_write = plat_sound_write_nearest;
	}
}

void plat_sdl_event_handler(void *event_)
{
}

int plat_init(void)
{
	plat_sound_write = plat_sound_write_nearest;

	SDL_Init(SDL_INIT_VIDEO);
	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP * 8, SDL_SWSURFACE);
	if (screen == NULL) {
		PA_ERROR("%s, failed to set video mode\n", __func__);
		return -1;
	}
	
	clean_screen = SDL_CreateRGBSurface(SDL_SWSURFACE, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP * 8, 0,0,0,0);
	
	buffer_init();
	
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
	if (sample_rate && sample_rate != audio.in_sample_rate) {
		plat_sound_finish();

		if (plat_sound_init()) {
			PA_ERROR("SDL sound failed to init: %s\n", SDL_GetError());
			return -1;
		}
	} else {
		plat_sound_resize_buffer();
		plat_sound_select_resampler();
	}

	if (frame_rate != 0)
		frame_time = 1000000 / frame_rate;

	scale_update_scaler();
	return 0;
}

void plat_finish(void)
{
	plat_sound_finish();
	buffer_quit();
	SDL_FreeSurface(clean_screen);
	SDL_FreeSurface(screen);
	screen = NULL;
	SDL_Quit();
}
