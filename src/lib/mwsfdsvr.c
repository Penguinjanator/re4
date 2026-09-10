/* Sofdec MW player: server (decode / vsync / idle thread) entry points */
#include "cri_xpt.h"
#include "sj.h"
#include "mwsfd.h"
#include "adx_stm.h"

extern void mwPlySaveRsc(void);
extern void mwPlyRestoreRsc(void);
extern void ADXM_WaitVsync(void);
extern Sint32 ADXM_IsSetupThrd(void);
extern void SFD_ExecOne(void *sfd);
extern Sint32 SFD_IsHnSvrWait(void *sfd);
extern Sint32 SFD_IsSvrWait(void);
extern void SFD_VbIn(void);
extern Sint32 SFD_GetHnStat(void *sfd);
extern Sint32 SFD_SetConcatPlay(void *sfd);
extern Sint32 SFD_TermSupply(void *sfd);
extern Sint32 MWSTM_GetStat(ADXSTM stm);
extern void MWSTM_SetFileRange(ADXSTM stm, const Char8 *fname, void *dir, Sint32 ofst, Sint32 nsct);
extern Sint32 MWSTM_ReqStart(ADXSTM stm);
extern Sint32 MWSTM_IsFsStatErr(ADXSTM stm);
extern Sint32 MWSFLSC_IsFsStatErr(void *lsc);
extern Sint32 LSC_GetNumStm(void *lsc);
extern void MWSFCRE_SetSupplySj(MWPLY mwply);
extern void mwPlySfdStart(MWPLY mwply);
extern void mwSfdPause(MWPLY mwply, Sint32 sw);
extern void mwPlyChkSupply(void);
extern void MWSFSFX_DecideCompoMode(MWPLY mwply);
extern void MWSFSEE_ChkSupply(MWPLY mwply);

static Sint32 mwg_field_no = 0;
Sint32 mwg_vcnt = 0;
MWPLY mwsfd_hn_last = NULL;
static Sint32 mwsfd_svr_bdr_cnt = 0;

Sint32 mwSfdExecDecSvrHndl(MWPLY mwply);

/* dead-stripped; its string heads the unit's literal pool */
/* the first .rodata string is named so that the asm mwSfdExecDecSvrHndl below can address the string
 * pool through it (it is the compiler's `...rodata.0` base) COMPILER-DIFF: M1 */
static const Char8 mwsfsvr_msg_bdrhndl[] = "E2011101: MWSFSVR_IsSvrBdrHndl: handle is invalid.";

Bool MWSFSVR_IsSvrBdrHndl(MWPLY mwply)
{
	if (mwply == NULL || mwply->used != 1) {
		MWSFSVM_Error(mwsfsvr_msg_bdrhndl);
		return FALSE;
	}
	mwsfd_svr_bdr_cnt++;
	return mwply->sleep_bdr == 1;
}

static void mwsfd_SetSleepBdr(MWPLY mwply, Sint32 sw)
{
	MWSFD_LIBWORK *lw;

	lw = MWSFLIB_GetLibWorkPtr();
	mwply->sleep_bdr = sw;
	lw->svr_bdr = sw;
}

static void mwsfd_ClrSleepBdr(MWPLY mwply)
{
	MWSFD_LIBWORK *lw;

	lw = MWSFLIB_GetLibWorkPtr();
	mwply->sleep_bdr = 0;
	lw->svr_bdr = 0;
}

/* COMPILER-DIFF: M1 - two zero copies `mr r30, r28; mr r31, r28` for the inlined ClrSleepBdr's stores (every C form gives one `li`). Asm function (the original's instructions verbatim). */
#if 1 // COMPILER-DIFF: M1
asm void mwlSfdSleepDecSvr(MWPLY mwply)
{
	nofralloc
	stwu r1, -32(r1)
	mflr r0
	stw r0, 36(r1)
	stmw r27, 12(r1)
	mr r27, r3
	bl mwPlySaveRsc
	bl MWSFLIB_GetLibWorkPtr
	li r0, 1
	stw r0, 96(r27)
	stw r0, 36(r3)
	bl MWSFSVM_GotoIdleBorder
	bl MWSFLIB_GetLibWorkPtr
	li r0, 0
	stw r0, 96(r27)
	stw r0, 36(r3)
	bl mwPlyRestoreRsc
	lwz r0, 100(r27)
	cmpwi r0, 1
	bne L90
	li r28, 0
	li r29, 1
	mr r30, r28
	mr r31, r28
L5c:
	bl MWSFLIB_GetLibWorkPtr
	stw r29, 96(r27)
	stw r29, 36(r3)
	bl ADXM_WaitVsync
	bl MWSFLIB_GetLibWorkPtr
	stw r30, 96(r27)
	stw r31, 36(r3)
	lwz r0, 100(r27)
	cmpwi r0, 0
	beq L90
	addi r28, r28, 1
	cmpwi r28, 10
	blt L5c
L90:
	lmw r27, 12(r1)
	lwz r0, 36(r1)
	mtlr r0
	addi r1, r1, 32
	blr
}
#else
void mwlSfdSleepDecSvr(MWPLY mwply)
{
	Sint32 i;

	mwPlySaveRsc();
	mwsfd_SetSleepBdr(mwply, 1);
	MWSFSVM_GotoIdleBorder();
	mwsfd_ClrSleepBdr(mwply);
	mwPlyRestoreRsc();
	if (mwply->mwply_svr_flg == 1) {
		for (i = 0; i < 10; i++) {
			mwsfd_SetSleepBdr(mwply, 1);
			ADXM_WaitVsync();
			mwsfd_ClrSleepBdr(mwply);
			if (mwply->mwply_svr_flg == 0) {
				break;
			}
		}
	}
}
#endif

