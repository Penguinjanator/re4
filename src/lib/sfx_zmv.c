/* Sofdec SFX: Z (depth) movie conversion tables (sfx_zmv.c). The Z component of an SFX frame is
 * 8-bit; the conversion table maps it to a 16/32-bit Z buffer value through the original Z range
 * ("ZMFSIZE"/"ZMFDATA" tags of the stream) and the near/far clip planes of the handle. The
 * frame-format converters of this file were dead-stripped by the linker (their error strings and
 * constants remain in .rodata). */
#include "cri_xpt.h"
#include <string.h>
#include <stdio.h>
#include "sj.h"
#include "sfx.h"

#define SFXZ_MAX_HN 8
#define SFXZ_INVALID_Z 0x80000000
#define SFXZ_MAX_Z 0x7FFFFFFF

typedef struct SFXZ_OBJ {
	Sint32 used;               /* 0x00 */
	Sint32 zbit;               /* 0x04 output Z depth (16/32) */
	Sint32 taginf_flg;         /* 0x08 */
	Uint8 *tag_a;              /* 0x0C */
	Sint32 tag_b;              /* 0x10 */
	Sint32 pad14;
	Sint32 hdr_flg;            /* 0x18 "ZMHDR" searched */
	Sint32 hdr_a;              /* 0x1C */
	Sint32 hdr_b;              /* 0x20 */
	Sint32 pad24;
	Sint32 frm_flg;            /* 0x28 "ZMVFRM" searched */
	Uint8 *frm_ptr;            /* 0x2C */
	Sint32 frm_len;            /* 0x30 */
	Sint32 pad34;
	Sint32 pad38;
	Float32 zmin;              /* 0x3C */
	Float32 zmax;              /* 0x40 */
	void (*cnvfunc)(Uint32 *orgtbl, void *tbl, Float32 zmin, Float32 zmax); /* 0x44 user table maker */
	Sint32 pad48;
} SFXZ_OBJ;

typedef struct {
	Sint32 cnt;                /* 0x00 */
	Sint32 linear;             /* 0x04 linear Z mapping */
	Sint32 nobj;               /* 0x08 */
	SFXZ_OBJ obj[SFXZ_MAX_HN]; /* 0x0C */
} SFXZ_WORK;

extern Sint32 SFX_GetCcirFx(void);
extern Sint32 sscanf(const Char8 *s, const Char8 *fmt, ...);

SFXZ_WORK sfxz_work;

void sfxzmv_MakeOrgZ32TblByCCIR(SFXZ_OBJ *sfxz, Uint32 zmf_dat, Uint32 zmf_siz, Uint32 *tbl);
void sfxzmv_MakeOrgZ32TblByDirect(SFXZ_OBJ *sfxz, Uint32 zmf_dat, Uint32 zmf_siz, Uint32 *tbl);

/* the first .rodata string is named so that the asm sfxzmv_MakeCnvZTbl below can address the string
 * pool through it (it is the compiler's `...rodata.0` base) COMPILER-DIFF: M1 */
extern Uint32 __cvt_fp2unsigned(Float64 v); /* runtime call of the asm functions */
static const Char8 sfxz_msg_zbit[] = "E201313: sfxcnv_CnvZbitToCft : zbit is invalid.";

/* dead */
static Sint32 sfxcnv_CnvZbitToCft(Sint32 zbit)
{
	Sint32 ret;

	switch (zbit) {
	case 16:
		ret = 1;
		break;
	case 24:
		ret = 2;
		break;
	case 32:
		ret = 3;
		break;
	default:
		SFXLIB_Error(NULL, NULL, sfxz_msg_zbit);
		ret = 0;
		break;
	}
	return ret;
}

/* dead */
static void sfxcnv_CnvFrmZcmn(SFX_OBJ *sfx, SFX_FRM *frm, void *buf)
{
	SFXZ_OBJ *sfxz = sfx->sfxz;

	if (sfx->compo == 0) {
		sfx->compo = SFXINF_GetStmInf(frm, "COMPO");
	}
	if (sfxz->zmin == 0.0f && sfxz->zmax == 0.0f) {
		SFXLIB_Error(sfx, frm, "E201315: sfxcnv_CnvFrmZcmn : zclip is not set.");
		return;
	}
	sfxz->zbit = sfxcnv_CnvZbitToCft(sfxz->zbit);
}

/* dead */
void SFX_CnvFrmZcmn(SFX_OBJ *sfx, SFX_FRM *frm, void *buf)
{
	if (frm->frmfmt != 3) {
		SFXLIB_Error(sfx, frm, "E201191: SFX_CnvFrmZcmn : frmfmt is not support.");
		return;
	}
	sfxcnv_CnvFrmZcmn(sfx, frm, buf);
}

/* dead */
void SFXZ_SetZclip(SFXZ_OBJ *sfxz, Float32 zmin, Float32 zmax)
{
	if (zmin < -14.0f || zmax <= zmin) {
		SFXLIB_Error(NULL, NULL, "E201314: SFXZ_SetZclip : zclip is invalid.");
		return;
	}
	sfxz->zmin = zmin;
	sfxz->zmax = zmax;
}

/* the 32-bit conversion table from the original Z values: linear mapping keeps the upper bits,
 * otherwise the perspective mapping between the clip planes (inlined into sfxzmv_MakeCnvZTbl) */
