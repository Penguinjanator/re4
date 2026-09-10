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

/* COMPILER-DIFF: M5/M1 - shift forwarding and register numbering of the 4-bit decode loop (see mpvabdec's M5 note). Asm function (the original's instructions verbatim). */
#if 1 // COMPILER-DIFF: M5/M1
asm Sint32 ADX_DecodeSte4AsSte(Sint8 *src, Sint32 nfrm, Sint16 *outl, Sint16 *histl, Sint16 *outr, Sint16 *histr,
                           Sint16 c1, Sint16 c2, Sint16 *scl, Sint16 smul, Sint16 sadd)
{
	nofralloc
	stwu r1, -64(r1)
	srwi r0, r4, 31
	extsh r10, r10
	extsh r9, r9
	stmw r19, 12(r1)
	add r19, r0, r4
	lis r20, AdxQtbl@ha
	lwz r12, 72(r1)
	lha r11, 78(r1)
	srawi r19, r19, 1
	lha r0, 82(r1)
	addi r22, r20, AdxQtbl@l
	li r27, 0
	lha r28, 0(r6)
	lha r31, 2(r6)
	lha r29, 0(r8)
	lha r30, 2(r8)
	b L29c
Lac:
	lha r21, 0(r3)
	rlwinm. r20, r21, 0, 16, 16
	beq Lc0
	slwi r3, r27, 1
	b L2b8
Lc0:
	lha r23, 0(r12)
	mullw r20, r23, r11
	xor r21, r21, r23
	clrlwi r21, r21, 19
	addi r21, r21, 1
	add r20, r0, r20
	sth r20, 0(r12)
	extsh r24, r21
	lha r20, 0(r12)
	clrlwi r20, r20, 17
	sth r20, 0(r12)
	lha r23, 18(r3)
	rlwinm. r20, r23, 0, 16, 16
	beq L100
	slwi r3, r27, 1
	b L2b8
L100:
	lha r25, 0(r12)
	li r20, 16
	mullw r21, r25, r11
	xor r23, r23, r25
	clrlwi r23, r23, 19
	addi r23, r23, 1
	add r21, r0, r21
	sth r21, 0(r12)
	extsh r23, r23
	lha r21, 0(r12)
	clrlwi r21, r21, 17
	sth r21, 0(r12)
	mtctr r20
	addi r3, r3, 2
L138:
	lbz r26, 0(r3)
	mullw r20, r9, r28
	lbz r25, 18(r3)
	addi r3, r3, 1
	extsb r26, r26
	extsb r25, r25
	srawi r21, r26, 4
	mullw r31, r10, r31
	add r20, r20, r31
	mullw r21, r21, r24
	srawi r20, r20, 12
	add r31, r21, r20
	cmpwi r31, 32767
	bgt L178
	cmpwi r31, -32768
	bge L194
L178:
	cmpwi r31, -32768
	bge L188
	li r31, -32768
	b L194
L188:
	cmpwi r31, 32767
	ble L194
	li r31, 32767
L194:
	mullw r21, r9, r29
	srawi r20, r25, 4
	mullw r30, r10, r30
	add r21, r21, r30
	mullw r20, r20, r23
	srawi r21, r21, 12
	add r20, r20, r21
	cmpwi r20, 32767
	bgt L1c0
	cmpwi r20, -32768
	bge L1dc
L1c0:
	cmpwi r20, -32768
	bge L1d0
	li r20, -32768
	b L1dc
L1d0:
	cmpwi r20, 32767
	ble L1dc
	li r20, 32767
L1dc:
	rlwinm r21, r26, 2, 26, 29
	sth r31, 0(r5)
	lwzx r26, r22, r21
	mullw r21, r9, r31
	rlwinm r25, r25, 2, 26, 29
	sth r20, 0(r7)
	lwzx r25, r22, r25
	mullw r28, r10, r28
	add r21, r21, r28
	mullw r26, r26, r24
	srawi r21, r21, 12
	add r28, r26, r21
	cmpwi r28, 32767
	bgt L21c
	cmpwi r28, -32768
	bge L238
L21c:
	cmpwi r28, -32768
	bge L22c
	li r28, -32768
	b L238
L22c:
	cmpwi r28, 32767
	ble L238
	li r28, 32767
L238:
	mullw r21, r9, r20
	mullw r26, r10, r29
	add r21, r21, r26
	mullw r25, r25, r23
	srawi r21, r21, 12
	add r29, r25, r21
	cmpwi r29, 32767
	bgt L260
	cmpwi r29, -32768
	bge L27c
L260:
	cmpwi r29, -32768
	bge L270
	li r29, -32768
	b L27c
L270:
	cmpwi r29, 32767
	ble L27c
	li r29, 32767
L27c:
	sth r28, 2(r5)
	mr r30, r20
	addi r5, r5, 4
	sth r29, 2(r7)
	addi r7, r7, 4
	bdnz L138
	addi r3, r3, 18
	addi r27, r27, 1
L29c:
	cmpw r27, r19
	blt Lac
	sth r28, 0(r6)
	mr r3, r4
	sth r31, 2(r6)
	sth r29, 0(r8)
	sth r30, 2(r8)
L2b8:
	lmw r19, 12(r1)
	addi r1, r1, 64
	blr
}
#else
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
#endif