void MWSFSVR_SetHnSfdSvrFlg(MWPLY mwply, Sint32 flg)
{
	mwply->sfd_svr_flg = flg;
}

void MWSFSVR_SetHnMwplySvrFlg(MWPLY mwply, Sint32 flg)
{
	mwply->mwply_svr_flg = flg;
}

void MWSFSVR_SetMwsfdSvrFlg(Sint32 flg)
{
	MWSFD_LIBWORK *lw;

	lw = MWSFLIB_GetLibWorkPtr();
	lw->svr_flg = flg;
}

/* OPEN: our 2.4.7 auto-inlines this 0xB8-byte helper and MWSFSVR_DecodeServer, the original did not */
#pragma dont_inline on
/* COMPILER-DIFF: M3 - the original did not inline this helper while inlining smaller ones (auto-inlining decision). Asm function (the original's instructions verbatim). */
#if 1 // COMPILER-DIFF: M3
asm static Sint32 mwsfd_ExecSvrHndl(MWPLY mwply)
{
	nofralloc
	stwu r1, -16(r1)
	mflr r0
	li r5, 1
	stw r0, 20(r1)
	stw r31, 12(r1)
	mr r31, r3
	stw r30, 8(r1)
	lwz r30, 64(r3)
	stw r5, 100(r3)
	lwz r0, 4(r3)
	cmpwi r0, 1
	beq L124
	li r0, 0
	li r3, 0
	stw r0, 100(r31)
	b L184
L124:
	lis r4, mwsfd_hn_last@ha
	mr r3, r30
	stw r31, mwsfd_hn_last@l(r4)
	stw r5, 104(r31)
	bl SFD_ExecOne
	li r3, 0
	stw r3, 104(r31)
	lwz r0, 8(r31)
	cmpwi r0, 0
	bne L154
	stw r3, 108(r31)
	b L164
L154:
	li r0, 1
	mr r3, r31
	stw r0, 108(r31)
	bl mwSfdExecDecSvrHndl
L164:
	li r0, 0
	mr r3, r30
	stw r0, 100(r31)
	bl SFD_IsHnSvrWait
	subfic r4, r3, 1
	addi r0, r3, -1
	or r0, r4, r0
	srwi r3, r0, 31
L184:
	lwz r0, 20(r1)
	lwz r31, 12(r1)
	lwz r30, 8(r1)
	mtlr r0
	addi r1, r1, 16
	blr
}
#else
static Sint32 mwsfd_ExecSvrHndl(MWPLY mwply)
{
	void *sfd;

	sfd = mwply->sfd;
	mwply->mwply_svr_flg = 1;
	if (mwply->used != 1) {
		mwply->mwply_svr_flg = 0;
		return 0;
	}
	mwsfd_hn_last = mwply;
	mwply->sfd_svr_flg = 1;
	SFD_ExecOne(sfd);
	mwply->sfd_svr_flg = 0;
	if (mwply->stat == MWSFD_STAT_STOP) {
		mwply->dec_svr_flg = 0;
	} else {
		mwply->dec_svr_flg = 1;
		mwSfdExecDecSvrHndl(mwply);
	}
	mwply->mwply_svr_flg = 0;
	return SFD_IsHnSvrWait(sfd) != 1;
}
#endif
#pragma dont_inline off

