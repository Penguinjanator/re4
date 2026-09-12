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

/* COMPILER-DIFF: M1 - register ranking of the 4-bit decode loop (115 words): the original keeps
 * c1/c2 extended in place (r9/r10), the table pointer in r22 below sc_l/sc_r, one callee-saved
 * register less (r19..r31) and hands r21 out before t (r20) and nblk (r19). The scales are Sint16
 * locals (the extsh is their definition, as in ADX_DecodeMono4), the table values are the last-
 * declared locals defined before the stores, the declaration order is the original's colouring
 * order (pass 34: chaitin.py reproduces the target up to the r21 node once the nfrm ghost is gone).
 * Pass 72: the table is indexed directly (its address is the backend's hoisted preheader temporary,
 * coloured r22 after sc_r like the original's). Pass 73: the original's graph keeps the c1/c2
 * parameter copies (two coalesced ghosts dying at the widening `extsh`, one more neighbour on the
 * entry-block values sadd/smul/scl/nblk/l1/l2/rr1/rr2), which is what puts sadd/smul in the top level
 * (r0/r11) with all three stack loads before the `add`; the table value is written back into the
 * nibble (`d = AdxQtbl[d & 0xF]`, a range-split web) as in the mono decoder. Pass 78: the original
 * pops the `c1 * t` product of the right channel's second sample before `c2 * rr1` (one more
 * neighbour on it), reproduced by the `x` copy of `t` below (3 -> 0 words). */
Sint32 ADX_DecodeSte4AsSte(Sint8 *src, Sint32 nfrm, Sint16 *outl, Sint16 *histl, Sint16 *outr, Sint16 *histr,
                           register Sint16 c1, register Sint16 c2, Sint16 *scl, Sint16 smul, Sint16 sadd)
{
	Sint32 l2;
	Sint32 rr2;
	Sint32 rr1;
	register Sint32 l1;
	Sint32 i;
	Sint32 d;
	Sint32 dr;
	Sint16 sc_l;
	Sint16 sc_r;
	Sint32 s;
	register Sint32 t;
	Sint32 nblk;
	Sint32 key;
	Sint32 j;
	register Sint32 x;

	nblk = nfrm / 2;
	/* COMPILER-DIFF: M1 (kept parameter copies) - dead writes into the already pinned r6: the copies
	 * `mr r38,r9` / `mr r39,r10` survive to the allocator and coalesce into r9/r10 (pass 73: 11 -> 3
	 * words; r6 is already out of the colour set, so nothing else moves and both writes are deleted). */
	asm { mr r6, c1 } asm { mr r6, c2 }
	l1 = histl[0];
	l2 = histl[1];
	rr1 = histr[0];
	rr2 = histr[1];
	/* COMPILER-DIFF: M1 (neighbour pin) - the deleted copy makes histl (r6) a coalesced web, one more never-removed
	 * neighbour on every loop value (pass 44: 115 -> 41 words; pass 72: on l1 as in ADX_DecodeSte4AsMono). */
	asm { mr r6, l1 }
	for (i = 0; i < nblk; i++) {
		s = *(Sint16 *)src;
		if (s & 0x8000) {
			return i * 2;
		}
		key = *scl;
		sc_l = ((s ^ key) & 0x1FFF) + 1;
		key = sadd + key * smul;
		*scl = key;
		*scl = *scl & 0x7FFF;
		s = *(Sint16 *)(src + 0x12);
		if (s & 0x8000) {
			return i * 2;
		}
		key = *scl;
		sc_r = ((s ^ key) & 0x1FFF) + 1;
		key = sadd + key * smul;
		*scl = key;
		*scl = *scl & 0x7FFF;
		src += 2;
		for (j = 0; j < 16; j++) {
			d = src[0];
			dr = src[0x12];
			src++;
			l2 = (d >> 4) * sc_l + ((c1 * l1 + c2 * l2) >> 12);
			ADX_CLAMP(l2);
			t = (dr >> 4) * sc_r;
			t += (c1 * rr1 + c2 * rr2) >> 12;
			ADX_CLAMP(t);
			d = AdxQtbl[d & 0xF];
			outl[0] = l2;
			dr = AdxQtbl[dr & 0xF];
			outr[0] = t;
			l1 = d * sc_l + ((c1 * l2 + c2 * l1) >> 12);
			ADX_CLAMP(l1);
			/* COMPILER-DIFF: M1 (neighbour copy) - the second sample's `c1 * t` product must be
			 * coloured before `c2 * rr1` (one more neighbour, pass 78). `t` has three reaching
			 * definitions (the clamp), so the backend keeps this copy; `x` takes the dying `t`'s
			 * register and the `mr r20,r20` is deleted after allocation: `x` is a real node live
			 * across both products, `rr2 = x` is the original's `mr r30,r20`. */
			asm { mr x, t }
			rr1 = dr * sc_r + ((c1 * x + c2 * rr1) >> 12);
			ADX_CLAMP(rr1);
			outl[1] = l1;
			rr2 = x;
			outl += 2;
			outr[1] = rr1;
			outr += 2;
		}
		src += 0x12;
	}
	histl[0] = l1;
	histl[1] = l2;
	histr[0] = rr1;
	histr[1] = rr2;
	return nfrm;
}

