#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core.h"
#include "main.h"
#include "options.h"
#include "scale.h"
#include "scaler_neon.h"

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

// from gambatte-dms
//from RGB565
#define cR(A) (((A) & 0xf800) >> 11)
#define cG(A) (((A) & 0x7e0) >> 5)
#define cB(A) ((A) & 0x1f)
//to RGB565
#define Weight1_1(A, B)  ((((cR(A) + cR(B)) >> 1) & 0x1f) << 11 | (((cG(A) + cG(B)) >> 1) & 0x3f) << 5 | (((cB(A) + cB(B)) >> 1) & 0x1f))
#define Weight1_2(A, B)  ((((cR(A) + (cR(B) << 1)) / 3) & 0x1f) << 11 | (((cG(A) + (cG(B) << 1)) / 3) & 0x3f) << 5 | (((cB(A) + (cB(B) << 1)) / 3) & 0x1f))
#define Weight2_1(A, B)  ((((cR(B) + (cR(A) << 1)) / 3) & 0x1f) << 11 | (((cG(B) + (cG(A) << 1)) / 3) & 0x3f) << 5 | (((cB(B) + (cB(A) << 1)) / 3) & 0x1f))
#define Weight1_3(A, B)  ((((cR(A) + (cR(B) * 3)) >> 2) & 0x1f) << 11 | (((cG(A) + (cG(B) * 3)) >> 2) & 0x3f) << 5 | (((cB(A) + (cB(B) * 3)) >> 2) & 0x1f))
#define Weight3_1(A, B)  ((((cR(B) + (cR(A) * 3)) >> 2) & 0x1f) << 11 | (((cG(B) + (cG(A) * 3)) >> 2) & 0x3f) << 5 | (((cB(B) + (cB(A) * 3)) >> 2) & 0x1f))
#define Weight1_4(A, B)  ((((cR(A) + (cR(B) << 2)) / 5) & 0x1f) << 11 | (((cG(A) + (cG(B) << 2)) / 5) & 0x3f) << 5 | (((cB(A) + (cB(B) << 2)) / 5) & 0x1f))
#define Weight4_1(A, B)  ((((cR(B) + (cR(A) << 2)) / 5) & 0x1f) << 11 | (((cG(B) + (cG(A) << 2)) / 5) & 0x3f) << 5 | (((cB(B) + (cB(A) << 2)) / 5) & 0x1f))
#define Weight2_3(A, B)  (((((cR(A) << 1) + (cR(B) * 3)) / 5) & 0x1f) << 11 | ((((cG(A) << 1) + (cG(B) * 3)) / 5) & 0x3f) << 5 | ((((cB(A) << 1) + (cB(B) * 3)) / 5) & 0x1f))
#define Weight3_2(A, B)  (((((cR(B) << 1) + (cR(A) * 3)) / 5) & 0x1f) << 11 | ((((cG(B) << 1) + (cG(A) * 3)) / 5) & 0x3f) << 5 | ((((cB(B) << 1) + (cB(A) * 3)) / 5) & 0x1f))
#define Weight1_1_1_1(A, B, C, D)  ((((cR(A) + cR(B) + cR(C) + cR(D)) >> 2) & 0x1f) << 11 | (((cG(A) + cG(B) + cG(C) + cG(D)) >> 2) & 0x3f) << 5 | (((cB(A) + cB(B) + cB(C) + cB(D)) >> 2) & 0x1f))

static inline int gcd(int a, int b) {
	return b ? gcd(b, a % b) : a;
}

static void scale_null(unsigned w, unsigned h, size_t pitch, const void *src, void *dst) {}
static void scale1x(unsigned w, unsigned h, size_t pitch, const void *src, void *dst) {
	scale1x_n16(src, dst+dst_offs, w, h, pitch, SCREEN_PITCH);
	return;

	dst += dst_offs;
	for (unsigned y = 0; y < h; y++) {
		memcpy(dst + y * SCREEN_PITCH, src + y * pitch, w * SCREEN_BPP);
	}
}