Sint32 mwSfdExecSvrHndl(MWPLY mwply)
{
	MWSFD_LIBWORK *lw;

	if (mwsfd_init_flag != 1) {
		return 0;
	}
	if (mwply == NULL) {
		MWSFSVM_Error("E1071901 mwPlyExecSvrHndl: NULL handle.");
		return 0;
	}
	if (mwply->used != 1) {
		return 0;
	}
	if (mwply->mwply_svr_flg == 1) {
		return 0;
	}
	lw = MWSFLIB_GetLibWorkPtr();
	if (lw->svr_bdr == 1) {
		return 0;
	}
	return mwsfd_ExecSvrHndl(mwply);
}

#pragma dont_inline on
Sint32 MWSFSVR_DecodeServer(void *obj)
{
	MWSFD_LIBWORK *lw;
	MWSFD_LIBWORK *lw2;
	Sint32 i;
	Sint32 wait;
	MWPLY mwply;
	Sint32 (*func)(void *obj);
	void *fobj;

	if (mwsfd_init_flag != 1) {
		return 0;
	}
	lw = MWSFLIB_GetLibWorkPtr();
	if (MWSFSVM_TestAndSet(&lw->svr_flg) == 0) {
		return 0;
	}
	lw2 = MWSFLIB_GetLibWorkPtr();
	func = lw2->pre_func;
	fobj = lw2->pre_obj;
	if (func != NULL) {
		func(fobj);
	}
	for (i = 0; i < MWSFD_MAX_HN; i++) {
		mwply = &lw->hn[i];
		if (mwply != NULL) {
			mwSfdExecSvrHndl(mwply);
		}
	}
	lw2 = MWSFLIB_GetLibWorkPtr();
	lw2->svr_flg = 0;
	wait = SFD_IsSvrWait() != 1;
	lw2 = MWSFLIB_GetLibWorkPtr();
	func = lw2->post_func;
	fobj = lw2->post_obj;
	if (func != NULL) {
		func(fobj);
	}
	if (wait == 0) {
		lw2 = MWSFLIB_GetLibWorkPtr();
		if (lw2->svr_bdr != 1) {
			lw2 = MWSFLIB_GetLibWorkPtr();
			func = lw2->idle_func;
			fobj = lw2->idle_obj;
			if (func != NULL) {
				func(fobj);
			}
		}
	}
	return wait;
}
#pragma dont_inline off

void mwSfdVsync(void)
{
	MWSFD_LIBWORK *lw;

	mwg_field_no++;
	mwg_vcnt++;
	if (mwsfd_init_flag != 1) {
		return;
	}
	lw = MWSFLIB_GetLibWorkPtr();
	if (MWSFSVM_TestAndSet(&lw->x5c) == 0) {
		return;
	}
	if (mwsfd_init_flag == 1) {
		SFD_VbIn();
	}
	lw->x5c = 0;
}

Sint32 MWSFSVR_IdleThrdProc(void *obj)
{
	Sint32 ret = 0;
	MWSFD_LIBWORK *lw;

	if (ADXM_IsSetupThrd() == 1) {
		lw = MWSFLIB_GetLibWorkPtr();
		if (lw->x10 != 1) {
			ret = MWSFSVR_DecodeServer(obj);
		}
	}
	return ret;
}

Sint32 MWSFSVR_MainThrdProc(void *obj)
{
	Sint32 ret = 0;
	MWSFD_LIBWORK *lw;

	if (ADXM_IsSetupThrd() == 1) {
		lw = MWSFLIB_GetLibWorkPtr();
		if (lw->x10 == 1) {
			ret = MWSFSVR_DecodeServer(obj);
		}
	} else {
		mwSfdVsync();
		ret = MWSFSVR_DecodeServer(obj);
	}
	return ret;
}

Sint32 MWSFSVR_VsyncThrdProc(void *obj)
{
	if (ADXM_IsSetupThrd() == 1) {
		mwSfdVsync();
		return 0;
	}
	return 0;
}

static Sint32 mwsfd_StartStm(MWPLY mwply)
{
	if (MWSTM_GetStat(mwply->stm) == ADXSTM_STAT_EXEC) {
		return -1;
	}
	if (mwply->sji != NULL) {
		SJ_Reset(mwply->sji);
	}
	MWSTM_SetFileRange(mwply->stm, mwply->fname, mwply->dir, mwply->ofst, mwply->nsct);
	if (MWSTM_ReqStart(mwply->stm) == -1) {
		mwply->stat = MWSFD_STAT_ERROR;
		MWSFLIB_SetErrCode(-102);
		MWSFSVM_Error("E211141 MWSTM_ReqStart: can't start '%s'", mwply->fname);
		mwply->stm_start_req = 0;
		return -1;
	}
	MWSFCRE_SetSupplySj(mwply);
	return 1;
}

