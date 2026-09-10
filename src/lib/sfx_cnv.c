/* Sofdec SFX: conversion table setup and frame layout helpers */
#include "cri_xpt.h"
#include "sfx.h"

/* dead-stripped by the linker */
void SFX_MakeTblZ32(SFX_OBJ *sfx, SFX_FRM *frm)
{
	if (sfx->x30 == 0) {
		SFXLIB_Error(sfx, frm, "E202281: SFX_MakeTblZ32 : zclip is not set.");
		return;
	}
	SFXZ_MakeCnvZTbl(sfx->sfxz, frm->tblsrc, sfx->buf[0]);
}

/* dead-stripped by the linker */
void SFX_MakeTblZ16(SFX_OBJ *sfx, SFX_FRM *frm)
{
	if (sfx->x30 == 0) {
		SFXLIB_Error(sfx, frm, "E202282: SFX_MakeTblZ16 : zclip is not set.");
		return;
	}
	SFXZ_MakeCnvZTbl(sfx->sfxz, frm->tblsrc, sfx->buf[0]);
}

/* COMPILER-DIFF: M1 - SFX_MakeTable's LUMI table loop: the original shares one `li r0, 0` between the
 * unroller's guard compare and the 16 zero stores and numbers the eight fp-conversion stack slots /
 * FPRs of the unrolled 1.164f loop differently (constant CSE across the unroller guard, a
 * compiler-build difference; no C spelling or pin reproduces the slot order). The function is an
 * asm function (the original's instructions verbatim over named literals); the C body it encodes
 * is kept under #else. */
static const Float32 sfxcnv_lumi_gain = 1.164f; // COMPILER-DIFF: M1
static const Float64 sfxcnv_cvt = 4503601774854144.0; /* 0x43300000_80000000, the Sint32 -> float bias */
/* .rodata order: a string referenced from asm is emitted when the reference is parsed, a scalar
 * literal at the end of the referencing function, so the two literals are referenced from this dead
 * (stripped) function ahead of the string's reference in SFX_MakeTable */
asm void sfxcnv_pool_order(void) // COMPILER-DIFF: M1
{
	nofralloc
	lis r3, sfxcnv_lumi_gain@ha
	lis r3, sfxcnv_cvt@ha
	blr
}
static const Char8 sfxcnv_msg_compo[] = "E201311: sfxcnv_MakeTable : compo is not support.";

