#include <string.h>
#include "core.h"
#include "options.h"
#include "scale.h"

#define prefetch(a,b)   __builtin_prefetch(a,b)
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

typedef void (*scaler_t)(unsigned w, unsigned h, size_t pitch, const void *src, void *dst);

struct dimensions {
	unsigned w;
	unsigned h;
	size_t pitch;
};

static scaler_t scaler;
static unsigned dst_w, dst_h, dst_offs;
struct dimensions prev;

static void scale_memcpy(unsigned w, unsigned h, size_t pitch, const void *src, void *dst) {
	dst += dst_offs;
	memcpy(dst, src, h * pitch);
}

static void scale_1x(unsigned w, unsigned h, size_t pitch, const void *src, void *dst) {
	dst += dst_offs;

	for (int y = 0; y < h; y++) {
		memcpy(dst + y * SCREEN_PITCH, src + y * pitch, pitch);
	}
}

static void scale_nearest(unsigned w, unsigned h, size_t pitch, const void *src_bytes, void *dst_bytes) {
	const uint16_t *src = (const uint16_t *)src_bytes;
	uint16_t *dst = (uint16_t *)dst_bytes;
	int dy = -(dst_h / 2);
	unsigned lines = 0;

	dst += dst_offs / sizeof(dst[0]);

	while (lines < h) {
		int dx = -(dst_w / 2);
		const uint16_t *psrc = src;
		uint16_t *pdst = dst;
		bool copy = false;

		if (copy) {
			copy = false;
			memcpy(dst, dst - SCREEN_PITCH / sizeof(dst[0]), SCREEN_PITCH);
		} else if (dy <= 0) {
			while (psrc - src < w) {
				if (dx <= 0) {
					*pdst++ = *psrc;
					dx += w;
				}

				if (dx > 0) {
					psrc++;
					dx -= dst_w;
				}
			}
		}

		if (dy <= 0) {
			dy += h;
			dst += SCREEN_PITCH / sizeof(dst[0]);
		}

		if (dy > 0) {
			dy -= dst_h;
			src += pitch / sizeof(src[0]);
			lines++;
		} else {
			copy = true;
		}
	}
}

