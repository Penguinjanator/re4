/* CRI Sofdec MPEG video: "unified" motion compensation front end (mpv_umc.c). Macroblocks are
 * built from the reference frame planes through the 8x8 (chroma) / 16x16 (luma) one-reference
 * kernels of mpv_mc.c / mpv_mcy.c and merged with the IDCT output into the word-packed output
 * frame; skipped macroblocks are copied from the reference. The paired-single merge kernels
 * (`mpvumc_PpicSkipMb`, `mpvumc_BiMakeMb`, `mpvumc_OneMakeMb`, `mpvumc_OutputIntra6blk`) are
 * transcribed as inline assembly (no C form reproduced them; the paired-single ones need the
 * scheduler ON to reproduce two swapped adjacent instructions, PpicSkipMb needs it off).
 * OPEN: mpvumc_OneReadMb (the kernel selection by the motion vector) keeps the same operations
 * but a different schedule/registers; the Forward/Backward/BiDirect/Intra wrappers differ in the
 * argument register choices (M1). */
#include "cri_xpt.h"
#include "mpv.h"

/* a frame's planes: the two chroma planes first, then luma, with their pitches */
typedef struct {
	Uint8 *pln[3];             /* 0x00 */
	Sint16 cpitch;             /* 0x0C */
	Sint16 ypitch;             /* 0x0E */
} MPVUMC_RFB;

/* the MPV object as seen here */
/* the MC work pointers (MPV_OBJ + 0x110) */
typedef struct {
	Uint8 *clip;               /* 0x00 */
	Uint8 *buf0;               /* 0x04 */
	Uint8 *work;               /* 0x08 the first prediction */
	Uint8 *work2;              /* 0x0C the second prediction */
} MPVUMC_WORK;

typedef struct {
	Uint8 pad0[0x40];
	Uint8 *clip_base;          /* 0x040 */
	Uint8 pad44[0xCC - 0x44];
	Uint8 pad_mc[0x44];        /* 0x0CC MPVMC (its 0x44 kernel fields) */
	MPVUMC_WORK mcwk;          /* 0x110 */
	Sint32 ccnt_rt;            /* 0x120 */
	MPVCMC_REF oi_rt[6];       /* 0x124 */
	Uint8 pad154[0x19C - 0x154];
	Sint32 mcflag;             /* 0x19C cond[3] */
	Uint8 pad1a0[0x1D0 - 0x1A0];
	Sint32 width;              /* 0x1D0 */
	Sint32 height;             /* 0x1D4 */
	Sint32 mb_width;           /* 0x1D8 */
	Uint8 pad1dc[0x264 - 0x1DC];
	MPVUMC_RFB out;            /* 0x264 */
	MPVUMC_RFB ref;            /* 0x274 */
	Uint8 *frmbuf;             /* 0x284 */
	Uint8 pad288[0x294 - 0x288];
	Uint8 *out_pln[3];         /* 0x294 output planes (chroma, chroma, luma) */
	Sint16 out_cpitch;         /* 0x2A0 */
	Sint16 out_ypitch;         /* 0x2A2 */
	Uint8 pad2a4[0x2D4 - 0x2A4];
	void (*mc_func[4])(void *mpv); /* 0x2D4 */
	Uint8 pad2e4[0x2EC - 0x2E4];
	MPV_MV fwd;                /* 0x2EC */
	MPV_MV bwd;                /* 0x310 */
	Sint32 mb_addr;            /* 0x334 */
	Sint32 mb_y;               /* 0x338 */
	Sint32 mb_x;               /* 0x33C */
	Uint8 pad340[0x348 - 0x340];
	Sint32 cbp_code;           /* 0x348 */
	Uint8 pad34c[0x380 - 0x34C];
	Uint8 mcbuf[0x100];        /* 0x380 */
} MPVUMC_OBJ;

typedef void (*MPVUMC_MCFUNC)(MPVMC *mc);

extern void MPVMC08_OneRef1p_TuneC(MPVMC *mc);
extern void MPVMC08_OneRefH2_TuneC(MPVMC *mc);
extern void MPVMC08_OneRefV2_TuneC(MPVMC *mc);
extern void MPVMC08_OneRef4p_TuneC(MPVMC *mc);
extern void MPVMC16_OneRef1p_TuneC(MPVMC *mc);
extern void MPVMC16_OneRefH2_TuneC(MPVMC *mc);
extern void MPVMC16_OneRefV2_TuneC(MPVMC *mc);
extern void MPVMC16_OneRef4p_TuneC(MPVMC *mc);

/* 1.0f of the paired-single average (compiler constant `const_mem$89` in the original) */
static const Float32 mpvumc_ps_one = 1.0f;

MPVUMC_MCFUNC mpvumc_oneref_y[2][2][2];
static MPVUMC_MCFUNC mpvumc_oneref[2][2][2];

void mpvumc_PpicSkipMb(Sint32 *ofs, MPVUMC_RFB *out, MPVUMC_RFB *ref);
void mpvumc_BiMakeMb(MPVUMC_WORK *wk, MPVCMC_REF *oi_rt, Sint32 cbp);
void mpvumc_OneMakeMb(MPVUMC_WORK *wk, MPVCMC_REF *oi_rt, Sint32 cbp);
void mpvumc_OneReadMb(MPVUMC_OBJ *mpv, Uint8 *dst, Sint32 *ofs, MPVUMC_RFB *rfb, MPV_MV *mv);
void mpvumc_OutputIntra6blk(Uint8 *blk, MPVCMC_REF *oi_rt, Uint8 *clip);

/* B picture: `n` skipped macroblocks repeat the previous macroblock's prediction */
void MPVUMC_BpicSkipped(MPVUMC_OBJ *mpv, Sint32 n)
{
	Sint32 mb_end;
	void (*func)(void *mpv);

	mpv->cbp_code = 0;
	mb_end = mpv->mb_addr;
	func = mpv->mc_func[0];
	mpv->mb_addr -= n - 1;
	mpv->mb_x -= n - 1;
	while (mpv->mb_x < 0) {
		mpv->mb_x += mpv->mb_width;
		mpv->mb_y--;
	}
	while (mpv->mb_addr < mb_end) {
		func(mpv);
		mpv->mb_x++;
		if (mpv->mb_x >= mpv->mb_width) {
			mpv->mb_x = 0;
			mpv->mb_y++;
		}
		mpv->mb_addr++;
	}
}