static void sfxzmv_MakeZ32Tbl(SFXZ_OBJ *sfxz, Uint32 *orgtbl, Uint32 *tbl)
{
	Float64 zmin = sfxz->zmin;
	Float64 zmax = sfxz->zmax;

	if (sfxz_work.linear == 1) {
		Sint32 i;
		Uint32 *src = orgtbl;
		Uint32 *dst = tbl;

		for (i = 0; i < 256; i++) {
			*dst = *src++ & 0x7FFFFF80;
			*dst <<= 1;
			dst++;
		}
	} else {
		Sint32 i;
		Uint32 *src = orgtbl;
		Uint32 *dst = tbl;
		Float64 rcp;
		Float64 a;
		Float64 b;

		rcp = 1.0 / (zmax - zmin);
		a = 16777215.0 * zmax * rcp;
		b = zmax * (16777215.0 * rcp * zmin);
		for (i = 0; i < 256; i++) {
			if (*src == 0) {
				*src = 1;
			}
			*dst++ = (Uint32)(a - b / (zmax * (Float64)*src / 2147483647.0));
			src++;
		}
	}
}

/* the 16-bit conversion table */
static void sfxzmv_MakeZ16Tbl(SFXZ_OBJ *sfxz, Uint32 *orgtbl, Uint16 *tbl)
{
	Float64 zmin = sfxz->zmin;
	Float64 zmax = sfxz->zmax;

	if (sfxz_work.linear == 1) {
		Sint32 i;
		Uint32 *src = orgtbl;
		Uint16 *dst = tbl;

		for (i = 0; i < 256; i++) {
			*dst++ = (Uint16)(*src++ >> 15);
		}
	} else {
		Sint32 i;
		Uint32 *src = orgtbl;
		Uint16 *dst = tbl;
		Float64 rcp;
		Float64 a;
		Float64 b;

		rcp = 1.0 / (zmax - zmin);
		a = 65535.0 * zmax * rcp;
		b = zmax * (65535.0 * rcp * zmin);
		for (i = 0; i < 256; i++) {
			if (*src == 0) {
				*src = 1;
			}
			*dst++ = (Sint32)(a - b / (zmax * (Float64)*src / 2147483647.0));
			src++;
		}
	}
}

/* the 256-entry conversion table at tbl (Uint16 or Uint32 per zbit) from the original 32-bit Z
 * values built at tbl + 0x400 (or by the handle's own table maker). (M1: the linear loops' src/dst
 * copies are r3/r4 in the original, ours r4/r3) */
