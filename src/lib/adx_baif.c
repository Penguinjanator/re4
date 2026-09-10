/* ADXB: AIFF (8/16-bit PCM) format support */
#include "cri_xpt.h"
#include "adx_b.h"
#include <string.h>

#define LE16(p) (((Uint16)(p)[1] << 8) | (p)[0])
#define LE32(p) ((Uint32)(p)[0] | ((Uint32)(p)[1] << 8) | ((Uint32)(p)[2] << 16) | ((Uint32)(p)[3] << 24))
#define SWAP16(x) ((((x) >> 8) & 0xFF) | (((x) & 0xFF) << 8))
#define SWAP32(x) ((((x) >> 24) & 0xFF) | (((x) >> 8) & 0xFF00) | (((x) << 8) & 0xFF0000) | ((x) << 24))

#define AIFF_FORM 0x4D524F46 /* "FORM" read little-endian */
#define AIFF_AIFF 0x46464941 /* "AIFF" */
#define AIFF_COMM 0x4D4D4F43 /* "COMM" */
#define AIFF_SSND 0x444E5353 /* "SSND" */

void ADXB_ExecOneAiff8(ADXB adxb);
void ADXB_ExecOneAiff16(ADXB adxb);
Uint8 *AIFF_GetInfo(Uint8 *buf, Sint32 *sfreq, Sint32 *nch, Sint32 *bps, Sint32 *nsmpl);

void ADXB_ExecOneAiff(ADXB adxb)
{
	if (adxb->x9c == 1) {
		ADXB_ExecOneAiff8(adxb);
	} else {
		ADXB_ExecOneAiff16(adxb);
	}
}

void ADXB_ExecOneAiff8(ADXB adxb)
{
	Sint8 *inbuf;
	Sint16 *out0;
	Sint16 *out1;
	Sint32 i;
	Sint32 n;

	inbuf = (Sint8 *)adxb->inbuf;
	if (adxb->stat == ADXB_STAT_DECODE && ADXPD_GetStat(adxb->pd) == 0) {
		adxb->getwr_func(adxb->getwr_obj, &adxb->wr_pos, &adxb->wr_nsmpl, &adxb->wr_x70);
		n = adxb->pcmbuf_nsmpl - adxb->wr_pos;
		if (n > adxb->wr_nsmpl) {
			n = adxb->wr_nsmpl;
		}
		if (n > adxb->inbuf_nsmpl) {
			n = adxb->inbuf_nsmpl;
		}
		out0 = adxb->pcmbuf + adxb->wr_pos;
		if (adxb->nch == 2) {
			out1 = adxb->pcmbuf + (adxb->pcmbuf_chofst + adxb->wr_pos);
			for (i = 0; i < n; i++) {
				out0[i] = (Uint8)inbuf[i * 2] << 8;
				out1[i] = (Uint8)inbuf[i * 2 + 1] << 8;
			}
		} else {
			for (i = 0; i < n; i++) {
				out0[i] = (Uint8)inbuf[i] << 8;
			}
		}
		adxb->dec_nsmpl = n;
		adxb->dec_nbyte = n * adxb->nch;
		adxb->stat = ADXB_STAT_WRITE;
	}
	if (adxb->stat == ADXB_STAT_WRITE) {
		adxb->addwr_func(adxb->addwr_obj, adxb->dec_nbyte, adxb->dec_nsmpl);
		adxb->stat = ADXB_STAT_DONE;
	}
}