#pragma scheduling off
/* copy the macroblock at ofs[] (chroma, luma) from the reference to the output frame */
void mpvumc_PpicSkipMb(Sint32 *ofs, MPVUMC_RFB *out, MPVUMC_RFB *ref)
{
	asm {
		lwz r7, 0x0(r3)
		lha r6, 0xc(r5)
		clrlwi. r0, r7, 27
		srawi r0, r6, 3
		addze r9, r0
		bne L_8020607C
		lwz r0, 0x0(r5)
		lwz r6, 0x0(r4)
		add r10, r0, r7
		add r6, r6, r7
		dcbz r0, r10
		slwi r7, r9, 3
		lfd f0, 0x0(r6)
		dcbz r10, r7
		slwi r8, r9, 4
		lfdx f1, r6, r7
		add r6, r6, r8
		stfd f0, 0x0(r10)
		stfdx f1, r10, r7
		add r10, r10, r8
		dcbz r0, r10
		lfd f0, 0x0(r6)
		dcbz r10, r7
		lfdx f1, r6, r7
		add r6, r6, r8
		stfd f0, 0x0(r10)
		stfdx f1, r10, r7
		add r10, r10, r8
		dcbz r0, r10
		lfd f0, 0x0(r6)
		dcbz r10, r7
		lfdx f1, r6, r7
		add r6, r6, r8
		stfd f0, 0x0(r10)
		stfdx f1, r10, r7
		add r10, r10, r8
		dcbz r0, r10
		lfd f0, 0x0(r6)
		dcbz r10, r7
		lfdx f1, r6, r7
		stfd f0, 0x0(r10)
		stfdx f1, r10, r7
		lwz r6, 0x4(r4)
		lwz r9, 0x0(r3)
		lwz r0, 0x4(r5)
		add r6, r6, r9
		add r9, r0, r9
		dcbz r0, r9
		lfd f0, 0x0(r6)
		dcbz r9, r7
		lfdx f1, r6, r7
		add r6, r6, r8
		stfd f0, 0x0(r9)
		stfdx f1, r9, r7
		add r9, r9, r8
		dcbz r0, r9
		lfd f0, 0x0(r6)
		dcbz r9, r7
		lfdx f1, r6, r7
		add r6, r6, r8
		stfd f0, 0x0(r9)
		stfdx f1, r9, r7
		add r9, r9, r8
		dcbz r0, r9
		lfd f0, 0x0(r6)
		dcbz r9, r7
		lfdx f1, r6, r7
		add r6, r6, r8
		stfd f0, 0x0(r9)
		stfdx f1, r9, r7
		add r9, r9, r8
		dcbz r0, r9
		lfd f0, 0x0(r6)
		dcbz r9, r7
		lfdx f1, r6, r7
		stfd f0, 0x0(r9)
		stfdx f1, r9, r7
		b L_80206158
L_8020607C:
		lwz r6, 0x0(r4)
		slwi r8, r9, 3
		lwz r0, 0x0(r5)
		slwi r9, r9, 4
		add r6, r6, r7
		lfd f0, 0x0(r6)
		add r7, r0, r7
		lfdx f1, r6, r8
		add r6, r6, r9
		stfd f0, 0x0(r7)
		stfdx f1, r7, r8
		add r7, r7, r9
		lfd f0, 0x0(r6)
		lfdx f1, r6, r8
		add r6, r6, r9
		stfd f0, 0x0(r7)
		stfdx f1, r7, r8
		add r7, r7, r9
		lfd f0, 0x0(r6)
		lfdx f1, r6, r8
		add r6, r6, r9
		stfd f0, 0x0(r7)
		stfdx f1, r7, r8
		add r7, r7, r9
		lfd f0, 0x0(r6)
		lfdx f1, r6, r8
		stfd f0, 0x0(r7)
		stfdx f1, r7, r8
		lwz r6, 0x4(r4)
		lwz r7, 0x0(r3)
		lwz r0, 0x4(r5)
		add r6, r6, r7
		lfd f0, 0x0(r6)
		add r7, r0, r7
		lfdx f1, r6, r8
		add r6, r6, r9
		stfd f0, 0x0(r7)
		stfdx f1, r7, r8
		add r7, r7, r9
		lfd f0, 0x0(r6)
		lfdx f1, r6, r8
		add r6, r6, r9
		stfd f0, 0x0(r7)
		stfdx f1, r7, r8
		add r7, r7, r9
		lfd f0, 0x0(r6)
		lfdx f1, r6, r8
		add r6, r6, r9
		stfd f0, 0x0(r7)
		stfdx f1, r7, r8
		add r7, r7, r9
		lfdx f1, r6, r8
		lfd f0, 0x0(r6)
		stfd f0, 0x0(r7)
		stfdx f1, r7, r8
L_80206158:
		lwz r6, 0x4(r3)
		lha r3, 0xe(r5)
		clrlwi. r0, r6, 27
		srawi r0, r3, 3
		addze r7, r0
		bne L_80206340
		lwz r3, 0x8(r4)
		lwz r0, 0x8(r5)
		add r3, r3, r6
		add r4, r0, r6
		dcbz r0, r4
		slwi r0, r7, 3
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r0
		stfd f0, 0x0(r4)
		stfd f1, 0x8(r4)
		add r4, r4, r0
		dcbz r0, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r0
		stfd f0, 0x0(r4)
		stfd f1, 0x8(r4)
		add r4, r4, r0
		dcbz r0, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r0
		stfd f0, 0x0(r4)
		stfd f1, 0x8(r4)
		add r4, r4, r0
		dcbz r0, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r0
		stfd f0, 0x0(r4)
		stfd f1, 0x8(r4)
		add r4, r4, r0
		dcbz r0, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r0
		stfd f0, 0x0(r4)
		stfd f1, 0x8(r4)
		add r4, r4, r0
		dcbz r0, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r0
		stfd f0, 0x0(r4)
		stfd f1, 0x8(r4)
		add r4, r4, r0
		dcbz r0, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r0
		stfd f0, 0x0(r4)
		stfd f1, 0x8(r4)
		add r4, r4, r0
		dcbz r0, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r0
		stfd f0, 0x0(r4)
		stfd f1, 0x8(r4)
		add r4, r4, r0
		dcbz r0, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r0
		stfd f0, 0x0(r4)
		stfd f1, 0x8(r4)
		add r4, r4, r0
		dcbz r0, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r0
		stfd f0, 0x0(r4)
		stfd f1, 0x8(r4)
		add r4, r4, r0
		dcbz r0, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r0
		stfd f0, 0x0(r4)
		stfd f1, 0x8(r4)
		add r4, r4, r0
		dcbz r0, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r0
		stfd f0, 0x0(r4)
		stfd f1, 0x8(r4)
		add r4, r4, r0
		dcbz r0, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r0
		stfd f0, 0x0(r4)
		stfd f1, 0x8(r4)
		add r4, r4, r0
		dcbz r0, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r0
		stfd f0, 0x0(r4)
		stfd f1, 0x8(r4)
		add r4, r4, r0
		dcbz r0, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r0
		stfd f0, 0x0(r4)
		stfd f1, 0x8(r4)
		add r4, r4, r0
		dcbz r0, r4
		lfd f1, 0x8(r3)
		lfd f0, 0x0(r3)
		stfd f0, 0x0(r4)
		stfd f1, 0x8(r4)
		blr
L_80206340:
		lwz r3, 0x8(r4)
		slwi r4, r7, 3
		lwz r0, 0x8(r5)
		add r3, r3, r6
		lfd f0, 0x0(r3)
		add r5, r0, r6
		lfd f1, 0x8(r3)
		add r3, r3, r4
		stfd f0, 0x0(r5)
		stfd f1, 0x8(r5)
		add r5, r5, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r4
		stfd f0, 0x0(r5)
		stfd f1, 0x8(r5)
		add r5, r5, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r4
		stfd f0, 0x0(r5)
		stfd f1, 0x8(r5)
		add r5, r5, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r4
		stfd f0, 0x0(r5)
		stfd f1, 0x8(r5)
		add r5, r5, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r4
		stfd f0, 0x0(r5)
		stfd f1, 0x8(r5)
		add r5, r5, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r4
		stfd f0, 0x0(r5)
		stfd f1, 0x8(r5)
		add r5, r5, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r4
		stfd f0, 0x0(r5)
		stfd f1, 0x8(r5)
		add r5, r5, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r4
		stfd f0, 0x0(r5)
		stfd f1, 0x8(r5)
		add r5, r5, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r4
		stfd f0, 0x0(r5)
		stfd f1, 0x8(r5)
		add r5, r5, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r4
		stfd f0, 0x0(r5)
		stfd f1, 0x8(r5)
		add r5, r5, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r4
		stfd f0, 0x0(r5)
		stfd f1, 0x8(r5)
		add r5, r5, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r4
		stfd f0, 0x0(r5)
		stfd f1, 0x8(r5)
		add r5, r5, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r4
		stfd f0, 0x0(r5)
		stfd f1, 0x8(r5)
		add r5, r5, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r4
		stfd f0, 0x0(r5)
		stfd f1, 0x8(r5)
		add r5, r5, r4
		lfd f0, 0x0(r3)
		lfd f1, 0x8(r3)
		add r3, r3, r4
		stfd f0, 0x0(r5)
		stfd f1, 0x8(r5)
		add r5, r5, r4
		lfd f1, 0x8(r3)
		lfd f0, 0x0(r3)
		stfd f0, 0x0(r5)
		stfd f1, 0x8(r5)
	}
}
#pragma scheduling on