/* COMPILER-DIFF: M1 - inlined helper src/dst r3/r4 and the pool base; the string pool is addressed through sfxz_msg_zbit. Asm function (the original's instructions verbatim). */
asm void sfxzmv_MakeCnvZTbl(SFXZ_OBJ *sfxz, Uint32 zmf_dat, Uint32 zmf_siz, void *tbl) // COMPILER-DIFF: M1
{
	nofralloc
	stwu r1, -128(r1)
	mflr r0
	stw r0, 132(r1)
	stfd f31, 112(r1)
	psq_st f31, 120(r1), 0, 0
	stfd f30, 96(r1)
	psq_st f30, 104(r1), 0, 0
	stfd f29, 80(r1)
	psq_st f29, 88(r1), 0, 0
	stfd f28, 64(r1)
	psq_st f28, 72(r1), 0, 0
	stfd f27, 48(r1)
	psq_st f27, 56(r1), 0, 0
	stmw r26, 24(r1)
	mr r31, r6
	lis r6, sfxz_msg_zbit@ha
	mr r26, r3
	mr r29, r4
	mr r28, r5
	mr r3, r31
	addi r30, r6, sfxz_msg_zbit@l
	addi r27, r31, 1024
	li r4, 0
	li r5, 1024
	bl memset
	bl SFX_GetCcirFx
	cmpwi r3, 1
	bne L88
	mr r3, r26
	mr r4, r29
	mr r5, r28
	mr r6, r27
	bl sfxzmv_MakeOrgZ32TblByCCIR
	b L9c
L88:
	mr r3, r26
	mr r4, r29
	mr r5, r28
	mr r6, r27
	bl sfxzmv_MakeOrgZ32TblByDirect
L9c:
	lwz r12, 68(r26)
	cmplwi r12, 0
	bne L43c
	lwz r0, 4(r26)
	cmpwi r0, 16
	bne L2a0
	lis r3, sfxz_work@ha
	lfs f6, 60(r26)
	addi r3, r3, sfxz_work@l
	lfs f5, 64(r26)
	lwz r0, 4(r3)
	cmpwi r0, 1
	bne L1bc
	li r0, 0
	cmpwi r0, 256
	bge L454
	li r0, 16
	mr r3, r27
	mr r4, r31
	mtctr r0
Lec:
	lwz r0, 0(r3)
	rlwinm r0, r0, 17, 16, 31
	sth r0, 0(r4)
	lwz r0, 4(r3)
	rlwinm r0, r0, 17, 16, 31
	sth r0, 2(r4)
	lwz r0, 8(r3)
	rlwinm r0, r0, 17, 16, 31
	sth r0, 4(r4)
	lwz r0, 12(r3)
	rlwinm r0, r0, 17, 16, 31
	sth r0, 6(r4)
	lwz r0, 16(r3)
	rlwinm r0, r0, 17, 16, 31
	sth r0, 8(r4)
	lwz r0, 20(r3)
	rlwinm r0, r0, 17, 16, 31
	sth r0, 10(r4)
	lwz r0, 24(r3)
	rlwinm r0, r0, 17, 16, 31
	sth r0, 12(r4)
	lwz r0, 28(r3)
	rlwinm r0, r0, 17, 16, 31
	sth r0, 14(r4)
	lwz r0, 32(r3)
	rlwinm r0, r0, 17, 16, 31
	sth r0, 16(r4)
	lwz r0, 36(r3)
	rlwinm r0, r0, 17, 16, 31
	sth r0, 18(r4)
	lwz r0, 40(r3)
	rlwinm r0, r0, 17, 16, 31
	sth r0, 20(r4)
	lwz r0, 44(r3)
	rlwinm r0, r0, 17, 16, 31
	sth r0, 22(r4)
	lwz r0, 48(r3)
	rlwinm r0, r0, 17, 16, 31
	sth r0, 24(r4)
	lwz r0, 52(r3)
	rlwinm r0, r0, 17, 16, 31
	sth r0, 26(r4)
	lwz r0, 56(r3)
	rlwinm r0, r0, 17, 16, 31
	sth r0, 28(r4)
	lwz r0, 60(r3)
	addi r3, r3, 64
	rlwinm r0, r0, 17, 16, 31
	sth r0, 30(r4)
	addi r4, r4, 32
	bdnz Lec
	b L454
L1bc:
	fsub f0, f5, f6
	lfd f1, 208(r30)
	lfd f3, 240(r30)
	li r0, 128
	mr r5, r27
	mr r6, r31
	fdiv f4, f1, f0
	lfd f2, 232(r30)
	lfd f1, 224(r30)
	li r7, 0
	lis r3, 17200
	li r4, 1
	fmul f0, f3, f4
	fmul f3, f3, f5
	fmul f0, f0, f6
	fmul f4, f3, f4
	fmul f3, f5, f0
	mtctr r0
L204:
	lwz r0, 0(r5)
	cmplwi r0, 0
	bne L214
	stw r4, 0(r5)
L214:
	lwz r0, 0(r5)
	stw r3, 8(r1)
	stw r0, 12(r1)
	lfd f0, 8(r1)
	fsub f0, f0, f2
	fmul f0, f5, f0
	fdiv f0, f0, f1
	fdiv f0, f3, f0
	fsub f0, f4, f0
	fctiwz f0, f0
	stfd f0, 16(r1)
	lwz r0, 20(r1)
	sth r0, 0(r6)
	lwz r0, 4(r5)
	cmplwi r0, 0
	bne L258
	stw r4, 4(r5)
L258:
	lwz r0, 4(r5)
	addi r5, r5, 8
	stw r3, 8(r1)
	addi r7, r7, 1
	stw r0, 12(r1)
	lfd f0, 8(r1)
	fsub f0, f0, f2
	fmul f0, f5, f0
	fdiv f0, f0, f1
	fdiv f0, f3, f0
	fsub f0, f4, f0
	fctiwz f0, f0
	stfd f0, 16(r1)
	lwz r0, 20(r1)
	sth r0, 2(r6)
	addi r6, r6, 4
	bdnz L204
	b L454
L2a0:
	lis r3, sfxz_work@ha
	lfs f3, 60(r26)
	addi r3, r3, sfxz_work@l
	lfs f27, 64(r26)
	lwz r0, 4(r3)
	cmpwi r0, 1
	bne L3a8
	li r0, 0
	cmpwi r0, 256
	bge L454
	li r0, 32
	mr r3, r27
	mr r4, r31
	mtctr r0
L2d8:
	lwz r0, 0(r3)
	rlwinm r0, r0, 0, 1, 24
	stw r0, 0(r4)
	lwz r0, 0(r4)
	slwi r0, r0, 1
	stw r0, 0(r4)
	lwz r0, 4(r3)
	rlwinm r0, r0, 0, 1, 24
	stw r0, 4(r4)
	lwz r0, 4(r4)
	slwi r0, r0, 1
	stw r0, 4(r4)
	lwz r0, 8(r3)
	rlwinm r0, r0, 0, 1, 24
	stw r0, 8(r4)
	lwz r0, 8(r4)
	slwi r0, r0, 1
	stw r0, 8(r4)
	lwz r0, 12(r3)
	rlwinm r0, r0, 0, 1, 24
	stw r0, 12(r4)
	lwz r0, 12(r4)
	slwi r0, r0, 1
	stw r0, 12(r4)
	lwz r0, 16(r3)
	rlwinm r0, r0, 0, 1, 24
	stw r0, 16(r4)
	lwz r0, 16(r4)
	slwi r0, r0, 1
	stw r0, 16(r4)
	lwz r0, 20(r3)
	rlwinm r0, r0, 0, 1, 24
	stw r0, 20(r4)
	lwz r0, 20(r4)
	slwi r0, r0, 1
	stw r0, 20(r4)
	lwz r0, 24(r3)
	rlwinm r0, r0, 0, 1, 24
	stw r0, 24(r4)
	lwz r0, 24(r4)
	slwi r0, r0, 1
	stw r0, 24(r4)
	lwz r0, 28(r3)
	addi r3, r3, 32
	rlwinm r0, r0, 0, 1, 24
	stw r0, 28(r4)
	lwz r0, 28(r4)
	slwi r0, r0, 1
	stw r0, 28(r4)
	addi r4, r4, 32
	bdnz L2d8
	b L454
L3a8:
	fsub f0, f27, f3
	lfd f2, 208(r30)
	lfd f1, 216(r30)
	mr r29, r27
	lfd f30, 232(r30)
	mr r28, r31
	fdiv f2, f2, f0
	lfd f31, 224(r30)
	li r27, 0
	li r30, 1
	lis r31, 17200
	fmul f0, f1, f2
	fmul f1, f1, f27
	fmul f0, f0, f3
	fmul f28, f1, f2
	fmul f29, f27, f0
L3e8:
	lwz r0, 0(r29)
	cmplwi r0, 0
	bne L3f8
	stw r30, 0(r29)
L3f8:
	lwz r0, 0(r29)
	stw r31, 16(r1)
	stw r0, 20(r1)
	lfd f0, 16(r1)
	fsub f0, f0, f30
	fmul f0, f27, f0
	fdiv f0, f0, f31
	fdiv f0, f29, f0
	fsub f1, f28, f0
	bl __cvt_fp2unsigned
	addi r27, r27, 1
	stw r3, 0(r28)
	cmpwi r27, 256
	addi r29, r29, 4
	addi r28, r28, 4
	blt L3e8
	b L454
L43c:
	mr r3, r27
	mr r4, r31
	lfs f1, 60(r26)
	lfs f2, 64(r26)
	mtctr r12
	bctrl
L454:
	psq_l f31, 120(r1), 0, 0
	lfd f31, 112(r1)
	psq_l f30, 104(r1), 0, 0
	lfd f30, 96(r1)
	psq_l f29, 88(r1), 0, 0
	lfd f29, 80(r1)
	psq_l f28, 72(r1), 0, 0
	lfd f28, 64(r1)
	psq_l f27, 56(r1), 0, 0
	lfd f27, 48(r1)
	lmw r26, 24(r1)
	lwz r0, 132(r1)
	mtlr r0
	addi r1, r1, 128
	blr
}
/* the C body, kept compiled (dead, stripped by strip_unused) so that its literals stay in .rodata */
void sfxzmv_MakeCnvZTbl_c(SFXZ_OBJ *sfxz, Uint32 zmf_dat, Uint32 zmf_siz, void *tbl)
{
	Uint32 *orgtbl;

	orgtbl = (Uint32 *)((Uint8 *)tbl + 0x400);
	memset(tbl, 0, 0x400);
	if (SFX_GetCcirFx() == 1) {
		sfxzmv_MakeOrgZ32TblByCCIR(sfxz, zmf_dat, zmf_siz, orgtbl);
	} else {
		sfxzmv_MakeOrgZ32TblByDirect(sfxz, zmf_dat, zmf_siz, orgtbl);
	}
	if (sfxz->cnvfunc == NULL) {
		if (sfxz->zbit == 16) {
			sfxzmv_MakeZ16Tbl(sfxz, orgtbl, (Uint16 *)tbl);
		} else {
			sfxzmv_MakeZ32Tbl(sfxz, orgtbl, (Uint32 *)tbl);
		}
	} else {
		sfxz->cnvfunc(orgtbl, tbl, sfxz->zmin, sfxz->zmax);
	}
}