/* drowsnug's nofilter upscaler, edited by eggs for smoothness */
#define AVERAGE16(c1, c2) (((c1) + (c2) + (((c1) ^ (c2)) & 0x0821))>>1)  //More accurate
static void scale_sharp_240x160_320xXXX(unsigned _w, unsigned _h, size_t _pitch, const void *src_bytes, void *dst_bytes)
{
	int Eh = 0;
	int dh = 0;
	int width = 240;
	int vf = 0;
	const uint16_t *src = (const uint16_t *)src_bytes;
	uint16_t *dst = (uint16_t *)dst_bytes;

	dst += dst_offs / sizeof(uint16_t);

	int x, y;
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
			*dst++ = AVERAGE16(AVERAGE16(a,b),b);
			*dst++ = AVERAGE16(b,AVERAGE16(b,c));
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
	int Eh = 0;
	int dh = 0;
	int vf = 0;
	const uint16_t *src = (const uint16_t *)src_bytes;
	uint16_t *dst = (uint16_t *)dst_bytes;
	size_t pxpitch = pitch / 2;

	dst += dst_offs / sizeof(uint16_t);

	int x, y;
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
			*dst++ = AVERAGE16(AVERAGE16(a,b),b);
			*dst++ = AVERAGE16(b,c);
			*dst++ = AVERAGE16(c,AVERAGE16(c,d));
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

/* gpsp-bittboy bilinear scalers https://github.com/retrofirmware/gpsp-bittboy */

#define Average(A, B) ((((A) & 0xF7DE) >> 1) + (((B) & 0xF7DE) >> 1) + ((A) & (B) & 0x0821))

/* Calculates the average of two pairs of RGB565 pixels. The result is, in
 * the lower bits, the average of both lower pixels, and in the upper bits,
 * the average of both upper pixels. */
#define Average32(A, B) ((((A) & 0xF7DEF7DE) >> 1) + (((B) & 0xF7DEF7DE) >> 1) + ((A) & (B) & 0x08210821))

/* Raises a pixel from the lower half to the upper half of a pair. */
#define Raise(N) ((N) << 16)

/* Extracts the upper pixel of a pair into the lower pixel of a pair. */
#define Hi(N) ((N) >> 16)

/* Extracts the lower pixel of a pair. */
#define Lo(N) ((N) & 0xFFFF)

/* Calculates the average of two RGB565 pixels. The source of the pixels is
 * the lower 16 bits of both parameters. The result is in the lower 16 bits.
 * The average is weighted so that the first pixel contributes 3/4 of its
 * color and the second pixel contributes 1/4. */
#define AverageQuarters3_1(A, B) ( (((A) & 0xF7DE) >> 1) + (((A) & 0xE79C) >> 2) + (((B) & 0xE79C) >> 2) + ((( (( ((A) & 0x1863) + ((A) & 0x0821) ) << 1) + ((B) & 0x1863) ) >> 2) & 0x1863) )

static inline void scale_bilinearish_240x160_320x240(unsigned src_x, unsigned src_y, size_t src_pitch, const void *src, void *dst)
{
	/* Before:
	 *    a b c d e f
	 *    g h i j k l
	 *
	 * After (multiple letters = average):
	 *    a    ab   bc   c    d    de   ef   f
	 *    ag   abgh bchi ci   dj   dejk efkl fl
	 *    g    gh   hi   i    j    jk   kl   l
	 */
	uint16_t *to   = (uint16_t *)dst;
	const uint16_t *from = (const uint16_t *)src;
	const uint32_t dst_pitch = SCREEN_PITCH;

	const uint32_t dst_x = src_x * 4 / 3;
	const uint32_t src_skip = src_pitch - src_x * sizeof(uint16_t),
		       dst_skip = dst_pitch - dst_x * sizeof(uint16_t);

	uint32_t x, y;

	for (y = 0; y < src_y; y += 2) {
		for (x = 0; x < src_x / 6; x++) {
			// -- Row 1 --
			// Read RGB565 elements in the source grid.
			// The notation is high_low (little-endian).
			uint32_t b_a = (*(uint32_t*) (from    )),
				d_c = (*(uint32_t*) (from + 2)),
				f_e = (*(uint32_t*) (from + 4));

			// Generate ab_a from b_a.
			*(uint32_t*) (to) = likely(Hi(b_a) == Lo(b_a))
				? b_a
				: Lo(b_a) /* 'a' verbatim to low pixel */ |
				Raise(Average(Hi(b_a), Lo(b_a))) /* ba to high pixel */;

			// Generate c_bc from b_a and d_c.
			*(uint32_t*) (to + 2) = likely(Hi(b_a) == Lo(d_c))
				? Lo(d_c) | Raise(Lo(d_c))
				: Raise(Lo(d_c)) /* 'c' verbatim to high pixel */ |
				Average(Lo(d_c), Hi(b_a)) /* bc to low pixel */;

			// Generate de_d from d_c and f_e.
			*(uint32_t*) (to + 4) = likely(Hi(d_c) == Lo(f_e))
				? Lo(f_e) | Raise(Lo(f_e))
				: Hi(d_c) /* 'd' verbatim to low pixel */ |
				Raise(Average(Lo(f_e), Hi(d_c))) /* de to high pixel */;

			// Generate f_ef from f_e.
			*(uint32_t*) (to + 6) = likely(Hi(f_e) == Lo(f_e))
				? f_e
				: Raise(Hi(f_e)) /* 'f' verbatim to high pixel */ |
				Average(Hi(f_e), Lo(f_e)) /* ef to low pixel */;

			if (likely(y + 1 < src_y))  // Is there a source row 2?
			{
				// -- Row 2 --
				uint32_t h_g = (*(uint32_t*) ((uint8_t*) from + src_pitch    )),
					j_i = (*(uint32_t*) ((uint8_t*) from + src_pitch + 4)),
					l_k = (*(uint32_t*) ((uint8_t*) from + src_pitch + 8));

				// Generate abgh_ag from b_a and h_g.
				uint32_t bh_ag = Average32(b_a, h_g);
				*(uint32_t*) ((uint8_t*) to + dst_pitch) = likely(Hi(bh_ag) == Lo(bh_ag))
					? bh_ag
					: Lo(bh_ag) /* ag verbatim to low pixel */ |
					Raise(Average(Hi(bh_ag), Lo(bh_ag))) /* abgh to high pixel */;

				// Generate ci_bchi from b_a, d_c, h_g and j_i.
				uint32_t ci_bh =
					Hi(bh_ag) /* bh verbatim to low pixel */ |
					Raise(Average(Lo(d_c), Lo(j_i))) /* ci to high pixel */;
				*(uint32_t*) ((uint8_t*) to + dst_pitch + 4) = likely(Hi(ci_bh) == Lo(ci_bh))
					? ci_bh
					: Raise(Hi(ci_bh)) /* ci verbatim to high pixel */ |
					Average(Hi(ci_bh), Lo(ci_bh)) /* bchi to low pixel */;

				// Generate fl_efkl from f_e and l_k.
				uint32_t fl_ek = Average32(f_e, l_k);
				*(uint32_t*) ((uint8_t*) to + dst_pitch + 12) = likely(Hi(fl_ek) == Lo(fl_ek))
					? fl_ek
					: Raise(Hi(fl_ek)) /* fl verbatim to high pixel */ |
					Average(Hi(fl_ek), Lo(fl_ek)) /* efkl to low pixel */;

				// Generate dejk_dj from d_c, f_e, j_i and l_k.
				uint32_t ek_dj =
					Raise(Lo(fl_ek)) /* ek verbatim to high pixel */ |
					Average(Hi(d_c), Hi(j_i)) /* dj to low pixel */;
				*(uint32_t*) ((uint8_t*) to + dst_pitch + 8) = likely(Hi(ek_dj) == Lo(ek_dj))
					? ek_dj
					: Lo(ek_dj) /* dj verbatim to low pixel */ |
					Raise(Average(Hi(ek_dj), Lo(ek_dj))) /* dejk to high pixel */;

				// -- Row 3 --
				// Generate gh_g from h_g.
				*(uint32_t*) ((uint8_t*) to + dst_pitch * 2) = likely(Hi(h_g) == Lo(h_g))
					? h_g
					: Lo(h_g) /* 'g' verbatim to low pixel */ |
					Raise(Average(Hi(h_g), Lo(h_g))) /* gh to high pixel */;

				// Generate i_hi from g_h and j_i.
				*(uint32_t*) ((uint8_t*) to + dst_pitch * 2 + 4) = likely(Hi(h_g) == Lo(j_i))
					? Lo(j_i) | Raise(Lo(j_i))
					: Raise(Lo(j_i)) /* 'i' verbatim to high pixel */ |
					Average(Lo(j_i), Hi(h_g)) /* hi to low pixel */;

				// Generate jk_j from j_i and l_k.
				*(uint32_t*) ((uint8_t*) to + dst_pitch * 2 + 8) = likely(Hi(j_i) == Lo(l_k))
					? Lo(l_k) | Raise(Lo(l_k))
					: Hi(j_i) /* 'j' verbatim to low pixel */ |
					Raise(Average(Hi(j_i), Lo(l_k))) /* jk to high pixel */;

				// Generate l_kl from l_k.
				*(uint32_t*) ((uint8_t*) to + dst_pitch * 2 + 12) = likely(Hi(l_k) == Lo(l_k))
					? l_k
					: Raise(Hi(l_k)) /* 'l' verbatim to high pixel */ |
					Average(Hi(l_k), Lo(l_k)) /* kl to low pixel */;
			}

			from += 6;
			to += 8;
		}

		// Skip past the waste at the end of the first line, if any,
		// then past 1 whole lines of source and 2 of destination.
		from = (uint16_t*) ((uint8_t*) from + src_skip +     src_pitch);
		to   = (uint16_t*) ((uint8_t*) to   + dst_skip + 2 * dst_pitch);
	}
}

static inline void scale_bilinearish_240x160_320x213(unsigned src_x, unsigned src_y, size_t src_pitch, const void *src, void *dst)
{
	/* Before:
	 *    a b c d e f
	 *    g h i j k l
	 *    m n o p q r
	 *
	 * After (multiple letters = average):
	 *    a    ab   bc   c    d    de   ef   f
	 *    ag   abgh bchi ci   dj   dejk efkl fl
	 *    gm   ghmn hino io   jp   jkpq klqr lr
	 *    m    mn   no   o    p    pq   qr   r
	 */

	uint16_t *to   = (uint16_t *)dst + dst_offs / sizeof(uint16_t);
	const uint16_t *from = (const uint16_t *)src;
	const uint32_t dst_pitch = SCREEN_PITCH;

	const uint32_t dst_x = src_x * 4 / 3;
	const uint32_t src_skip = src_pitch - src_x * sizeof(uint16_t),
		       dst_skip = dst_pitch - dst_x * sizeof(uint16_t);

	uint32_t x, y;

	for (y = 0; y < src_y; y += 3) {
		for (x = 0; x < src_x / 6; x++) {
			// -- Row 1 --
			// Read RGB565 elements in the source grid.
			// The notation is high_low (little-endian).
			uint32_t b_a = (*(uint32_t*) (from    )),
				d_c = (*(uint32_t*) (from + 2)),
				f_e = (*(uint32_t*) (from + 4));

			// Generate ab_a from b_a.
			*(uint32_t*) (to) = likely(Hi(b_a) == Lo(b_a))
				? b_a
				: Lo(b_a) /* 'a' verbatim to low pixel */ |
				Raise(Average(Hi(b_a), Lo(b_a))) /* ba to high pixel */;

			// Generate c_bc from b_a and d_c.
			*(uint32_t*) (to + 2) = likely(Hi(b_a) == Lo(d_c))
				? Lo(d_c) | Raise(Lo(d_c))
				: Raise(Lo(d_c)) /* 'c' verbatim to high pixel */ |
				Average(Lo(d_c), Hi(b_a)) /* bc to low pixel */;

			// Generate de_d from d_c and f_e.
			*(uint32_t*) (to + 4) = likely(Hi(d_c) == Lo(f_e))
				? Lo(f_e) | Raise(Lo(f_e))
				: Hi(d_c) /* 'd' verbatim to low pixel */ |
				Raise(Average(Lo(f_e), Hi(d_c))) /* de to high pixel */;

			// Generate f_ef from f_e.
			*(uint32_t*) (to + 6) = likely(Hi(f_e) == Lo(f_e))
				? f_e
				: Raise(Hi(f_e)) /* 'f' verbatim to high pixel */ |
				Average(Hi(f_e), Lo(f_e)) /* ef to low pixel */;

			if (likely(y + 1 < src_y))  // Is there a source row 2?
			{
				// -- Row 2 --
				uint32_t h_g = (*(uint32_t*) ((uint8_t*) from + src_pitch    )),
					j_i = (*(uint32_t*) ((uint8_t*) from + src_pitch + 4)),
					l_k = (*(uint32_t*) ((uint8_t*) from + src_pitch + 8));

				// Generate abgh_ag from b_a and h_g.
				uint32_t bh_ag = Average32(b_a, h_g);
				*(uint32_t*) ((uint8_t*) to + dst_pitch) = likely(Hi(bh_ag) == Lo(bh_ag))
					? bh_ag
					: Lo(bh_ag) /* ag verbatim to low pixel */ |
					Raise(Average(Hi(bh_ag), Lo(bh_ag))) /* abgh to high pixel */;

				// Generate ci_bchi from b_a, d_c, h_g and j_i.
				uint32_t ci_bh =
					Hi(bh_ag) /* bh verbatim to low pixel */ |
					Raise(Average(Lo(d_c), Lo(j_i))) /* ci to high pixel */;
				*(uint32_t*) ((uint8_t*) to + dst_pitch + 4) = likely(Hi(ci_bh) == Lo(ci_bh))
					? ci_bh
					: Raise(Hi(ci_bh)) /* ci verbatim to high pixel */ |
					Average(Hi(ci_bh), Lo(ci_bh)) /* bchi to low pixel */;

				// Generate fl_efkl from f_e and l_k.
				uint32_t fl_ek = Average32(f_e, l_k);
				*(uint32_t*) ((uint8_t*) to + dst_pitch + 12) = likely(Hi(fl_ek) == Lo(fl_ek))
					? fl_ek
					: Raise(Hi(fl_ek)) /* fl verbatim to high pixel */ |
					Average(Hi(fl_ek), Lo(fl_ek)) /* efkl to low pixel */;

				// Generate dejk_dj from d_c, f_e, j_i and l_k.
				uint32_t ek_dj =
					Raise(Lo(fl_ek)) /* ek verbatim to high pixel */ |
					Average(Hi(d_c), Hi(j_i)) /* dj to low pixel */;
				*(uint32_t*) ((uint8_t*) to + dst_pitch + 8) = likely(Hi(ek_dj) == Lo(ek_dj))
					? ek_dj
					: Lo(ek_dj) /* dj verbatim to low pixel */ |
					Raise(Average(Hi(ek_dj), Lo(ek_dj))) /* dejk to high pixel */;

				if (likely(y + 2 < src_y))  // Is there a source row 3?
				{
					// -- Row 3 --
					uint32_t n_m = (*(uint32_t*) ((uint8_t*) from + src_pitch * 2    )),
						p_o = (*(uint32_t*) ((uint8_t*) from + src_pitch * 2 + 4)),
						r_q = (*(uint32_t*) ((uint8_t*) from + src_pitch * 2 + 8));

					// Generate ghmn_gm from h_g and n_m.
					uint32_t hn_gm = Average32(h_g, n_m);
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 2) = likely(Hi(hn_gm) == Lo(hn_gm))
						? hn_gm
						: Lo(hn_gm) /* gm verbatim to low pixel */ |
						Raise(Average(Hi(hn_gm), Lo(hn_gm))) /* ghmn to high pixel */;

					// Generate io_hino from h_g, j_i, n_m and p_o.
					uint32_t io_hn =
						Hi(hn_gm) /* hn verbatim to low pixel */ |
						Raise(Average(Lo(j_i), Lo(p_o))) /* io to high pixel */;
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 2 + 4) = likely(Hi(io_hn) == Lo(io_hn))
						? io_hn
						: Raise(Hi(io_hn)) /* io verbatim to high pixel */ |
						Average(Hi(io_hn), Lo(io_hn)) /* hino to low pixel */;

					// Generate lr_klqr from l_k and r_q.
					uint32_t lr_kq = Average32(l_k, r_q);
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 2 + 12) = likely(Hi(lr_kq) == Lo(lr_kq))
						? lr_kq
						: Raise(Hi(lr_kq)) /* lr verbatim to high pixel */ |
						Average(Hi(lr_kq), Lo(lr_kq)) /* klqr to low pixel */;

					// Generate jkpq_jp from j_i, l_k, p_o and r_q.
					uint32_t kq_jp =
						Raise(Lo(lr_kq)) /* kq verbatim to high pixel */ |
						Average(Hi(j_i), Hi(p_o)) /* jp to low pixel */;
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 2 + 8) = likely(Hi(kq_jp) == Lo(kq_jp))
						? kq_jp
						: Lo(kq_jp) /* jp verbatim to low pixel */ |
						Raise(Average(Hi(kq_jp), Lo(kq_jp))) /* jkpq to high pixel */;

					// -- Row 4 --
					// Generate mn_m from n_m.
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 3) = likely(Hi(n_m) == Lo(n_m))
						? n_m
						: Lo(n_m) /* 'm' verbatim to low pixel */ |
						Raise(Average(Hi(n_m), Lo(n_m))) /* mn to high pixel */;

					// Generate o_no from n_m and p_o.
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 3 + 4) = likely(Hi(n_m) == Lo(p_o))
						? Lo(p_o) | Raise(Lo(p_o))
						: Raise(Lo(p_o)) /* 'o' verbatim to high pixel */ |
						Average(Lo(p_o), Hi(n_m)) /* no to low pixel */;

					// Generate pq_p from p_o and r_q.
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 3 + 8) = likely(Hi(p_o) == Lo(r_q))
						? Lo(r_q) | Raise(Lo(r_q))
						: Hi(p_o) /* 'p' verbatim to low pixel */ |
						Raise(Average(Hi(p_o), Lo(r_q))) /* pq to high pixel */;

					// Generate r_qr from r_q.
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 3 + 12) = likely(Hi(r_q) == Lo(r_q))
						? r_q
						: Raise(Hi(r_q)) /* 'r' verbatim to high pixel */ |
						Average(Hi(r_q), Lo(r_q)) /* qr to low pixel */;
				}
			}

			from += 6;
			to += 8;
		}

		// Skip past the waste at the end of the first line, if any,
		// then past 2 whole lines of source and 3 of destination.
		from = (uint16_t*) ((uint8_t*) from + src_skip + 2 * src_pitch);
		to   = (uint16_t*) ((uint8_t*) to   + dst_skip + 3 * dst_pitch);
	}
}

