#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "core.h"
#include "main.h"
#include "options.h"
#include "scale.h"

typedef void (*scaler_t)(unsigned w, unsigned h, size_t pitch, const void *src, void *dst);

struct dimensions {
	unsigned w;
	unsigned h;
	size_t pitch;
};

struct blend_args {
	int w_ratio_in;
	int w_ratio_out;
	uint16_t w_bp[2];
	int h_ratio_in;
	int h_ratio_out;
	uint16_t h_bp[2];
	uint16_t *blend_line;
} blend_args;

static scaler_t scaler;
static unsigned dst_w, dst_h, dst_offs;
struct dimensions prev;

#if __ARM_ARCH >= 5
static inline uint32_t average16(uint32_t c1, uint32_t c2) {
	uint32_t ret, lowbits = 0x0821;
	asm ("eor %0, %2, %3\r\n"
	     "and %0, %0, %1\r\n"
	     "add %0, %3, %0\r\n"
	     "add %0, %0, %2\r\n"
	     "lsr %0, %0, #1\r\n"
	     : "=&r" (ret) : "r" (lowbits), "r" (c1), "r" (c2) : );
	return ret;
}

static inline uint32_t average32(uint32_t c1, uint32_t c2) {
	uint32_t ret, lowbits = 0x08210821;

	asm ("eor %0, %3, %1\r\n"
	     "and %0, %0, %2\r\n"
	     "adds %0, %1, %0\r\n"
	     "and %1, %1, #0\r\n"
	     "movcs %1, #0x80000000\r\n"
	     "adds %0, %0, %3\r\n"
	     "rrx %0, %0\r\n"
	     "orr %0, %0, %1\r\n"
	     : "=&r" (ret), "+r" (c2) : "r" (lowbits), "r" (c1) : "cc" );

	return ret;
}

#define AVERAGE16_NOCHK(c1, c2) (average16((c1), (c2)))
#define AVERAGE32_NOCHK(c1, c2) (average32((c1), (c2)))

#else

static inline uint32_t average16(uint32_t c1, uint32_t c2) {
	return (c1 + c2 + ((c1 ^ c2) & 0x0821))>>1;
}

static inline uint32_t average32(uint32_t c1, uint32_t c2) {
	uint32_t sum = c1 + c2;
	uint32_t ret = sum + ((c1 ^ c2) & 0x08210821);
	uint32_t of = ((sum < c1) | (ret < sum)) << 31;

	return (ret >> 1) | of;
}

#define AVERAGE16_NOCHK(c1, c2) (average16((c1), (c2)))
#define AVERAGE32_NOCHK(c1, c2) (average32((c1), (c2)))

#endif


#define AVERAGE16(c1, c2) ((c1) == (c2) ? (c1) : AVERAGE16_NOCHK((c1), (c2)))
#define AVERAGE16_1_3(c1, c2) ((c1) == (c2) ? (c1) : (AVERAGE16_NOCHK(AVERAGE16_NOCHK((c1), (c2)), (c2))))

#define AVERAGE32(c1, c2) ((c1) == (c2) ? (c1) : AVERAGE32_NOCHK((c1), (c2)))
#define AVERAGE32_1_3(c1, c2) ((c1) == (c2) ? (c1) : (AVERAGE32_NOCHK(AVERAGE32_NOCHK((c1), (c2)), (c2))))

static inline int gcd(int a, int b) {
	return b ? gcd(b, a % b) : a;
}

static void scale_memcpy(unsigned w, unsigned h, size_t pitch, const void *src, void *dst) {
	dst += dst_offs;
	memcpy(dst, src, h * pitch);
}

static void scale_1x(unsigned w, unsigned h, size_t pitch, const void *src, void *dst) {
	dst += dst_offs;

	for (unsigned y = 0; y < h; y++) {
		memcpy(dst + y * SCREEN_PITCH, src + y * pitch, w * SCREEN_BPP);
	}
}

static void scale_nearest(unsigned w, unsigned h, size_t pitch, const void *src, void *dst) {
	int dy = -dst_h;
	unsigned lines = h;
	bool copy = false;

	dst += dst_offs;

	while (lines) {
		int dx = -dst_w;
		const uint16_t *psrc16 = src;
		uint16_t *pdst16 = dst;

		if (copy) {
			copy = false;
			memcpy(dst, dst - SCREEN_PITCH, SCREEN_PITCH);
			dst += SCREEN_PITCH;
			dy += h;
		} else if (dy < 0) {
			int col = w;
			while(col--) {
				while (dx < 0) {
					*pdst16++ = *psrc16;
					dx += w;
				}

				dx -= dst_w;
				psrc16++;
			}

			dst += SCREEN_PITCH;
			dy += h;
		}

		if (dy >= 0) {
			dy -= dst_h;
			src += pitch;
			lines--;
		} else {
			copy = true;
		}
	}
}

/* Generic blend based on % of dest pixel in next src pixel, using
 * rough quintiles: aaaa, aaab, aabb, abbb, bbbb. Quintile breakpoints
 * can be adjusted for sharper or smoother blending. Default 0-20%,
 * 20%-50% (round down), 50%(down)-50%(up), 50%(round up)-80%,
 * 80%-100%. This matches existing scalers */