/* the original Z range of the frame from the "ZMFSIZE"/"ZMFDATA" tags of the stream information
 * (the frame's entry is `n * src` bytes into the data tag) */
#pragma dont_inline on
void SFXZ_MakeCnvZTbl(SFXZ_OBJ *sfxz, void *src, void *tbl)
{
	SJCK out1;
	SJCK inf1;
	SJCK out2;
	SJCK inf2;
	Uint32 zmf_c;
	Uint32 zmf_b;
	Uint32 zmf_a;
	Uint32 n;
	Char8 *p;
	Uint32 zmf_dat;
	Uint32 zmf_siz;

	if (sfxz->frm_flg != 1 || sfxz->frm_ptr == NULL) {
		p = NULL;
	} else {
		inf1.data = sfxz->frm_ptr;
		inf1.len = sfxz->frm_len;
		SJ_SearchTag(&inf1, "ZMFSIZE", "SFXINFE", &out1);
		p = (Char8 *)out1.data;
	}
	if (p == NULL) {
		zmf_dat = 0;
		zmf_siz = SFXZ_MAX_Z;
	} else {
		sscanf(p, "%lx", &n);
		if (sfxz->frm_flg != 1 || sfxz->frm_ptr == NULL) {
			p = NULL;
		} else {
			inf2.data = sfxz->frm_ptr;
			inf2.len = sfxz->frm_len;
			SJ_SearchTag(&inf2, "ZMFDATA", "SFXINFE", &out2);
			p = (Char8 *)out2.data;
		}
		if (p == NULL) {
			zmf_dat = SFXZ_MAX_Z;
		} else {
			sscanf(p + n * (Uint32)src, "%lx %lx %lx", &zmf_a, &zmf_b, &zmf_c);
			zmf_dat = zmf_b;
			zmf_siz = zmf_c;
		}
	}
	sfxzmv_MakeCnvZTbl(sfxz, zmf_dat, zmf_siz, tbl);
}
#pragma dont_inline off