/*
 * Approximately bilinear scaler, 256x224 to 320x240
 *
 * Copyright (C) 2014 hi-ban, Nebuleon <nebuleon.fumika@gmail.com>
 *
 * This function and all auxiliary functions are free software; you can
 * redistribute them and/or modify them under the terms of the GNU Lesser
 * General Public License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * These functions are distributed in the hope that they will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

// Support math
#define Half(A) (((A) >> 1) & 0x7BEF)
#define Quarter(A) (((A) >> 2) & 0x39E7)
// Error correction expressions to piece back the lower bits together
#define RestHalf(A) ((A) & 0x0821)
#define RestQuarter(A) ((A) & 0x1863)

// Error correction expressions for quarters of pixels
#define Corr1_3(A, B)     Quarter(RestQuarter(A) + (RestHalf(B) << 1) + RestQuarter(B))
#define Corr3_1(A, B)     Quarter((RestHalf(A) << 1) + RestQuarter(A) + RestQuarter(B))

// Error correction expressions for halves
#define Corr1_1(A, B)     ((A) & (B) & 0x0821)

// Quarters
#define Weight1_3(A, B)   (Quarter(A) + Half(B) + Quarter(B) + Corr1_3(A, B))
#define Weight3_1(A, B)   (Half(A) + Quarter(A) + Quarter(B) + Corr3_1(A, B))

// Halves
#define Weight1_1(A, B)   (Half(A) + Half(B) + Corr1_1(A, B))

/* Upscales a 256x224 image to 320x240 using an approximate bilinear
 * resampling algorithm that only uses integer math.
 *
 * Input:
 *   src: A packed 256x224 pixel image. The pixel format of this image is
 *     RGB 565.
 *   width: The width of the source image. Should always be 256.
 * Output:
 *   dst: A packed 320x240 pixel image. The pixel format of this image is
 *     RGB 565.
 */
