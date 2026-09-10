/* ADXB: Sun AU (.snd) format support: u-law / 8-bit / 16-bit linear PCM */
#include "cri_xpt.h"
#include "adx_b.h"
#include <string.h>

#define LE16(p) (((Uint16)(p)[1] << 8) | (p)[0])
#define LE32(p) ((Uint32)(p)[0] | ((Uint32)(p)[1] << 8) | ((Uint32)(p)[2] << 16) | ((Uint32)(p)[3] << 24))
#define SWAP32(x) ((((x) >> 24) & 0xFF) | (((x) >> 8) & 0xFF00) | (((x) << 8) & 0xFF0000) | ((x) << 24))

#define AU_MAGIC1 0x0064732E /* ".sd" read little-endian */
#define AU_MAGIC2 0x646E732E /* ".snd" */

#define AU_ENC_ULAW 1
#define AU_ENC_PCM8 2
#define AU_ENC_PCM16 3

void ADXB_ExecOneAuUlaw(ADXB adxb);
void ADXB_ExecOneAu8(ADXB adxb);
void ADXB_ExecOneAu16(ADXB adxb);
Uint8 *AU_GetInfo(Uint8 *buf, Sint32 bsize, Sint32 *sfreq, Sint32 *nch, Sint32 *bps, Sint32 *nsmpl, Sint32 *type);

Sint16 ulaw_exp_table[256] = {
	-32124, -31100, -30076, -29052, -28028, -27004, -25980, -24956,
	-23932, -22908, -21884, -20860, -19836, -18812, -17788, -16764,
	-15996, -15484, -14972, -14460, -13948, -13436, -12924, -12412,
	-11900, -11388, -10876, -10364, -9852, -9340, -8828, -8316,
	-7932, -7676, -7420, -7164, -6908, -6652, -6396, -6140,
	-5884, -5628, -5372, -5116, -4860, -4604, -4348, -4092,
	-3900, -3772, -3644, -3516, -3388, -3260, -3132, -3004,
	-2876, -2748, -2620, -2492, -2364, -2236, -2108, -1980,
	-1884, -1820, -1756, -1692, -1628, -1564, -1500, -1436,
	-1372, -1308, -1244, -1180, -1116, -1052, -988, -924,
	-876, -844, -812, -780, -748, -716, -684, -652,
	-620, -588, -556, -524, -492, -460, -428, -396,
	-372, -356, -340, -324, -308, -292, -276, -260,
	-244, -228, -212, -196, -180, -164, -148, -132,
	-120, -112, -104, -96, -88, -80, -72, -64,
	-56, -48, -40, -32, -24, -16, -8, 0,
	32124, 31100, 30076, 29052, 28028, 27004, 25980, 24956,
	23932, 22908, 21884, 20860, 19836, 18812, 17788, 16764,
	15996, 15484, 14972, 14460, 13948, 13436, 12924, 12412,
	11900, 11388, 10876, 10364, 9852, 9340, 8828, 8316,
	7932, 7676, 7420, 7164, 6908, 6652, 6396, 6140,
	5884, 5628, 5372, 5116, 4860, 4604, 4348, 4092,
	3900, 3772, 3644, 3516, 3388, 3260, 3132, 3004,
	2876, 2748, 2620, 2492, 2364, 2236, 2108, 1980,
	1884, 1820, 1756, 1692, 1628, 1564, 1500, 1436,
	1372, 1308, 1244, 1180, 1116, 1052, 988, 924,
	876, 844, 812, 780, 748, 716, 684, 652,
	620, 588, 556, 524, 492, 460, 428, 396,
	372, 356, 340, 324, 308, 292, 276, 260,
	244, 228, 212, 196, 180, 164, 148, 132,
	120, 112, 104, 96, 88, 80, 72, 64,
	56, 48, 40, 32, 24, 16, 8, 0,
};

void ADXB_ExecOneAu(ADXB adxb)
{
	if (adxb->x9c == 2) {
		ADXB_ExecOneAuUlaw(adxb);
	} else if (adxb->x9c == 1) {
		ADXB_ExecOneAu8(adxb);
	} else {
		ADXB_ExecOneAu16(adxb);
	}
}