/* the 32-bit Z of each of the 256 Z index values: 0 below 9, the near value up to 16, a ramp
 * from the near to the far value up to 0xDF, the far value up to 0xEF and the maximum above */
#define SFXZ_MAKE_ORG_Z32_TBL(zmf_dat, zmf_siz, tbl)                                          \
	{                                                                                      \
		Sint32 i;                                                                      \
		Uint32 *p;                                                                     \
		if ((zmf_siz) == SFXZ_INVALID_Z) {                                             \
			(zmf_siz) = SFXZ_MAX_Z;                                                \
		}                                                                              \
		if ((zmf_dat) == SFXZ_INVALID_Z) {                                             \
			(zmf_dat) = SFXZ_MAX_Z;                                                \
		}                                                                              \
		p = (tbl);                                                                     \
		for (i = 0; i < 9; i++) {                                                      \
			*p++ = 0;                                                              \
		}                                                                              \
		for (i = 9; i < 17; i++) {                                                     \
			(tbl)[i] = (zmf_dat);                                                  \
		}                                                                              \
		if ((zmf_dat) == (zmf_siz)) {                                                  \
			for (i = 17; i < 0xE0; i++) {                                          \
				(tbl)[i] = (zmf_dat);                                          \
			}                                                                      \
		} else {                                                                       \
			for (i = 17; i < 0xE0; i++) {                                          \
				(tbl)[i] = (zmf_dat) + (i - 17) * (((zmf_siz) - (zmf_dat)) / (0xE0 - 17)); \
			}                                                                      \
		}                                                                              \
		for (i = 0xE0; i < 0xF0; i++) {                                                \
			(tbl)[i] = (zmf_siz);                                                  \
		}                                                                              \
		for (i = 0xF0; i < 0x100; i++) {                                               \
			(tbl)[i] = SFXZ_MAX_Z;                                                 \
		}                                                                              \
	}

/* the 32-bit Z of each 8-bit Z sample: the CCIR601 luma range (16..235) is stretched to 0..255
 * first, then the index maps to the original range. (OPEN: the 8x unrolled 1.164 loop is scheduled
 * differently, the original computes the eight `i - 16` values up front; same instructions) */
static const Float32 sfxz_lumi_gain = 1.164f; // COMPILER-DIFF: M1 (the asm function's literals)
static const Float64 sfxz_cvt = 4503601774854144.0; /* 0x43300000_80000000, the Sint32 -> float bias */