static void scale2x(unsigned w, unsigned h, size_t pitch, const void *src, void *dst) {
	scale2x_n16(src, dst+dst_offs, w, h, pitch, SCREEN_PITCH);
	return;
	
	dst += dst_offs;
	for (unsigned y = 0; y < h; y++) {
		uint16_t* src_row = src + y * pitch;
		uint16_t* dst_row = dst + y * SCREEN_PITCH * 2;
		for (unsigned x = 0; x < w; x++) {
			uint16_t s = *src_row;
			
			// row 1
			*(dst_row                       ) = s;
			*(dst_row + 1                   ) = s;
			
			// row 2
			*(dst_row + SCREEN_WIDTH * 1    ) = s;
			*(dst_row + SCREEN_WIDTH * 1 + 1) = s;
			
			src_row += 1;
			dst_row += 2;
		}
	}
}
static void scale2x_lcd(unsigned w, unsigned h, size_t pitch, const void *src, void *dst) {
	dst += dst_offs;

	uint16_t k = 0x0000;
	for (unsigned y = 0; y < h; y++) {
		uint16_t* src_row = src + y * pitch;
		uint16_t* dst_row = dst + y * SCREEN_PITCH * 2;
		for (unsigned x = 0; x < w; x++) {
			uint16_t s = *src_row;
            uint16_t r = (s & 0b1111100000000000);
            uint16_t g = (s & 0b0000011111100000);
            uint16_t b = (s & 0b0000000000011111);
			
			*(dst_row                       ) = r;
			*(dst_row + 1                   ) = b;
			
			*(dst_row + SCREEN_WIDTH * 1    ) = g;
			*(dst_row + SCREEN_WIDTH * 1 + 1) = k;
			
			src_row += 1;
			dst_row += 2;
		}
	}
}
static void scale2x_scanline(unsigned w, unsigned h, size_t pitch, const void *src, void *dst) {
	dst += dst_offs;

	// uint16_t k = 0x0000;
	for (unsigned y = 0; y < h; y++) {
		uint16_t* src_row = src + y * pitch;
		uint16_t* dst_row = dst + y * SCREEN_PITCH * 2;
		for (unsigned x = 0; x < w; x++) {
			uint16_t s = *src_row;
			
			*(dst_row                       ) = s;
			*(dst_row + 1                   ) = s;
			
			// *(dst_row + SCREEN_WIDTH * 1    ) = k;
			// *(dst_row + SCREEN_WIDTH * 1 + 1) = k;
			
			src_row += 1;
			dst_row += 2;
		}
	}
}