/* P picture: `n` skipped macroblocks are copied from the reference frame */
void MPVUMC_PpicSkipped(MPVUMC_OBJ *mpv, Sint32 n)
{
	Sint32 mb_end;
	MPVUMC_RFB *out;
	MPVUMC_RFB *ref;
	Sint32 ofs[2];
	Sint32 y8;
	Sint32 y16;

	out = &mpv->out;
	mb_end = mpv->mb_addr;
	ref = out + 1;
	mpv->mb_addr -= n - 1;
	mpv->mb_x -= n - 1;
	while (mpv->mb_x < 0) {
		mpv->mb_x += mpv->mb_width;
		mpv->mb_y--;
	}
	while (mpv->mb_addr < mb_end) {
		/* the row offsets as named locals (y16 derived from y8): `mb_y * 8 * cpitch` inline puts cpitch
		 * first in the mullw and recomputes the shifts */
		y8 = mpv->mb_y * 8;
		y16 = y8 * 2;
		ofs[0] = mpv->mb_x * 8 + y8 * out->cpitch;
		ofs[1] = mpv->mb_x * 16 + y16 * out->ypitch;
		mpvumc_PpicSkipMb(ofs, out, ref);
		mpv->mb_x++;
		if (mpv->mb_x >= mpv->mb_width) {
			mpv->mb_x = 0;
			mpv->mb_y++;
		}
		mpv->mb_addr++;
	}
}