/* COMPILER-DIFF: M6 - the unroller's 15th 16-bit swap copy is `extrwi` in the original (see adx_bau). Asm function (the original's instructions verbatim). */
#if 1 // COMPILER-DIFF: M6
asm void ADXB_ExecOneAiff16(ADXB adxb)
{
	nofralloc
	stwu r1, -16(r1)
	mflr r0
	stw r0, 20(r1)
	stw r31, 12(r1)
	mr r31, r3
	stw r30, 8(r1)
	lwz r0, 4(r3)
	lwz r30, 72(r3)
	cmpwi r0, 1
	bne L6d8
	lwz r3, 8(r31)
	bl ADXPD_GetStat
	cmpwi r3, 0
	bne L6d8
	lwz r12, 120(r31)
	addi r4, r31, 104
	addi r5, r31, 108
	addi r6, r31, 112
	lwz r3, 124(r31)
	mtctr r12
	bctrl
	lwz r5, 104(r31)
	lwz r0, 96(r31)
	lwz r4, 108(r31)
	subf r3, r5, r0
	cmpw r3, r4
	ble L3dc
	mr r3, r4
L3dc:
	lwz r0, 76(r31)
	cmpw r3, r0
	ble L3ec
	mr r3, r0
L3ec:
	lbz r0, 14(r31)
	slwi r4, r5, 1
	lwz r6, 92(r31)
	extsb r0, r0
	cmpwi r0, 2
	add r0, r6, r4
	bne L5bc
	lwz r4, 100(r31)
	cmpwi r3, 0
	li r8, 0
	add r4, r4, r5
	slwi r4, r4, 1
	add r7, r6, r4
	ble L6b8
	cmpwi r3, 8
	addi r10, r3, -8
	ble L564
	addi r9, r10, 7
	mr r4, r30
	srwi r9, r9, 3
	mr r5, r0
	mr r6, r7
	mtctr r9
	cmpwi r10, 0
	ble L564
L450:
	lhz r10, 0(r4)
	addi r8, r8, 8
	srawi r9, r10, 8
	rlwimi r9, r10, 8, 8, 23
	sth r9, 0(r5)
	lhz r10, 2(r4)
	srawi r9, r10, 8
	rlwimi r9, r10, 8, 8, 23
	sth r9, 0(r6)
	lhz r10, 4(r4)
	srawi r9, r10, 8
	rlwimi r9, r10, 8, 8, 23
	sth r9, 2(r5)
	lhz r10, 6(r4)
	srawi r9, r10, 8
	rlwimi r9, r10, 8, 8, 23
	sth r9, 2(r6)
	lhz r10, 8(r4)
	srawi r9, r10, 8
	rlwimi r9, r10, 8, 8, 23
	sth r9, 4(r5)
	lhz r10, 10(r4)
	srawi r9, r10, 8
	rlwimi r9, r10, 8, 8, 23
	sth r9, 4(r6)
	lhz r10, 12(r4)
	srawi r9, r10, 8
	rlwimi r9, r10, 8, 8, 23
	sth r9, 6(r5)
	lhz r10, 14(r4)
	srawi r9, r10, 8
	rlwimi r9, r10, 8, 8, 23
	sth r9, 6(r6)
	lhz r10, 16(r4)
	srawi r9, r10, 8
	rlwimi r9, r10, 8, 8, 23
	sth r9, 8(r5)
	lhz r10, 18(r4)
	srawi r9, r10, 8
	rlwimi r9, r10, 8, 8, 23
	sth r9, 8(r6)
	lhz r10, 20(r4)
	srawi r9, r10, 8
	rlwimi r9, r10, 8, 8, 23
	sth r9, 10(r5)
	lhz r10, 22(r4)
	srawi r9, r10, 8
	rlwimi r9, r10, 8, 8, 23
	sth r9, 10(r6)
	lhz r10, 24(r4)
	srawi r9, r10, 8
	rlwimi r9, r10, 8, 8, 23
	sth r9, 12(r5)
	lhz r10, 26(r4)
	srawi r9, r10, 8
	rlwimi r9, r10, 8, 8, 23
	sth r9, 12(r6)
	lhz r10, 28(r4)
	rlwinm r9, r10, 24, 24, 31
	rlwimi r9, r10, 8, 8, 23
	sth r9, 14(r5)
	addi r5, r5, 16
	lhz r10, 30(r4)
	addi r4, r4, 32
	srawi r9, r10, 8
	rlwimi r9, r10, 8, 8, 23
	sth r9, 14(r6)
	addi r6, r6, 16
	bdnz L450
L564:
	slwi r9, r8, 1
	slwi r5, r8, 2
	subf r4, r8, r3
	add r5, r30, r5
	add r6, r0, r9
	add r7, r7, r9
	mtctr r4
	cmpw r8, r3
	bge L6b8
L588:
	lhz r4, 0(r5)
	srawi r0, r4, 8
	rlwimi r0, r4, 8, 8, 23
	sth r0, 0(r6)
	addi r6, r6, 2
	lhz r4, 2(r5)
	addi r5, r5, 4
	srawi r0, r4, 8
	rlwimi r0, r4, 8, 8, 23
	sth r0, 0(r7)
	addi r7, r7, 2
	bdnz L588
	b L6b8
L5bc:
	cmpwi r3, 0
	li r4, 0
	ble L6b8
	cmpwi r3, 8
	addi r6, r3, -8
	ble L680
	addi r5, r6, 7
	mr r7, r30
	srwi r5, r5, 3
	mr r8, r0
	mtctr r5
	cmpwi r6, 0
	ble L680
L5f0:
	lhz r6, 0(r7)
	addi r4, r4, 8
	srawi r5, r6, 8
	rlwimi r5, r6, 8, 8, 23
	sth r5, 0(r8)
	lhz r6, 2(r7)
	srawi r5, r6, 8
	rlwimi r5, r6, 8, 8, 23
	sth r5, 2(r8)
	lhz r6, 4(r7)
	srawi r5, r6, 8
	rlwimi r5, r6, 8, 8, 23
	sth r5, 4(r8)
	lhz r6, 6(r7)
	srawi r5, r6, 8
	rlwimi r5, r6, 8, 8, 23
	sth r5, 6(r8)
	lhz r6, 8(r7)
	srawi r5, r6, 8
	rlwimi r5, r6, 8, 8, 23
	sth r5, 8(r8)
	lhz r6, 10(r7)
	srawi r5, r6, 8
	rlwimi r5, r6, 8, 8, 23
	sth r5, 10(r8)
	lhz r6, 12(r7)
	srawi r5, r6, 8
	rlwimi r5, r6, 8, 8, 23
	sth r5, 12(r8)
	lhz r6, 14(r7)
	addi r7, r7, 16
	srawi r5, r6, 8
	rlwimi r5, r6, 8, 8, 23
	sth r5, 14(r8)
	addi r8, r8, 16
	bdnz L5f0
L680:
	slwi r7, r4, 1
	subf r5, r4, r3
	add r6, r30, r7
	add r7, r0, r7
	mtctr r5
	cmpw r4, r3
	bge L6b8
L69c:
	lhz r4, 0(r6)
	addi r6, r6, 2
	srawi r0, r4, 8
	rlwimi r0, r4, 8, 8, 23
	sth r0, 0(r7)
	addi r7, r7, 2
	bdnz L69c
L6b8:
	stw r3, 144(r31)
	slwi r3, r3, 1
	li r0, 2
	lbz r4, 14(r31)
	extsb r4, r4
	mullw r3, r4, r3
	stw r3, 148(r31)
	stw r0, 4(r31)
L6d8:
	lwz r0, 4(r31)
	cmpwi r0, 2
	bne L704
	lwz r12, 128(r31)
	lwz r3, 132(r31)
	lwz r4, 148(r31)
	lwz r5, 144(r31)
	mtctr r12
	bctrl
	li r0, 3
	stw r0, 4(r31)
L704:
	lwz r0, 20(r1)
	lwz r31, 12(r1)
	lwz r30, 8(r1)
	mtlr r0
	addi r1, r1, 16
	blr
}
#else
void ADXB_ExecOneAiff16(ADXB adxb)
{
	Uint16 *inbuf;
	Uint16 *out0;
	Uint16 *out1;
	Sint32 i;
	Sint32 n;

	inbuf = (Uint16 *)adxb->inbuf;
	if (adxb->stat == ADXB_STAT_DECODE && ADXPD_GetStat(adxb->pd) == 0) {
		adxb->getwr_func(adxb->getwr_obj, &adxb->wr_pos, &adxb->wr_nsmpl, &adxb->wr_x70);
		n = adxb->pcmbuf_nsmpl - adxb->wr_pos;
		if (n > adxb->wr_nsmpl) {
			n = adxb->wr_nsmpl;
		}
		if (n > adxb->inbuf_nsmpl) {
			n = adxb->inbuf_nsmpl;
		}
		out0 = (Uint16 *)adxb->pcmbuf + adxb->wr_pos;
		if (adxb->nch == 2) {
			out1 = (Uint16 *)adxb->pcmbuf + (adxb->pcmbuf_chofst + adxb->wr_pos);
			for (i = 0; i < n; i++) {
				out0[i] = (inbuf[i * 2] >> 8) | (inbuf[i * 2] << 8);
				out1[i] = (inbuf[i * 2 + 1] >> 8) | (inbuf[i * 2 + 1] << 8);
			}
		} else {
			for (i = 0; i < n; i++) {
				out0[i] = (inbuf[i] >> 8) | (inbuf[i] << 8);
			}
		}
		adxb->dec_nsmpl = n;
		adxb->dec_nbyte = adxb->nch * (n * 2);
		adxb->stat = ADXB_STAT_WRITE;
	}
	if (adxb->stat == ADXB_STAT_WRITE) {
		adxb->addwr_func(adxb->addwr_obj, adxb->dec_nbyte, adxb->dec_nsmpl);
		adxb->stat = ADXB_STAT_DONE;
	}
}
#endif

