#include "cri_xpt.h"

typedef struct {
	void *p;
	Sint32 n;
} MPVCMC_REF;

typedef struct {
	Uint8 pad0[0xCC];
	Uint8 mc[0x120 - 0xCC]; /* 0xCC */
	Sint32 ccnt_rt;         /* 0x120 */
	MPVCMC_REF oi_rt[6];    /* 0x124 */
	Sint32 ccnt;            /* 0x154 */
	MPVCMC_REF oi[6];       /* 0x158 */
	Uint8 pad188[0x1A4 - 0x188];
	Sint32 mcflag;          /* 0x1A4 */
	Uint8 pad1A8[0x280 - 0x1A8];
	Sint16 width;           /* 0x280 */
	Sint16 height;          /* 0x282 */
	Uint8 pad284[0xD00 - 0x284];
	Uint8 work[1];          /* 0xD00 */
} MPV_OBJ;

extern void MPVMC08_Init(void *mc);
extern void MPVMC16_Init(void *mc);
extern void MPVMC08_OneRef1p_TuneC(void);
extern void MPVMC08_OneRefH2_TuneC(void);
extern void MPVMC08_OneRefV2_TuneC(void);
extern void MPVMC08_OneRef4p_TuneC(void);

void (*mpvcmc_oneref[8])(void);

void MPVCMC_SetCcnt(MPV_OBJ *mpv)
{
	Sint32 ccnt;

	if (mpv->mcflag == 0) {
		ccnt = -1;
	} else {
		ccnt = 4;
	}
	mpv->ccnt = ccnt;
	mpv->ccnt_rt = ccnt;
}

/* COMPILER-DIFF: M1 - the original keeps `addi r5, r3, 0x124` as a separate base for six stores into a member array while every C form folds the offsets into r3 (OPEN since pass 1). Asm function (the original's instructions verbatim). */
#if 1 // COMPILER-DIFF: M1
asm void MPVCMC_InitMcOiRt(MPV_OBJ *mpv)
{
	nofralloc
	lwz r0, 420(r3)
	addi r5, r3, 292
	li r4, 4
	cmpwi r0, 0
	bne L38
	li r4, -1
L38:
	stw r4, 288(r3)
	lha r0, 640(r3)
	stw r0, 4(r5)
	stw r0, 12(r5)
	lha r0, 642(r3)
	stw r0, 20(r5)
	stw r0, 28(r5)
	stw r0, 36(r5)
	stw r0, 44(r5)
	blr
}
#else
void MPVCMC_InitMcOiRt(MPV_OBJ *mpv)
{
	MPVCMC_REF *oi = mpv->oi_rt;
	Sint32 ccnt;
	Sint32 i;

	if (mpv->mcflag == 0) {
		ccnt = -1;
	} else {
		ccnt = 4;
	}
	mpv->ccnt_rt = ccnt;
	for (i = 0; i < 2; i++) {
		oi[i].n = mpv->width;
	}
	for (i = 2; i < 6; i++) {
		oi[i].n = mpv->height;
	}
}
#endif

/* COMPILER-DIFF: M1 - same separate member-array base as MPVCMC_InitMcOiRt. Asm function (the original's instructions verbatim). */
#if 1 // COMPILER-DIFF: M1
asm void MPVCMC_InitObj(MPV_OBJ *mpv)
{
	nofralloc
	stwu r1, -16(r1)
	mflr r0
	stw r0, 20(r1)
	stw r31, 12(r1)
	mr r31, r3
	addi r3, r31, 204
	bl MPVMC08_Init
	addi r3, r31, 204
	bl MPVMC16_Init
	lwz r0, 420(r31)
	addi r5, r31, 344
	addi r4, r31, 3328
	li r3, 4
	cmpwi r0, 0
	bne La0
	li r3, -1
La0:
	stw r3, 340(r31)
	li r0, 8
	stw r4, 0(r5)
	stw r4, 8(r5)
	stw r4, 16(r5)
	stw r4, 24(r5)
	stw r4, 32(r5)
	stw r4, 40(r5)
	stw r0, 4(r5)
	stw r0, 12(r5)
	stw r0, 20(r5)
	stw r0, 28(r5)
	stw r0, 36(r5)
	stw r0, 44(r5)
	lwz r31, 12(r1)
	lwz r0, 20(r1)
	mtlr r0
	addi r1, r1, 16
	blr
}
#else
void MPVCMC_InitObj(MPV_OBJ *mpv)
{
	MPVCMC_REF *oi;
	Sint32 ccnt;
	Sint32 i;

	MPVMC08_Init(mpv->mc);
	MPVMC16_Init(mpv->mc);
	oi = mpv->oi;
	if (mpv->mcflag == 0) {
		ccnt = -1;
	} else {
		ccnt = 4;
	}
	mpv->ccnt = ccnt;
	for (i = 0; i < 6; i++) {
		oi[i].p = mpv->work;
	}
	for (i = 0; i < 6; i++) {
		oi[i].n = 8;
	}
}
#endif

void MPVCMC_Init(void)
{
	mpvcmc_oneref[0] = MPVMC08_OneRef1p_TuneC;
	mpvcmc_oneref[1] = MPVMC08_OneRefH2_TuneC;
	mpvcmc_oneref[2] = MPVMC08_OneRefV2_TuneC;
	mpvcmc_oneref[3] = MPVMC08_OneRef4p_TuneC;
	mpvcmc_oneref[4] = MPVMC08_OneRef1p_TuneC;
	mpvcmc_oneref[5] = MPVMC08_OneRefH2_TuneC;
	mpvcmc_oneref[6] = MPVMC08_OneRefV2_TuneC;
	mpvcmc_oneref[7] = MPVMC08_OneRefV2_TuneC;
}