/* COMPILER-DIFF: M5/M1 - as ADX_DecodeSte4AsSte. Asm function (the original's instructions verbatim). */
#if 1 // COMPILER-DIFF: M5/M1
asm Sint32 ADX_DecodeSte4AsMono(Sint8 *src, Sint32 nfrm, Sint16 *outl, Sint16 *histl, Sint16 *outr, Sint16 *histr,
                            Sint16 c1, Sint16 c2, Sint16 *scl, Sint16 smul, Sint16 sadd)
{
	nofralloc
	stwu r1, -64(r1)
	srwi r0, r4, 31
	add r12, r0, r4
	extsh r10, r10
	stmw r18, 8(r1)
	lis r20, 26214
	lis r21, AdxQtbl@ha
	srawi r18, r12, 1
	lwz r11, 72(r1)
	extsh r9, r9
	lha r19, 78(r1)
	addi r22, r20, 26215
	lha r0, 82(r1)
	addi r23, r21, AdxQtbl@l
	li r28, 0
	lha r29, 0(r6)
	lha r12, 2(r6)
	lha r30, 0(r8)
	lha r31, 2(r8)
	b L58c
L314:
	lha r21, 0(r3)
	rlwinm. r20, r21, 0, 16, 16
	beq L328
	slwi r3, r28, 1
	b L5a8
L328:
	lha r24, 0(r11)
	mullw r20, r24, r19
	xor r21, r21, r24
	clrlwi r21, r21, 19
	addi r21, r21, 1
	add r20, r0, r20
	sth r20, 0(r11)
	extsh r25, r21
	lha r20, 0(r11)
	clrlwi r20, r20, 17
	sth r20, 0(r11)
	lha r21, 18(r3)
	rlwinm. r20, r21, 0, 16, 16
	beq L368
	slwi r3, r28, 1
	b L5a8
L368:
	lha r24, 0(r11)
	li r26, 16
	mullw r20, r24, r19
	xor r21, r21, r24
	clrlwi r21, r21, 19
	addi r21, r21, 1
	add r20, r0, r20
	sth r20, 0(r11)
	extsh r24, r21
	lha r20, 0(r11)
	clrlwi r20, r20, 17
	sth r20, 0(r11)
	mtctr r26
	addi r3, r3, 2
L3a0:
	mullw r20, r9, r29
	lbz r27, 0(r3)
	lbz r26, 18(r3)
	addi r3, r3, 1
	extsb r27, r27
	extsb r26, r26
	mullw r12, r10, r12
	srawi r21, r27, 4
	add r12, r20, r12
	mullw r20, r21, r25
	srawi r12, r12, 12
	add r12, r20, r12
	cmpwi r12, 32767
	bgt L3e0
	cmpwi r12, -32768
	bge L3fc
L3e0:
	cmpwi r12, -32768
	bge L3f0
	li r12, -32768
	b L3fc
L3f0:
	cmpwi r12, 32767
	ble L3fc
	li r12, 32767
L3fc:
	mullw r21, r9, r30
	srawi r20, r26, 4
	mullw r31, r10, r31
	add r21, r21, r31
	mullw r20, r20, r24
	srawi r21, r21, 12
	add r21, r20, r21
	cmpwi r21, 32767
	bgt L428
	cmpwi r21, -32768
	bge L444
L428:
	cmpwi r21, -32768
	bge L438
	li r21, -32768
	b L444
L438:
	cmpwi r21, 32767
	ble L444
	li r21, 32767
L444:
	add r20, r12, r21
	mr r31, r21
	mulli r20, r20, 7
	mulhw r20, r22, r20
	srawi r20, r20, 2
	srwi r21, r20, 31
	add r21, r20, r21
	cmpwi r21, 32767
	bgt L470
	cmpwi r21, -32768
	bge L48c
L470:
	cmpwi r21, -32768
	bge L480
	li r21, -32768
	b L48c
L480:
	cmpwi r21, 32767
	ble L48c
	li r21, 32767
L48c:
	sth r21, 0(r7)
	rlwinm r20, r27, 2, 26, 29
	rlwinm r26, r26, 2, 26, 29
	lwzx r20, r23, r20
	sth r21, 0(r5)
	mullw r27, r9, r12
	lwzx r21, r23, r26
	mullw r26, r10, r29
	add r26, r27, r26
	mullw r27, r20, r25
	srawi r20, r26, 12
	add r29, r27, r20
	cmpwi r29, 32767
	bgt L4cc
	cmpwi r29, -32768
	bge L4e8
L4cc:
	cmpwi r29, -32768
	bge L4dc
	li r29, -32768
	b L4e8
L4dc:
	cmpwi r29, 32767
	ble L4e8
	li r29, 32767
L4e8:
	mullw r20, r9, r31
	mullw r26, r10, r30
	add r20, r20, r26
	mullw r21, r21, r24
	srawi r20, r20, 12
	add r30, r21, r20
	cmpwi r30, 32767
	bgt L510
	cmpwi r30, -32768
	bge L52c
L510:
	cmpwi r30, -32768
	bge L520
	li r30, -32768
	b L52c
L520:
	cmpwi r30, 32767
	ble L52c
	li r30, 32767
L52c:
	add r20, r29, r30
	mulli r20, r20, 7
	mulhw r20, r22, r20
	srawi r20, r20, 2
	srwi r21, r20, 31
	add r20, r20, r21
	cmpwi r20, 32767
	bgt L554
	cmpwi r20, -32768
	bge L570
L554:
	cmpwi r20, -32768
	bge L564
	li r20, -32768
	b L570
L564:
	cmpwi r20, 32767
	ble L570
	li r20, 32767
L570:
	sth r20, 2(r7)
	addi r7, r7, 4
	sth r20, 2(r5)
	addi r5, r5, 4
	bdnz L3a0
	addi r3, r3, 18
	addi r28, r28, 1
L58c:
	cmpw r28, r18
	blt L314
	sth r29, 0(r6)
	mr r3, r4
	sth r12, 2(r6)
	sth r30, 0(r8)
	sth r31, 2(r8)
L5a8:
	lmw r18, 8(r1)
	addi r1, r1, 64
	blr
}
#else
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
#endif