static void mwsfd_StartPlay(MWPLY mwply)
{
	mwPlySfdStart(mwply);
	if (mwply->pause_flg == 0) {
		mwSfdPause(mwply, 0);
	}
	if (mwply->linkstm == 1) {
		if (SFD_SetConcatPlay(mwply->sfd) != 0) {
			MWSFSVM_Error("E99072103 mwPlyStartXX: can't link stream");
		}
	}
}

/* COMPILER-DIFF: M1 - the pool base `lis r4` above the prologue stores (M1); the string pool is addressed through mwsfsvr_msg_bdrhndl. Asm function (the original's instructions verbatim). */
asm Sint32 mwSfdExecDecSvrHndl(MWPLY mwply) // COMPILER-DIFF: M1
{
	nofralloc
	stwu r1, -32(r1)
	mflr r0
	lis r4, mwsfsvr_msg_bdrhndl@ha
	stw r0, 36(r1)
	stmw r27, 12(r1)
	mr r29, r3
	addi r31, r4, mwsfsvr_msg_bdrhndl@l
	lwz r0, 8(r3)
	cmpwi r0, 2
	beq L800
	bge L864
	cmpwi r0, 0
	beq L864
	bge L60c
	b L864
	b L864
L60c:
	lwz r0, 448(r29)
	lwz r30, 64(r29)
	cmpwi r0, 1
	bne L6c4
	lwz r3, 68(r29)
	bl MWSTM_GetStat
	cmpwi r3, 2
	bne L634
	li r3, -1
	b L6b4
L634:
	lwz r3, 464(r29)
	cmplwi r3, 0
	beq L650
	lwz r4, 0(r3)
	lwz r12, 20(r4)
	mtctr r12
	bctrl
L650:
	lwz r3, 68(r29)
	lwz r4, 440(r29)
	lwz r5, 452(r29)
	lwz r6, 456(r29)
	lwz r7, 460(r29)
	bl MWSTM_SetFileRange
	lwz r3, 68(r29)
	bl MWSTM_ReqStart
	cmpwi r3, -1
	bne L6a8
	li r0, 4
	li r3, -102
	stw r0, 8(r29)
	bl MWSFLIB_SetErrCode
	lwz r4, 440(r29)
	addi r3, r31, 92
	crclr 4*cr1+eq
	bl MWSFSVM_Error
	li r0, 0
	li r3, -1
	stw r0, 448(r29)
	b L6b4
L6a8:
	mr r3, r29
	bl MWSFCRE_SetSupplySj
	li r3, 1
L6b4:
	cmpwi r3, 1
	bne L6c4
	li r0, 0
	stw r0, 448(r29)
L6c4:
	lwz r0, 660(r29)
	cmpwi r0, 1
	bne L77c
	lwz r3, 64(r29)
	addi r27, r29, 660
	bl SFD_GetHnStat
	mr r28, r3
	mr r3, r27
	bl MWSST_GetStat
	cmpwi r28, 3
	bne L7d4
	cmpwi r3, 2
	beq L718
	lwz r3, 12(r27)
	li r4, 1
	lwz r5, 0(r3)
	lwz r12, 36(r5)
	mtctr r12
	bctrl
	cmpwi r3, 0
	bne L7d4
L718:
	mr r3, r29
	bl mwPlySfdStart
	lbz r0, 118(r29)
	extsb. r0, r0
	bne L738
	mr r3, r29
	li r4, 0
	bl mwSfdPause
L738:
	lbz r0, 116(r29)
	cmpwi r0, 1
	bne L760
	lwz r3, 64(r29)
	bl SFD_SetConcatPlay
	cmpwi r3, 0
	beq L760
	addi r3, r31, 136
	crclr 4*cr1+eq
	bl MWSFSVM_Error
L760:
	lbz r0, 118(r29)
	extsb. r0, r0
	bne L7d4
	mr r3, r27
	li r4, 0
	bl MWSST_Pause
	b L7d4
L77c:
	lwz r3, 64(r29)
	bl SFD_GetHnStat
	cmpwi r3, 3
	bne L7d4
	mr r3, r29
	bl mwPlySfdStart
	lbz r0, 118(r29)
	extsb. r0, r0
	bne L7ac
	mr r3, r29
	li r4, 0
	bl mwSfdPause
L7ac:
	lbz r0, 116(r29)
	cmpwi r0, 1
	bne L7d4
	lwz r3, 64(r29)
	bl SFD_SetConcatPlay
	cmpwi r3, 0
	beq L7d4
	addi r3, r31, 136
	crclr 4*cr1+eq
	bl MWSFSVM_Error
L7d4:
	mr r3, r30
	bl SFD_GetHnStat
	cmpwi r3, 4
	beq L7ec
	cmpwi r3, 6
	bne L864
L7ec:
	li r0, 2
	mr r3, r29
	stw r0, 8(r29)
	bl MWSFSFX_DecideCompoMode
	b L864
L800:
	lbz r0, 117(r29)
	lwz r28, 64(r29)
	cmpwi r0, 1
	bne L848
	lwz r3, 76(r29)
	bl LSC_GetNumStm
	cmpwi r3, 0
	bne L84c
	mr r3, r28
	bl SFD_TermSupply
	cmpwi r3, 0
	beq L83c
	addi r3, r31, 180
	crclr 4*cr1+eq
	bl MWSFSVM_Error
L83c:
	li r0, 0
	stb r0, 117(r29)
	b L84c
L848:
	bl mwPlyChkSupply
L84c:
	mr r3, r28
	bl SFD_GetHnStat
	cmpwi r3, 6
	bne L864
	li r0, 3
	stw r0, 8(r29)
L864:
	lwz r3, 68(r29)
	cmplwi r3, 0
	beq L884
	bl MWSTM_IsFsStatErr
	cmpwi r3, 0
	beq L884
	li r0, 4
	stw r0, 8(r29)
L884:
	lwz r3, 76(r29)
	cmplwi r3, 0
	beq L8a4
	bl MWSFLSC_IsFsStatErr
	cmpwi r3, 1
	bne L8a4
	li r0, 4
	stw r0, 8(r29)
L8a4:
	mr r3, r29
	bl MWSFSEE_ChkSupply
	lmw r27, 12(r1)
	li r3, 0
	lwz r0, 36(r1)
	mtlr r0
	addi r1, r1, 32
	blr
}
/* the C body, kept compiled (dead, stripped by strip_unused) so that its literals stay in .rodata */
Sint32 mwSfdExecDecSvrHndl_c(MWPLY mwply)
{
	switch (mwply->stat) {
	case MWSFD_STAT_STOP:
		break;
	case MWSFD_STAT_PREP: {
		void *sfd;
		Sint32 sfdstat;
		Sint32 sststat;
		MWSST sst;

		sfd = mwply->sfd;
		if (mwply->stm_start_req == 1) {
			if (mwsfd_StartStm(mwply) == 1) {
				mwply->stm_start_req = 0;
			}
		}
		if (mwply->sst.used == 1) {
			sst = &mwply->sst;
			sfdstat = SFD_GetHnStat(mwply->sfd);
			sststat = MWSST_GetStat(sst);
			if (sfdstat == 3 && (sststat == 2 || SJ_GetNumData(sst->sj, SJ_CK_DATA) == 0)) {
				mwsfd_StartPlay(mwply);
				if (mwply->pause_flg == 0) {
					MWSST_Pause(sst, 0);
				}
			}
		} else {
			if (SFD_GetHnStat(mwply->sfd) == 3) {
				mwsfd_StartPlay(mwply);
			}
		}
		sfdstat = SFD_GetHnStat(sfd);
		if (sfdstat == 4 || sfdstat == 6) {
			mwply->stat = MWSFD_STAT_PLAYING;
			MWSFSFX_DecideCompoMode(mwply);
		}
		break;
	}
	case MWSFD_STAT_PLAYING: {
		void *sfd;

		sfd = mwply->sfd;
		if (mwply->linkstm_req == 1) {
			if (LSC_GetNumStm(mwply->lsc) == 0) {
				if (SFD_TermSupply(sfd) != 0) {
					MWSFSVM_Error("E99072102 mwlSfdExecDecSvrPlaying: can't term");
				}
				mwply->linkstm_req = 0;
			}
		} else {
			mwPlyChkSupply();
		}
		if (SFD_GetHnStat(sfd) == 6) {
			mwply->stat = MWSFD_STAT_PLAYEND;
		}
		break;
	}
	case MWSFD_STAT_PLAYEND:
		break;
	}
	if (mwply->stm != NULL && MWSTM_IsFsStatErr(mwply->stm) != 0) {
		mwply->stat = MWSFD_STAT_ERROR;
	}
	if (mwply->lsc != NULL && MWSFLSC_IsFsStatErr(mwply->lsc) == 1) {
		mwply->stat = MWSFD_STAT_ERROR;
	}
	MWSFSEE_ChkSupply(mwply);
	return 0;
}
