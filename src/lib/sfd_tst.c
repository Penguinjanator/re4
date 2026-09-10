/* CRI Sofdec: time stabiliser (sfd_tst.c). Smooths the master clock against a helper clock: the
 * difference history is averaged and the output time base is shifted in tolerance steps. */
#include "cri_xpt.h"
#include "sfd.h"
#include <string.h>
#include <stdio.h>

extern Sint32 sfadxt_stat;

/* 64-bit time: count / unit */
typedef struct {
	Sint64 cnt;
	Sint64 unit;
} SFTST_TIME;

#define SFTST_HIST_NUM 60
#define SFTST_DEBOUT_MARGIN 0x400

typedef struct {
	Sint32 tstflg;             /* 0x000 */
	Sint32 pastat;             /* 0x004 pause */
	Sint32 adjmode;            /* 0x008 */
	Sint32 adjflg;             /* 0x00C */
	Sint32 movave_range;       /* 0x010 */
	Sint32 hist_idx;           /* 0x014 */
	Sint32 hist[SFTST_HIST_NUM]; /* 0x018 */
	SFTST_TIME mt;             /* 0x108 last master time */
	SFTST_TIME hlp;            /* 0x118 last helper time */
	SFTST_TIME out;            /* 0x128 last output time */
	SFTST_TIME tolerance;      /* 0x138 */
	SFTST_TIME excesserr;      /* 0x148 */
	SFTST_TIME adjstart;       /* 0x158 */
	SFTST_TIME adjpoff;        /* 0x168 */
	Sint64 base_hlp;           /* 0x178 helper time at the last base (-1: none) */
	Sint64 base_out;           /* 0x180 output time at the last base */
	Sint64 mt_max;             /* 0x188 */
	Sint32 adj_limit;          /* 0x190 */
	Sint32 adj_front;          /* 0x194 */
	Sint32 adj_rear;           /* 0x198 */
	Sint32 resethist;          /* 0x19C */
	Sint32 excesserr_cnt;      /* 0x1A0 */
	Sint32 movave_1st;         /* 0x1A4 */
	Sint32 movave_2nd;         /* 0x1A8 */
	Sint32 diff_l_max;         /* 0x1AC */
	Sint32 diff_l_min;         /* 0x1B0 */
	Sint32 diff_a_max;         /* 0x1B4 */
	Sint32 diff_a_min;         /* 0x1B8 */
	Sint32 rsv;                /* 0x1BC */
} SFTST_WORK;

typedef SFTST_WORK *SFTST;

Sint32 sftst_debout_siz;
Char8 *sftst_debout_buf;
Char8 *sftst_debout_round;
Char8 *sftst_debout_write;
SFTST sftst_last;

/* debug log buffer (dead: keeps the .bss order siz, buf, round, write, last) */
void SFTST_SetDebugOut(Char8 *buf, Sint32 siz)
{
	sftst_debout_siz = siz;
	sftst_debout_buf = buf;
	sftst_debout_round = buf;
	sftst_debout_write = buf;
}

static void sftst_ResetHist(SFTST tst)
{
	memset(tst->hist, 0, sizeof(tst->hist));
	tst->hist_idx = 0;
	tst->resethist++;
}

/* a * b / c in the unit of another time */
static Sint64 sftst_Conv(SFTST_TIME *t, Sint64 unit)
{
	return unit * t->cnt / t->unit;
}

static Sint32 sftst_SumHist(SFTST tst)
{
	Sint32 sum;
	Sint32 i;

	sum = 0;
	for (i = 0; i < tst->movave_range; i++) {
		sum += tst->hist[i];
	}
	return sum;
}

/* COMPILER-DIFF: M1 - the original emitted SFTST_Create's header string before SFTST_Calc's format
 * (its .text follows); both are named here (a named string is emitted at its declaration) and the
 * two functions are asm functions addressing them, their C bodies dead `_c` twins */
extern Sint64 __div2i(Sint64 a, Sint64 b); /* the 64-bit division runtime call of the asm functions */
static const Char8 sftst_msg_hdr[] = "tst, help_time_sec, help_time_msec, help_time_64, help_time, mt_max, master_time, out_time,  mt_ot, mtmax_ot,  diff_l_max, diff_l_min, diff_a_max, tst->diff_a_min, pastat, adjmode, resethist, excesserr, adj_limit, adj_front, adj_rear,  movave_1st, movave_2nd,  adxt_stat \n\n"; // COMPILER-DIFF: M1
static const Char8 sftst_msg_fmt[] = "%p, %ld, %ld, %08lX%08lX, %ld, %ld, %ld, %ld,   %ld, %ld,   %ld, %ld, %ld, %ld,   %ld, %ld,   %ld, %ld, %ld, %ld, %ld,   %ld, %ld,   %ld \n";