/* average the two predictions in oi[] with the IDCT blocks into the output blocks oi_rt[] */
void mpvumc_BiMakeMb(MPVUMC_WORK *wk, MPVCMC_REF *oi_rt, Sint32 cbp)
{
	asm {
		lis r6, mpvumc_ps_one@ha
		lwz r8, 0x4(r3)
		addi r6, r6, mpvumc_ps_one@l
		lwz r7, 0x8(r3)
		psq_l f8, 0x0(r6), 1, 0
		addi r9, r4, 0x4
		lwz r3, 0xc(r3)
		li r10, 0x0
		ps_merge11 f8, f8, f8
L_80206618:
		cmpwi r5, 0x0
		lwz r4, 0x0(r9)
		lwz r6, 0x4(r9)
		addi r9, r9, 0x8
		blt L_802067C4
		clrlwi. r0, r4, 27
		bne L_80206700
		li r0, 0x4
		mtctr r0
		addi r8, r8, 0x80
L_80206640:
		dcbz r0, r4
		psq_l f0, 0x0(r7), 0, 4
		psq_l f4, 0x0(r3), 0, 4
		psq_l f1, 0x2(r7), 0, 4
		psq_l f5, 0x2(r3), 0, 4
		psq_l f2, 0x4(r7), 0, 4
		psq_l f6, 0x4(r3), 0, 4
		psq_l f3, 0x6(r7), 0, 4
		psq_l f7, 0x6(r3), 0, 4
		ps_add f0, f0, f4
		ps_add f1, f1, f5
		ps_add f2, f2, f6
		ps_add f3, f3, f7
		ps_add f0, f0, f8
		ps_add f1, f1, f8
		ps_add f2, f2, f8
		ps_add f3, f3, f8
		psq_st f0, 0x0(r4), 0, 5
		psq_st f1, 0x2(r4), 0, 5
		psq_st f2, 0x4(r4), 0, 5
		psq_st f3, 0x6(r4), 0, 5
		add r4, r4, r6
		dcbz r0, r4
		psq_l f0, 0x8(r7), 0, 4
		psq_l f4, 0x8(r3), 0, 4
		psq_l f1, 0xa(r7), 0, 4
		psq_l f5, 0xa(r3), 0, 4
		psq_l f2, 0xc(r7), 0, 4
		psq_l f6, 0xc(r3), 0, 4
		psq_l f3, 0xe(r7), 0, 4
		psq_l f7, 0xe(r3), 0, 4
		ps_add f0, f0, f4
		ps_add f1, f1, f5
		ps_add f2, f2, f6
		ps_add f3, f3, f7
		ps_add f0, f0, f8
		ps_add f1, f1, f8
		ps_add f2, f2, f8
		ps_add f3, f3, f8
		psq_st f0, 0x0(r4), 0, 5
		psq_st f1, 0x2(r4), 0, 5
		psq_st f2, 0x4(r4), 0, 5
		psq_st f3, 0x6(r4), 0, 5
		add r4, r4, r6
		addi r7, r7, 0x10
		addi r3, r3, 0x10
		bdnz L_80206640
		b L_802069D8
L_80206700:
		li r0, 0x4
		mtctr r0
		addi r8, r8, 0x80
L_8020670C:
		psq_l f0, 0x0(r7), 0, 4
		psq_l f4, 0x0(r3), 0, 4
		psq_l f1, 0x2(r7), 0, 4
		psq_l f5, 0x2(r3), 0, 4
		psq_l f2, 0x4(r7), 0, 4
		psq_l f6, 0x4(r3), 0, 4
		psq_l f3, 0x6(r7), 0, 4
		psq_l f7, 0x6(r3), 0, 4
		ps_add f0, f0, f4
		ps_add f1, f1, f5
		ps_add f2, f2, f6
		ps_add f3, f3, f7
		ps_add f0, f0, f8
		ps_add f1, f1, f8
		ps_add f2, f2, f8
		ps_add f3, f3, f8
		psq_st f0, 0x0(r4), 0, 5
		psq_st f1, 0x2(r4), 0, 5
		psq_st f2, 0x4(r4), 0, 5
		psq_st f3, 0x6(r4), 0, 5
		add r4, r4, r6
		psq_l f0, 0x8(r7), 0, 4
		psq_l f4, 0x8(r3), 0, 4
		psq_l f1, 0xa(r7), 0, 4
		psq_l f5, 0xa(r3), 0, 4
		psq_l f2, 0xc(r7), 0, 4
		psq_l f6, 0xc(r3), 0, 4
		psq_l f3, 0xe(r7), 0, 4
		psq_l f7, 0xe(r3), 0, 4
		ps_add f0, f0, f4
		ps_add f1, f1, f5
		ps_add f2, f2, f6
		ps_add f3, f3, f7
		ps_add f0, f0, f8
		ps_add f1, f1, f8
		ps_add f2, f2, f8
		ps_add f3, f3, f8
		psq_st f0, 0x0(r4), 0, 5
		psq_st f1, 0x2(r4), 0, 5
		psq_st f2, 0x4(r4), 0, 5
		psq_st f3, 0x6(r4), 0, 5
		add r4, r4, r6
		addi r7, r7, 0x10
		addi r3, r3, 0x10
		bdnz L_8020670C
		b L_802069D8
L_802067C4:
		clrlwi. r0, r4, 27
		bne L_802068D8
		li r0, 0x4
		mtctr r0
L_802067D4:
		dcbz r0, r4
		psq_l f0, 0x0(r7), 0, 4
		psq_l f4, 0x0(r3), 0, 4
		psq_l f1, 0x2(r7), 0, 4
		psq_l f5, 0x2(r3), 0, 4
		psq_l f2, 0x4(r7), 0, 4
		psq_l f6, 0x4(r3), 0, 4
		psq_l f3, 0x6(r7), 0, 4
		psq_l f7, 0x6(r3), 0, 4
		ps_add f0, f0, f4
		ps_add f1, f1, f5
		ps_add f2, f2, f6
		ps_add f3, f3, f7
		psq_l f4, 0x0(r8), 0, 5
		psq_l f5, 0x4(r8), 0, 5
		psq_l f6, 0x8(r8), 0, 5
		psq_l f7, 0xc(r8), 0, 5
		ps_add f0, f0, f8
		ps_add f1, f1, f8
		ps_add f2, f2, f8
		ps_add f3, f3, f8
		ps_add f0, f0, f4
		ps_add f1, f1, f5
		ps_add f2, f2, f6
		ps_add f3, f3, f7
		psq_st f0, 0x0(r4), 0, 5
		psq_st f1, 0x2(r4), 0, 5
		psq_st f2, 0x4(r4), 0, 5
		psq_st f3, 0x6(r4), 0, 5
		add r4, r4, r6
		dcbz r0, r4
		psq_l f0, 0x8(r7), 0, 4
		psq_l f4, 0x8(r3), 0, 4
		psq_l f1, 0xa(r7), 0, 4
		psq_l f5, 0xa(r3), 0, 4
		psq_l f2, 0xc(r7), 0, 4
		psq_l f6, 0xc(r3), 0, 4
		psq_l f3, 0xe(r7), 0, 4
		psq_l f7, 0xe(r3), 0, 4
		ps_add f0, f0, f4
		ps_add f1, f1, f5
		ps_add f2, f2, f6
		ps_add f3, f3, f7
		psq_l f4, 0x10(r8), 0, 5
		psq_l f5, 0x14(r8), 0, 5
		psq_l f6, 0x18(r8), 0, 5
		psq_l f7, 0x1c(r8), 0, 5
		ps_add f0, f0, f8
		ps_add f1, f1, f8
		ps_add f2, f2, f8
		ps_add f3, f3, f8
		ps_add f0, f0, f4
		ps_add f1, f1, f5
		ps_add f2, f2, f6
		ps_add f3, f3, f7
		psq_st f0, 0x0(r4), 0, 5
		psq_st f1, 0x2(r4), 0, 5
		psq_st f2, 0x4(r4), 0, 5
		psq_st f3, 0x6(r4), 0, 5
		add r4, r4, r6
		addi r7, r7, 0x10
		addi r3, r3, 0x10
		addi r8, r8, 0x20
		bdnz L_802067D4
		b L_802069D8
L_802068D8:
		li r0, 0x4
		mtctr r0
L_802068E0:
		psq_l f0, 0x0(r7), 0, 4
		psq_l f4, 0x0(r3), 0, 4
		psq_l f1, 0x2(r7), 0, 4
		psq_l f5, 0x2(r3), 0, 4
		psq_l f2, 0x4(r7), 0, 4
		psq_l f6, 0x4(r3), 0, 4
		psq_l f3, 0x6(r7), 0, 4
		psq_l f7, 0x6(r3), 0, 4
		ps_add f0, f0, f4
		ps_add f1, f1, f5
		ps_add f2, f2, f6
		ps_add f3, f3, f7
		psq_l f4, 0x0(r8), 0, 5
		psq_l f5, 0x4(r8), 0, 5
		psq_l f6, 0x8(r8), 0, 5
		psq_l f7, 0xc(r8), 0, 5
		ps_add f0, f0, f8
		ps_add f1, f1, f8
		ps_add f2, f2, f8
		ps_add f3, f3, f8
		ps_add f0, f0, f4
		ps_add f1, f1, f5
		ps_add f2, f2, f6
		ps_add f3, f3, f7
		psq_st f0, 0x0(r4), 0, 5
		psq_st f1, 0x2(r4), 0, 5
		psq_st f2, 0x4(r4), 0, 5
		psq_st f3, 0x6(r4), 0, 5
		add r4, r4, r6
		psq_l f0, 0x8(r7), 0, 4
		psq_l f4, 0x8(r3), 0, 4
		psq_l f1, 0xa(r7), 0, 4
		psq_l f5, 0xa(r3), 0, 4
		psq_l f2, 0xc(r7), 0, 4
		psq_l f6, 0xc(r3), 0, 4
		psq_l f3, 0xe(r7), 0, 4
		psq_l f7, 0xe(r3), 0, 4
		ps_add f0, f0, f4
		ps_add f1, f1, f5
		ps_add f2, f2, f6
		ps_add f3, f3, f7
		psq_l f4, 0x10(r8), 0, 5
		psq_l f5, 0x14(r8), 0, 5
		psq_l f6, 0x18(r8), 0, 5
		psq_l f7, 0x1c(r8), 0, 5
		ps_add f0, f0, f8
		ps_add f1, f1, f8
		ps_add f2, f2, f8
		ps_add f3, f3, f8
		ps_add f0, f0, f4
		ps_add f1, f1, f5
		ps_add f2, f2, f6
		ps_add f3, f3, f7
		psq_st f0, 0x0(r4), 0, 5
		psq_st f1, 0x2(r4), 0, 5
		psq_st f2, 0x4(r4), 0, 5
		psq_st f3, 0x6(r4), 0, 5
		add r4, r4, r6
		addi r7, r7, 0x10
		addi r3, r3, 0x10
		addi r8, r8, 0x20
		bdnz L_802068E0
L_802069D8:
		addi r10, r10, 0x1
		slwi r5, r5, 1
		cmpwi r10, 0x6
		blt L_80206618
	}
}