#if 1 // COMPILER-DIFF: M1
asm void SFX_MakeTable(SFX_OBJ *sfx, SFX_FRM *frm, Sint32 type)
{
	nofralloc
	stwu r1, -160(r1)
	mflr r0
	stw r0, 164(r1)
	stw r31, 156(r1)
	li r31, 1
	stw r30, 152(r1)
	mr r30, r5
	stw r29, 148(r1)
	mr r29, r4
	stw r28, 144(r1)
	mr r28, r3
	lwz r0, 60(r3)
	cmpwi r0, 100
	bne L40
	li r31, 0
	b Lbc
L40:
	cmpw r0, r30
	bne Lbc
	cmpwi r30, 11
	bge L80
	cmpwi r30, 2
	beq La0
	bge L6c
	cmpwi r30, 0
	beq Lbc
	bge Lb8
	b Lbc
L6c:
	cmpwi r30, 6
	bge Lbc
	cmpwi r30, 4
	bge Lb8
	b Lbc
L80:
	cmpwi r30, 23
	bge L94
	cmpwi r30, 21
	bge Lb8
	b Lbc
L94:
	cmpwi r30, 100
	beq Lbc
	b Lbc
La0:
	lwz r3, 52(r28)
	bl SFXA_IsNeedUpdateLumiTbl
	cmpwi r3, 1
	beq Lbc
	li r31, 0
	b Lbc
Lb8:
	li r31, 0
Lbc:
	cmpwi r31, 1
	bne L458
	cmpwi r30, 11
	stw r30, 60(r28)
	beq L130
	bge L100
	cmpwi r30, 3
	beq L444
	bge Lf0
	cmpwi r30, 1
	beq L198
	bge L144
	b L444
Lf0:
	cmpwi r30, 5
	beq L16c
	bge L444
	b L158
L100:
	cmpwi r30, 22
	beq L18c
	bge L124
	cmpwi r30, 13
	beq L130
	blt L444
	cmpwi r30, 21
	bge L180
	b L444
L124:
	cmpwi r30, 100
	beq L444
	b L444
L130:
	lwz r3, 40(r28)
	lwz r4, 76(r29)
	lwz r5, 64(r28)
	bl SFXZ_MakeCnvZTbl
	b L458
L144:
	lwz r3, 52(r28)
	lwz r4, 76(r29)
	lwz r5, 64(r28)
	bl SFXA_MakeAlpLumiTbl
	b L458
L158:
	lwz r3, 52(r28)
	lwz r4, 76(r29)
	lwz r5, 64(r28)
	bl SFXA_MakeAlp3110Tbl
	b L458
L16c:
	lwz r3, 52(r28)
	lwz r4, 76(r29)
	lwz r5, 64(r28)
	bl SFXA_MakeAlp3211Tbl
	b L458
L180:
	lwz r3, 64(r28)
	bl CFT_MakeArgb8888ColAdjTbl
	b L458
L18c:
	lwz r3, 64(r28)
	bl CFT_MakeYcc422ColAdjTbl
	b L458
L198:
	li r0, 0
	lwz r3, 64(r28)
	cmpwi r0, 15
	bgt L1e8
	stb r0, 0(r3)
	stb r0, 1(r3)
	stb r0, 2(r3)
	stb r0, 3(r3)
	stb r0, 4(r3)
	stb r0, 5(r3)
	stb r0, 6(r3)
	stb r0, 7(r3)
	stb r0, 8(r3)
	stb r0, 9(r3)
	stb r0, 10(r3)
	stb r0, 11(r3)
	stb r0, 12(r3)
	stb r0, 13(r3)
	stb r0, 14(r3)
	stb r0, 15(r3)
L1e8:
	li r4, 16
	cmpwi r4, 235
	bgt L3e0
	lis r6, sfxcnv_lumi_gain@ha
	lis r5, sfxcnv_cvt@ha
	li r0, 27
	lfs f1, sfxcnv_lumi_gain@l(r6)
	lfd f0, sfxcnv_cvt@l(r5)
	lis r10, 17200
	mtctr r0
L210:
	addi r0, r4, -16
	stw r10, 8(r1)
	xoris r6, r0, 32768
	addi r5, r4, -15
	stw r6, 12(r1)
	xoris r6, r5, 32768
	addi r0, r4, -14
	addi r5, r4, -12
	lfd f2, 8(r1)
	xoris r9, r0, 32768
	stw r6, 28(r1)
	addi r0, r4, -13
	fsubs f3, f2, f0
	xoris r8, r0, 32768
	stw r10, 24(r1)
	xoris r7, r5, 32768
	addi r0, r4, -11
	add r11, r3, r4
	fmuls f4, f1, f3
	lfd f2, 24(r1)
	stw r9, 44(r1)
	xoris r6, r0, 32768
	fsubs f3, f2, f0
	addi r0, r4, -10
	fctiwz f6, f4
	stw r10, 40(r1)
	fmuls f4, f1, f3
	xoris r5, r0, 32768
	lfd f2, 40(r1)
	addi r0, r4, -9
	fsubs f3, f2, f0
	stw r8, 60(r1)
	fctiwz f5, f4
	xoris r0, r0, 32768
	stw r10, 56(r1)
	addi r4, r4, 8
	lfd f2, 56(r1)
	fmuls f3, f1, f3
	stw r7, 76(r1)
	fsubs f2, f2, f0
	fctiwz f4, f3
	stw r10, 72(r1)
	fmuls f2, f1, f2
	stfd f6, 16(r1)
	stw r6, 92(r1)
	fctiwz f3, f2
	lfd f2, 72(r1)
	stw r10, 88(r1)
	fsubs f2, f2, f0
	lwz r8, 20(r1)
	stfd f5, 32(r1)
	fmuls f2, f1, f2
	stw r5, 108(r1)
	lwz r7, 36(r1)
	stw r10, 104(r1)
	fctiwz f2, f2
	stfd f3, 64(r1)
	stfd f2, 80(r1)
	lfd f2, 88(r1)
	stb r8, 0(r11)
	fsubs f2, f2, f0
	lwz r6, 84(r1)
	stfd f4, 48(r1)
	fmuls f3, f1, f2
	stb r7, 1(r11)
	lwz r8, 52(r1)
	lfd f2, 104(r1)
	fctiwz f4, f3
	stw r0, 124(r1)
	fsubs f3, f2, f0
	lwz r7, 68(r1)
	stw r10, 120(r1)
	lfd f2, 120(r1)
	fmuls f3, f1, f3
	stb r8, 2(r11)
	fsubs f2, f2, f0
	fctiwz f3, f3
	stb r7, 3(r11)
	fmuls f2, f1, f2
	stfd f4, 96(r1)
	stb r6, 4(r11)
	fctiwz f2, f2
	lwz r0, 100(r1)
	stfd f3, 112(r1)
	stb r0, 5(r11)
	lwz r0, 116(r1)
	stfd f2, 128(r1)
	stb r0, 6(r11)
	lwz r0, 132(r1)
	stb r0, 7(r11)
	bdnz L210
	lis r5, sfxcnv_cvt@ha
	lis r6, sfxcnv_lumi_gain@ha
	addi r7, r5, sfxcnv_cvt@l
	subfic r0, r4, 236
	lfs f2, sfxcnv_lumi_gain@l(r6)
	add r5, r3, r4
	lis r6, 17200
	lfd f1, 0(r7)
	mtctr r0
	cmpwi r4, 235
	bgt L3e0
L3a8:
	addi r0, r4, -16
	stw r6, 128(r1)
	xoris r0, r0, 32768
	addi r4, r4, 1
	stw r0, 132(r1)
	lfd f0, 128(r1)
	fsubs f0, f0, f1
	fmuls f0, f2, f0
	fctiwz f0, f0
	stfd f0, 120(r1)
	lwz r0, 124(r1)
	stb r0, 0(r5)
	addi r5, r5, 1
	bdnz L3a8
L3e0:
	li r0, 236
	cmpwi r0, 255
	bgt L458
	li r0, 255
	stb r0, 236(r3)
	stb r0, 237(r3)
	stb r0, 238(r3)
	stb r0, 239(r3)
	stb r0, 240(r3)
	stb r0, 241(r3)
	stb r0, 242(r3)
	stb r0, 243(r3)
	stb r0, 244(r3)
	stb r0, 245(r3)
	stb r0, 246(r3)
	stb r0, 247(r3)
	stb r0, 248(r3)
	stb r0, 249(r3)
	stb r0, 250(r3)
	stb r0, 251(r3)
	stb r0, 252(r3)
	stb r0, 253(r3)
	stb r0, 254(r3)
	stb r0, 255(r3)
	b L458
L444:
	lis r4, sfxcnv_msg_compo@ha
	mr r3, r28
	addi r5, r4, sfxcnv_msg_compo@l
	mr r4, r29
	bl SFXLIB_Error
L458:
	lwz r0, 164(r1)
	lwz r31, 156(r1)
	lwz r30, 152(r1)
	lwz r29, 148(r1)
	lwz r28, 144(r1)
	mtlr r0
	addi r1, r1, 160
	blr
}
#else
void SFX_MakeTable(SFX_OBJ *sfx, SFX_FRM *frm, Sint32 type)
{
	Bool need;
	Sint32 i;
	Uint8 *tbl;

	need = TRUE;
	if (sfx->tbl_type == SFX_TBL_NONE) {
		need = FALSE;
	} else if (sfx->tbl_type == type) {
		switch (type) {
		case SFX_TBL_ALP_LUMI:
			if (SFXA_IsNeedUpdateLumiTbl(sfx->sfxa) != TRUE) {
				need = FALSE;
			}
			break;
		case SFX_TBL_LUMI:
		case SFX_TBL_ALP3110:
		case SFX_TBL_ALP3211:
		case SFX_TBL_ARGB8888_COLADJ:
		case SFX_TBL_YCC422_COLADJ:
			need = FALSE;
			break;
		case 0:
		case SFX_TBL_Z32:
		case SFX_TBL_Z32 + 1:
		case SFX_TBL_NONE:
		default:
			break;
		}
	}
	if (need != TRUE) {
		return;
	}
	sfx->tbl_type = type;
	switch (type) {
	case SFX_TBL_Z32:
	case SFX_TBL_Z16:
		SFXZ_MakeCnvZTbl(sfx->sfxz, frm->tblsrc, sfx->buf[0]);
		break;
	case SFX_TBL_ALP_LUMI:
		SFXA_MakeAlpLumiTbl(sfx->sfxa, frm->tblsrc, sfx->buf[0]);
		break;
	case SFX_TBL_ALP3110:
		SFXA_MakeAlp3110Tbl(sfx->sfxa, frm->tblsrc, sfx->buf[0]);
		break;
	case SFX_TBL_ALP3211:
		SFXA_MakeAlp3211Tbl(sfx->sfxa, frm->tblsrc, sfx->buf[0]);
		break;
	case SFX_TBL_ARGB8888_COLADJ:
		CFT_MakeArgb8888ColAdjTbl(sfx->buf[0]);
		break;
	case SFX_TBL_YCC422_COLADJ:
		CFT_MakeYcc422ColAdjTbl(sfx->buf[0]);
		break;
	case SFX_TBL_LUMI:
		i = 0;
		tbl = sfx->buf[0];
		for (; i <= 15; i++) {
			tbl[i] = 0;
		}
		for (i = 16; i <= 235; i++) {
			tbl[i] = (Uint8)(Sint32)(1.164f * (Float32)(i - 16));
		}
		for (i = 236; i <= 255; i++) {
			tbl[i] = 0xFF;
		}
		break;
	case 0:
	case SFX_TBL_NONE:
	default:
		SFXLIB_Error(sfx, frm, "E201311: sfxcnv_MakeTable : compo is not support.");
		break;
	}
}
#endif