static void scale3x(unsigned w, unsigned h, size_t pitch, const void *src, void *dst) {
	scale3x_n16(src, dst+dst_offs, w, h, pitch, SCREEN_PITCH);
	return;

	dst += dst_offs;
	for (unsigned y = 0; y < h; y++) {
		uint16_t* src_row = src + y * pitch;
		uint16_t* dst_row = dst + y * SCREEN_PITCH * 3;
		for (unsigned x = 0; x < w; x++) {
			uint16_t s = *src_row;
			
			// row 1
			*(dst_row                       ) = s;
			*(dst_row                    + 1) = s;
			*(dst_row                    + 2) = s;
			
			// row 2
			*(dst_row + SCREEN_WIDTH * 1    ) = s;
			*(dst_row + SCREEN_WIDTH * 1 + 1) = s;
			*(dst_row + SCREEN_WIDTH * 1 + 2) = s;

			// row 3
			*(dst_row + SCREEN_WIDTH * 2    ) = s;
			*(dst_row + SCREEN_WIDTH * 2 + 1) = s;
			*(dst_row + SCREEN_WIDTH * 2 + 2) = s;

			src_row += 1;
			dst_row += 3;
		}
	}
}
static void scale3x_lcd(unsigned w, unsigned h, size_t pitch, const void *src, void *dst) {
	dst += dst_offs;

	uint16_t k = 0x0000;
	for (unsigned y = 0; y < h; y++) {
		uint16_t* src_row = src + y * pitch;
		uint16_t* dst_row = dst + y * SCREEN_PITCH * 3;
		for (unsigned x = 0; x < w; x++) {
			uint16_t s = *src_row;
            uint16_t r = (s & 0b1111100000000000);
            uint16_t g = (s & 0b0000011111100000);
            uint16_t b = (s & 0b0000000000011111);
			
			// row 1
			*(dst_row                       ) = k;
			*(dst_row                    + 1) = g;
			*(dst_row                    + 2) = k;
			
			// row 2
			*(dst_row + SCREEN_WIDTH * 1    ) = r;
			*(dst_row + SCREEN_WIDTH * 1 + 1) = g;
			*(dst_row + SCREEN_WIDTH * 1 + 2) = b;

			// row 3
			*(dst_row + SCREEN_WIDTH * 2    ) = r;
			*(dst_row + SCREEN_WIDTH * 2 + 1) = k;
			*(dst_row + SCREEN_WIDTH * 2 + 2) = b;

			src_row += 1;
			dst_row += 3;
		}
	}
}
static void scale3x_dmg(unsigned w, unsigned h, size_t pitch, const void *src, void *dst) {
	dst += dst_offs;

	uint16_t g = 0xffff;
	for (unsigned y = 0; y < h; y++) {
		uint16_t* src_row = src + y * pitch;
		uint16_t* dst_row = dst + y * SCREEN_PITCH * 3;
		for (unsigned x = 0; x < w; x++) {
			uint16_t _1 = *src_row;
            uint16_t _2 = Weight3_2( _1, g);
            uint16_t _3 = Weight2_3( _1, g);
			
			// row 1
			*(dst_row                       ) = _2;
			*(dst_row                    + 1) = _1;
			*(dst_row                    + 2) = _1;
			
			// row 2
			*(dst_row + SCREEN_WIDTH * 1    ) = _2;
			*(dst_row + SCREEN_WIDTH * 1 + 1) = _1;
			*(dst_row + SCREEN_WIDTH * 1 + 2) = _1;

			// row 3
			*(dst_row + SCREEN_WIDTH * 2    ) = _3;
			*(dst_row + SCREEN_WIDTH * 2 + 1) = _2;
			*(dst_row + SCREEN_WIDTH * 2 + 2) = _2;

			src_row += 1;
			dst_row += 3;
		}
	}
}
static void scale3x_scanline(unsigned w, unsigned h, size_t pitch, const void *src, void *dst) {
	dst += dst_offs;

	// uint16_t k = 0x0000;
	for (unsigned y = 0; y < h; y++) {
		uint16_t* src_row = src + y * pitch;
		uint16_t* dst_row = dst + y * SCREEN_PITCH * 3;
		for (unsigned x = 0; x < w; x++) {
			uint16_t s = *src_row;
			
			// row 1
			*(dst_row                       ) = s;
			*(dst_row                    + 1) = s;
			// *(dst_row                    + 2) = k;
			
			// row 2
			*(dst_row + SCREEN_WIDTH * 1    ) = s;
			*(dst_row + SCREEN_WIDTH * 1 + 1) = s;
			// *(dst_row + SCREEN_WIDTH * 1 + 2) = k;

			// row 3
			// *(dst_row + SCREEN_WIDTH * 2    ) = k;
			// *(dst_row + SCREEN_WIDTH * 2 + 1) = k;
			// *(dst_row + SCREEN_WIDTH * 2 + 2) = k;

			src_row += 1;
			dst_row += 3;
		}
	}
}