/* add the IDCT blocks to the prediction in oi[] into the output blocks oi_rt[] */
void mpvumc_OneMakeMb(MPVUMC_WORK *wk, MPVCMC_REF *oi_rt, Sint32 cbp)
{
	asm {
		lwz r6, 0x4(r3)
		addi r7, r4, 0x4
		lwz r3, 0x8(r3)
		li r9, 0x0
L_802069FC:
		cmpwi r5, 0x0
		lwz r4, 0x0(r7)
		lwz r8, 0x4(r7)
		addi r7, r7, 0x8
		blt L_80206B08
		clrlwi. r0, r4, 27
		addi r6, r6, 0x80
		srwi r0, r8, 3
		bne L_80206AA4
		dcbz r0, r4
		slwi r0, r0, 3
		lfd f8, 0x0(r3)
		add r8, r4, r0
		lfd f9, 0x8(r3)
		lfd f10, 0x10(r3)
		lfd f11, 0x18(r3)
		stfd f8, 0x0(r4)
		dcbz r0, r8
		stfd f9, 0x0(r8)
		add r8, r8, r0
		dcbz r0, r8
		stfd f10, 0x0(r8)
		add r8, r8, r0
		dcbz r0, r8
		stfd f11, 0x0(r8)
		add r8, r8, r0
		dcbz r0, r8
		lfd f9, 0x28(r3)
		lfd f10, 0x30(r3)
		lfd f11, 0x38(r3)
		lfd f8, 0x20(r3)
		stfd f8, 0x0(r8)
		add r8, r8, r0
		dcbz r0, r8
		stfd f9, 0x0(r8)
		add r8, r8, r0
		dcbz r0, r8
		stfd f10, 0x0(r8)
		add r8, r8, r0
		dcbz r0, r8
		stfd f11, 0x0(r8)
		b L_80206B00
L_80206AA4:
		lfd f8, 0x0(r3)
		slwi r0, r0, 3
		lfd f9, 0x8(r3)
		add r8, r4, r0
		lfd f10, 0x10(r3)
		lfd f11, 0x18(r3)
		stfd f8, 0x0(r4)
		stfd f9, 0x0(r8)
		add r8, r8, r0
		stfd f10, 0x0(r8)
		add r8, r8, r0
		stfd f11, 0x0(r8)
		add r8, r8, r0
		lfd f9, 0x28(r3)
		lfd f10, 0x30(r3)
		lfd f11, 0x38(r3)
		lfd f8, 0x20(r3)
		stfd f8, 0x0(r8)
		add r8, r8, r0
		stfd f9, 0x0(r8)
		add r8, r8, r0
		stfd f10, 0x0(r8)
		stfdx f11, r8, r0
L_80206B00:
		addi r3, r3, 0x40
		b L_80206C54
L_80206B08:
		clrlwi. r0, r4, 27
		bne L_80206BB8
		li r0, 0x4
		mtctr r0
L_80206B18:
		dcbz r0, r4
		psq_l f0, 0x0(r6), 0, 3
		psq_l f4, 0x0(r3), 0, 4
		psq_l f1, 0x4(r6), 0, 3
		psq_l f5, 0x2(r3), 0, 4
		psq_l f2, 0x8(r6), 0, 3
		psq_l f6, 0x4(r3), 0, 4
		psq_l f3, 0xc(r6), 0, 3
		psq_l f7, 0x6(r3), 0, 4
		ps_add f0, f0, f4
		ps_add f1, f1, f5
		ps_add f2, f2, f6
		ps_add f3, f3, f7
		psq_st f0, 0x0(r4), 0, 4
		psq_st f1, 0x2(r4), 0, 4
		psq_st f2, 0x4(r4), 0, 4
		psq_st f3, 0x6(r4), 0, 4
		add r4, r4, r8
		dcbz r0, r4
		psq_l f0, 0x10(r6), 0, 3
		psq_l f4, 0x8(r3), 0, 4
		psq_l f1, 0x14(r6), 0, 3
		psq_l f5, 0xa(r3), 0, 4
		psq_l f2, 0x18(r6), 0, 3
		psq_l f6, 0xc(r3), 0, 4
		psq_l f3, 0x1c(r6), 0, 3
		psq_l f7, 0xe(r3), 0, 4
		ps_add f0, f0, f4
		ps_add f1, f1, f5
		ps_add f2, f2, f6
		ps_add f3, f3, f7
		psq_st f0, 0x0(r4), 0, 4
		psq_st f1, 0x2(r4), 0, 4
		psq_st f2, 0x4(r4), 0, 4
		psq_st f3, 0x6(r4), 0, 4
		add r4, r4, r8
		addi r3, r3, 0x10
		addi r6, r6, 0x20
		bdnz L_80206B18
		b L_80206C54
L_80206BB8:
		li r0, 0x4
		mtctr r0
L_80206BC0:
		psq_l f0, 0x0(r6), 0, 3
		psq_l f4, 0x0(r3), 0, 4
		psq_l f1, 0x4(r6), 0, 3
		psq_l f5, 0x2(r3), 0, 4
		psq_l f2, 0x8(r6), 0, 3
		psq_l f6, 0x4(r3), 0, 4
		psq_l f3, 0xc(r6), 0, 3
		psq_l f7, 0x6(r3), 0, 4
		ps_add f0, f0, f4
		ps_add f1, f1, f5
		ps_add f2, f2, f6
		ps_add f3, f3, f7
		psq_st f0, 0x0(r4), 0, 4
		psq_st f1, 0x2(r4), 0, 4
		psq_st f2, 0x4(r4), 0, 4
		psq_st f3, 0x6(r4), 0, 4
		add r4, r4, r8
		psq_l f0, 0x10(r6), 0, 3
		psq_l f4, 0x8(r3), 0, 4
		psq_l f1, 0x14(r6), 0, 3
		psq_l f5, 0xa(r3), 0, 4
		psq_l f2, 0x18(r6), 0, 3
		psq_l f6, 0xc(r3), 0, 4
		psq_l f3, 0x1c(r6), 0, 3
		psq_l f7, 0xe(r3), 0, 4
		ps_add f0, f0, f4
		ps_add f1, f1, f5
		ps_add f2, f2, f6
		ps_add f3, f3, f7
		psq_st f0, 0x0(r4), 0, 4
		psq_st f1, 0x2(r4), 0, 4
		psq_st f2, 0x4(r4), 0, 4
		psq_st f3, 0x6(r4), 0, 4
		add r4, r4, r8
		addi r3, r3, 0x10
		addi r6, r6, 0x20
		bdnz L_80206BC0
L_80206C54:
		addi r9, r9, 0x1
		slwi r5, r5, 1
		cmpwi r9, 0x6
		blt L_802069FC
	}
}