static void scale_blend(unsigned w, unsigned h, size_t pitch, const void *src, void *dst) {
	int dy = 0;
	int lines = h;

	int rat_w = blend_args.w_ratio_in;
	int rat_dst_w = blend_args.w_ratio_out;
	uint16_t *bw = blend_args.w_bp;

	int rat_h = blend_args.h_ratio_in;
	int rat_dst_h = blend_args.h_ratio_out;
	uint16_t *bh = blend_args.h_bp;

	dst += dst_offs;

	while (lines--) {
		while (dy < rat_dst_h) {
			uint16_t *dst16 = (uint16_t *)dst;
			uint16_t *pblend = (uint16_t *)blend_args.blend_line;
			int col = w;
			int dx = 0;

			uint16_t *pnext = (uint16_t *)(src + pitch);
			if (!lines)
				pnext -= (pitch / sizeof(uint16_t));

			if (dy > rat_dst_h - bh[0]) {
				pblend = pnext;
			} else if (dy <= bh[0]) {
				/* Drops const, won't get touched later though */
				pblend = (uint16_t *)src;
			} else {
				const uint32_t *src32 = (const uint32_t *)src;
				const uint32_t *pnext32 = (const uint32_t *)pnext;
				uint32_t *pblend32 = (uint32_t *)pblend;
				int count = w / 2;

				if (dy <= bh[1]) {
					const uint32_t *tmp = pnext32;
					pnext32 = src32;
					src32 = tmp;
				}

				if (dy > rat_dst_h - bh[1] || dy <= bh[1]) {
					while(count--) {
						*pblend32++ = AVERAGE32_1_3(*src32, *pnext32);
						src32++;
						pnext32++;
					}
				} else {
					while(count--) {
						*pblend32++ = AVERAGE32(*src32, *pnext32);
						src32++;
						pnext32++;
					}
				}
			}

			while (col--) {
				uint16_t a, b, out;

				a = *pblend;
				b = *(pblend+1);

				while (dx < rat_dst_w) {
					if (a == b) {
						out = a;
					} else if (dx > rat_dst_w - bw[0]) { // top quintile, bbbb
						out = b;
					} else if (dx <= bw[0]) { // last quintile, aaaa
						out = a;
					} else {
						if (dx > rat_dst_w - bw[1]) { // 2nd quintile, abbb
							a = AVERAGE16_NOCHK(a,b);
						} else if (dx <= bw[1]) { // 4th quintile, aaab
							b = AVERAGE16_NOCHK(a,b);
						}

						out = AVERAGE16_NOCHK(a, b); // also 3rd quintile, aabb
					}
					*dst16++ = out;
					dx += rat_w;
				}

				dx -= rat_dst_w;
				pblend++;
			}

			dy += rat_h;
			dst += SCREEN_PITCH;
		}

		dy -= rat_dst_h;
		src += pitch;
	}
}

/* drowsnug's nofilter upscaler, edited by eggs for smoothness */
static void scale_sharp_240x160_320xXXX(unsigned _w, unsigned _h, size_t _pitch, const void *src_bytes, void *dst_bytes)
{
	unsigned Eh = 0;
	int dh = 0;
	int width = 240;
	int vf = 0;
	const uint16_t *src = (const uint16_t *)src_bytes;
	uint16_t *dst = (uint16_t *)dst_bytes;

	dst += dst_offs / sizeof(uint16_t);

	unsigned x, y;
	for (y = 0; y < dst_h; y++)
	{
		int source = dh * width;
		for (x = 0; x < 320/4; x++)
		{
			register uint16_t a, b, c;

			a = src[source];
			b = src[source+1];
			c = src[source+2];

			if(vf == 1){
				a = AVERAGE16(a, src[source+width]);
				b = AVERAGE16(b, src[source+width+1]);
				c = AVERAGE16(c, src[source+width+2]);
			}

			*dst++ = a;
			*dst++ = AVERAGE16_1_3(a,b);
			*dst++ = AVERAGE16_1_3(c,b);
			*dst++ = c;
			source+=3;

		}
		Eh += 160;
		if(Eh >= dst_h) {
			Eh -= dst_h;
			dh++;
			vf = 0;
		}
		else
			vf = 1;
	}
}

static void scale_sharp_256xXXX_320xXXX(unsigned w, unsigned h, size_t pitch, const void *src_bytes, void *dst_bytes)
{
	unsigned Eh = 0;
	int dh = 0;
	int vf = 0;
	const uint16_t *src = (const uint16_t *)src_bytes;
	uint16_t *dst = (uint16_t *)dst_bytes;
	size_t pxpitch = pitch / 2;

	dst += dst_offs / sizeof(uint16_t);

	unsigned x, y;
	for (y = 0; y < dst_h; y++)
	{
		int source = dh * pxpitch;
		for (x = 0; x < 320/5; x++)
		{
			register uint16_t a, b, c, d;

			a = src[source];
			b = src[source+1];
			c = src[source+2];
			d = src[source+3];

			if(vf == 1){
				a = AVERAGE16(a, src[source+pxpitch]);
				b = AVERAGE16(b, src[source+pxpitch+1]);
				c = AVERAGE16(c, src[source+pxpitch+2]);
				d = AVERAGE16(d, src[source+pxpitch+3]);
			}

			*dst++ = a;
			*dst++ = AVERAGE16_1_3(a,b);
			*dst++ = AVERAGE16(b,c);
			*dst++ = AVERAGE16_1_3(d,c);
			*dst++ = d;
			source+=4;

		}
		Eh += h;
		if(Eh >= dst_h) {
			Eh -= dst_h;
			dh++;
			vf = 0;
		}
		else
			vf = 1;
	}
}

