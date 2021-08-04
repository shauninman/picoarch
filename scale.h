#ifndef __SCALE_H__
#define __SCALE_H__

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define SCREEN_BPP 2
#define SCREEN_PITCH (SCREEN_BPP * SCREEN_WIDTH)

enum scale_size {
	SCALE_SIZE_NONE,
	SCALE_SIZE_ASPECT,
	SCALE_SIZE_FULL,
};

enum scale_filter {
	SCALE_FILTER_NEAREST,
	SCALE_FILTER_SHARP,
	SCALE_FILTER_SMOOTH,
};

void scale_update_scaler(void);
void scale(unsigned w, unsigned h, size_t pitch, const void *src, void *dst);

#endif
