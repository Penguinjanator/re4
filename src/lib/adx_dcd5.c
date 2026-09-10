/* ADX 4-bit decoder (adx_dcd5.c): 18-byte frames of a 2-byte scale and 32 nibbles, second-order
 * prediction with the coefficients c1/c2 (12-bit fixed point); the scale header is descrambled
 * with the running key *scl (key = key * smul + sadd). Stereo frames are interleaved (L, R). */
#include "cri_xpt.h"

extern Sint32 adx_decode_output_mono_flag;

#define ADX_CLAMP(v) \
	if ((v) > 0x7FFF || (v) < -0x8000) { \
		if ((v) < -0x8000) { \
			(v) = -0x8000; \
		} else if ((v) > 0x7FFF) { \
			(v) = 0x7FFF; \
		} \
	}

const Sint32 AdxQtbl[16] = {
	0, 1, 2, 3, 4, 5, 6, 7, -8, -7, -6, -5, -4, -3, -2, -1,
};

Sint32 ADX_DecodeSte4AsSte(Sint8 *src, Sint32 nfrm, Sint16 *outl, Sint16 *histl, Sint16 *outr, Sint16 *histr,
                           Sint16 c1, Sint16 c2, Sint16 *scl, Sint16 smul, Sint16 sadd);
Sint32 ADX_DecodeSte4AsMono(Sint8 *src, Sint32 nfrm, Sint16 *outl, Sint16 *histl, Sint16 *outr, Sint16 *histr,
                            Sint16 c1, Sint16 c2, Sint16 *scl, Sint16 smul, Sint16 sadd);

Sint32 ADX_DecodeSte4(Sint8 *src, Sint32 nfrm, Sint16 *outl, Sint16 *histl, Sint16 *outr, Sint16 *histr,
                      Sint16 c1, Sint16 c2, Sint16 *scl, Sint16 smul, Sint16 sadd)
{
	if (adx_decode_output_mono_flag == 0) {
		return ADX_DecodeSte4AsSte(src, nfrm, outl, histl, outr, histr, c1, c2, scl, smul, sadd);
	}
	return ADX_DecodeSte4AsMono(src, nfrm, outl, histl, outr, histr, c1, c2, scl, smul, sadd);
}

/* COMPILER-DIFF: M5/M1 - shift forwarding and register numbering of the 4-bit decode loop (see mpvabdec's M5 note). Pure C by project decision (CRI pass 8). */
Sint32 ADX_DecodeSte4AsSte(Sint8 *src, Sint32 nfrm, Sint16 *outl, Sint16 *histl, Sint16 *outr, Sint16 *histr,
                           Sint16 c1, Sint16 c2, Sint16 *scl, Sint16 smul, Sint16 sadd)
{
	Sint32 nblk;
	Sint32 i;
	Sint32 j;
	Sint32 l1;
	Sint32 l2;
	Sint32 r1;
	Sint32 r2;
	Sint32 s;
	Sint32 key;
	Sint32 sc_l;
	Sint32 sc_r;
	Sint32 d;
	Sint32 dr;
	Sint32 t;

	nblk = nfrm / 2;
	l1 = histl[0];
	l2 = histl[1];
	r1 = histr[0];
	r2 = histr[1];
	for (i = 0; i < nblk; i++) {
		s = *(Sint16 *)src;
		if (s & 0x8000) {
			return i * 2;
		}
		key = *scl;
		*scl = sadd + key * smul;
		sc_l = (Sint16)(((s ^ key) & 0x1FFF) + 1);
		*scl = *scl & 0x7FFF;
		s = *(Sint16 *)(src + 0x12);
		if (s & 0x8000) {
			return i * 2;
		}
		key = *scl;
		*scl = sadd + key * smul;
		sc_r = (Sint16)(((s ^ key) & 0x1FFF) + 1);
		*scl = *scl & 0x7FFF;
		src += 2;
		for (j = 0; j < 16; j++) {
			d = src[0];
			dr = src[0x12];
			src++;
			l2 = (d >> 4) * sc_l + ((c1 * l1 + c2 * l2) >> 12);
			ADX_CLAMP(l2);
			t = (dr >> 4) * sc_r + ((c1 * r1 + c2 * r2) >> 12);
			ADX_CLAMP(t);
			outl[0] = l2;
			outr[0] = t;
			l1 = sc_l * AdxQtbl[d & 0xF] + ((c1 * l2 + c2 * l1) >> 12);
			ADX_CLAMP(l1);
			r1 = sc_r * AdxQtbl[dr & 0xF] + ((c1 * t + c2 * r1) >> 12);
			ADX_CLAMP(r1);
			outl[1] = l1;
			r2 = t;
			outl += 2;
			outr[1] = r1;
			outr += 2;
		}
		src += 0x12;
	}
	histl[0] = l1;
	histl[1] = l2;
	histr[0] = r1;
	histr[1] = r2;
	return nfrm;
}

