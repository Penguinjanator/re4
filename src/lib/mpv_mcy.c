/* CRI Sofdec MPEG video: 16x16 block motion compensation, "tuned C" versions. One reference block
 * is copied (1p), horizontally (H2) / vertically (V2) / four-point (4p) half-pel averaged into the
 * word-packed destination (four 8x8 blocks: rows 0-7 into blocks 0/1 at d[0]/d[1] and d[16]/d[17],
 * rows 8-15 into blocks 2/3 after the +0x40 step); the source alignment selects the load strategy.
 * Pure C reconstruction of the original's instruction stream (CRI paired-single kernels pass 1): the
 * words/bytes of a row are read into variables before the stores (each byte is loaded once), the
 * 8-iteration copy loops are unrolled by the compiler (1p case 0 is the hand software-pipelined
 * two-row form), the 16-row loops keep the `i == 7` block step. Residue: instruction schedule and
 * register assignment (see AGENTS.md "CRI paired-single kernels pass 1"). */
#include "cri_xpt.h"
#include "mpv.h"

/* four-point average of the pixel pairs (a, a+1) x (b, b+1), packed as four bytes */
#define MPVMC16_AVG4(p0, p1, p2, p3) \
	((((p0) << 22) & 0xFF000000) | (((p1) << 14) & 0x00FF0000) | (((p2) << 6) & 0x0000FF00) | (((p3) >> 2) & 0x000000FF))

/* byte-wise average of two packed words */
#define MPVMC16_AVG2(w, a, x) (((w) & (a)) + (((x) & 0xFEFEFEFE) >> 1) + ((x) & 0x01010101))
/* the same through mask VARIABLES (propagated to the same constants): the frontend then keeps the
 * target's association `(w & a) + (sh + (x & m2))`; with literal masks it rebuilds `sh + ((w & a) + (x & m2))` */
#define MPVMC16_AVG2V(w, a, x, m1, m2) (((w) & (a)) + (((x) & (m1)) >> 1) + ((x) & (m2)))