void scale_bilinearish_256x224_320x240(unsigned src_x, unsigned src_y, size_t src_pitch, const void *src, void *dst)
{
	uint16_t* Src16 = (uint16_t*) src;
	uint16_t* Dst16 = (uint16_t*) dst;
	unsigned pxpitch = src_pitch / 2;
	// There are 64 blocks of 4 pixels horizontally, and 14 of 16 vertically.
	// Each block of 4x16 becomes 5x17.
	uint32_t BlockX, BlockY;
	uint16_t* BlockSrc;
	uint16_t* BlockDst;
	for (BlockY = 0; BlockY < 14; BlockY++)
	{
		BlockSrc = Src16 + BlockY * pxpitch * 16;
		BlockDst = Dst16 + BlockY * 320 * 17;
		for (BlockX = 0; BlockX < 64; BlockX++)
		{
			/* Horizontally:
			 * Before(4):
			 * (a)(b)(c)(d)
			 * After(5):
			 * (a)(abbb)(bc)(cccd)(d)
			 *
			 * Vertically:
			 * Before(16): After(17):
			 * (a)       (a)
			 * (b)       (b)
			 * (c)       (c)
			 * (d)       (cddd)
			 * (e)       (deee)
			 * (f)       (efff)
			 * (g)       (fggg)
			 * (h)       (ghhh)
			 * (i)       (hi)
			 * (j)       (iiij)
			 * (k)       (jjjk)
			 * (l)       (kkkl)
			 * (m)       (lllm)
			 * (n)       (mmmn)
			 * (o)       (n)
			 * (p)       (o)
			 *           (p)
			 */

			// -- Row 1 --
			uint16_t  _1 = *(BlockSrc               );
			*(BlockDst               ) = _1;
			uint16_t  _2 = *(BlockSrc            + 1);
			*(BlockDst            + 1) = Weight1_3( _1,  _2);
			uint16_t  _3 = *(BlockSrc            + 2);
			*(BlockDst            + 2) = Weight1_1( _2,  _3);
			uint16_t  _4 = *(BlockSrc            + 3);
			*(BlockDst            + 3) = Weight3_1( _3,  _4);
			*(BlockDst            + 4) = _4;

			// -- Row 2 --
			uint16_t  _5 = *(BlockSrc + pxpitch *  1    );
			*(BlockDst + 320 *  1    ) = _5;
			uint16_t  _6 = *(BlockSrc + pxpitch *  1 + 1);
			*(BlockDst + 320 *  1 + 1) = Weight1_3( _5,  _6);
			uint16_t  _7 = *(BlockSrc + pxpitch *  1 + 2);
			*(BlockDst + 320 *  1 + 2) = Weight1_1( _6,  _7);
			uint16_t  _8 = *(BlockSrc + pxpitch *  1 + 3);
			*(BlockDst + 320 *  1 + 3) = Weight3_1( _7,  _8);
			*(BlockDst + 320 *  1 + 4) = _8;

			// -- Row 3 --
			uint16_t  _9 = *(BlockSrc + pxpitch *  2    );
			*(BlockDst + 320 *  2    ) = _9;
			uint16_t  _10 = *(BlockSrc + pxpitch *  2 + 1);
			*(BlockDst + 320 *  2 + 1) = Weight1_3( _9, _10);
			uint16_t  _11 = *(BlockSrc + pxpitch *  2 + 2);
			*(BlockDst + 320 *  2 + 2) = Weight1_1(_10, _11);
			uint16_t  _12 = *(BlockSrc + pxpitch *  2 + 3);
			*(BlockDst + 320 *  2 + 3) = Weight3_1(_11, _12);
			*(BlockDst + 320 *  2 + 4) = _12;

			// -- Row 4 --
			uint16_t _13 = *(BlockSrc + pxpitch *  3    );
			*(BlockDst + 320 *  3    ) = Weight1_3( _9, _13);
			uint16_t _14 = *(BlockSrc + pxpitch *  3 + 1);
			*(BlockDst + 320 *  3 + 1) = Weight1_3(Weight1_3( _9, _10), Weight1_3(_13, _14));
			uint16_t _15 = *(BlockSrc + pxpitch *  3 + 2);
			*(BlockDst + 320 *  3 + 2) = Weight1_3(Weight1_1(_10, _11), Weight1_1(_14, _15));
			uint16_t _16 = *(BlockSrc + pxpitch *  3 + 3);
			*(BlockDst + 320 *  3 + 3) = Weight1_3(Weight3_1(_11, _12), Weight3_1(_15, _16));
			*(BlockDst + 320 *  3 + 4) = Weight1_3(_12, _16);

			// -- Row 5 --
			uint16_t _17 = *(BlockSrc + pxpitch *  4    );
			*(BlockDst + 320 *  4    ) = Weight1_3(_13, _17);
			uint16_t _18 = *(BlockSrc + pxpitch *  4 + 1);
			*(BlockDst + 320 *  4 + 1) = Weight1_3(Weight1_3(_13, _14), Weight1_3(_17, _18));
			uint16_t _19 = *(BlockSrc + pxpitch *  4 + 2);
			*(BlockDst + 320 *  4 + 2) = Weight1_3(Weight1_1(_14, _15), Weight1_1(_18, _19));
			uint16_t _20 = *(BlockSrc + pxpitch *  4 + 3);
			*(BlockDst + 320 *  4 + 3) = Weight1_3(Weight3_1(_15, _16), Weight3_1(_19, _20));
			*(BlockDst + 320 *  4 + 4) = Weight1_3(_16, _20);

			// -- Row 6 --
			uint16_t _21 = *(BlockSrc + pxpitch *  5    );
			*(BlockDst + 320 *  5    ) = Weight1_3(_17, _21);
			uint16_t _22 = *(BlockSrc + pxpitch *  5 + 1);
			*(BlockDst + 320 *  5 + 1) = Weight1_3(Weight1_3(_17, _18), Weight1_3(_21, _22));
			uint16_t _23 = *(BlockSrc + pxpitch *  5 + 2);
			*(BlockDst + 320 *  5 + 2) = Weight1_3(Weight1_1(_18, _19), Weight1_1(_22, _23));
			uint16_t _24 = *(BlockSrc + pxpitch *  5 + 3);
			*(BlockDst + 320 *  5 + 3) = Weight1_3(Weight3_1(_19, _20), Weight3_1(_23, _24));
			*(BlockDst + 320 *  5 + 4) = Weight1_3(_20, _24);

			// -- Row 7 --
			uint16_t _25 = *(BlockSrc + pxpitch *  6    );
			*(BlockDst + 320 *  6    ) = Weight1_3(_21, _25);
			uint16_t _26 = *(BlockSrc + pxpitch *  6 + 1);
			*(BlockDst + 320 *  6 + 1) = Weight1_3(Weight1_3(_21, _22), Weight1_3(_25, _26));
			uint16_t _27 = *(BlockSrc + pxpitch *  6 + 2);
			*(BlockDst + 320 *  6 + 2) = Weight1_3(Weight1_1(_22, _23), Weight1_1(_26, _27));
			uint16_t _28 = *(BlockSrc + pxpitch *  6 + 3);
			*(BlockDst + 320 *  6 + 3) = Weight1_3(Weight3_1(_23, _24), Weight3_1(_27, _28));
			*(BlockDst + 320 *  6 + 4) = Weight1_3(_24, _28);

			// -- Row 8 --
			uint16_t _29 = *(BlockSrc + pxpitch *  7    );
			*(BlockDst + 320 *  7    ) = Weight1_3(_25, _29);
			uint16_t _30 = *(BlockSrc + pxpitch *  7 + 1);
			*(BlockDst + 320 *  7 + 1) = Weight1_3(Weight1_3(_25, _26), Weight1_3(_29, _30));
			uint16_t _31 = *(BlockSrc + pxpitch *  7 + 2);
			*(BlockDst + 320 *  7 + 2) = Weight1_3(Weight1_1(_26, _27), Weight1_1(_30, _31));
			uint16_t _32 = *(BlockSrc + pxpitch *  7 + 3);
			*(BlockDst + 320 *  7 + 3) = Weight1_3(Weight3_1(_27, _28), Weight3_1(_31, _32));
			*(BlockDst + 320 *  7 + 4) = Weight1_3(_28, _32);

			// -- Row 9 --
			uint16_t _33 = *(BlockSrc + pxpitch *  8    );
			*(BlockDst + 320 *  8    ) = Weight1_1(_29, _33);
			uint16_t _34 = *(BlockSrc + pxpitch *  8 + 1);
			*(BlockDst + 320 *  8 + 1) = Weight1_1(Weight1_3(_29, _30), Weight1_3(_33, _34));
			uint16_t _35 = *(BlockSrc + pxpitch *  8 + 2);
			*(BlockDst + 320 *  8 + 2) = Weight1_1(Weight1_1(_30, _31), Weight1_1(_34, _35));
			uint16_t _36 = *(BlockSrc + pxpitch *  8 + 3);
			*(BlockDst + 320 *  8 + 3) = Weight1_1(Weight3_1(_31, _32), Weight3_1(_35, _36));
			*(BlockDst + 320 *  8 + 4) = Weight1_1(_32, _36);

			// -- Row 10 --
			uint16_t _37 = *(BlockSrc + pxpitch *  9    );
			*(BlockDst + 320 *  9    ) = Weight3_1(_33, _37);
			uint16_t _38 = *(BlockSrc + pxpitch *  9 + 1);
			*(BlockDst + 320 *  9 + 1) = Weight3_1(Weight1_3(_33, _34), Weight1_3(_37, _38));
			uint16_t _39 = *(BlockSrc + pxpitch *  9 + 2);
			*(BlockDst + 320 *  9 + 2) = Weight3_1(Weight1_1(_34, _35), Weight1_1(_38, _39));
			uint16_t _40 = *(BlockSrc + pxpitch *  9 + 3);
			*(BlockDst + 320 *  9 + 3) = Weight3_1(Weight3_1(_35, _36), Weight3_1(_39, _40));
			*(BlockDst + 320 *  9 + 4) = Weight3_1(_36, _40);

			// -- Row 11 --
			uint16_t _41 = *(BlockSrc + pxpitch * 10    );
			*(BlockDst + 320 * 10    ) = Weight3_1(_37, _41);
			uint16_t _42 = *(BlockSrc + pxpitch * 10 + 1);
			*(BlockDst + 320 * 10 + 1) = Weight3_1(Weight1_3(_37, _38), Weight1_3(_41, _42));
			uint16_t _43 = *(BlockSrc + pxpitch * 10 + 2);
			*(BlockDst + 320 * 10 + 2) = Weight3_1(Weight1_1(_38, _39), Weight1_1(_42, _43));
			uint16_t _44 = *(BlockSrc + pxpitch * 10 + 3);
			*(BlockDst + 320 * 10 + 3) = Weight3_1(Weight3_1(_39, _40), Weight3_1(_43, _44));
			*(BlockDst + 320 * 10 + 4) = Weight3_1(_40, _44);

			// -- Row 12 --
			uint16_t _45 = *(BlockSrc + pxpitch * 11    );
			*(BlockDst + 320 * 11    ) = Weight3_1(_41, _45);
			uint16_t _46 = *(BlockSrc + pxpitch * 11 + 1);
			*(BlockDst + 320 * 11 + 1) = Weight3_1(Weight1_3(_41, _42), Weight1_3(_45, _46));
			uint16_t _47 = *(BlockSrc + pxpitch * 11 + 2);
			*(BlockDst + 320 * 11 + 2) = Weight3_1(Weight1_1(_42, _43), Weight1_1(_46, _47));
			uint16_t _48 = *(BlockSrc + pxpitch * 11 + 3);
			*(BlockDst + 320 * 11 + 3) = Weight3_1(Weight3_1(_43, _44), Weight3_1(_47, _48));
			*(BlockDst + 320 * 11 + 4) = Weight3_1(_44, _48);

			// -- Row 13 --
			uint16_t _49 = *(BlockSrc + pxpitch * 12    );
			*(BlockDst + 320 * 12    ) = Weight3_1(_45, _49);
			uint16_t _50 = *(BlockSrc + pxpitch * 12 + 1);
			*(BlockDst + 320 * 12 + 1) = Weight3_1(Weight1_3(_45, _46), Weight1_3(_49, _50));
			uint16_t _51 = *(BlockSrc + pxpitch * 12 + 2);
			*(BlockDst + 320 * 12 + 2) = Weight3_1(Weight1_1(_46, _47), Weight1_1(_50, _51));
			uint16_t _52 = *(BlockSrc + pxpitch * 12 + 3);
			*(BlockDst + 320 * 12 + 3) = Weight3_1(Weight3_1(_47, _48), Weight3_1(_51, _52));
			*(BlockDst + 320 * 12 + 4) = Weight3_1(_48, _52);

			// -- Row 14 --
			uint16_t _53 = *(BlockSrc + pxpitch * 13    );
			*(BlockDst + 320 * 13    ) = Weight3_1(_49, _53);
			uint16_t _54 = *(BlockSrc + pxpitch * 13 + 1);
			*(BlockDst + 320 * 13 + 1) = Weight3_1(Weight1_3(_49, _50), Weight1_3(_53, _54));
			uint16_t _55 = *(BlockSrc + pxpitch * 13 + 2);
			*(BlockDst + 320 * 13 + 2) = Weight3_1(Weight1_1(_50, _51), Weight1_1(_54, _55));
			uint16_t _56 = *(BlockSrc + pxpitch * 13 + 3);
			*(BlockDst + 320 * 13 + 3) = Weight3_1(Weight3_1(_51, _52), Weight3_1(_55, _56));
			*(BlockDst + 320 * 13 + 4) = Weight3_1(_52, _56);

			// -- Row 15 --
			*(BlockDst + 320 * 14    ) = _53;
			*(BlockDst + 320 * 14 + 1) = Weight1_3(_53, _54);
			*(BlockDst + 320 * 14 + 2) = Weight1_1(_54, _55);
			*(BlockDst + 320 * 14 + 3) = Weight3_1(_55, _56);
			*(BlockDst + 320 * 14 + 4) = _56;

			// -- Row 16 --
			uint16_t _57 = *(BlockSrc + pxpitch * 14    );
			*(BlockDst + 320 * 15    ) = _57;
			uint16_t _58 = *(BlockSrc + pxpitch * 14 + 1);
			*(BlockDst + 320 * 15 + 1) = Weight1_3(_57, _58);
			uint16_t _59 = *(BlockSrc + pxpitch * 14 + 2);
			*(BlockDst + 320 * 15 + 2) = Weight1_1(_58, _59);
			uint16_t _60 = *(BlockSrc + pxpitch * 14 + 3);
			*(BlockDst + 320 * 15 + 3) = Weight3_1(_59, _60);
			*(BlockDst + 320 * 15 + 4) = _60;

			// -- Row 17 --
			uint16_t _61 = *(BlockSrc + pxpitch * 15    );
			*(BlockDst + 320 * 16    ) = _61;
			uint16_t _62 = *(BlockSrc + pxpitch * 15 + 1);
			*(BlockDst + 320 * 16 + 1) = Weight1_3(_61, _62);
			uint16_t _63 = *(BlockSrc + pxpitch * 15 + 2);
			*(BlockDst + 320 * 16 + 2) = Weight1_1(_62, _63);
			uint16_t _64 = *(BlockSrc + pxpitch * 15 + 3);
			*(BlockDst + 320 * 16 + 3) = Weight3_1(_63, _64);
			*(BlockDst + 320 * 16 + 4) = _64;

			BlockSrc += 4;
			BlockDst += 5;
		}
	}
}