Sint32 SFX_DecideTableAlph3(SFX_OBJ *sfx, Sint32 compo)
{
	Sint32 fxtype;
	Sint32 ret;

	if (compo == SFX_COMPO_0x51) {
		return SFX_TBL_ALP3110;
	}
	if (compo == SFX_COMPO_0x61) {
		return SFX_TBL_ALP3211;
	}
	fxtype = SFX_GetFxType(sfx);
	if (fxtype == SFX_COMPO_0x51) {
		ret = SFX_TBL_ALP3110;
	} else if (fxtype == SFX_COMPO_0x61) {
		ret = SFX_TBL_ALP3211;
	} else {
		ret = SFX_TBL_ALP3211;
	}
	return ret;
}

Sint32 sfxcnv_IsCnvUpHalf(SFX_OBJ *sfx)
{
	switch (sfx->compo) {
	case SFX_COMPO_YCC420PLN:
	case SFX_COMPO_0x31:
	case SFX_COMPO_0x41:
	case SFX_COMPO_0x51:
	case SFX_COMPO_0x61:
	case SFX_COMPO_0x71:
	case SFX_COMPO_0xF1:
	case SFX_COMPO_0x111:
	case SFX_COMPO_0x1001:
		return 0;
	case SFX_COMPO_YCC420PLN_UPHALF:
	case SFX_COMPO_0x101:
		return 1;
	default:
		SFXLIB_Error(NULL, NULL, "E201312: sfxcnv_IsCnvUpHalf : compo is invalid.");
		break;
	}
	return 0;
}

void SFX_SetBottomUpPlnBuf(SFX_PLN *pln)
{
	pln->buf += pln->pitch * (pln->height - 1);
	pln->pitch = -pln->pitch;
}