void MPVMC16_OneRef4p_TuneC(MPVMC *mc)
{
	/* Three rotating pixel pairs (a0/b0, a1/b1, a2/b2: pixel k lives in pair k % 3) and the sums p0..p7
	 * reused for both halves (the 9th sum is p0 again, computed after the d[1] store like a copy-pasted
	 * second half): the first webs of the pairs are own locals and every later web a range-split
	 * frontend temporary, which gives the original's register classes -- a6, a7, a2, a8 (the pixels
	 * whose degree stays >= 29 at their scan) and p2, p3, p4 form the level coloured first (r7-r12),
	 * the long-lived pixels a3/b2/a4/a5/b5 and the sums are coloured last, and the second half's
	 * sums stay variables (q/p in one register) except p6' (CRI SWAR kernels pass 12). The pixel
	 * words are declared BEFORE the sums and the loop variables (pass 10: the five loop variables
	 * must be the only nodes of the top level). Residue: the original's first block ends after the
	 * 9th sum (its initial-code count crosses 100 there; ours crosses it two statements later). */
	Uint32 a0, b0, a1, b1, a2, b2;
	Uint32 p0, p1, p2, p3, p4, p5, p6, p7;
	Sint32 i;
	Sint32 stride;
	Uint8 *s0;
	Uint8 *s1;
	Uint32 *d;

	stride = mc->stride;
	s0 = mc->src;
	s1 = mc->src2;
	d = mc->dst;
	for (i = 0; i < 16; i++) {
		__dcbt(s1, stride);
		a0 = s0[0];
		b0 = s1[0];
		a1 = s0[1];
		b1 = s1[1];
		p0 = (Uint32)a0 + (Uint32)a1 + (Uint32)b0 + (Uint32)b1 + 2;
		a2 = s0[2];
		b2 = s1[2];
		p1 = (Uint32)a1 + (Uint32)a2 + (Uint32)b1 + (Uint32)b2 + 2;
		a0 = s0[3];
		b0 = s1[3];
		p2 = (Uint32)a2 + (Uint32)a0 + (Uint32)b2 + (Uint32)b0 + 2;
		a1 = s0[4];
		b1 = s1[4];
		p3 = (Uint32)a0 + (Uint32)a1 + (Uint32)b0 + (Uint32)b1 + 2;
		a2 = s0[5];
		b2 = s1[5];
		p4 = (Uint32)a1 + (Uint32)a2 + (Uint32)b1 + (Uint32)b2 + 2;
		a0 = s0[6];
		b0 = s1[6];
		p5 = (Uint32)a2 + (Uint32)a0 + (Uint32)b2 + (Uint32)b0 + 2;
		a1 = s0[7];
		b1 = s1[7];
		p6 = (Uint32)a0 + (Uint32)a1 + (Uint32)b0 + (Uint32)b1 + 2;
		a2 = s0[8];
		b2 = s1[8];
		p7 = (Uint32)a1 + (Uint32)a2 + (Uint32)b1 + (Uint32)b2 + 2;
		d[0] = MPVMC16_AVG4(p0, p1, p2, p3);
		d[1] = MPVMC16_AVG4(p4, p5, p6, p7);
		a0 = s0[9];
		b0 = s1[9];
		p0 = (Uint32)a2 + (Uint32)a0 + (Uint32)b2 + (Uint32)b0 + 2;
		a1 = s0[10];
		b1 = s1[10];
		p1 = (Uint32)a0 + (Uint32)a1 + (Uint32)b0 + (Uint32)b1 + 2;
		a2 = s0[11];
		b2 = s1[11];
		p2 = (Uint32)a1 + (Uint32)a2 + (Uint32)b1 + (Uint32)b2 + 2;
		a0 = s0[12];
		b0 = s1[12];
		p3 = (Uint32)a2 + (Uint32)a0 + (Uint32)b2 + (Uint32)b0 + 2;
		a1 = s0[13];
		b1 = s1[13];
		p4 = (Uint32)a0 + (Uint32)a1 + (Uint32)b0 + (Uint32)b1 + 2;
		a2 = s0[14];
		b2 = s1[14];
		p5 = (Uint32)a1 + (Uint32)a2 + (Uint32)b1 + (Uint32)b2 + 2;
		a0 = s0[15];
		b0 = s1[15];
		p6 = (Uint32)a2 + (Uint32)a0 + (Uint32)b2 + (Uint32)b0 + 2;
		a1 = s0[16];
		b1 = s1[16];
		p7 = (Uint32)a0 + (Uint32)a1 + (Uint32)b0 + (Uint32)b1 + 2;
		d[16] = MPVMC16_AVG4(p0, p1, p2, p3);
		d[17] = MPVMC16_AVG4(p4, p5, p6, p7);
		s0 += stride;
		s1 += stride;
		d += 2;
		if (i == 7) {
			d += 16;
		}
	}
}