/* COMPILER-DIFF: M1 (neighbour pin) - as ADX_DecodeSte4AsSte: the deleted copy makes histl (r6) a
 * coalesced web, one more never-removed neighbour on every loop value (pass 67: 143 -> 62 words).
 * The body is the stereo body with the mix in `t` (the original keeps `mr r31,r21` = rr2 = t and the
 * mix in t's register, so t is redefined between the copy and `c1 * rr2`), the same declaration
 * order as the stereo decoder, and the right channel's second sample predicted from the OLD rr1
 * (`c2 * rr1` = `mullw r26,r10,r30` in the original; the pass-44 split read the new one). Pass 73:
 * the c1/c2 parameter copies are kept as in the stereo decoder (the two `lis` pairs then colour like
 * the original's), and the table value is written back into the nibble (`d = AdxQtbl[d & 0xF]`): a
 * range-split web of a variable first defined before `t`, so it is coloured before the first mix
 * (q_l r20, the mix r21). Residue 5 words: the `nfrm / 2` add temporary takes r0 instead of r12
 * (sadd's load must precede the `srawi` in the pre-RA order) and the `(c1*l2 + c2*l1) >> 12` shift
 * temporary is coloured after the `d * sc_l` product (the original has one more neighbour on it). */
Sint32 ADX_DecodeSte4AsMono(Sint8 *src, Sint32 nfrm, Sint16 *outl, Sint16 *histl, Sint16 *outr, Sint16 *histr,
                            register Sint16 c1, register Sint16 c2, Sint16 *scl, Sint16 smul, Sint16 sadd)
{
	Sint32 l2;
	Sint32 rr2;
	Sint32 rr1;
	register Sint32 l1;
	Sint32 i;
	Sint32 d;
	Sint32 dr;
	Sint16 sc_l;
	Sint16 sc_r;
	Sint32 s;
	Sint32 t;
	Sint32 nblk;
	Sint32 key;
	Sint32 j;
	Sint16 *ps;

	nblk = nfrm / 2;
	/* COMPILER-DIFF: M1 (kept parameter copies) - as in ADX_DecodeSte4AsSte (pass 73: 26 -> 22 words). */
	asm { mr r6, c1 } asm { mr r6, c2 }
	/* CRI pass 70: the scramble addend is read through its address, so its loop value is the
	 * hoisted load (a backend temporary above the c1/c2 widenings in the Chaitin scan) instead
	 * of the entry load of the stack parameter (62 -> 28 words). */
	ps = &sadd;
	l1 = histl[0];
	l2 = histl[1];
	rr1 = histr[0];
	rr2 = histr[1];
	for (i = 0; i < nblk; i++) {
		s = *(Sint16 *)src;
		if (s & 0x8000) {
			return i * 2;
		}
		key = *scl;
		sc_l = ((s ^ key) & 0x1FFF) + 1;
		key = *ps + key * smul;
		*scl = key;
		*scl = *scl & 0x7FFF;
		s = *(Sint16 *)(src + 0x12);
		if (s & 0x8000) {
			return i * 2;
		}
		key = *scl;
		sc_r = ((s ^ key) & 0x1FFF) + 1;
		key = *ps + key * smul;
		*scl = key;
		*scl = *scl & 0x7FFF;
		src += 2;
		for (j = 0; j < 16; j++) {
			d = src[0];
			dr = src[0x12];
			src++;
			l2 = (d >> 4) * sc_l + ((c1 * l1 + c2 * l2) >> 12);
			ADX_CLAMP(l2);
			t = (dr >> 4) * sc_r + ((c1 * rr1 + c2 * rr2) >> 12);
			ADX_CLAMP(t);
			rr2 = t;
			t = (l2 + t) * 7 / 10;
			ADX_CLAMP(t);
			d = AdxQtbl[d & 0xF];
			outr[0] = t;
			dr = AdxQtbl[dr & 0xF];
			outl[0] = t;
			l1 = d * sc_l + ((c1 * l2 + c2 * l1) >> 12);
			ADX_CLAMP(l1);
			rr1 = dr * sc_r + ((c1 * rr2 + c2 * rr1) >> 12);
			ADX_CLAMP(rr1);
			t = (l1 + rr1) * 7 / 10;
			ADX_CLAMP(t);
			outr[1] = t;
			outr += 2;
			outl[1] = t;
			outl += 2;
		}
		src += 0x12;
	}
	/* The pin sits after the loop: in the entry block its copy took the issue slot that the
	 * original gives the nblk `srawi` (pass 70: 28 -> 26 words). */
	asm { mr r6, l1 }
	histl[0] = l1;
	histl[1] = l2;
	histr[0] = rr1;
	histr[1] = rr2;
	return nfrm;
}