/* one reference's prediction of the macroblock into dst (two 8x8 chroma blocks, one 16x16 luma
 * block) through the half-pel kernels selected by the motion vector; ofs[] receives the
 * macroblock's chroma/luma offsets in the frame */
void mpvumc_OneReadMb(MPVUMC_OBJ *mpv, Uint8 *dst, Sint32 *ofs, MPVUMC_RFB *rfb, MPV_MV *mv)
{
	MPVMC *mc = (MPVMC *)mpv->pad_mc;
	MPVUMC_MCFUNC (*tbl_y)[2];
	MPVUMC_MCFUNC (*tbl_c)[2];
	MPVUMC_MCFUNC fn_y;
	MPVUMC_MCFUNC fn_c;
	Sint32 mcflag;
	Sint32 cpitch;
	Sint32 ypitch;
	Sint32 mbx;
	Sint32 mby;
	Sint32 vx;
	Sint32 vy;
	Sint32 cvx;
	Sint32 cvy;
	Sint32 cpos;
	Sint32 ypos;
	Sint32 chx;
	Sint32 yhx;
	Uint8 *src;

	mby = mpv->mb_y;
	cpitch = rfb->cpitch;
	mbx = mpv->mb_x;
	mcflag = mpv->mcflag;
	ypitch = rfb->ypitch;
	ofs[0] = mbx * 8 + mby * 8 * cpitch;
	ofs[1] = mbx * 16 + mby * 16 * rfb->ypitch;
	tbl_y = mpvumc_oneref_y[mcflag];
	tbl_c = mpvumc_oneref[mcflag];
	vx = mv->vec[0];
	vy = mv->vec[1];
	cvx = vx / 2;
	cvy = vy / 2;
	fn_y = tbl_y[vy & 1][vx & 1];
	fn_c = tbl_c[cvy & 1][cvx & 1];
	chx = (cvx & 1) & mcflag;
	yhx = (vx & 1) & mcflag;
	cpos = ofs[0] + (cvy >> 1) * cpitch + (cvx >> 1);
	ypos = ofs[1] + (vy >> 1) * ypitch + (vx >> 1);
	mc->stride = cpitch;
	mc->dst = (Uint32 *)dst;
	src = rfb->pln[0] + cpos;
	mc->src = src;
	mc->src2 = src + cpitch + chx;
	fn_c(mc);
	mc->dst = (Uint32 *)(dst + 0x40);
	src = rfb->pln[1] + cpos;
	mc->src = src;
	mc->src2 = src + cpitch + chx;
	fn_c(mc);
	mc->stride = ypitch;
	mc->dst = (Uint32 *)(dst + 0x80);
	src = rfb->pln[2] + ypos;
	mc->src = src;
	mc->src2 = src + ypitch + yhx;
	fn_y(mc);
}