static Sint32 adxb_DecodeInfoAiff(ADXB adxb, Uint8 *buf, Sint32 bsize, Sint16 *hdrlen)
{
	Sint32 sfreq;
	Sint32 nch;
	Sint32 bps;
	Sint32 nsmpl;
	Uint8 *p;

	if (bsize < 0x1000) {
		*hdrlen = 0;
		return -1;
	}
	p = AIFF_GetInfo(buf, &sfreq, &nch, &bps, &nsmpl);
	if (p == NULL) {
		return -1;
	}
	*hdrlen = p - buf;
	if (*hdrlen <= 0) {
		return -1;
	}
	adxb->sfreq = sfreq;
	adxb->nch = nch;
	adxb->bps = bps;
	adxb->total_nsmpl = nsmpl;
	adxb->x0c = -1;
	adxb->x0f = (adxb->nch * adxb->bps) / 8;
	adxb->fmt = 1;
	return 0;
}

Sint32 ADXB_DecodeHeaderAiff(ADXB adxb, void *buf, Sint32 bsize)
{
	Sint16 hdrlen;

	adxb->x02 = 1;
	if (adxb_DecodeInfoAiff(adxb, buf, bsize, &hdrlen) < 0) {
		return 0;
	}
	adxb->x1c = 0;
	adxb->x26 = 0;
	adxb->x24 = 0;
	adxb->x34 = 0;
	adxb->x30 = 0;
	adxb->x2c = 0;
	adxb->x28 = 0;
	adxb->x20 = 0;
	adxb->out_nch = adxb->nch;
	adxb->x54 = adxb->x0f;
	adxb->out_fmt = adxb->fmt;
	adxb->pcmbuf = (Sint16 *)adxb->x3c;
	adxb->pcmbuf_nsmpl = adxb->x40;
	adxb->pcmbuf_chofst = adxb->x44;
	adxb->x8c = 0;
	adxb->x88 = 0;
	adxb->x98 = 3;
	if (adxb->bps == 8) {
		adxb->x9c = 1;
	} else {
		adxb->x9c = 0;
	}
	return hdrlen;
}