void MPVMC16_OneRefH2_TuneC(MPVMC *mc)
{
	Sint32 i;
	Uint8 *s = mc->src;
	Sint32 stride = mc->stride;
	Uint32 *d = mc->dst;
	Uint32 w0, w1, w2, w3, w4, a0, a1, a2, a3, x0, x1, x2, x3;
	Uint32 m1 = 0xFEFEFEFE;
	Uint32 m2 = 0x01010101;

	switch ((Uint32)s & 3) {
	case 0:
		for (i = 0; i < 16; i++) {
			__dcbt(s, stride);
			w0 = ((Uint32 *)s)[0];
			w1 = ((Uint32 *)s)[1];
			w2 = ((Uint32 *)s)[2];
			w3 = ((Uint32 *)s)[3];
						a0 = (w0 << 8) | (w1 >> 24);
			a1 = (w1 << 8) | (w2 >> 24);
			a2 = (w2 << 8) | (w3 >> 24);
			a3 = s[16];
			a3 = (w3 << 8) | a3;
			x0 = w0 ^ a0;
			x1 = w1 ^ a1;
			x2 = w2 ^ a2;
			x3 = w3 ^ a3;
			d[0] = MPVMC16_AVG2V(w0, a0, x0, m1, m2);
			d[1] = MPVMC16_AVG2V(w1, a1, x1, m1, m2);
			d[16] = MPVMC16_AVG2V(w2, a2, x2, m1, m2);
			d[17] = MPVMC16_AVG2V(w3, a3, x3, m1, m2);
			s += stride;
			d += 2;
			if (i == 7) {
				d += 16;
			}
		}
		break;
	case 1:
		s -= 1;
		for (i = 0; i < 16; i++) {
			__dcbt(s, stride);
			w0 = ((Uint32 *)s)[0];
			w1 = ((Uint32 *)s)[1];
			w2 = ((Uint32 *)s)[2];
			w3 = ((Uint32 *)s)[3];
			w4 = ((Uint32 *)s)[4];
			a0 = (w0 << 8) | (w1 >> 24);
			w0 = (w0 << 16) | (w1 >> 16);
			a1 = (w1 << 8) | (w2 >> 24);
			w1 = (w1 << 16) | (w2 >> 16);
			a2 = (w2 << 8) | (w3 >> 24);
			w2 = (w2 << 16) | (w3 >> 16);
			a3 = (w3 << 8) | (w4 >> 24);
			w3 = (w3 << 16) | (w4 >> 16);
			x0 = a0 ^ w0;
			x1 = a1 ^ w1;
			x2 = a2 ^ w2;
			x3 = a3 ^ w3;
			d[0] = MPVMC16_AVG2V(a0, w0, x0, m1, m2);
			d[1] = MPVMC16_AVG2V(a1, w1, x1, m1, m2);
			d[16] = MPVMC16_AVG2V(a2, w2, x2, m1, m2);
			d[17] = MPVMC16_AVG2V(a3, w3, x3, m1, m2);
			s += stride;
			d += 2;
			if (i == 7) {
				d += 16;
			}
		}
		break;
	case 2:
		s -= 2;
		for (i = 0; i < 16; i++) {
			__dcbt(s, stride);
			w0 = ((Uint32 *)s)[0];
			w1 = ((Uint32 *)s)[1];
			w2 = ((Uint32 *)s)[2];
			w3 = ((Uint32 *)s)[3];
			w4 = ((Uint32 *)s)[4];
			a0 = (w0 << 24) | (w1 >> 8);
			w0 = (w0 << 16) | (w1 >> 16);
			a1 = (w1 << 24) | (w2 >> 8);
			w1 = (w1 << 16) | (w2 >> 16);
			a2 = (w2 << 24) | (w3 >> 8);
			w2 = (w2 << 16) | (w3 >> 16);
			a3 = (w3 << 24) | (w4 >> 8);
			w3 = (w3 << 16) | (w4 >> 16);
			x0 = w0 ^ a0;
			x1 = w1 ^ a1;
			x2 = w2 ^ a2;
			x3 = w3 ^ a3;
			d[0] = MPVMC16_AVG2(w0, a0, x0);
			d[1] = MPVMC16_AVG2(w1, a1, x1);
			d[16] = MPVMC16_AVG2(w2, a2, x2);
			d[17] = MPVMC16_AVG2(w3, a3, x3);
			s += stride;
			d += 2;
			if (i == 7) {
				d += 16;
			}
		}
		break;
	case 3:
		s -= 3;
		for (i = 0; i < 16; i++) {
			__dcbt(s, stride);
			w0 = ((Uint32 *)s)[0];
			a0 = ((Uint32 *)s)[1];
			a1 = ((Uint32 *)s)[2];
			a2 = ((Uint32 *)s)[3];
			a3 = ((Uint32 *)s)[4];
			w0 = (w0 << 24) | (a0 >> 8);
			w1 = (a0 << 24) | (a1 >> 8);
			w2 = (a1 << 24) | (a2 >> 8);
			w3 = (a2 << 24) | (a3 >> 8);
			x0 = w0 ^ a0;
			x1 = w1 ^ a1;
			x2 = w2 ^ a2;
			x3 = w3 ^ a3;
			d[0] = MPVMC16_AVG2(w0, a0, x0);
			d[1] = MPVMC16_AVG2(w1, a1, x1);
			d[16] = MPVMC16_AVG2(w2, a2, x2);
			d[17] = MPVMC16_AVG2(w3, a3, x3);
			s += stride;
			d += 2;
			if (i == 7) {
				d += 16;
			}
		}
		break;
	}
}