/* the output block pointers of the current macroblock (two chroma blocks, four luma blocks) */
#define MPVUMC_SET_OUT_BLOCKS(mpv, cofs, yofs, ypitch)                                         \
	(mpv)->oi_rt[0].p = (mpv)->out_pln[0] + (cofs);                                        \
	(mpv)->oi_rt[1].p = (mpv)->out_pln[1] + (cofs);                                        \
	(mpv)->oi_rt[2].p = (mpv)->out_pln[2] + (yofs);                                        \
	(mpv)->oi_rt[3].p = (Uint8 *)(mpv)->oi_rt[2].p + 8;                                    \
	(mpv)->oi_rt[4].p = (Uint8 *)(mpv)->oi_rt[2].p + (ypitch) * 8;                         \
	(mpv)->oi_rt[5].p = (Uint8 *)(mpv)->oi_rt[4].p + 8

void MPVUMC_BiDirect(MPVUMC_OBJ *mpv)
{
	Sint32 ofs[2];
	MPVUMC_RFB *out = &mpv->out;
	MPVUMC_WORK *wk = &mpv->mcwk;

	mpvumc_OneReadMb(mpv, mpv->mcwk.work, ofs, out, &mpv->fwd);
	mpvumc_OneReadMb(mpv, wk->work2, ofs, out + 1, &mpv->bwd);
	MPVUMC_SET_OUT_BLOCKS(mpv, ofs[0], ofs[1], mpv->out_ypitch);
	mpvumc_BiMakeMb(wk, (MPVCMC_REF *)&mpv->ccnt_rt, mpv->cbp_code);
}

void MPVUMC_Backward(MPVUMC_OBJ *mpv)
{
	Sint32 ofs[2];
	MPVUMC_WORK *wk = &mpv->mcwk;

	mpvumc_OneReadMb(mpv, mpv->mcwk.work, ofs, &mpv->ref, &mpv->bwd);
	MPVUMC_SET_OUT_BLOCKS(mpv, ofs[0], ofs[1], mpv->out_ypitch);
	mpvumc_OneMakeMb(wk, (MPVCMC_REF *)&mpv->ccnt_rt, mpv->cbp_code);
}

void MPVUMC_Forward(MPVUMC_OBJ *mpv)
{
	Sint32 ofs[2];
	MPVUMC_WORK *wk = &mpv->mcwk;

	mpvumc_OneReadMb(mpv, mpv->mcwk.work, ofs, &mpv->out, &mpv->fwd);
	MPVUMC_SET_OUT_BLOCKS(mpv, ofs[0], ofs[1], mpv->out_ypitch);
	mpvumc_OneMakeMb(wk, (MPVCMC_REF *)&mpv->ccnt_rt, mpv->cbp_code);
}

/* clip the six IDCT blocks of an intra macroblock into the output blocks oi_rt[] (`addi r5, 0, 6`
 * instead of `li`: the inline assembler hoists an `li` above the preceding independent `addi`) */
void mpvumc_OutputIntra6blk(Uint8 *blk, MPVCMC_REF *oi_rt, Uint8 *clip)
{
	asm {
		addi r4, r4, 0x4
		addi r5, 0, 0x6
L_80207050:
		lwz r7, 0x0(r4)
		lwz r6, 0x4(r4)
		addi r4, r4, 0x8
		clrlwi. r0, r7, 27
		bne L_80207118
		li r0, 0x2
		mtctr r0
L_8020706C:
		dcbz r0, r7
		psq_l f0, 0x0(r3), 0, 3
		psq_l f1, 0x4(r3), 0, 3
		psq_l f2, 0x8(r3), 0, 3
		psq_l f3, 0xc(r3), 0, 3
		psq_st f0, 0x0(r7), 0, 3
		psq_st f1, 0x2(r7), 0, 3
		psq_st f2, 0x4(r7), 0, 3
		psq_st f3, 0x6(r7), 0, 3
		add r7, r7, r6
		dcbz r0, r7
		psq_l f0, 0x10(r3), 0, 3
		psq_l f1, 0x14(r3), 0, 3
		psq_l f2, 0x18(r3), 0, 3
		psq_l f3, 0x1c(r3), 0, 3
		psq_st f0, 0x0(r7), 0, 3
		psq_st f1, 0x2(r7), 0, 3
		psq_st f2, 0x4(r7), 0, 3
		psq_st f3, 0x6(r7), 0, 3
		add r7, r7, r6
		dcbz r0, r7
		psq_l f0, 0x20(r3), 0, 3
		psq_l f1, 0x24(r3), 0, 3
		psq_l f2, 0x28(r3), 0, 3
		psq_l f3, 0x2c(r3), 0, 3
		psq_st f0, 0x0(r7), 0, 3
		psq_st f1, 0x2(r7), 0, 3
		psq_st f2, 0x4(r7), 0, 3
		psq_st f3, 0x6(r7), 0, 3
		add r7, r7, r6
		dcbz r0, r7
		psq_l f0, 0x30(r3), 0, 3
		psq_l f1, 0x34(r3), 0, 3
		psq_l f2, 0x38(r3), 0, 3
		psq_l f3, 0x3c(r3), 0, 3
		psq_st f0, 0x0(r7), 0, 3
		psq_st f1, 0x2(r7), 0, 3
		psq_st f2, 0x4(r7), 0, 3
		psq_st f3, 0x6(r7), 0, 3
		add r7, r7, r6
		addi r3, r3, 0x40
		bdnz L_8020706C
		b L_802071B8
L_80207118:
		li r0, 0x2
		mtctr r0
L_80207120:
		psq_l f0, 0x0(r3), 0, 3
		psq_l f1, 0x4(r3), 0, 3
		psq_l f2, 0x8(r3), 0, 3
		psq_l f3, 0xc(r3), 0, 3
		psq_st f0, 0x0(r7), 0, 3
		psq_st f1, 0x2(r7), 0, 3
		psq_st f2, 0x4(r7), 0, 3
		psq_st f3, 0x6(r7), 0, 3
		add r7, r7, r6
		psq_l f0, 0x10(r3), 0, 3
		psq_l f1, 0x14(r3), 0, 3
		psq_l f2, 0x18(r3), 0, 3
		psq_l f3, 0x1c(r3), 0, 3
		psq_st f0, 0x0(r7), 0, 3
		psq_st f1, 0x2(r7), 0, 3
		psq_st f2, 0x4(r7), 0, 3
		psq_st f3, 0x6(r7), 0, 3
		add r7, r7, r6
		psq_l f0, 0x20(r3), 0, 3
		psq_l f1, 0x24(r3), 0, 3
		psq_l f2, 0x28(r3), 0, 3
		psq_l f3, 0x2c(r3), 0, 3
		psq_st f0, 0x0(r7), 0, 3
		psq_st f1, 0x2(r7), 0, 3
		psq_st f2, 0x4(r7), 0, 3
		psq_st f3, 0x6(r7), 0, 3
		add r7, r7, r6
		psq_l f0, 0x30(r3), 0, 3
		psq_l f1, 0x34(r3), 0, 3
		psq_l f2, 0x38(r3), 0, 3
		psq_l f3, 0x3c(r3), 0, 3
		psq_st f0, 0x0(r7), 0, 3
		psq_st f1, 0x2(r7), 0, 3
		psq_st f2, 0x4(r7), 0, 3
		psq_st f3, 0x6(r7), 0, 3
		add r7, r7, r6
		addi r3, r3, 0x40
		bdnz L_80207120
L_802071B8:
		subic. r5, r5, 0x1
		bgt L_80207050
	}
}