/* COMPILER-DIFF: M1 - the 64-bit abs diamond kept in place with two `mr` (if/else and ternary forms get sunk to the compare). Asm function (the original's instructions verbatim). */
asm void SFTST_Calc(SFTST tst, SFTST_TIME *mt, SFTST_TIME *hlp, SFTST_TIME *out) // COMPILER-DIFF: M1
{
	nofralloc
	stwu r1, -400(r1)
	mflr r0
	lis r9, sftst_debout_siz@ha
	stw r0, 404(r1)
	li r0, 0
	stmw r21, 356(r1)
	mr r29, r5
	mr r31, r3
	mr r30, r4
	mr r28, r6
	addi r26, r9, sftst_debout_siz@l
	lwz r7, 8(r5)
	lwz r8, 12(r5)
	li r5, 1
	xor r0, r7, r0
	xor r3, r8, r5
	or. r0, r3, r0
	beq L54
	lwz r0, 0(r31)
	cmpwi r0, 0
	bne L68
L54:
	lfd f1, 0(r30)
	lfd f0, 8(r30)
	stfd f1, 0(r28)
	stfd f0, 8(r28)
	b La3c
L68:
	lwz r5, 392(r31)
	lwz r7, 0(r30)
	lwz r6, 396(r31)
	xoris r3, r5, 32768
	lwz r8, 4(r30)
	xoris r4, r7, 32768
	subfc r0, r6, r8
	subfe r3, r3, r4
	subfe r3, r4, r4
	neg. r3, r3
	beq L98
	b La0
L98:
	mr r6, r8
	mr r5, r7
La0:
	stw r6, 4(r30)
	stw r5, 0(r30)
	lwz r0, 4(r31)
	cmpwi r0, 1
	bne Lc0
	li r0, 0
	stw r0, 8(r31)
	b L1fc
Lc0:
	lwz r0, 8(r31)
	cmpwi r0, 0
	bne L1fc
	lwz r3, 0(r30)
	lwz r0, 392(r31)
	lwz r6, 4(r30)
	xoris r3, r3, 32768
	lwz r5, 396(r31)
	xoris r4, r0, 32768
	subfc r0, r6, r5
	subfe r3, r3, r4
	subfe r3, r4, r4
	neg. r3, r3
	beq L1e8
	li r0, 1
	li r3, -1
	stw r0, 8(r31)
	lwz r0, 376(r31)
	lwz r4, 380(r31)
	xor r0, r0, r3
	xor r3, r4, r3
	or. r0, r3, r0
	bne L15c
	lwz r8, 12(r30)
	lwz r4, 348(r31)
	lwz r3, 8(r30)
	mulhwu r0, r8, r4
	lwz r7, 344(r31)
	lwz r5, 352(r31)
	lwz r6, 356(r31)
	mullw r3, r3, r4
	add r3, r0, r3
	mullw r0, r8, r7
	mullw r4, r8, r4
	add r3, r3, r0
	bl __div2i
	mr r8, r4
	mr r7, r3
	b L198
L15c:
	lwz r8, 12(r30)
	lwz r4, 364(r31)
	lwz r3, 8(r30)
	mulhwu r0, r8, r4
	lwz r7, 360(r31)
	lwz r5, 368(r31)
	lwz r6, 372(r31)
	mullw r3, r3, r4
	add r3, r0, r3
	mullw r0, r8, r7
	mullw r4, r8, r4
	add r3, r3, r0
	bl __div2i
	mr r8, r4
	mr r7, r3
L198:
	lwz r0, 0(r29)
	addi r3, r31, 24
	lwz r6, 4(r29)
	li r4, 0
	li r5, 240
	stw r6, 380(r31)
	stw r0, 376(r31)
	lwz r0, 4(r30)
	lwz r6, 0(r30)
	addc r0, r0, r8
	stw r0, 388(r31)
	adde r0, r6, r7
	stw r0, 384(r31)
	bl memset
	li r0, 0
	stw r0, 20(r31)
	lwz r3, 412(r31)
	addi r0, r3, 1
	stw r0, 412(r31)
	b L1fc
L1e8:
	lwz r0, 12(r31)
	cmpwi r0, 0
	bne L1fc
	li r0, 1
	stw r0, 8(r31)
L1fc:
	lwz r5, 392(r31)
	lwz r7, 0(r30)
	lwz r6, 396(r31)
	xoris r3, r5, 32768
	lwz r8, 4(r30)
	xoris r4, r7, 32768
	subfc r0, r6, r8
	subfe r3, r3, r4
	subfe r3, r4, r4
	neg. r3, r3
	beq L22c
	b L234
L22c:
	mr r6, r8
	mr r5, r7
L234:
	stw r6, 396(r31)
	li r3, -1
	stw r5, 392(r31)
	lwz r6, 376(r31)
	lwz r4, 380(r31)
	xor r0, r6, r3
	xor r3, r4, r3
	or. r0, r3, r0
	bne L264
	li r27, 0
	mr r24, r27
	b L2b0
L264:
	lwz r0, 4(r29)
	lwz r9, 12(r30)
	subfc r4, r4, r0
	lwz r3, 0(r29)
	lwz r8, 8(r30)
	mulhwu r0, r9, r4
	subfe r7, r6, r3
	lwz r5, 8(r29)
	lwz r6, 12(r29)
	mullw r3, r8, r4
	add r3, r0, r3
	mullw r0, r9, r7
	mullw r4, r9, r4
	add r3, r3, r0
	bl __div2i
	lwz r5, 388(r31)
	lwz r0, 384(r31)
	addc r27, r5, r4
	adde r24, r0, r3
L2b0:
	lwz r0, 8(r31)
	cmpwi r0, 0
	bne L340
	lwz r0, 392(r31)
	xoris r3, r24, 32768
	lwz r5, 396(r31)
	xoris r4, r0, 32768
	subfc r0, r27, r5
	subfe r3, r3, r4
	subfe r3, r4, r4
	neg. r3, r3
	beq L76c
	lwz r0, 12(r31)
	cmpwi r0, 0
	beq L310
	lwz r0, 0(r29)
	lwz r3, 4(r29)
	stw r3, 380(r31)
	stw r0, 376(r31)
	lwz r0, 392(r31)
	lwz r3, 396(r31)
	stw r3, 388(r31)
	stw r0, 384(r31)
	b L330
L310:
	lwz r0, 0(r29)
	lwz r3, 4(r29)
	stw r3, 380(r31)
	stw r0, 376(r31)
	lwz r0, 296(r31)
	lwz r3, 300(r31)
	stw r3, 388(r31)
	stw r0, 384(r31)
L330:
	lwz r3, 400(r31)
	addi r0, r3, 1
	stw r0, 400(r31)
	b L76c
L340:
	lwz r0, 12(r31)
	cmpwi r0, 1
	bne L76c
	lwz r5, 4(r30)
	li r0, 0
	lwz r4, 0(r30)
	xoris r3, r0, 32768
	subfc r25, r27, r5
	subfe r23, r24, r4
	xoris r4, r23, 32768
	subfc r0, r0, r25
	subfe r3, r3, r4
	subfe r3, r4, r4
	neg. r3, r3
	beq L388
	subfic r22, r25, 0
	subfze r23, r23
	b L38c
L388:
	mr r22, r25
L38c:
	lwz r8, 12(r30)
	lwz r4, 332(r31)
	lwz r3, 8(r30)
	mulhwu r0, r8, r4
	lwz r7, 328(r31)
	lwz r5, 336(r31)
	lwz r6, 340(r31)
	mullw r3, r3, r4
	add r3, r0, r3
	mullw r0, r8, r7
	mullw r4, r8, r4
	add r3, r3, r0
	bl __div2i
	xoris r5, r3, 32768
	xoris r3, r23, 32768
	subfc r0, r22, r4
	subfe r3, r3, r5
	subfe r3, r5, r5
	neg. r3, r3
	beq L430
	lwz r0, 0(r29)
	addi r3, r31, 24
	lwz r6, 4(r29)
	li r4, 0
	li r5, 240
	stw r6, 380(r31)
	stw r0, 376(r31)
	lwz r0, 0(r30)
	lwz r6, 4(r30)
	stw r6, 388(r31)
	stw r0, 384(r31)
	bl memset
	li r0, 0
	stw r0, 20(r31)
	lwz r3, 412(r31)
	addi r0, r3, 1
	stw r0, 412(r31)
	lwz r3, 416(r31)
	addi r0, r3, 1
	stw r0, 416(r31)
	b L76c
L430:
	lwz r6, 20(r31)
	li r0, 0
	mr r3, r0
	addi r4, r6, 1
	stw r4, 20(r31)
	lwz r5, 16(r31)
	divw r4, r6, r5
	mullw r4, r4, r5
	subf r4, r4, r6
	slwi r4, r4, 2
	add r4, r31, r4
	stw r25, 24(r4)
	lwz r7, 16(r31)
	cmpwi r7, 0
	ble L504
	cmpwi r7, 8
	addi r5, r7, -8
	ble L4dc
	addi r4, r5, 7
	mr r6, r31
	srwi r4, r4, 3
	mtctr r4
	cmpwi r5, 0
	ble L4dc
L490:
	lwz r5, 24(r6)
	addi r3, r3, 8
	lwz r4, 28(r6)
	add r0, r0, r5
	lwz r5, 32(r6)
	add r0, r0, r4
	lwz r4, 36(r6)
	add r0, r0, r5
	lwz r5, 40(r6)
	add r0, r0, r4
	lwz r4, 44(r6)
	add r0, r0, r5
	lwz r5, 48(r6)
	add r0, r0, r4
	lwz r4, 52(r6)
	add r0, r0, r5
	addi r6, r6, 32
	add r0, r0, r4
	bdnz L490
L4dc:
	slwi r5, r3, 2
	subf r4, r3, r7
	add r5, r31, r5
	mtctr r4
	cmpw r3, r7
	bge L504
L4f4:
	lwz r3, 24(r5)
	addi r5, r5, 4
	add r0, r0, r3
	bdnz L4f4
L504:
	divw r0, r0, r7
	stw r0, 420(r31)
	mr r25, r0
	srawi r22, r0, 31
	stw r0, 424(r31)
	lwz r8, 12(r30)
	lwz r4, 316(r31)
	lwz r3, 8(r30)
	mulhwu r0, r8, r4
	lwz r7, 312(r31)
	lwz r5, 320(r31)
	lwz r6, 324(r31)
	mullw r3, r3, r4
	add r3, r0, r3
	mullw r0, r8, r7
	mullw r4, r8, r4
	add r3, r3, r0
	bl __div2i
	li r0, 0
	xoris r6, r22, 32768
	xoris r5, r0, 32768
	mr r21, r4
	subfc r0, r0, r25
	mr r23, r3
	subfe r5, r5, r6
	subfe r5, r6, r6
	neg. r5, r5
	beq L580
	subfic r5, r25, 0
	subfze r0, r22
	b L588
L580:
	mr r5, r25
	mr r0, r22
L588:
	xoris r3, r0, 32768
	xoris r4, r23, 32768
	subfc r0, r5, r21
	subfe r3, r3, r4
	subfe r3, r4, r4
	neg. r3, r3
	beq L76c
	xoris r3, r22, 32768
	subfc r0, r25, r21
	subfe r3, r3, r4
	subfe r3, r4, r4
	neg. r3, r3
	beq L5f8
	slwi r3, r22, 1
	slwi r4, r25, 1
	rlwimi r3, r25, 1, 31, 31
	mr r5, r23
	mr r6, r21
	bl __div2i
	li r5, -1
	lwz r0, 404(r31)
	addc r4, r4, r5
	add r0, r0, r4
	adde r3, r3, r5
	stw r0, 404(r31)
	mr r8, r4
	mr r5, r3
	b L63c
L5f8:
	slwi r3, r22, 1
	slwi r4, r25, 1
	rlwimi r3, r25, 1, 31, 31
	mr r5, r23
	mr r6, r21
	bl __div2i
	li r5, 1
	li r0, 0
	addc r5, r4, r5
	lwz r4, 408(r31)
	adde r3, r3, r0
	subfic r0, r5, 0
	mr r8, r5
	add r0, r4, r0
	mr r5, r3
	stw r0, 408(r31)
	subfze r0, r3
L63c:
	mulhwu r4, r8, r21
	lwz r0, 0(r29)
	lwz r3, 4(r29)
	mr r7, r31
	li r6, 0
	stw r3, 380(r31)
	mullw r3, r5, r21
	stw r0, 376(r31)
	mullw r5, r8, r21
	add r3, r4, r3
	mullw r0, r8, r23
	rotlwi r4, r5, 31
	add r3, r3, r0
	mr r0, r3
	rlwimi r0, r5, 0, 31, 31
	rlwimi r4, r3, 31, 0, 0
	srawi r3, r0, 1
	addze r4, r4
	addze r3, r3
	addc r0, r27, r4
	stw r0, 388(r31)
	adde r0, r24, r3
	stw r0, 384(r31)
	b L6b0
L69c:
	lwz r0, 24(r7)
	addi r6, r6, 1
	subf r0, r4, r0
	stw r0, 24(r7)
	addi r7, r7, 4
L6b0:
	lwz r8, 16(r31)
	cmpw r6, r8
	blt L69c
	cmpwi r8, 0
	li r4, 0
	mr r5, r4
	ble L764
	cmpwi r8, 8
	addi r3, r8, -8
	ble L73c
	addi r0, r3, 7
	mr r6, r31
	srwi r0, r0, 3
	mtctr r0
	cmpwi r3, 0
	ble L73c
L6f0:
	lwz r3, 24(r6)
	addi r5, r5, 8
	lwz r0, 28(r6)
	add r4, r4, r3
	lwz r3, 32(r6)
	add r4, r4, r0
	lwz r0, 36(r6)
	add r4, r4, r3
	lwz r3, 40(r6)
	add r4, r4, r0
	lwz r0, 44(r6)
	add r4, r4, r3
	lwz r3, 48(r6)
	add r4, r4, r0
	lwz r0, 52(r6)
	add r4, r4, r3
	addi r6, r6, 32
	add r4, r4, r0
	bdnz L6f0
L73c:
	slwi r3, r5, 2
	subf r0, r5, r8
	add r3, r31, r3
	mtctr r0
	cmpw r5, r8
	bge L764
L754:
	lwz r0, 24(r3)
	addi r3, r3, 4
	add r4, r4, r0
	bdnz L754
L764:
	divw r0, r4, r8
	stw r0, 424(r31)
L76c:
	lwz r6, 376(r31)
	li r3, -1
	lwz r4, 380(r31)
	xor r0, r6, r3
	xor r3, r4, r3
	or. r0, r3, r0
	bne L794
	li r4, 0
	mr r0, r4
	b L7e0
L794:
	lwz r0, 4(r29)
	lwz r9, 12(r30)
	subfc r4, r4, r0
	lwz r3, 0(r29)
	lwz r8, 8(r30)
	mulhwu r0, r9, r4
	subfe r7, r6, r3
	lwz r5, 8(r29)
	lwz r6, 12(r29)
	mullw r3, r8, r4
	add r3, r0, r3
	mullw r0, r9, r7
	mullw r4, r9, r4
	add r3, r3, r0
	bl __div2i
	lwz r5, 388(r31)
	lwz r0, 384(r31)
	addc r4, r5, r4
	adde r0, r0, r3
L7e0:
	stw r4, 4(r28)
	stw r0, 0(r28)
	lwz r0, 8(r30)
	lwz r3, 12(r30)
	stw r3, 12(r28)
	stw r0, 8(r28)
	lwz r3, 0(r28)
	lwz r0, 296(r31)
	xoris r4, r3, 32768
	lwz r6, 4(r28)
	lwz r5, 300(r31)
	xoris r3, r0, 32768
	subfc r0, r5, r6
	subfe r3, r3, r4
	subfe r3, r4, r4
	neg. r3, r3
	beq L834
	lfd f1, 296(r31)
	lfd f0, 304(r31)
	stfd f1, 0(r28)
	stfd f0, 8(r28)
L834:
	lfd f1, 0(r30)
	lfd f0, 8(r30)
	stfd f1, 264(r31)
	stfd f0, 272(r31)
	lfd f1, 0(r29)
	lfd f0, 8(r29)
	stfd f1, 280(r31)
	stfd f0, 288(r31)
	lfd f1, 0(r28)
	lfd f0, 8(r28)
	stfd f1, 296(r31)
	stfd f0, 304(r31)
	lwz r0, 8(r31)
	lwz r5, 4(r30)
	lwz r3, 4(r28)
	cmpwi r0, 0
	subfc r3, r3, r5
	mr r5, r3
	bne L8b0
	lwz r0, 428(r31)
	cmpw r0, r3
	ble L890
	mr r3, r0
L890:
	stw r3, 428(r31)
	mr r0, r5
	lwz r3, 432(r31)
	cmpw r3, r5
	bge L8a8
	mr r0, r3
L8a8:
	stw r0, 432(r31)
	b L8dc
L8b0:
	lwz r0, 436(r31)
	cmpw r0, r3
	ble L8c0
	mr r3, r0
L8c0:
	stw r3, 436(r31)
	mr r0, r5
	lwz r3, 440(r31)
	cmpw r3, r5
	bge L8d8
	mr r0, r3
L8d8:
	stw r0, 440(r31)
L8dc:
	lwz r0, 4(r26)
	stw r31, 16(r26)
	cmplwi r0, 0
	beq La3c
	lwz r30, 284(r31)
	li r3, 1000
	lwz r22, 296(r31)
	lwz r23, 300(r31)
	mr r4, r30
	lwz r21, 264(r31)
	lwz r25, 268(r31)
	lwz r27, 392(r31)
	lwz r28, 396(r31)
	lwz r29, 280(r31)
	lwz r5, 292(r31)
	bl UTY_MulDiv
	mr r24, r3
	lwz r4, 284(r31)
	lwz r3, 280(r31)
	lwz r5, 288(r31)
	lwz r6, 292(r31)
	bl __div2i
	stw r28, 8(r1)
	subfc r9, r23, r25
	subfe r0, r22, r21
	lis r3, -32768
	stw r25, 12(r1)
	addi r0, r3, -1
	lis r7, sfadxt_stat@ha
	subfc r8, r23, r28
	stw r23, 16(r1)
	addi r11, r7, sfadxt_stat@l
	and r10, r30, r0
	lis r5, sftst_msg_fmt@ha
	stw r9, 20(r1)
	mr r6, r4
	addi r4, r5, sftst_msg_fmt@l
	mr r5, r31
	stw r8, 24(r1)
	mr r7, r24
	mr r8, r29
	mr r9, r30
	lwz r3, 428(r31)
	stw r3, 28(r1)
	addi r3, r1, 88
	lwz r0, 432(r31)
	stw r0, 32(r1)
	lwz r0, 436(r31)
	stw r0, 36(r1)
	lwz r0, 440(r31)
	stw r0, 40(r1)
	lwz r0, 4(r31)
	stw r0, 44(r1)
	lwz r0, 8(r31)
	stw r0, 48(r1)
	lwz r0, 412(r31)
	stw r0, 52(r1)
	lwz r0, 416(r31)
	stw r0, 56(r1)
	lwz r0, 400(r31)
	stw r0, 60(r1)
	lwz r0, 404(r31)
	stw r0, 64(r1)
	lwz r0, 408(r31)
	stw r0, 68(r1)
	lwz r0, 420(r31)
	stw r0, 72(r1)
	lwz r0, 424(r31)
	stw r0, 76(r1)
	lwz r0, 0(r11)
	stw r0, 80(r1)
	crclr 4*cr1+eq
	bl sprintf
	mr r21, r3
	lwz r3, 12(r26)
	addi r4, r1, 88
	bl strcpy
	lwz r3, 0(r26)
	lwz r5, 12(r26)
	lwz r4, 4(r26)
	addi r0, r3, -1024
	add r3, r5, r21
	add r0, r4, r0
	stw r3, 12(r26)
	cmplw r3, r0
	blt La3c
	lwz r0, 8(r26)
	stw r0, 12(r26)
La3c:
	lmw r21, 356(r1)
	lwz r0, 404(r1)
	mtlr r0
	addi r1, r1, 400
	blr
}
/* the C body, kept compiled (dead, stripped by strip_unused) so that its literals stay in .rodata */
void SFTST_Calc_c(SFTST tst, SFTST_TIME *mt, SFTST_TIME *hlp, SFTST_TIME *out)
{
	Sint64 est;
	Sint64 q;
	Sint64 adj;
	Sint64 diff;
	Sint64 adiff;
	Sint64 ave;
	Sint64 aave;
	Sint64 tol;
	Sint64 step;
	Sint32 idx;
	Sint32 i;
	Sint32 d;
	Sint64 t;
	Sint64 excess;
	Char8 buf[0x100];
	Sint32 n;

	if (hlp->unit == 1 || tst->tstflg == 0) {
		*out = *mt;
		return;
	}
	mt->cnt = (tst->mt_max > mt->cnt) ? tst->mt_max : mt->cnt;
	if (tst->pastat == 1) {
		tst->adjmode = 0;
	} else if (tst->adjmode == 0) {
		if (mt->cnt > tst->mt_max) {
			tst->adjmode = 1;
			if (tst->base_hlp == -1) {
				adj = sftst_Conv(&tst->adjstart, mt->unit);
			} else {
				adj = sftst_Conv(&tst->adjpoff, mt->unit);
			}
			tst->base_hlp = hlp->cnt;
			tst->base_out = mt->cnt + adj;
			sftst_ResetHist(tst);
		} else if (tst->adjflg == 0) {
			tst->adjmode = 1;
		}
	}
	tst->mt_max = (tst->mt_max > mt->cnt) ? tst->mt_max : mt->cnt;
	if (tst->base_hlp == -1) {
		est = 0;
	} else {
		est = tst->base_out + mt->unit * (hlp->cnt - tst->base_hlp) / hlp->unit;
	}
	if (tst->adjmode == 0) {
		if (tst->mt_max < est) {
			if (tst->adjflg != 0) {
				tst->base_hlp = hlp->cnt;
				tst->base_out = tst->mt_max;
			} else {
				tst->base_hlp = hlp->cnt;
				tst->base_out = tst->out.cnt;
			}
			tst->adj_limit++;
		}
	} else if (tst->adjflg == 1) {
		diff = mt->cnt - est;
		adiff = diff;
		if (diff < 0) {
			adiff = -diff;
		}
		excess = sftst_Conv(&tst->excesserr, mt->unit);
		if (excess < adiff) {
			tst->base_hlp = hlp->cnt;
			tst->base_out = mt->cnt;
			sftst_ResetHist(tst);
			tst->excesserr_cnt++;
		} else {
			idx = tst->hist_idx;
			tst->hist_idx = idx + 1;
			tst->hist[idx % tst->movave_range] = (Sint32)diff;
			ave = sftst_SumHist(tst) / tst->movave_range;
			tst->movave_1st = (Sint32)ave;
			tst->movave_2nd = (Sint32)ave;
			tol = sftst_Conv(&tst->tolerance, mt->unit);
			aave = (ave < 0) ? -ave : ave;
			if (tol < aave) {
				if (tol < ave) {
					q = ave * 2 / tol - 1;
					tst->adj_front += (Sint32)q;
				} else {
					q = ave * 2 / tol + 1;
					tst->adj_rear += (Sint32)-q;
				}
				adj = q;
				step = adj * tol / 2;
				tst->base_hlp = hlp->cnt;
				tst->base_out = est + step;
				for (i = 0; i < tst->movave_range; i++) {
					tst->hist[i] -= (Sint32)step;
				}
				tst->movave_2nd = sftst_SumHist(tst) / tst->movave_range;
			}
		}
	}
	if (tst->base_hlp == -1) {
		t = 0;
	} else {
		t = tst->base_out + mt->unit * (hlp->cnt - tst->base_hlp) / hlp->unit;
	}
	out->cnt = t;
	out->unit = mt->unit;
	if (out->cnt < tst->out.cnt) {
		*out = tst->out;
	}
	tst->mt = *mt;
	tst->hlp = *hlp;
	tst->out = *out;
	d = (Sint32)(mt->cnt - out->cnt);
	if (tst->adjmode == 0) {
		tst->diff_l_max = (tst->diff_l_max > d) ? tst->diff_l_max : d;
		tst->diff_l_min = (tst->diff_l_min < d) ? tst->diff_l_min : d;
	} else {
		tst->diff_a_max = (tst->diff_a_max > d) ? tst->diff_a_max : d;
		tst->diff_a_min = (tst->diff_a_min < d) ? tst->diff_a_min : d;
	}
	sftst_last = tst;
	if (sftst_debout_buf != NULL) {
		n = sprintf(buf, sftst_msg_fmt,
			tst, (Sint32)(tst->hlp.cnt / tst->hlp.unit),
			UTY_MulDiv(1000, (Sint32)tst->hlp.cnt, (Sint32)tst->hlp.unit),
			(Sint32)(tst->hlp.cnt >> 32), (Sint32)tst->hlp.cnt, (Sint32)(tst->hlp.cnt & 0x7FFFFFFF),
			(Sint32)tst->mt_max, (Sint32)tst->mt.cnt, (Sint32)tst->out.cnt,
			(Sint32)(tst->mt.cnt - tst->out.cnt), (Sint32)(tst->mt_max - tst->out.cnt),
			tst->diff_l_max, tst->diff_l_min, tst->diff_a_max, tst->diff_a_min,
			tst->pastat, tst->adjmode, tst->resethist, tst->excesserr_cnt,
			tst->adj_limit, tst->adj_front, tst->adj_rear, tst->movave_1st, tst->movave_2nd,
			sfadxt_stat);
		strcpy(sftst_debout_write, buf);
		sftst_debout_write += n;
		if (sftst_debout_write >= sftst_debout_buf + sftst_debout_siz - SFTST_DEBOUT_MARGIN) {
			sftst_debout_write = sftst_debout_round;
		}
	}
}

