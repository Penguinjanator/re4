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

/* COMPILER-DIFF: M1 - the original keeps `addi r5, r3, 0x124` as a separate base for six stores into a member array while every C form folds the offsets into r3 (OPEN since pass 1). Pure C by project decision (CRI pass 8). */
void MPVCMC_InitMcOiRt(MPV_OBJ *mpv)
{
	MPVCMC_REF *oi = mpv->oi_rt;
	Sint32 ccnt;
	Sint32 i;
	Sint32 w;
	Sint32 h;

	if (mpv->mcflag == 0) {
		ccnt = -1;
	} else {
		ccnt = 4;
	}
	mpv->ccnt_rt = ccnt;
	w = mpv->width;
	for (i = 0; i < 2; i++) {
		oi[i].n = w;
	}
	h = mpv->height;
	for (i = 2; i < 6; i++) {
		oi[i].n = h;
	}
}

/* COMPILER-DIFF: M1 - same separate member-array base as MPVCMC_InitMcOiRt. Pure C by project decision (CRI pass 8). */
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