void ADXB_ExecOneAuUlaw(ADXB adxb)
{
	Uint8 *inbuf;
	Sint16 *out0;
	Sint16 *out1;
	Sint32 i;
	Sint32 n;

	inbuf = (Uint8 *)adxb->inbuf;
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
				out0[i] = ulaw_exp_table[inbuf[i * 2]];
				out1[i] = ulaw_exp_table[inbuf[i * 2 + 1]];
			}
		} else {
			for (i = 0; i < n; i++) {
				out0[i] = ulaw_exp_table[inbuf[i]];
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

void ADXB_ExecOneAu8(ADXB adxb)
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

/* COMPILER-DIFF: M6 - the original's unroller spells the 15th of the 16 unrolled 16-bit swaps of the
 * 2ch loop as `extrwi 8,16` (zero-extended) where ours emits `srawi 8` for all 16; no C spelling
 * changes one copy only and the hand-unrolled forms re-rank the 1ch preheader temporaries (M1).
 * The function is therefore an asm function (the original's instructions verbatim); the C body it
 * encodes is kept under #else. */
#if 1 // COMPILER-DIFF: M6
asm void ADXB_ExecOneAu16(ADXB adxb)
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
	bne Laac
	lwz r3, 8(r31)
	bl ADXPD_GetStat
	cmpwi r3, 0
	bne Laac
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
	ble L7b0
	mr r3, r4
L7b0:
	lwz r0, 76(r31)
	cmpw r3, r0
	ble L7c0
	mr r3, r0
L7c0:
	lbz r0, 14(r31)
	slwi r4, r5, 1
	lwz r6, 92(r31)
	extsb r0, r0
	cmpwi r0, 2
	add r0, r6, r4
	bne L990
	lwz r4, 100(r31)
	cmpwi r3, 0
	li r8, 0
	add r4, r4, r5
	slwi r4, r4, 1
	add r7, r6, r4
	ble La8c
	cmpwi r3, 8
	addi r10, r3, -8
	ble L938
	addi r9, r10, 7
	mr r4, r30
	srwi r9, r9, 3
	mr r5, r0
	mr r6, r7
	mtctr r9
	cmpwi r10, 0
	ble L938
L824:
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
	bdnz L824
L938:
	slwi r9, r8, 1
	slwi r5, r8, 2
	subf r4, r8, r3
	add r5, r30, r5
	add r6, r0, r9
	add r7, r7, r9
	mtctr r4
	cmpw r8, r3
	bge La8c
L95c:
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
	bdnz L95c
	b La8c
L990:
	cmpwi r3, 0
	li r4, 0
	ble La8c
	cmpwi r3, 8
	addi r6, r3, -8
	ble La54
	addi r5, r6, 7
	mr r7, r30
	srwi r5, r5, 3
	mr r8, r0
	mtctr r5
	cmpwi r6, 0
	ble La54
L9c4:
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
	bdnz L9c4
La54:
	slwi r7, r4, 1
	subf r5, r4, r3
	add r6, r30, r7
	add r7, r0, r7
	mtctr r5
	cmpw r4, r3
	bge La8c
La70:
	lhz r4, 0(r6)
	addi r6, r6, 2
	srawi r0, r4, 8
	rlwimi r0, r4, 8, 8, 23
	sth r0, 0(r7)
	addi r7, r7, 2
	bdnz La70
La8c:
	stw r3, 144(r31)
	slwi r3, r3, 1
	li r0, 2
	lbz r4, 14(r31)
	extsb r4, r4
	mullw r3, r4, r3
	stw r3, 148(r31)
	stw r0, 4(r31)
Laac:
	lwz r0, 4(r31)
	cmpwi r0, 2
	bne Lad8
	lwz r12, 128(r31)
	lwz r3, 132(r31)
	lwz r4, 148(r31)
	lwz r5, 144(r31)
	mtctr r12
	bctrl
	li r0, 3
	stw r0, 4(r31)
Lad8:
	lwz r0, 20(r1)
	lwz r31, 12(r1)
	lwz r30, 8(r1)
	mtlr r0
	addi r1, r1, 16
	blr
}
#else
void ADXB_ExecOneAu16(ADXB adxb)
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

static Sint32 adxb_DecodeInfoAu(ADXB adxb, Uint8 *buf, Sint32 bsize, Sint16 *hdrlen, Sint32 *type)
{
	Sint32 sfreq;
	Sint32 nch;
	Sint32 bps;
	Sint32 nsmpl;
	Uint8 *p;

	if (bsize < 8) {
		*hdrlen = 0;
		return -1;
	}
	p = AU_GetInfo(buf, bsize, &sfreq, &nch, &bps, &nsmpl, type);
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

Sint32 ADXB_DecodeHeaderAu(ADXB adxb, void *buf, Sint32 bsize)
{
	Sint16 hdrlen;
	Sint32 type;

	adxb->x02 = 1;
	if (adxb_DecodeInfoAu(adxb, buf, bsize, &hdrlen, &type) < 0) {
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
	adxb->x98 = 4;
	adxb->x9c = type;
	return hdrlen;
}

Sint32 ADXB_CheckAu(Uint8 *buf)
{
	if (memcmp(buf, ".snd", 4) == 0 || memcmp(buf, ".sd", 4) == 0) {
		return 1;
	}
	return 0;
}

Uint8 *AU_GetInfo(Uint8 *buf, Sint32 bsize, Sint32 *sfreq, Sint32 *nch, Sint32 *bps, Sint32 *nsmpl, Sint32 *type)
{
	Uint32 magic;
	Sint32 hdrsize;
	Uint32 dsize;
	Uint32 enc;

	magic = LE32(buf);
	if (magic != AU_MAGIC1 && magic != AU_MAGIC2) {
		return NULL;
	}
	hdrsize = LE32(buf + 4);
	hdrsize = SWAP32(hdrsize);
	if (hdrsize > bsize) {
		return NULL;
	}
	dsize = LE32(buf + 8);
	enc = LE32(buf + 0xC);
	enc = SWAP32(enc);
	dsize = SWAP32(dsize);
	switch (enc) {
	case AU_ENC_ULAW:
		*type = 2;
		*bps = 8;
		break;
	case AU_ENC_PCM8:
		*type = 1;
		*bps = 8;
		break;
	case AU_ENC_PCM16:
		*type = 0;
		*bps = 16;
		break;
	default:
		return NULL;
	}
	*sfreq = LE32(buf + 0x10);
	*sfreq = SWAP32(*sfreq);
	*nch = LE32(buf + 0x14);
	*nch = SWAP32(*nch);
	if (*type == 2) {
		*nsmpl = (Sint32)dsize / *nch;
	} else if (*type == 1) {
		*nsmpl = (Sint32)dsize / *nch;
	} else if (*type == 0) {
		*nsmpl = ((Sint32)dsize / 2) / *nch;
	} else {
		*nsmpl = 0x7FFF0000;
	}
	return buf + hdrsize;
}