/* COMPILER-DIFF: M1 - the unrolled luma-table conversion loop's slot/register order (see sfx_cnv). Asm function (the original's instructions verbatim). */
#if 1 // COMPILER-DIFF: M1
asm void sfxzmv_MakeOrgZ32TblByCCIR(SFXZ_OBJ *sfxz, Uint32 zmf_dat, Uint32 zmf_siz, Uint32 *tbl)
{
	nofralloc
	stwu r1, -160(r1)
	li r0, 0
	addi r9, r6, 1024
	stmw r27, 140(r1)
	cmpwi r0, 15
	addi r8, r9, 1024
	bgt L628
	li r0, 0
	stb r0, 0(r8)
	stb r0, 1(r8)
	stb r0, 2(r8)
	stb r0, 3(r8)
	stb r0, 4(r8)
	stb r0, 5(r8)
	stb r0, 6(r8)
	stb r0, 7(r8)
	stb r0, 8(r8)
	stb r0, 9(r8)
	stb r0, 10(r8)
	stb r0, 11(r8)
	stb r0, 12(r8)
	stb r0, 13(r8)
	stb r0, 14(r8)
	stb r0, 15(r8)
L628:
	li r3, 16
	cmpwi r3, 235
	bgt L824
	lis r7, sfxz_lumi_gain@ha
	lis r10, sfxz_cvt@ha
	addi r11, r7, sfxz_lumi_gain@l
	lfd f0, sfxz_cvt@l(r10)
	li r7, 27
	lfs f1, 0(r11)
	lis r0, 17200
	mtctr r7
L654:
	addi r7, r3, -16
	addi r10, r3, -15
	xoris r11, r7, 32768
	addi r7, r3, -14
	stw r11, 12(r1)
	xoris r11, r10, 32768
	xoris r28, r7, 32768
	addi r7, r3, -13
	stw r0, 8(r1)
	xoris r27, r7, 32768
	addi r10, r3, -12
	addi r7, r3, -11
	lfd f2, 8(r1)
	xoris r12, r10, 32768
	stw r11, 28(r1)
	xoris r11, r7, 32768
	fsubs f3, f2, f0
	addi r7, r3, -10
	stw r0, 24(r1)
	xoris r10, r7, 32768
	addi r7, r3, -9
	add r29, r8, r3
	lfd f2, 24(r1)
	fmuls f4, f1, f3
	stw r28, 44(r1)
	xoris r7, r7, 32768
	fsubs f3, f2, f0
	addi r3, r3, 8
	stw r0, 40(r1)
	fctiwz f6, f4
	lfd f2, 40(r1)
	fmuls f4, f1, f3
	stw r27, 60(r1)
	fsubs f3, f2, f0
	stw r0, 56(r1)
	fctiwz f5, f4
	lfd f2, 56(r1)
	fmuls f3, f1, f3
	stfd f6, 16(r1)
	fsubs f2, f2, f0
	fctiwz f4, f3
	stfd f5, 32(r1)
	lwz r27, 20(r1)
	fmuls f2, f1, f2
	stw r12, 76(r1)
	lwz r12, 36(r1)
	stw r0, 72(r1)
	fctiwz f3, f2
	lfd f2, 72(r1)
	stb r27, 0(r29)
	fsubs f2, f2, f0
	stfd f4, 48(r1)
	fmuls f2, f1, f2
	stfd f3, 64(r1)
	lwz r27, 52(r1)
	stb r12, 1(r29)
	fctiwz f2, f2
	lwz r12, 68(r1)
	stb r27, 2(r29)
	stfd f2, 80(r1)
	stw r11, 92(r1)
	lwz r11, 84(r1)
	stw r0, 88(r1)
	lfd f2, 88(r1)
	stb r12, 3(r29)
	fsubs f2, f2, f0
	stw r10, 108(r1)
	stw r0, 104(r1)
	fmuls f3, f1, f2
	lfd f2, 104(r1)
	fctiwz f4, f3
	stw r7, 124(r1)
	fsubs f3, f2, f0
	stw r0, 120(r1)
	lfd f2, 120(r1)
	fmuls f3, f1, f3
	stfd f4, 96(r1)
	fsubs f2, f2, f0
	fctiwz f3, f3
	stb r11, 4(r29)
	lwz r7, 100(r1)
	fmuls f2, f1, f2
	stfd f3, 112(r1)
	fctiwz f2, f2
	stb r7, 5(r29)
	lwz r7, 116(r1)
	stfd f2, 128(r1)
	stb r7, 6(r29)
	lwz r7, 132(r1)
	stb r7, 7(r29)
	bdnz L654
	lis r7, sfxz_cvt@ha
	lis r10, sfxz_lumi_gain@ha
	addi r11, r7, sfxz_cvt@l
	subfic r0, r3, 236
	lfs f2, sfxz_lumi_gain@l(r10)
	add r7, r8, r3
	lis r10, 17200
	lfd f1, 0(r11)
	mtctr r0
	cmpwi r3, 235
	bgt L824
L7ec:
	addi r0, r3, -16
	stw r10, 128(r1)
	xoris r0, r0, 32768
	addi r3, r3, 1
	stw r0, 132(r1)
	lfd f0, 128(r1)
	fsubs f0, f0, f1
	fmuls f0, f2, f0
	fctiwz f0, f0
	stfd f0, 120(r1)
	lwz r0, 124(r1)
	stb r0, 0(r7)
	addi r7, r7, 1
	bdnz L7ec
L824:
	li r0, 236
	cmpwi r0, 255
	bgt L884
	li r0, 255
	stb r0, 236(r8)
	stb r0, 237(r8)
	stb r0, 238(r8)
	stb r0, 239(r8)
	stb r0, 240(r8)
	stb r0, 241(r8)
	stb r0, 242(r8)
	stb r0, 243(r8)
	stb r0, 244(r8)
	stb r0, 245(r8)
	stb r0, 246(r8)
	stb r0, 247(r8)
	stb r0, 248(r8)
	stb r0, 249(r8)
	stb r0, 250(r8)
	stb r0, 251(r8)
	stb r0, 252(r8)
	stb r0, 253(r8)
	stb r0, 254(r8)
	stb r0, 255(r8)
L884:
	addis r0, r5, -32768
	cmplwi r0, 0
	bne L898
	lis r3, -32768
	addi r5, r3, -1
L898:
	addis r0, r4, -32768
	cmplwi r0, 0
	bne L8ac
	lis r3, -32768
	addi r4, r3, -1
L8ac:
	li r0, 0
	cmpwi r0, 9
	bge L8e0
	li r0, 0
	stw r0, 0(r9)
	stw r0, 4(r9)
	stw r0, 8(r9)
	stw r0, 12(r9)
	stw r0, 16(r9)
	stw r0, 20(r9)
	stw r0, 24(r9)
	stw r0, 28(r9)
	stw r0, 32(r9)
L8e0:
	stw r4, 36(r9)
	cmplw r4, r5
	stw r4, 40(r9)
	stw r4, 44(r9)
	stw r4, 48(r9)
	stw r4, 52(r9)
	stw r4, 56(r9)
	stw r4, 60(r9)
	stw r4, 64(r9)
	bne L9f4
	li r7, 17
	cmpwi r7, 224
	bge Lae0
	li r0, 5
	addi r3, r9, 68
	mtctr r0
L920:
	stw r4, 0(r3)
	addi r7, r7, 40
	stw r4, 4(r3)
	stw r4, 8(r3)
	stw r4, 12(r3)
	stw r4, 16(r3)
	stw r4, 20(r3)
	stw r4, 24(r3)
	stw r4, 28(r3)
	stw r4, 32(r3)
	stw r4, 36(r3)
	stw r4, 40(r3)
	stw r4, 44(r3)
	stw r4, 48(r3)
	stw r4, 52(r3)
	stw r4, 56(r3)
	stw r4, 60(r3)
	stw r4, 64(r3)
	stw r4, 68(r3)
	stw r4, 72(r3)
	stw r4, 76(r3)
	stw r4, 80(r3)
	stw r4, 84(r3)
	stw r4, 88(r3)
	stw r4, 92(r3)
	stw r4, 96(r3)
	stw r4, 100(r3)
	stw r4, 104(r3)
	stw r4, 108(r3)
	stw r4, 112(r3)
	stw r4, 116(r3)
	stw r4, 120(r3)
	stw r4, 124(r3)
	stw r4, 128(r3)
	stw r4, 132(r3)
	stw r4, 136(r3)
	stw r4, 140(r3)
	stw r4, 144(r3)
	stw r4, 148(r3)
	stw r4, 152(r3)
	stw r4, 156(r3)
	addi r3, r3, 160
	bdnz L920
	slwi r3, r7, 2
	subfic r0, r7, 224
	add r3, r9, r3
	mtctr r0
	cmpwi r7, 224
	bge Lae0
L9e4:
	stw r4, 0(r3)
	addi r3, r3, 4
	bdnz L9e4
	b Lae0
L9f4:
	li r7, 17
	cmpwi r7, 224
	bge Lae0
	lis r3, 10131
	subf r0, r4, r5
	addi r3, r3, 11081
	li r10, 25
	mulhwu r0, r3, r0
	addi r3, r9, 68
	srwi r0, r0, 5
	mtctr r10
La20:
	addi r11, r7, -17
	addi r10, r7, -16
	mullw r27, r11, r0
	addi r29, r7, -15
	addi r30, r7, -14
	addi r31, r7, -13
	addi r12, r7, -12
	addi r11, r7, -11
	mullw r28, r10, r0
	add r10, r4, r27
	stw r10, 0(r3)
	addi r10, r7, -10
	addi r7, r7, 8
	mullw r29, r29, r0
	add r28, r4, r28
	stw r28, 4(r3)
	mullw r30, r30, r0
	add r29, r4, r29
	stw r29, 8(r3)
	mullw r31, r31, r0
	add r30, r4, r30
	stw r30, 12(r3)
	mullw r12, r12, r0
	add r31, r4, r31
	stw r31, 16(r3)
	mullw r11, r11, r0
	add r12, r4, r12
	stw r12, 20(r3)
	mullw r10, r10, r0
	add r11, r4, r11
	stw r11, 24(r3)
	add r10, r4, r10
	stw r10, 28(r3)
	addi r3, r3, 32
	bdnz La20
	slwi r10, r7, 2
	subfic r3, r7, 224
	add r10, r9, r10
	mtctr r3
	cmpwi r7, 224
	bge Lae0
Lac4:
	addi r3, r7, -17
	addi r7, r7, 1
	mullw r3, r3, r0
	add r3, r4, r3
	stw r3, 0(r10)
	addi r10, r10, 4
	bdnz Lac4
Lae0:
	li r0, 224
	cmpwi r0, 240
	bge Lb2c
	stw r5, 896(r9)
	stw r5, 900(r9)
	stw r5, 904(r9)
	stw r5, 908(r9)
	stw r5, 912(r9)
	stw r5, 916(r9)
	stw r5, 920(r9)
	stw r5, 924(r9)
	stw r5, 928(r9)
	stw r5, 932(r9)
	stw r5, 936(r9)
	stw r5, 940(r9)
	stw r5, 944(r9)
	stw r5, 948(r9)
	stw r5, 952(r9)
	stw r5, 956(r9)
Lb2c:
	li r0, 240
	cmpwi r0, 256
	bge Lb80
	lis r3, -32768
	addi r0, r3, -1
	stw r0, 960(r9)
	stw r0, 964(r9)
	stw r0, 968(r9)
	stw r0, 972(r9)
	stw r0, 976(r9)
	stw r0, 980(r9)
	stw r0, 984(r9)
	stw r0, 988(r9)
	stw r0, 992(r9)
	stw r0, 996(r9)
	stw r0, 1000(r9)
	stw r0, 1004(r9)
	stw r0, 1008(r9)
	stw r0, 1012(r9)
	stw r0, 1016(r9)
	stw r0, 1020(r9)
Lb80:
	li r4, 0
	cmpwi r4, 255
	bgt Lcac
	li r0, 16
	mtctr r0
Lb94:
	add r3, r8, r4
	addi r4, r4, 8
	lbz r0, 0(r3)
	slwi r0, r0, 2
	lwzx r0, r9, r0
	stw r0, 0(r6)
	lbz r0, 1(r3)
	slwi r0, r0, 2
	lwzx r0, r9, r0
	stw r0, 4(r6)
	lbz r0, 2(r3)
	slwi r0, r0, 2
	lwzx r0, r9, r0
	stw r0, 8(r6)
	lbz r0, 3(r3)
	slwi r0, r0, 2
	lwzx r0, r9, r0
	stw r0, 12(r6)
	lbz r0, 4(r3)
	slwi r0, r0, 2
	lwzx r0, r9, r0
	stw r0, 16(r6)
	lbz r0, 5(r3)
	slwi r0, r0, 2
	lwzx r0, r9, r0
	stw r0, 20(r6)
	lbz r0, 6(r3)
	slwi r0, r0, 2
	lwzx r0, r9, r0
	stw r0, 24(r6)
	lbz r0, 7(r3)
	add r3, r8, r4
	addi r4, r4, 8
	slwi r0, r0, 2
	lwzx r0, r9, r0
	stw r0, 28(r6)
	lbz r0, 0(r3)
	slwi r0, r0, 2
	lwzx r0, r9, r0
	stw r0, 32(r6)
	lbz r0, 1(r3)
	slwi r0, r0, 2
	lwzx r0, r9, r0
	stw r0, 36(r6)
	lbz r0, 2(r3)
	slwi r0, r0, 2
	lwzx r0, r9, r0
	stw r0, 40(r6)
	lbz r0, 3(r3)
	slwi r0, r0, 2
	lwzx r0, r9, r0
	stw r0, 44(r6)
	lbz r0, 4(r3)
	slwi r0, r0, 2
	lwzx r0, r9, r0
	stw r0, 48(r6)
	lbz r0, 5(r3)
	slwi r0, r0, 2
	lwzx r0, r9, r0
	stw r0, 52(r6)
	lbz r0, 6(r3)
	slwi r0, r0, 2
	lwzx r0, r9, r0
	stw r0, 56(r6)
	lbz r0, 7(r3)
	slwi r0, r0, 2
	lwzx r0, r9, r0
	stw r0, 60(r6)
	addi r6, r6, 64
	bdnz Lb94
Lcac:
	lmw r27, 140(r1)
	addi r1, r1, 160
	blr
}
#else
void sfxzmv_MakeOrgZ32TblByCCIR(SFXZ_OBJ *sfxz, Uint32 zmf_dat, Uint32 zmf_siz, Uint32 *tbl)
{
	Sint32 i;
	Uint8 *ytbl;
	Uint32 *ztbl;

	ztbl = tbl + 0x100;
	ytbl = (Uint8 *)(ztbl + 0x100);
	for (i = 0; i <= 0x0F; i++) {
		ytbl[i] = 0;
	}
	for (i = 0x10; i <= 0xEB; i++) {
		ytbl[i] = 1.164f * (Float32)(i - 16);
	}
	for (i = 0xEC; i <= 0xFF; i++) {
		ytbl[i] = 0xFF;
	}
	SFXZ_MAKE_ORG_Z32_TBL(zmf_dat, zmf_siz, ztbl);
	for (i = 0; i <= 0xFF; i++) {
		tbl[i] = ztbl[ytbl[i]];
	}
}
#endif