void MPVMC16_OneRefV2_TuneC(MPVMC *mc)
{
	Uint32 *d;
	Uint8 *s0;
	Uint8 *s1;
	Sint32 stride;
	Uint32 x2, x0, x3;
	Sint32 i;
	Uint32 w1, a1, w0, a0, w2, a2, w3, a3, x1, w4, a4;
	Uint32 m1 = 0xFEFEFEFE;
	Uint32 m2 = 0x01010101;

	s1 = mc->src2;
	s0 = mc->src;
	d = mc->dst;
	stride = mc->stride;
	__dcbt(s1, 0);
	switch ((Uint32)s0 & 3) {
	case 0:
		for (i = 0; i < 16; i++) {
			__dcbt(s1, stride);
			w0 = ((Uint32 *)s0)[0];
			a0 = ((Uint32 *)s1)[0];
			w1 = ((Uint32 *)s0)[1];
			a1 = ((Uint32 *)s1)[1];
			w2 = ((Uint32 *)s0)[2];
			a2 = ((Uint32 *)s1)[2];
			w3 = ((Uint32 *)s0)[3];
			a3 = ((Uint32 *)s1)[3];
			x0 = w0 ^ a0;
			x1 = w1 ^ a1;
			x2 = w2 ^ a2;
			x3 = w3 ^ a3;
			d[0] = MPVMC16_AVG2V(w0, a0, x0, m1, m2);
			d[1] = MPVMC16_AVG2V(w1, a1, x1, m1, m2);
			d[16] = MPVMC16_AVG2V(w2, a2, x2, m1, m2);
			d[17] = MPVMC16_AVG2V(w3, a3, x3, m1, m2);
			s0 += stride;
			s1 += stride;
			d += 2;
			if (i == 7) {
				d += 16;
			}
		}
		break;
	case 1:
		s0 -= 1;
		s1 -= 1;
		for (i = 0; i < 16; i++) {
			__dcbt(s1, stride);
			w1 = ((Uint32 *)s0)[1];
			a1 = ((Uint32 *)s1)[1];
			w2 = ((Uint32 *)s0)[2];
			w0 = ((Uint32 *)s0)[0];
			a0 = ((Uint32 *)s1)[0];
			a2 = ((Uint32 *)s1)[2];
			w3 = ((Uint32 *)s0)[3];
			w4 = s0[16];
			a3 = ((Uint32 *)s1)[3];
			a4 = s1[16];
			w0 = (w0 << 8) | (w1 >> 24);
			a0 = (a0 << 8) | (a1 >> 24);
			w1 = (w1 << 8) | (w2 >> 24);
			a1 = (a1 << 8) | (a2 >> 24);
			w2 = (w2 << 8) | (w3 >> 24);
			a2 = (a2 << 8) | (a3 >> 24);
			w3 = (w3 << 8) | w4;
			a3 = (a3 << 8) | a4;
			x0 = w0 ^ a0;
			x1 = w1 ^ a1;
			x2 = w2 ^ a2;
			x3 = w3 ^ a3;
			d[0] = MPVMC16_AVG2V(w0, a0, x0, m1, m2);
			d[1] = MPVMC16_AVG2V(w1, a1, x1, m1, m2);
			d[16] = MPVMC16_AVG2V(w2, a2, x2, m1, m2);
			d[17] = MPVMC16_AVG2V(w3, a3, x3, m1, m2);
			s0 += stride;
			s1 += stride;
			d += 2;
			if (i == 7) {
				d += 16;
			}
		}
		break;
	case 2:
		s0 -= 2;
		s1 -= 2;
		for (i = 0; i < 16; i++) {
			__dcbt(s1, stride);
			w1 = ((Uint32 *)s0)[1];
			a1 = ((Uint32 *)s1)[1];
			w2 = ((Uint32 *)s0)[2];
			w0 = ((Uint32 *)s0)[0];
			a0 = ((Uint32 *)s1)[0];
			a2 = ((Uint32 *)s1)[2];
			w3 = ((Uint32 *)s0)[3];
			w4 = *(Uint16 *)(s0 + 16);
			a3 = ((Uint32 *)s1)[3];
			a4 = *(Uint16 *)(s1 + 16);
			w0 = (w0 << 16) | (w1 >> 16);
			a0 = (a0 << 16) | (a1 >> 16);
			w1 = (w1 << 16) | (w2 >> 16);
			a1 = (a1 << 16) | (a2 >> 16);
			w2 = (w2 << 16) | (w3 >> 16);
			a2 = (a2 << 16) | (a3 >> 16);
			w3 = (w3 << 16) | w4;
			a3 = (a3 << 16) | a4;
			x0 = w0 ^ a0;
			x1 = w1 ^ a1;
			x2 = w2 ^ a2;
			x3 = w3 ^ a3;
			d[0] = MPVMC16_AVG2V(w0, a0, x0, m1, m2);
			d[1] = MPVMC16_AVG2V(w1, a1, x1, m1, m2);
			d[16] = MPVMC16_AVG2V(w2, a2, x2, m1, m2);
			d[17] = MPVMC16_AVG2V(w3, a3, x3, m1, m2);
			s0 += stride;
			s1 += stride;
			d += 2;
			if (i == 7) {
				d += 16;
			}
		}
		break;
	case 3:
		s0 -= 3;
		s1 -= 3;
		for (i = 0; i < 16; i++) {
			__dcbt(s1, stride);
			w1 = ((Uint32 *)s0)[1];
			a1 = ((Uint32 *)s1)[1];
			w2 = ((Uint32 *)s0)[2];
			w0 = ((Uint32 *)s0)[0];
			a0 = ((Uint32 *)s1)[0];
			a2 = ((Uint32 *)s1)[2];
			a3 = ((Uint32 *)s1)[3];
			a4 = ((Uint32 *)s1)[4];
			w4 = ((Uint32 *)s0)[4];
			w3 = ((Uint32 *)s0)[3];
			w0 = (w0 << 24) | (w1 >> 8);
			a0 = (a0 << 24) | (a1 >> 8);
			w1 = (w1 << 24) | (w2 >> 8);
			a1 = (a1 << 24) | (a2 >> 8);
			w2 = (w2 << 24) | (w3 >> 8);
			a2 = (a2 << 24) | (a3 >> 8);
			w3 = (w3 << 24) | (w4 >> 8);
			a3 = (a3 << 24) | (a4 >> 8);
			x0 = w0 ^ a0;
			x1 = w1 ^ a1;
			x2 = w2 ^ a2;
			x3 = w3 ^ a3;
			d[0] = MPVMC16_AVG2V(w0, a0, x0, m1, m2);
			d[1] = MPVMC16_AVG2V(w1, a1, x1, m1, m2);
			d[16] = MPVMC16_AVG2V(w2, a2, x2, m1, m2);
			d[17] = MPVMC16_AVG2V(w3, a3, x3, m1, m2);
			s0 += stride;
			s1 += stride;
			d += 2;
			if (i == 7) {
				d += 16;
			}
		}
		break;
	}
}

