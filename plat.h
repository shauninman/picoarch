#ifndef __PLAT_H__
#define __PLAT_H__

#include "libpicofe/plat.h"

struct audio_frame {
	int16_t left;
	int16_t right;
};

#define HUD_LEN 39

int  plat_init(void);
void plat_finish(void);
void plat_minimize(void);
void *plat_prepare_screenshot(int *w, int *h, int *bpp);

void plat_video_open(void);
void plat_video_set_msg(const char *new_msg);
void plat_video_process(const void *data, unsigned width, unsigned height, size_t pitch);
void plat_video_flip(void);
void plat_video_close(void);

float plat_sound_capacity(void);
void plat_sound_write(const struct audio_frame *data, int frames);
void plat_sound_resize_buffer(void);

#endif /* __PLAT_H__ */