static void scale6x_dmg(unsigned w, unsigned h, size_t pitch, const void *src, void *dst) {
	dst += dst_offs;

	uint16_t g = 0xffff;
	for (unsigned y = 0; y < h; y++) {
		uint16_t* src_row = src + y * pitch;
		uint16_t* dst_row = dst + y * SCREEN_PITCH * 6;
		for (unsigned x = 0; x < w; x++) {
			uint16_t _1 = *src_row;
            uint16_t _2 = Weight4_1( _1, g);
            uint16_t _3 = Weight3_1( _1, g);
			
			// row 1
			*(dst_row                       ) = _2;
			*(dst_row                    + 1) = _1;
			*(dst_row                    + 2) = _1;
			*(dst_row                    + 3) = _1;
			*(dst_row                    + 4) = _1;
			*(dst_row                    + 5) = _1;
			
			// row 2
			*(dst_row + SCREEN_WIDTH * 1    ) = _2;
			*(dst_row + SCREEN_WIDTH * 1 + 1) = _1;
			*(dst_row + SCREEN_WIDTH * 1 + 2) = _1;
			*(dst_row + SCREEN_WIDTH * 1 + 3) = _1;
			*(dst_row + SCREEN_WIDTH * 1 + 4) = _1;
			*(dst_row + SCREEN_WIDTH * 1 + 5) = _1;
			
			// row 3
			*(dst_row + SCREEN_WIDTH * 2    ) = _2;
			*(dst_row + SCREEN_WIDTH * 2 + 1) = _1;
			*(dst_row + SCREEN_WIDTH * 2 + 2) = _1;
			*(dst_row + SCREEN_WIDTH * 2 + 3) = _1;
			*(dst_row + SCREEN_WIDTH * 2 + 4) = _1;
			*(dst_row + SCREEN_WIDTH * 2 + 5) = _1;
			
			// row 4
			*(dst_row + SCREEN_WIDTH * 3    ) = _2;
			*(dst_row + SCREEN_WIDTH * 3 + 1) = _1;
			*(dst_row + SCREEN_WIDTH * 3 + 2) = _1;
			*(dst_row + SCREEN_WIDTH * 3 + 3) = _1;
			*(dst_row + SCREEN_WIDTH * 3 + 4) = _1;
			*(dst_row + SCREEN_WIDTH * 3 + 5) = _1;
			
			// row 5
			*(dst_row + SCREEN_WIDTH * 4    ) = _2;
			*(dst_row + SCREEN_WIDTH * 4 + 1) = _1;
			*(dst_row + SCREEN_WIDTH * 4 + 2) = _1;
			*(dst_row + SCREEN_WIDTH * 4 + 3) = _1;
			*(dst_row + SCREEN_WIDTH * 4 + 4) = _1;
			*(dst_row + SCREEN_WIDTH * 4 + 5) = _1;

			// row 6
			*(dst_row + SCREEN_WIDTH * 5    ) = _3;
			*(dst_row + SCREEN_WIDTH * 5 + 1) = _2;
			*(dst_row + SCREEN_WIDTH * 5 + 2) = _2;
			*(dst_row + SCREEN_WIDTH * 5 + 3) = _2;
			*(dst_row + SCREEN_WIDTH * 5 + 4) = _2;
			*(dst_row + SCREEN_WIDTH * 5 + 5) = _2;

			src_row += 1;
			dst_row += 6;
		}
	}
}