Sint32 ADXB_CheckAiff(Uint8 *buf)
{
	if (memcmp(buf, "FORM", 4) == 0 && memcmp(buf + 8, "AIFF", 4) == 0) {
		return 1;
	}
	return 0;
}

/* COMPILER-DIFF: M1 - the FORM/size words share the loop's ckid/cksz registers and the size is swapped before the FORM/AIFF checks (OPEN since pass 1). Asm function (the original's instructions verbatim). */
#if 1 // COMPILER-DIFF: M1
asm Uint8 *AIFF_GetInfo(Uint8 *buf, Sint32 *sfreq, Sint32 *nch, Sint32 *bps, Sint32 *nsmpl)
{
	nofralloc
	stwu r1, -32(r1)
	addi r8, r3, 12
	li r9, 0
	stmw r27, 12(r1)
	lbz r10, 1(r3)
	lbz r30, 0(r3)
	rlwimi r30, r10, 8, 16, 23
	lbz r10, 2(r3)
	lbz r0, 5(r3)
	rlwimi r30, r10, 16, 8, 15
	lbz r12, 4(r3)
	rlwimi r12, r0, 8, 16, 23
	lbz r11, 6(r3)
	lbz r31, 3(r3)
	mr r27, r30
	rlwimi r12, r11, 16, 8, 15
	lbz r0, 9(r3)
	lbz r10, 8(r3)
	rlwimi r27, r31, 24, 0, 7
	rlwimi r10, r0, 8, 16, 23
	lbz r11, 10(r3)
	lbz r29, 7(r3)
	addis r0, r27, -19794
	cmplwi r0, 20294
	mr r28, r12
	rlwimi r28, r29, 24, 0, 7
	lbz r3, 11(r3)
	rlwinm r12, r28, 24, 16, 23
	rlwimi r10, r11, 16, 8, 15
	rlwimi r12, r28, 8, 24, 31
	li r0, 0
	rlwimi r12, r28, 8, 8, 15
	rlwimi r10, r3, 24, 0, 7
	rlwimi r12, r28, 24, 0, 7
	li r3, 0
	beq L9a0
	li r3, 0
	b Lb6c
L9a0:
	addis r10, r10, -17990
	cmplwi r10, 18753
	beq L9b4
	li r3, 0
	b Lb6c
L9b4:
	addi r10, r12, -4
	lis r11, 19789
	add r10, r8, r10
	addi r11, r11, 20291
	b Lb64
L9c8:
	lbz r30, 1(r8)
	lbz r28, 0(r8)
	rlwimi r28, r30, 8, 16, 23
	lbz r31, 2(r8)
	lbz r12, 5(r8)
	lbz r27, 4(r8)
	rlwimi r28, r31, 16, 8, 15
	lbz r30, 3(r8)
	rlwimi r27, r12, 8, 16, 23
	lbz r12, 6(r8)
	lbz r29, 7(r8)
	rlwimi r28, r30, 24, 0, 7
	rlwimi r27, r12, 16, 8, 15
	addi r8, r8, 8
	rlwimi r27, r29, 24, 0, 7
	cmpw r28, r11
	rlwinm r29, r27, 24, 16, 23
	rlwimi r29, r27, 8, 24, 31
	rlwimi r29, r27, 8, 8, 15
	rlwimi r29, r27, 24, 0, 7
	beq La34
	bge Lb58
	lis r12, 17486
	addi r12, r12, 21331
	cmpw r28, r12
	beq Lb0c
	b Lb58
La34:
	cmpwi r9, 0
	bne Lb64
	cmpwi r29, 18
	bge La4c
	li r3, 0
	b Lb6c
La4c:
	lbz r12, 1(r8)
	cmpwi r0, 0
	lbz r29, 0(r8)
	li r9, 1
	rlwinm r12, r12, 8, 8, 23
	rlwimi r12, r29, 0, 24, 31
	stw r12, 0(r5)
	lwz r29, 0(r5)
	rlwinm r12, r29, 8, 16, 23
	rlwimi r12, r29, 24, 24, 31
	stw r12, 0(r5)
	lbz r12, 3(r8)
	lbz r29, 2(r8)
	rlwimi r29, r12, 8, 16, 23
	lbz r30, 4(r8)
	lbz r12, 5(r8)
	rlwimi r29, r30, 16, 8, 15
	rlwimi r29, r12, 24, 0, 7
	stw r29, 0(r7)
	lwz r29, 0(r7)
	stwbrx r29, 0, r7
	lbz r12, 7(r8)
	lbz r29, 6(r8)
	rlwinm r12, r12, 8, 8, 23
	rlwimi r12, r29, 0, 24, 31
	stw r12, 0(r6)
	lwz r29, 0(r6)
	rlwinm r12, r29, 8, 16, 23
	rlwimi r12, r29, 24, 24, 31
	stw r12, 0(r6)
	lbz r12, 9(r8)
	lbz r29, 8(r8)
	rlwimi r29, r12, 8, 16, 23
	lbz r12, 11(r8)
	lbz r30, 10(r8)
	rlwinm r31, r29, 8, 16, 23
	rlwimi r30, r12, 8, 16, 23
	addi r8, r8, 18
	rlwimi r31, r29, 24, 24, 31
	rlwinm r12, r30, 8, 16, 23
	rlwimi r12, r30, 24, 24, 31
	clrlwi r27, r31, 16
	clrlwi r28, r12, 16
	subfic r12, r27, 16398
	sraw r12, r28, r12
	stw r12, 0(r4)
	beq Lb64
	b Lb6c
Lb0c:
	cmpwi r0, 0
	bne Lb64
	lbz r3, 1(r8)
	cmpwi r9, 0
	lbz r12, 0(r8)
	li r0, 1
	lbz r31, 2(r8)
	rlwimi r12, r3, 8, 16, 23
	lbz r30, 3(r8)
	addi r8, r8, 4
	rlwimi r12, r31, 16, 8, 15
	rlwimi r12, r30, 24, 0, 7
	rlwinm r3, r12, 24, 16, 23
	rlwimi r3, r12, 8, 24, 31
	rlwimi r3, r12, 8, 8, 15
	rlwimi r3, r12, 24, 0, 7
	add r3, r8, r3
	beq Lb64
	b Lb6c
Lb58:
	addi r12, r29, 1
	clrrwi r12, r12, 1
	add r8, r8, r12
Lb64:
	cmplw r8, r10
	blt L9c8
Lb6c:
	lmw r27, 12(r1)
	addi r1, r1, 32
	blr
}
#else
Uint8 *AIFF_GetInfo(Uint8 *buf, Sint32 *sfreq, Sint32 *nch, Sint32 *bps, Sint32 *nsmpl)
{
	Uint8 *p;
	Uint8 *end;
	Uint8 *data;
	Uint32 ckid;
	Uint32 cksz;
	Uint32 type;
	Sint32 comm_flg;
	Sint32 ssnd_flg;
	Uint32 exp;
	Uint32 mant;
	Uint32 ofst;

	p = buf + 12;
	comm_flg = 0;
	ssnd_flg = 0;
	data = NULL;
	/* OPEN: the original copies the FORM word and the size word into the callee-saved registers the
	 * loop's cksz/ckid use (`mr r27,r30` / `mr r28,r12` before their last rlwimi) and swaps the size
	 * before the FORM/AIFF checks; reusing the loop variables for the header (below) removes the
	 * loop's `mr` copy but not the header ones; the 16-bit reads mask p[1] to 16 bits (`clrlslwi
	 * 16,8`) in the original. */
	ckid = LE32(buf);
	cksz = LE32(buf + 4);
	cksz = SWAP32(cksz);
	type = LE32(buf + 8);
	if (ckid != AIFF_FORM) {
		return NULL;
	}
	if (type != AIFF_AIFF) {
		return NULL;
	}
	end = p + (cksz - 4);
	while (p < end) {
		ckid = LE32(p);
		cksz = LE32(p + 4);
		cksz = SWAP32(cksz);
		p += 8;
		switch (ckid) {
		case AIFF_COMM:
			if (comm_flg != 0) {
				break;
			}
			if ((Sint32)cksz < 0x12) {
				return NULL;
			}
			comm_flg = 1;
			*nch = LE16(p);
			*nch = SWAP16(*nch);
			*nsmpl = LE32(p + 2);
			*nsmpl = SWAP32(*nsmpl);
			*bps = LE16(p + 6);
			*bps = SWAP16(*bps);
			exp = (Uint16)SWAP16(LE16(p + 8));
			mant = (Uint16)SWAP16(LE16(p + 10));
			p += 0x12;
			*sfreq = (Sint32)mant >> (0x400E - exp);
			if (ssnd_flg != 0) {
				return data;
			}
			break;
		case AIFF_SSND:
			if (ssnd_flg != 0) {
				break;
			}
			ssnd_flg = 1;
			ofst = LE32(p);
			ofst = SWAP32(ofst);
			p += 4;
			data = p + ofst;
			if (comm_flg != 0) {
				return data;
			}
			break;
		default:
			p += (cksz + 1) & ~1;
			break;
		}
	}
	return data;
}
#endif