static void scale_select_scaler(unsigned w, unsigned h, size_t pitch) {
	double current_aspect_ratio = aspect_ratio > 0 ? aspect_ratio : ((double)w / (double)h);

	/* mame2000 sets resolutions / aspect ratio without notifying
	 * of changes, new should always override old */
	if (!strcmp(core_name, "mame2000")) {
		current_aspect_ratio = ((double)w / (double)h);
	}

	scaler = NULL;

	if (blend_args.blend_line != NULL) {
		free(blend_args.blend_line);
		blend_args.blend_line = NULL;
	}

	if (scale_size == SCALE_SIZE_FULL) {
		dst_w = SCREEN_WIDTH;
		dst_h = SCREEN_HEIGHT;
		dst_offs = 0;
	} else if (scale_size == SCALE_SIZE_ASPECT ||
	           (scale_size == SCALE_SIZE_NONE && (w > SCREEN_WIDTH || h > SCREEN_HEIGHT))) {
		dst_w = SCREEN_WIDTH;
		dst_h = SCREEN_WIDTH / current_aspect_ratio + 0.5;
		dst_offs = ((SCREEN_HEIGHT-dst_h)/2) * SCREEN_PITCH;

		if (dst_h > SCREEN_HEIGHT) {
			dst_w = SCREEN_HEIGHT * current_aspect_ratio + 0.5;
			dst_h = SCREEN_HEIGHT;
			dst_offs = ((SCREEN_WIDTH-dst_w)/2) * SCREEN_BPP;
		}
	} else if (scale_size == SCALE_SIZE_NONE) {
		unsigned dst_x = ((SCREEN_WIDTH - w) * SCREEN_BPP / 2);
		unsigned dst_y = ((SCREEN_HEIGHT - h) / 2);
		dst_offs = dst_y * SCREEN_PITCH + dst_x;

		if (pitch == SCREEN_PITCH) {
			scaler = scale_memcpy;
		} else {
			scaler = scale_1x;
		}

		return;
	}

	if (!scaler && w == 240 && h == 160) {
		if (scale_filter == SCALE_FILTER_SHARP) {
			scaler = scale_sharp_240x160_320xXXX;
			return;
		}
	}

	if (!scaler &&
	    w == 256 &&
	    (current_aspect_ratio == 4.0f / 3.0f || scale_size == SCALE_SIZE_FULL))
	{
		if (scale_filter == SCALE_FILTER_SHARP) {
			scaler = scale_sharp_256xXXX_320xXXX;
			return;
		}
	}

	if (!scaler && scale_filter == SCALE_FILTER_NEAREST) {
		scaler = scale_nearest;
		return;
	}

	if (!scaler && (scale_filter == SCALE_FILTER_SHARP || scale_filter == SCALE_FILTER_SMOOTH)) {
		int gcd_w, div_w, gcd_h, div_h;
		blend_args.blend_line = calloc(w, sizeof(uint16_t));

		gcd_w = gcd(w, dst_w);
		blend_args.w_ratio_in = w / gcd_w;
		blend_args.w_ratio_out = dst_w / gcd_w;

		div_w = round(blend_args.w_ratio_out / 5.0);
		blend_args.w_bp[0] = div_w;
		blend_args.w_bp[1] = blend_args.w_ratio_out >> 1;

		gcd_h = gcd(h, dst_h);
		blend_args.h_ratio_in = h / gcd_h;
		blend_args.h_ratio_out = dst_h / gcd_h;

		div_h = round(blend_args.h_ratio_out / 5.0);
		blend_args.h_bp[0] = div_h;
		blend_args.h_bp[1] = blend_args.h_ratio_out >> 1;

		scaler = scale_blend;
		return;
	}

	if (!scaler) {
		scaler = scale_1x;
	}
}

void scale_update_scaler(void) {
	scale_select_scaler(prev.w, prev.h, prev.pitch);
}

void scale(unsigned w, unsigned h, size_t pitch, const void *src, void *dst) {
	if (w != prev.w || h != prev.h || pitch != prev.pitch) {
		PA_INFO("Dimensions changed to %dx%d\n", w, h);
		scale_select_scaler(w, h, pitch);
		memset(dst, 0, SCREEN_HEIGHT * SCREEN_PITCH);
		prev.w = w; prev.h = h; prev.pitch = pitch;
	}

	scaler(w, h, pitch, src, dst);
}