/* The scale is a Sint16 local: its definition is the `extsh` itself (the frontend's hoisted
 * `(long)sc` in the inner-loop preheader becomes a copy of it), so the scale keeps its own-local
 * register rank; the `key = ..; *scl = key;` redefinition between the scale and the loop keeps the
 * frontend from substituting the scale into the hoist. The table value is the local `q` (declared
 * last) defined before the store, so it takes the dying nibble's register. */
Sint32 ADX_DecodeMono4(Sint8 *src, Sint32 nfrm, Sint16 *out, Sint16 *hist, Sint16 c1, Sint16 c2, Sint16 *scl,
                       Sint16 smul, Sint16 sadd)
{
	Sint32 i;
	Sint32 l2;
	Sint32 l1;
	Sint32 j;
	Sint32 s;
	Sint32 key;
	Sint16 sc;
	Sint32 d;
	Sint32 t;
	Sint32 q;

	l1 = hist[0];
	l2 = hist[1];
	for (i = 0; i < nfrm; i++) {
		s = *(Sint16 *)src;
		if (s & 0x8000) {
			return i;
		}
		key = *scl;
		sc = ((s ^ key) & 0x1FFF) + 1;
		key = sadd + key * smul;
		*scl = key;
		*scl = *scl & 0x7FFF;
		src += 2;
		for (j = 0; j < 16; j++) {
			d = src[0];
			src++;
			t = (d >> 4) * sc + ((c1 * l1 + c2 * l2) >> 12);
			ADX_CLAMP(t);
			q = AdxQtbl[d & 0xF];
			out[0] = t;
			l1 = q * sc + ((c1 * t + c2 * l1) >> 12);
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