/* COMPILER-DIFF: M5/M1 - as ADX_DecodeSte4AsSte. Pure C by project decision (CRI pass 8). */
Sint32 ADX_DecodeSte4AsMono(Sint8 *src, Sint32 nfrm, Sint16 *outl, Sint16 *histl, Sint16 *outr, Sint16 *histr,
                            Sint16 c1, Sint16 c2, Sint16 *scl, Sint16 smul, Sint16 sadd)
{
	Sint32 nblk;
	Sint32 i;
	Sint32 j;
	Sint32 l1;
	Sint32 l2;
	Sint32 r1;
	Sint32 r2;
	Sint32 s;
	Sint32 key;
	Sint32 sc_l;
	Sint32 sc_r;
	Sint32 d;
	Sint32 dr;
	Sint32 t;
	Sint32 m;

	nblk = nfrm / 2;
	l1 = histl[0];
	l2 = histl[1];
	r1 = histr[0];
	r2 = histr[1];
	for (i = 0; i < nblk; i++) {
		s = *(Sint16 *)src;
		if (s & 0x8000) {
			return i * 2;
		}
		key = *scl;
		*scl = sadd + key * smul;
		sc_l = (Sint16)(((s ^ key) & 0x1FFF) + 1);
		*scl = *scl & 0x7FFF;
		s = *(Sint16 *)(src + 0x12);
		if (s & 0x8000) {
			return i * 2;
		}
		key = *scl;
		*scl = sadd + key * smul;
		sc_r = (Sint16)(((s ^ key) & 0x1FFF) + 1);
		*scl = *scl & 0x7FFF;
		src += 2;
		for (j = 0; j < 16; j++) {
			d = src[0];
			dr = src[0x12];
			src++;
			l2 = (d >> 4) * sc_l + ((c1 * l1 + c2 * l2) >> 12);
			ADX_CLAMP(l2);
			t = (dr >> 4) * sc_r + ((c1 * r1 + c2 * r2) >> 12);
			ADX_CLAMP(t);
			m = (l2 + t) * 7 / 10;
			r2 = t;
			ADX_CLAMP(m);
			outr[0] = m;
			outl[0] = m;
			l1 = sc_l * AdxQtbl[d & 0xF] + ((c1 * l2 + c2 * l1) >> 12);
			ADX_CLAMP(l1);
			r1 = sc_r * AdxQtbl[dr & 0xF] + ((c1 * t + c2 * r1) >> 12);
			ADX_CLAMP(r1);
			m = (l1 + r1) * 7 / 10;
			ADX_CLAMP(m);
			outr[1] = m;
			outr += 2;
			outl[1] = m;
			outl += 2;
		}
		src += 0x12;
	}
	histl[0] = l1;
	histl[1] = l2;
	histr[0] = r1;
	histr[1] = r2;
	return nfrm;
}

/* COMPILER-DIFF: M5/M1 - as ADX_DecodeSte4AsSte. Pure C by project decision (CRI pass 8). */
Sint32 ADX_DecodeMono4(Sint8 *src, Sint32 nfrm, Sint16 *out, Sint16 *hist, Sint16 c1, Sint16 c2, Sint16 *scl,
                       Sint16 smul, Sint16 sadd)
{
	Sint32 i;
	Sint32 j;
	Sint32 l1;
	Sint32 l2;
	Sint32 s;
	Sint32 key;
	Sint32 sc;
	Sint32 d;
	Sint32 t;

	l1 = hist[0];
	l2 = hist[1];
	for (i = 0; i < nfrm; i++) {
		s = *(Sint16 *)src;
		if (s & 0x8000) {
			return i;
		}
		key = *scl;
		*scl = sadd + key * smul;
		sc = (Sint16)(((s ^ key) & 0x1FFF) + 1);
		*scl = *scl & 0x7FFF;
		src += 2;
		for (j = 0; j < 16; j++) {
			d = src[0];
			src++;
			t = (d >> 4) * sc + ((c1 * l1 + c2 * l2) >> 12);
			ADX_CLAMP(t);
			out[0] = t;
			l1 = sc * AdxQtbl[d & 0xF] + ((c1 * t + c2 * l1) >> 12);
			ADX_CLAMP(l1);
			out[1] = l1;
			l2 = t;
			out += 2;
		}
	}
	hist[0] = l1;
	hist[1] = l2;
	return nfrm;
}