void MPVUMC_Intra(MPVUMC_OBJ *mpv)
{
	Sint32 cofs;
	Sint32 yofs;
	Sint32 ypitch;
	Sint32 y8;
	Sint32 y16;

	y8 = mpv->mb_y * 8;
	y16 = mpv->mb_y * 16;
	cofs = mpv->mb_x * 8 + y8 * mpv->out_cpitch;
	ypitch = mpv->out_ypitch;
	yofs = mpv->mb_x * 16 + y16 * ypitch;
	MPVUMC_SET_OUT_BLOCKS(mpv, cofs, yofs, ypitch);
	mpvumc_OutputIntra6blk(mpv->mcbuf, (MPVCMC_REF *)&mpv->ccnt_rt, mpv->clip_base);
}

/* GQR3: 8-bit unsigned loads/stores, GQR4: 16-bit signed (scale 0), GQR5: 8-bit unsigned with
 * scale 2^-63 (the 0..255 -> 0..1 range) */
void MPVUMC_SetGqr(void)
{
	asm {
		li r0, 0x4
		oris r0, r0, 0x4
		mtspr GQR4, r0
		li r0, 0x4
		oris r0, r0, 0x7
		mtspr GQR3, r0
		li r0, 0x3f04
		oris r0, r0, 0x3f07
		mtspr GQR5, r0
	}
}

void MPVUMC_EndOfFrame(MPVUMC_OBJ *mpv)
{
}

/* the output frame's planes from the picture size: chroma first (8 pixels per macroblock,
 * 32-byte aligned pitch), luma last */
void MPVUMC_InitOutRfb(MPVUMC_OBJ *mpv)
{
	Sint32 w = mpv->width;
	Sint32 h = mpv->height;
	Sint32 mbw;
	Sint32 mbh;
	Sint32 yw;
	Sint32 ypitch;
	Sint32 cpitch;
	Sint32 yh;
	Uint8 *buf = mpv->frmbuf;

	mbw = (w + 15) / 16;
	yw = mbw * 16;
	ypitch = (yw + 31) / 32 * 32;
	cpitch = (yw / 2 + 31) / 32 * 32;
	mpv->out_ypitch = ypitch;
	mpv->out_cpitch = cpitch;
	mbh = (h + 15) / 16;
	yh = mbh * 16;
	mpv->out_pln[2] = buf;
	mpv->out_pln[0] = mpv->out_pln[2] + yh * ypitch;
	mpv->out_pln[1] = mpv->out_pln[0] + yh / 2 * cpitch;
}

void MPVUMC_Finish(void)
{
}

/* the half-pel kernel tables of the two block sizes: [mcflag][vy&1 * 2 + vx&1]; the second
 * (mcflag) set repeats the first with the four-point entry replaced by the vertical one */
void MPVUMC_Init(void)
{
	mpvumc_oneref[0][0][0] = MPVMC08_OneRef1p_TuneC;
	mpvumc_oneref[0][0][1] = MPVMC08_OneRefH2_TuneC;
	mpvumc_oneref[0][1][0] = MPVMC08_OneRefV2_TuneC;
	mpvumc_oneref[0][1][1] = MPVMC08_OneRef4p_TuneC;
	mpvumc_oneref[1][0][0] = MPVMC08_OneRef1p_TuneC;
	mpvumc_oneref[1][0][1] = MPVMC08_OneRefH2_TuneC;
	mpvumc_oneref[1][1][0] = MPVMC08_OneRefV2_TuneC;
	mpvumc_oneref[1][1][1] = MPVMC08_OneRefV2_TuneC;
	mpvumc_oneref_y[0][0][0] = MPVMC16_OneRef1p_TuneC;
	mpvumc_oneref_y[0][0][1] = MPVMC16_OneRefH2_TuneC;
	mpvumc_oneref_y[0][1][0] = MPVMC16_OneRefV2_TuneC;
	mpvumc_oneref_y[0][1][1] = MPVMC16_OneRef4p_TuneC;
	mpvumc_oneref_y[1][0][0] = MPVMC16_OneRef1p_TuneC;
	mpvumc_oneref_y[1][0][1] = MPVMC16_OneRefH2_TuneC;
	mpvumc_oneref_y[1][1][0] = MPVMC16_OneRefV2_TuneC;
	mpvumc_oneref_y[1][1][1] = MPVMC16_OneRefV2_TuneC;
}