/* COMPILER-DIFF: M5/M1 - as ADX_DecodeSte4AsSte. Asm function (the original's instructions verbatim). */
#if 1 // COMPILER-DIFF: M5/M1
asm Sint32 ADX_DecodeMono4(Sint8 *src, Sint32 nfrm, Sint16 *out, Sint16 *hist, Sint16 c1, Sint16 c2, Sint16 *scl,
                       Sint16 smul, Sint16 sadd)
{
	nofralloc
	stwu r1, -32(r1)
	extsh r0, r10
	extsh r8, r8
	extsh r7, r7
	stmw r26, 8(r1)
	lis r28, AdxQtbl@ha
	lha r30, 42(r1)
	addi r28, r28, AdxQtbl@l
	li r10, 0
	lha r12, 0(r6)
	lha r11, 2(r6)
	b L6e8
L5e4:
	lha r31, 0(r3)
	rlwinm. r27, r31, 0, 16, 16
	beq L5f8
	mr r3, r10
	b L6fc
L5f8:
	lha r26, 0(r9)
	li r29, 16
	mullw r27, r26, r0
	xor r31, r31, r26
	clrlwi r31, r31, 19
	addi r31, r31, 1
	add r27, r30, r27
	sth r27, 0(r9)
	extsh r31, r31
	lha r27, 0(r9)
	clrlwi r27, r27, 17
	sth r27, 0(r9)
	mtctr r29
	addi r3, r3, 2
L630:
	mullw r29, r7, r12
	lbz r26, 0(r3)
	addi r3, r3, 1
	extsb r26, r26
	srawi r27, r26, 4
	mullw r11, r8, r11
	add r11, r29, r11
	mullw r29, r27, r31
	srawi r11, r11, 12
	add r27, r29, r11
	cmpwi r27, 32767
	bgt L668
	cmpwi r27, -32768
	bge L684
L668:
	cmpwi r27, -32768
	bge L678
	li r27, -32768
	b L684
L678:
	cmpwi r27, 32767
	ble L684
	li r27, 32767
L684:
	rlwinm r11, r26, 2, 26, 29
	sth r27, 0(r5)
	lwzx r26, r28, r11
	mullw r11, r8, r12
	mullw r29, r7, r27
	add r11, r29, r11
	mullw r12, r26, r31
	srawi r11, r11, 12
	add r12, r12, r11
	cmpwi r12, 32767
	bgt L6b8
	cmpwi r12, -32768
	bge L6d4
L6b8:
	cmpwi r12, -32768
	bge L6c8
	li r12, -32768
	b L6d4
L6c8:
	cmpwi r12, 32767
	ble L6d4
	li r12, 32767
L6d4:
	sth r12, 2(r5)
	mr r11, r27
	addi r5, r5, 4
	bdnz L630
	addi r10, r10, 1
L6e8:
	cmpw r10, r4
	blt L5e4
	sth r12, 0(r6)
	mr r3, r4
	sth r11, 2(r6)
L6fc:
	lmw r26, 8(r1)
	addi r1, r1, 32
	blr
}
#else
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
#endif