void SFTST_GoNextFrame(SFTST tst, SFTST_TIME *frm)
{
	if (tst->adjflg == 0) {
		tst->out.cnt += tst->out.unit * frm->cnt / frm->unit;
	}
}

void SFTST_SetAdjFlg(SFTST tst, Sint32 flg)
{
	tst->adjflg = flg;
}

void SFTST_Pause(SFTST tst, Sint32 sw)
{
	tst->pastat = sw;
}

void SFTST_SetMovaveRange(SFTST tst, Sint32 range)
{
	if (range > 0) {
		tst->movave_range = range;
	}
}

void SFTST_SetAdjPoff(SFTST tst, SFTST_TIME *t)
{
	tst->adjpoff = *t;
}

void SFTST_SetAdjStart(SFTST tst, SFTST_TIME *t)
{
	tst->adjstart = *t;
}

void SFTST_SetExcessErr(SFTST tst, SFTST_TIME *t)
{
	tst->excesserr = *t;
}

void SFTST_SetTolerance(SFTST tst, SFTST_TIME *t)
{
	tst->tolerance = *t;
}

void SFTST_SetTstFlg(SFTST tst, Sint32 flg)
{
	tst->tstflg = flg;
}

/* COMPILER-DIFF: M1 - the `lwz sftst_debout_buf` scheduled above the hdr copy tail. Asm function (the original's instructions verbatim). */
asm void SFTST_Create(SFTST tst) // COMPILER-DIFF: M1
{
	nofralloc
	stwu r1, -304(r1)
	mflr r0
	lis r4, sftst_debout_siz@ha
	li r5, 448
	stw r0, 308(r1)
	stw r31, 300(r1)
	addi r31, r4, sftst_debout_siz@l
	li r4, 0
	stw r30, 296(r1)
	mr r30, r3
	bl memset
	li r7, 1
	li r6, 0
	stw r7, 0(r30)
	li r0, 10
	addi r3, r30, 24
	li r4, 0
	stw r6, 4(r30)
	li r5, 240
	stw r6, 8(r30)
	stw r7, 12(r30)
	stw r0, 16(r30)
	bl memset
	li r12, 0
	lis r5, 15
	stw r12, 20(r30)
	lis r4, 3
	lis r3, sftst_msg_hdr@ha
	li r11, 1
	lwz r6, 412(r30)
	addi r3, r3, sftst_msg_hdr@l
	li r10, 16683
	addi r9, r5, 16960
	addi r0, r6, 1
	addi r8, r4, 3392
	stw r0, 412(r30)
	li r7, -16683
	li r6, -1
	li r0, 34
	stw r12, 268(r30)
	addi r5, r1, 4
	addi r4, r3, -4
	stw r12, 264(r30)
	stw r11, 276(r30)
	stw r12, 272(r30)
	stw r12, 284(r30)
	stw r12, 280(r30)
	stw r11, 292(r30)
	stw r12, 288(r30)
	stw r12, 300(r30)
	stw r12, 296(r30)
	stw r11, 308(r30)
	stw r12, 304(r30)
	stw r10, 316(r30)
	stw r12, 312(r30)
	stw r9, 324(r30)
	stw r12, 320(r30)
	stw r8, 332(r30)
	stw r12, 328(r30)
	stw r9, 340(r30)
	stw r12, 336(r30)
	stw r7, 348(r30)
	stw r6, 344(r30)
	stw r9, 356(r30)
	stw r12, 352(r30)
	stw r7, 364(r30)
	stw r6, 360(r30)
	stw r9, 372(r30)
	stw r12, 368(r30)
	stw r6, 380(r30)
	stw r6, 376(r30)
	stw r12, 388(r30)
	stw r12, 384(r30)
	stw r12, 396(r30)
	stw r12, 392(r30)
	stw r12, 400(r30)
	stw r12, 404(r30)
	stw r12, 408(r30)
	stw r12, 412(r30)
	stw r12, 416(r30)
	stw r12, 420(r30)
	stw r12, 424(r30)
	stw r12, 428(r30)
	stw r12, 432(r30)
	stw r12, 436(r30)
	stw r12, 440(r30)
	mtctr r0
Lca4:
	lwz r3, 4(r4)
	lwzu r0, 8(r4)
	stw r3, 4(r5)
	stwu r0, 8(r5)
	bdnz Lca4
	lhz r0, 4(r4)
	sth r0, 4(r5)
	lwz r3, 4(r31)
	cmplwi r3, 0
	beq Ld00
	lwz r5, 0(r31)
	li r4, 0
	bl memset
	lwz r3, 4(r31)
	addi r4, r1, 8
	stw r3, 12(r31)
	bl strcpy
	addi r3, r1, 8
	bl strlen
	lwz r0, 12(r31)
	add r0, r0, r3
	stw r0, 12(r31)
	stw r0, 8(r31)
Ld00:
	lwz r0, 308(r1)
	lwz r31, 300(r1)
	lwz r30, 296(r1)
	mtlr r0
	addi r1, r1, 304
	blr
}
/* the C body, kept compiled (dead, stripped by strip_unused) so that its literals stay in .rodata */
void SFTST_Create_c(SFTST tst)
{
	memset(tst, 0, sizeof(SFTST_WORK));
	tst->tstflg = 1;
	tst->pastat = 0;
	tst->adjmode = 0;
	tst->adjflg = 1;
	tst->movave_range = 10;
	sftst_ResetHist(tst);
	tst->mt.cnt = 0;
	tst->mt.unit = 1;
	tst->hlp.cnt = 0;
	tst->hlp.unit = 1;
	tst->out.cnt = 0;
	tst->out.unit = 1;
	tst->tolerance.cnt = 16683;
	tst->tolerance.unit = 1000000;
	tst->excesserr.cnt = 200000;
	tst->excesserr.unit = 1000000;
	tst->adjstart.cnt = -16683;
	tst->adjstart.unit = 1000000;
	tst->adjpoff.cnt = -16683;
	tst->adjpoff.unit = 1000000;
	tst->base_hlp = -1;
	tst->base_out = 0;
	tst->mt_max = 0;
	tst->adj_limit = 0;
	tst->adj_front = 0;
	tst->adj_rear = 0;
	tst->resethist = 0;
	tst->excesserr_cnt = 0;
	tst->movave_1st = 0;
	tst->movave_2nd = 0;
	tst->diff_l_max = 0;
	tst->diff_l_min = 0;
	tst->diff_a_max = 0;
	tst->diff_a_min = 0;
	{
		Char8 hdr[sizeof(sftst_msg_hdr)];

		memcpy(hdr, sftst_msg_hdr, sizeof(hdr));

		if (sftst_debout_buf != NULL) {
			memset(sftst_debout_buf, 0, sftst_debout_siz);
			sftst_debout_write = sftst_debout_buf;
			strcpy(sftst_debout_write, hdr);
			sftst_debout_write += strlen(hdr);
			sftst_debout_round = sftst_debout_write;
		}
	}
}