void MPVMC16_OneRef1p_TuneC(MPVMC *mc)
{
	Uint8 *s = mc->src;

	__dcbt(s, 0);
	switch ((Uint32)s & 7) {
	case 0: {
		Sint32 stride = mc->stride;
		Float64 *d = (Float64 *)mc->dst;
		Uint32 pitch = (Uint32)stride & ~7;
		Uint8 *p;
		Float64 a0, a1, b0, b1;

		a0 = ((Float64 *)s)[0];
		a1 = ((Float64 *)s)[1];
		p = s + pitch;
		b0 = ((Float64 *)p)[0];
		b1 = ((Float64 *)p)[1];
		p += pitch;
		d[0] = a0;
		d[8] = a1;
		a0 = ((Float64 *)p)[0];
		a1 = ((Float64 *)p)[1];
		p += pitch;
		d[1] = b0;
		d[9] = b1;
		b0 = ((Float64 *)p)[0];
		b1 = ((Float64 *)p)[1];
		p += pitch;
		d[2] = a0;
		d[10] = a1;
		a0 = ((Float64 *)p)[0];
		a1 = ((Float64 *)p)[1];
		p += pitch;
		d[3] = b0;
		d[11] = b1;
		b0 = ((Float64 *)p)[0];
		b1 = ((Float64 *)p)[1];
		p += pitch;
		d[4] = a0;
		d[12] = a1;
		a0 = ((Float64 *)p)[0];
		a1 = ((Float64 *)p)[1];
		p += pitch;
		d[5] = b0;
		d[13] = b1;
		b0 = ((Float64 *)p)[0];
		b1 = ((Float64 *)p)[1];
		p += pitch;
		d[6] = a0;
		d[14] = a1;
		a0 = ((Float64 *)p)[0];
		a1 = ((Float64 *)p)[1];
		p += pitch;
		d[7] = b0;
		d[15] = b1;
		b0 = ((Float64 *)p)[0];
		b1 = ((Float64 *)p)[1];
		p += pitch;
		d[16] = a0;
		d[24] = a1;
		a0 = ((Float64 *)p)[0];
		a1 = ((Float64 *)p)[1];
		p += pitch;
		d[17] = b0;
		d[25] = b1;
		b0 = ((Float64 *)p)[0];
		b1 = ((Float64 *)p)[1];
		p += pitch;
		d[18] = a0;
		d[26] = a1;
		a0 = ((Float64 *)p)[0];
		a1 = ((Float64 *)p)[1];
		p += pitch;
		d[19] = b0;
		d[27] = b1;
		b0 = ((Float64 *)p)[0];
		b1 = ((Float64 *)p)[1];
		p += pitch;
		d[20] = a0;
		d[28] = a1;
		a0 = ((Float64 *)p)[0];
		a1 = ((Float64 *)p)[1];
		p += pitch;
		d[21] = b0;
		d[29] = b1;
		b0 = ((Float64 *)p)[0];
		b1 = ((Float64 *)p)[1];
		d[22] = a0;
		d[30] = a1;
		d[23] = b0;
		d[31] = b1;
		break;
	}
	case 4: {
		Sint32 stride = mc->stride;
		Sint32 i;
		Uint32 *d = mc->dst;
		Uint32 pitch = (Uint32)stride & ~3;
		Uint8 *p = s;
		Uint32 w0, w1, w2, w3;

		for (i = 0; i < 8; i++) {
			w0 = ((Uint32 *)p)[0];
			w1 = ((Uint32 *)p)[1];
			w2 = ((Uint32 *)p)[2];
			w3 = ((Uint32 *)p)[3];
			p += pitch;
			d[0] = w0;
			d[1] = w1;
			d[16] = w2;
			d[17] = w3;
			d += 2;
		}
		d += 16;
		for (i = 0; i < 8; i++) {
			w0 = ((Uint32 *)p)[0];
			w1 = ((Uint32 *)p)[1];
			w2 = ((Uint32 *)p)[2];
			w3 = ((Uint32 *)p)[3];
			p += pitch;
			d[0] = w0;
			d[1] = w1;
			d[16] = w2;
			d[17] = w3;
			d += 2;
		}
		break;
	}
	case 2:
	case 6: {
		Sint32 stride = mc->stride;
		Sint32 i;
		Uint32 *d = mc->dst;
		Uint8 *p = s;
		Uint32 h0, w0, w1, w2, h1;

		/* Unaligned rows (2/6, 1/5, 3/7) are a sliding window shifted in place: the words are
		 * loaded in address order, the source pointer is stepped BEFORE the packs (so the first
		 * word's single-use load cannot be substituted into its use and stays a variable), and
		 * each word is redefined with its packed value (so the pack webs cannot be sunk into the
		 * stores: they are frontend temporaries numbered in load order, coloured r0/r3/r4 before
		 * the loop's own locals). The pitch is the frontend-hoisted invariant, not a local. */
		for (i = 0; i < 16; i++) {
			h0 = *(Uint16 *)p;
			w0 = *(Uint32 *)(p + 2);
			w1 = *(Uint32 *)(p + 6);
			w2 = *(Uint32 *)(p + 10);
			h1 = *(Uint16 *)(p + 14);
			p += (Uint32)stride & ~1;
			h0 = (h0 << 16) | (w0 >> 16);
			w0 = (w0 << 16) | (w1 >> 16);
			w1 = (w1 << 16) | (w2 >> 16);
			w2 = (w2 << 16) | h1;
			d[0] = h0;
			d[1] = w0;
			d[16] = w1;
			d[17] = w2;
			d += 2;
			if (i == 7) {
				d += 16;
			}
		}
		break;
	}
	case 1:
	case 5: {
		Sint32 stride;
		Sint32 i;
		Uint32 *d;
		Uint8 *p = s;
		Uint32 w0, w1, w2, w3, b;

		/* dst before stride: the two preheader loads are scheduled in statement order */
		d = mc->dst;
		stride = mc->stride;
		for (i = 0; i < 16; i++) {
			__dcbt(p, stride);
			w0 = *(Uint32 *)(p - 1);
			w1 = *(Uint32 *)(p + 3);
			w2 = *(Uint32 *)(p + 7);
			w3 = *(Uint32 *)(p + 11);
			b = p[15];
			p += stride;
			w0 = (w0 << 8) | (w1 >> 24);
			w1 = (w1 << 8) | (w2 >> 24);
			w2 = (w2 << 8) | (w3 >> 24);
			w3 = (w3 << 8) | b;
			d[0] = w0;
			d[1] = w1;
			d[16] = w2;
			d[17] = w3;
			d += 2;
			if (i == 7) {
				d += 16;
			}
		}
		break;
	}
	case 3:
	case 7: {
		Sint32 stride;
		Sint32 i;
		Uint32 *d;
		Uint8 *p = s;
		Uint32 w0, w1, w2, w3, w4;

		d = mc->dst;
		stride = mc->stride;
		for (i = 0; i < 16; i++) {
			__dcbt(p, stride);
			w0 = *(Uint32 *)(p - 3);
			w1 = *(Uint32 *)(p + 1);
			w2 = *(Uint32 *)(p + 5);
			w3 = *(Uint32 *)(p + 9);
			w4 = *(Uint32 *)(p + 13);
			p += stride;
			w0 = (w0 << 24) | (w1 >> 8);
			w1 = (w1 << 24) | (w2 >> 8);
			w2 = (w2 << 24) | (w3 >> 8);
			w3 = (w3 << 24) | (w4 >> 8);
			d[0] = w0;
			d[1] = w1;
			d[16] = w2;
			d[17] = w3;
			d += 2;
			if (i == 7) {
				d += 16;
			}
		}
		break;
	}
	}
}

/* the generic (non-tuned) versions were dead-stripped; the table keeps their slots */
void (*const mpvmc16_oneref1p_func_table[4])(MPVMC *mc) = {NULL, NULL, NULL, NULL};

void MPVMC16_Init(MPVMC *mc)
{
	mc->oneref16[0] = mpvmc16_oneref1p_func_table[0];
	mc->oneref16[1] = mpvmc16_oneref1p_func_table[1];
	mc->oneref16[2] = mpvmc16_oneref1p_func_table[2];
	mc->oneref16[3] = mpvmc16_oneref1p_func_table[3];
}