/* the same without the luma range correction */
void sfxzmv_MakeOrgZ32TblByDirect(SFXZ_OBJ *sfxz, Uint32 zmf_dat, Uint32 zmf_siz, Uint32 *tbl)
{
	SFXZ_MAKE_ORG_Z32_TBL(zmf_dat, zmf_siz, tbl);
}

/* the stream's SFX tag information: the "ZMHDR" and "ZMVFRM" sub-tags */
void SFXZ_SetTagInf(SFXZ_OBJ *sfxz, Uint8 *tag_a, Sint32 tag_b)
{
	SJCK out;
	SJCK inf;

	sfxz->taginf_flg = 1;
	sfxz->tag_a = tag_a;
	sfxz->tag_b = tag_b;
	if (sfxz->tag_a == NULL) {
		sfxz->hdr_flg = 1;
		sfxz->hdr_a = 0;
		sfxz->hdr_b = 0;
		sfxz->frm_flg = 1;
		sfxz->frm_ptr = NULL;
		sfxz->frm_len = 0;
	} else {
		inf.data = sfxz->tag_a;
		inf.len = sfxz->tag_b;
		SJ_SearchTag(&inf, "ZMHDR", "SFXINFE", &out);
		sfxz->hdr_flg = 1;
		sfxz->hdr_a = (Sint32)out.data;
		sfxz->hdr_b = out.len;
		SJ_SearchTag(&inf, "ZMVFRM", "SFXINFE", &out);
		sfxz->frm_flg = 1;
		sfxz->frm_ptr = out.data;
		sfxz->frm_len = out.len;
	}
}

void SFXZ_Destroy(SFXZ_OBJ *sfxz)
{
	if (sfxz == NULL) {
		return;
	}
	sfxz->used = 0;
	sfxz_work.cnt--;
}

static SFXZ_OBJ *sfxz_search_free(void)
{
	SFXZ_OBJ *sfxz;
	Sint32 i;

	sfxz = sfxz_work.obj;
	for (i = 0; i < sfxz_work.nobj; i++) {
		if (sfxz->used == 0) {
			return sfxz;
		}
		sfxz++;
	}
	return NULL;
}

SFXZ_OBJ *SFXZ_Create(void)
{
	SFXZ_OBJ *sfxz;

	sfxz = sfxz_search_free();
	if (sfxz == NULL) {
		return sfxz;
	}
	sfxz->zmin = 0.0f;
	sfxz->zmax = 0.0f;
	sfxz->cnvfunc = NULL;
	sfxz->pad48 = 0;
	sfxz->zbit = 0;
	sfxz_work.cnt++;
	sfxz->used = 1;
	return sfxz;
}

void SFXZ_Init(void)
{
	memset(&sfxz_work, 0, sizeof(sfxz_work));
	sfxz_work.nobj = SFXZ_MAX_HN;
	sfxz_work.linear = 0;
}