void scale_bilinearish_256x240_320x240(unsigned src_x, unsigned src_y, size_t src_pitch, const void *src, void *dst)
{
	uint16_t* Src16 = (uint16_t*) src;
	uint16_t* Dst16 = (uint16_t*) dst;
	unsigned pxpitch = src_pitch / 2;
	// There are 64 blocks of 4 pixels horizontally, and 239 of 1 vertically.
	// Each block of 4x1 becomes 5x1.
	uint32_t BlockX, BlockY;
	uint16_t* BlockSrc;
	uint16_t* BlockDst;
	for (BlockY = 0; BlockY < 239; BlockY++)
	{
		BlockSrc = Src16 + BlockY * pxpitch * 1;
		BlockDst = Dst16 + BlockY * 320 * 1;
		for (BlockX = 0; BlockX < 64; BlockX++)
		{
			/* Horizontally:
			 * Before(4):
			 * (a)(b)(c)(d)
			 * After(5):
			 * (a)(abbb)(bc)(cccd)(d)
			 */

			// -- Row 1 --
			uint16_t  _1 = *(BlockSrc               );
			*(BlockDst               ) = _1;
			uint16_t  _2 = *(BlockSrc            + 1);
			*(BlockDst            + 1) = Weight1_3( _1,  _2);
			uint16_t  _3 = *(BlockSrc            + 2);
			*(BlockDst            + 2) = Weight1_1( _2,  _3);
			uint16_t  _4 = *(BlockSrc            + 3);
			*(BlockDst            + 3) = Weight3_1( _3,  _4);
			*(BlockDst            + 4) = _4;

			BlockSrc += 4;
			BlockDst += 5;
		}
	}
}