static void scaleNN(unsigned w, unsigned h, size_t pitch, const void *src, void *dst) {
	int dy = -dst_h;
	unsigned lines = h;
	bool copy = false;
	size_t cpy_w = dst_w * SCREEN_BPP;

	dst += dst_offs;
	
	while (lines) {
		int dx = -dst_w;
		const uint16_t *psrc16 = src;
		uint16_t *pdst16 = dst;

		if (copy) {
			copy = false;
			memcpy(dst, dst - SCREEN_PITCH, cpy_w);
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
static void scaleNN_scanline(unsigned w, unsigned h, size_t pitch, const void *src, void *dst) {
	int dy = -dst_h;
	unsigned lines = h;

	dst += dst_offs;
	int row = 0;
	
	while (lines) {
		int dx = -dst_w;
		const uint16_t *psrc16 = src;
		uint16_t *pdst16 = dst;
		
		if (row%2==0) {
			int col = w;
			while(col--) {
				while (dx < 0) {
					*pdst16++ = *psrc16;
					dx += w;
				}

				dx -= dst_w;
				psrc16++;
			}
		}

		dst += SCREEN_PITCH;
		dy += h;
				
		if (dy >= 0) {
			dy -= dst_h;
			src += pitch;
			lines--;
		}
		row += 1;
	}
}

static void scaleNN_text(unsigned w, unsigned h, size_t pitch, const void *src, void *dst) {
	int dy = -dst_h;
	unsigned lines = h;
	bool copy = false;

	dst += dst_offs;
	size_t cpy_w = dst_w * SCREEN_BPP;
	
	int safe = w - 1; // don't look behind when there's nothing to see
	uint16_t l1,l2;
	while (lines) {
		int dx = -dst_w;
		const uint16_t *psrc16 = src;
		uint16_t *pdst16 = dst;
		l1 = l2 = 0x0;
		
		if (copy) {
			copy = false;
			memcpy(dst, dst - SCREEN_PITCH, cpy_w);
			dst += SCREEN_PITCH;
			dy += h;
		} else if (dy < 0) {
			int col = w;
			while(col--) {
				int d = 0;
				if (col<safe && l1!=l2) {
					// https://stackoverflow.com/a/71086522/145965
					uint16_t r = (l1 >> 10) & 0x3E;
					uint16_t g = (l1 >> 5) & 0x3F;
					uint16_t b = (l1 << 1) & 0x3E;
					uint16_t luma = (r * 218) + (g * 732) + (b * 74);
					luma = (luma >> 10) + ((luma >> 9) & 1); // 0-63
					d = luma > 24;
				}

				uint16_t s = *psrc16;
				
				while (dx < 0) {
					*pdst16++ = d ? l1 : s;
					dx += w;

					l2 = l1;
					l1 = s;
					d = 0;
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
static void scaleNN_text_scanline(unsigned w, unsigned h, size_t pitch, const void *src, void *dst) {
	int dy = -dst_h;
	unsigned lines = h;

	dst += dst_offs;
	
	int row = 0;
	int safe = w - 1; // don't look behind when there's nothing to see
	uint16_t l1,l2;
	while (lines) {
		int dx = -dst_w;
		const uint16_t *psrc16 = src;
		uint16_t *pdst16 = dst;
		l1 = l2 = 0x0;
		
		if (row%2==0) {
			int col = w;
			while(col--) {
				int d = 0;
				if (col<safe && l1!=l2) {
					// https://stackoverflow.com/a/71086522/145965
					uint16_t r = (l1 >> 10) & 0x3E;
					uint16_t g = (l1 >> 5) & 0x3F;
					uint16_t b = (l1 << 1) & 0x3E;
					uint16_t luma = (r * 218) + (g * 732) + (b * 74);
					luma = (luma >> 10) + ((luma >> 9) & 1); // 0-63
					d = luma > 24;
				}

				uint16_t s = *psrc16;
				
				while (dx < 0) {
					*pdst16++ = d ? l1 : s;
					dx += w;

					l2 = l1;
					l1 = s;
					d = 0;
				}

				dx -= dst_w;
				psrc16++;
			}
		}

		dst += SCREEN_PITCH;
		dy += h;
				
		if (dy >= 0) {
			dy -= dst_h;
			src += pitch;
			lines--;
		}
		row += 1;
	}
}

static void scale_select_scaler(unsigned w, unsigned h, size_t pitch) {
	scaler = scale_null;
	if (w == 0 || h == 0 || pitch == 0) return;
	
	bool use_nearest = true;
	unsigned scale = 0;
	
	if (aspect_ratio<=0) aspect_ratio = (float)w / h;
	
	// pretty sure these non-NONE branches are dead
	if (scale_size==SCALE_SIZE_FULL) {
		dst_w = SCREEN_WIDTH;
		dst_h = SCREEN_HEIGHT;
		
		// printf("full for %ix%i: %ix%i (%f)\n", w,h, dst_w, dst_h, aspect_ratio); fflush(stdout);
	}
	else if (scale_size==SCALE_SIZE_ASPECT) {
		dst_w = SCREEN_HEIGHT * aspect_ratio;
		dst_h = SCREEN_HEIGHT;
		if (dst_w>SCREEN_WIDTH) {
			dst_w = SCREEN_WIDTH;
			dst_h = SCREEN_WIDTH / aspect_ratio;
		}
		
		// printf("aspect for %ix%i: %ix%i (%f)\n", w,h, dst_w, dst_h, aspect_ratio); fflush(stdout);
	}
	else {
		unsigned sx = SCREEN_WIDTH / w;
		unsigned sy = SCREEN_HEIGHT / h;
		scale  = sx<sy ? sx : sy;
		use_nearest = false;
		double near_ratio;
	
		// printf("%ix%i (%i)\n", sx,sy,scale); fflush(stdout);
		
		if (scale<=1) {
			use_nearest = true;
			if (sy>sx) { // eg. Final Fantasy VII 368x240
				// printf("normal\n"); fflush(stdout);
				dst_h = h * sy;
				
			
				// if the aspect ratio of an unmodified
				// w to dst_h is within 20% of the target
				// aspect_ratio don't force
				near_ratio = (double)w / dst_h / aspect_ratio;
				if (near_ratio>=0.79 && near_ratio<=1.21) {
					dst_w = w;
				}
				else {
					dst_w = dst_h * aspect_ratio;
					dst_w -= dst_w % 2;
				}

				if (dst_w>SCREEN_WIDTH) {
					dst_w = SCREEN_WIDTH;
					dst_h = dst_w / aspect_ratio;
					dst_h -= dst_w % 2;
					if (dst_h>SCREEN_HEIGHT) dst_h = SCREEN_HEIGHT;
				}
			}
			else if (sx>sy) { // not encountered yet (something like 320x480, perhaps "Fantastic Night Dreams - Cotton Original (Japan) (SLPS-02034)")
				// printf("weird nearest scaling!\n"); fflush(stdout);
				dst_w = w * sx;
			
				// see above
				near_ratio = (double)dst_w / h / aspect_ratio;
				if (near_ratio>=0.79 && near_ratio<=1.21) {
					dst_h = h;
				}
				else {
					dst_h = dst_w / aspect_ratio;
					dst_h -= dst_w % 2;
				}
			
				if (dst_h>SCREEN_HEIGHT) {
					dst_h = SCREEN_HEIGHT;
					dst_w = dst_h * aspect_ratio;
					dst_w -= dst_w % 2;
					if (dst_w>SCREEN_WIDTH) dst_w = SCREEN_WIDTH;
				}
			}
			else { // eg. Tekken 3, 368x480
				// printf("weirdest\n"); fflush(stdout);
				dst_w = w * sx;
				dst_h = h * sy;
				
				// see above
				near_ratio = (double)dst_w / dst_h / aspect_ratio;
				if (near_ratio>=0.79 && near_ratio<=1.21) {
					// close enough
				}
				else {
					if (dst_h>dst_w) {
						dst_w = dst_h * aspect_ratio;
						dst_w -= dst_w % 2;
					}
					else {
						dst_h = dst_w / aspect_ratio;
						dst_h -= dst_w % 2;
					}
				}
				
				if (dst_w>SCREEN_WIDTH) {
					dst_w = SCREEN_WIDTH;
				}
				if (dst_h>SCREEN_HEIGHT) {
					dst_h = SCREEN_HEIGHT;
				}
			}
			
			// printf("nearest for %ix%i: %ix%i (%f)\n", w,h, dst_w, dst_h, aspect_ratio); fflush(stdout);
		}
		else {
			dst_w = w * scale;
			dst_h = h * scale;
			
			// printf("native for %ix%i: %ix%i (%f)\n", w,h, dst_w, dst_h, aspect_ratio); fflush(stdout);
		}
	}
	
	unsigned dst_x = (SCREEN_WIDTH - dst_w) * SCREEN_BPP / 2;
	unsigned dst_y = (SCREEN_HEIGHT - dst_h) / 2;
	
	dst_offs = dst_y * SCREEN_PITCH + dst_x;
	
	if (use_nearest) {
		goto nearest;
	}
	else if (scale==1) scaler = scale1x;
	else if (scale==2) {
		switch (scale_effect) {
			case SCALE_EFFECT_LCD: scaler = scale2x_lcd; break;
			case SCALE_EFFECT_SCANLINE: scaler = scale2x_scanline; break;
			default: scaler = scale2x; break;
		}
	}
	else if (scale==3) {
		switch (scale_effect) {
			case SCALE_EFFECT_LCD: scaler = scale3x_lcd; break;
			case SCALE_EFFECT_DMG: scaler = scale3x_dmg; break;
			case SCALE_EFFECT_SCANLINE: scaler = scale3x_scanline; break;
			default: scaler = scale3x; break;
		}
	}
	else if (scale==6 && scale_effect==SCALE_EFFECT_DMG) {
		scaler = scale6x_dmg;
	}
	else {
		goto nearest;
	}
	return;
	
nearest:
	switch (scale_effect) {
		case SCALE_EFFECT_SCANLINE: scaler = optimize_text ? scaleNN_text_scanline : scaleNN_scanline; break;
		default: scaler = optimize_text ? scaleNN_text : scaleNN; break;
	}
	return;
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

int scale_clean(const void *src, void *dst) {
	if (scale_size==SCALE_SIZE_NONE) {
		if (scaler==scale2x_lcd) {
			scale2x(prev.w,prev.h,prev.pitch,src,dst);
			return 1;
		}
		else if (scaler==scale3x_lcd) {
			scale3x(prev.w,prev.h,prev.pitch,src,dst);
			return 1;
		}
	}
	return 0;
}