static void scale_select_scaler(unsigned w, unsigned h, size_t pitch) {
	scaler = NULL;

	if (scale_size == SCALE_SIZE_FULL) {
		dst_w = SCREEN_WIDTH;
		dst_h = SCREEN_HEIGHT;
		dst_offs = 0;
	} else if (scale_size == SCALE_SIZE_ASPECT ||
	           (scale_size == SCALE_SIZE_NONE && w > SCREEN_WIDTH || h > SCREEN_HEIGHT)) {
		dst_w = SCREEN_WIDTH;
		dst_h = SCREEN_WIDTH / aspect_ratio + 0.5;
		dst_offs = ((SCREEN_HEIGHT-dst_h)/2) * SCREEN_PITCH;

		if (dst_h > SCREEN_HEIGHT) {
			dst_w = SCREEN_HEIGHT * aspect_ratio + 0.5;
			dst_h = SCREEN_HEIGHT;
			dst_offs = ((SCREEN_WIDTH-dst_w)/2);
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
		} else if (scale_filter == SCALE_FILTER_SMOOTH) {
			if (scale_size == SCALE_SIZE_ASPECT) {
				scaler = scale_bilinearish_240x160_320x213;
			} else {
				scaler = scale_bilinearish_240x160_320x240;
			}
			return;
		}
	}

	if (!scaler && aspect_ratio == 4.0f / 3.0f && w == 256) {
		if (scale_filter == SCALE_FILTER_SHARP) {
			scaler = scale_sharp_256xXXX_320xXXX;
		} else if (scale_filter == SCALE_FILTER_SMOOTH) {
			if (h == 240) {
				scaler = scale_bilinearish_256x240_320x240;
			} else if (h == 224) {
				scaler = scale_bilinearish_256x224_320x240;
			}
		}
	}

	if (!scaler && scale_filter == SCALE_FILTER_NEAREST) {
		scaler = scale_nearest;
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
		scale_select_scaler(w, h, pitch);
		prev.w = w; prev.h = h; prev.pitch = pitch;
	}

	scaler(w, h, pitch, src, dst);
}
