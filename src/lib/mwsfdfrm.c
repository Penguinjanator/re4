/* CRI Sofdec MW player: decoded frames (mwsfdfrm.c). Sofdec header analysis through the SFD header
 * callback, frame information conversion and mwPlyGetCurFrm / mwPlyRelCurFrm. */
#include "cri_xpt.h"
#include "mwsfd.h"
#include <string.h>

typedef struct SFH_OBJ *SFH;

/* picture user data attached to an SFD frame */
typedef struct {
	void *ptr;                 /* 0x00 */
	Sint32 len;                /* 0x04 */
} MWSFFRM_USRDAT;

/* decoded frame as the SFD library hands it out (SFD_VFRM: the 0x80-byte information block) */
typedef struct {
	Sint32 x00;                /* 0x00 -> MWS_FRM.width */
	Sint32 x04;                /* 0x04 -> MWS_FRM.height */
	Sint32 x08;
	Sint32 x0c;
	Sint32 pstruct;            /* 0x10 */
	Sint32 x14;
	Sint32 tunit;              /* 0x18 */
	Sint32 ptype;              /* 0x1C */
	void *bufadr;              /* 0x20 */
	Sint32 x24;
	Sint32 x28;
	Sint32 frmno;              /* 0x2C */
	Sint32 x30;
	Sint32 x34;
	MWSFFRM_USRDAT *usr;       /* 0x38 */
	Sint32 chromapos_h;        /* 0x3C */
	Sint32 chromapos_v;        /* 0x40 */
	Sint32 x44;
	Sint32 x48;                /* 0x48 start of the block copied into MWS_FRM.ext */
	Sint32 x4c;
	Sint32 x50;
	Sint32 x54;
	Sint32 pic_struct;         /* 0x58 */
	Sint32 chroma_format;      /* 0x5C */
	Sint32 x60;
	Sint32 x64;
	Sint32 x68;
	Sint8 x6c;                 /* 0x6C */
	Sint8 x6d;
	Sint8 x6e;
	Sint8 x6f;
	Sint32 x70;
	Sint32 x74;
	Sint32 x78;
	Sint32 x7c;
} MWSFFRM_VFRM;

/* SFD_CalcYccPlane output */
typedef struct {
	void *cb;                  /* 0x00 */
	void *cr;                  /* 0x04 */
	void *y;                   /* 0x08 */
	Sint16 cwidth;             /* 0x0C */
	Sint16 ywidth;             /* 0x0E */
} MWSFFRM_YCCPLN;

/* mwPlyCalcYccPlane output (CFT_YCC420PLN) */
typedef struct {
	void *y;
	void *cb;
	void *cr;
	Sint32 ywidth;
	Sint32 cbwidth;
	Sint32 crwidth;
} MWSFFRM_YCC420PLN;

/* MWSFFRM_AnalyTotalFrmNum output */
typedef struct {
	Uint8 pad0[0x14];
	Sint32 maxfrm;             /* 0x14 */
	Sint32 fxtype;             /* 0x18 */
	Sint32 numelem_vid;        /* 0x1C */
	Sint32 numelem_aud;        /* 0x20 */
} MWSFFRM_TOTINF;

#define MWSFFRM_SFH_SIZE 0x800
#define MWSFFRM_VID_STMID 0xE0

extern SFH SFH_Create(void *data, Sint32 size);
extern void SFH_Destroy(SFH sfh);
extern Sint32 SFH_IsSfdHeader(SFH sfh, Sint32 *result);
extern Sint32 SFH_IsExistStmId(SFH sfh, Sint32 stmid, Sint32 *result);
extern Sint32 SFH_AnlyFtrColType(SFH sfh, Sint32 stmid, Sint32 *coltype);
extern Sint32 SFH_AnlyFtrFxType(SFH sfh, Sint32 stmid, Sint32 *fxtype);
extern Sint32 SFH_AnlyMaxFrmNum(SFH sfh, Sint32 *num);
extern Sint32 SFH_AnlyNumElemVid(SFH sfh, Sint32 *num);
extern Sint32 SFH_AnlyNumElemAud(SFH sfh, Sint32 *num);
extern Sint32 SFD_SetCond(void *sfd, Sint32 id, Sint32 val);
extern Sint32 SFD_GetFrm(void *sfd, MWSFFRM_VFRM **frm);
extern Sint32 SFD_RelFrm(void *sfd, MWSFFRM_VFRM *frm);
extern Sint32 SFD_IsNextFrmReady(void *sfd);
extern Sint32 SFD_GetFps(void *sfd, Sint32 *fps);
extern void SFD_CalcYccPlane(void *buf, Sint32 width, Sint32 height, MWSFFRM_YCCPLN *pln);
extern Sint32 UTY_MulDiv(Sint32 a, Sint32 b, Sint32 c);
extern Sint32 MWSFD_GetUsePicUsr(void);
extern Sint32 MWSFD_IsFrmDivField(MWPLY mwply);
extern void MWSFD_SetColAdj(MWPLY mwply, Sint32 sw);
extern void MWSFSFX_SetColAdj(MWPLY mwply, Sint32 sw);
extern void MWSFSFX_SetFxType(MWPLY mwply, Sint32 fxtype);
extern void MWSFSFX_SetPicUsrDat(MWPLY mwply, void *dat, Sint32 len);
extern Sint32 MWSFSFX_IsFrmCcs(MWPLY mwply);
extern void MWSFTAG_UpdateTagInf(MWPLY mwply);
extern void SUD_SearchSudDat(void *dat, Sint32 len, void **found, Sint32 *flen);

/* dead */
Sint32 mwPlyGetNumRemainFrm(MWPLY mwply)
{
	if (MWSFD_IsEnableHndl(mwply) == 0) {
		MWSFSVM_Error("E409171: mwPlyGetNumRemainFrm: handle is invalid.");
		return 0;
	}
	return mwply->ngetfrm - mwply->nrelfrm;
}

/* SFX component layout of the video stream from the Sofdec header's feature information. A macro:
 * every use gets its own jump table (three in the original), a static helper would add a fourth
 * for its own body. */
#define MWSFFRM_CNV_FXTYPE(sfh, fxtype) \
	{ \
		Sint32 type; \
		if (SFH_AnlyFtrFxType(sfh, MWSFFRM_VID_STMID, &type) == 0) { \
			fxtype = SFX_COMPO_YCC420PLN; \
		} else { \
			switch (type) { \
			case 1: \
				fxtype = SFX_COMPO_YCC420PLN_UPHALF; \
				break; \
			case 3: \
				fxtype = SFX_COMPO_0x51; \
				break; \
			case 6: \
				fxtype = SFX_COMPO_0x61; \
				break; \
			case 0: \
			case 2: \
			case 4: \
			case 5: \
			case 7: \
			case 8: \
			default: \
				fxtype = SFX_COMPO_YCC420PLN; \
				break; \
			} \
		} \
	}

/* SFD header callback (cond 0x4B / 0x4C): record what the Sofdec header says about the video.
 * M1: the original keeps sfh in r31 and mwply in r30, ours the reverse; instruction stream identical. */
static void mwsffrm_AnalySofdecHeader(MWPLY mwply, void *data, Uint32 size)
{
	SFH sfh;
	Sint32 ccs;
	Sint32 maxfrm;
	Sint32 fxtype;
	Sint32 issfd;
	Sint32 coltype;
	Sint32 exist;
	Sint32 num;
	Sint32 wr;

	mwply->sfh_cnt++;
	if (size < MWSFFRM_SFH_SIZE || data == NULL) {
		return;
	}
	sfh = SFH_Create(data, size);
	if (sfh == NULL) {
		return;
	}
	if (SFH_IsSfdHeader(sfh, &issfd) == 0 || issfd == 0) {
		SFH_Destroy(sfh);
		return;
	}
	ccs = 0;
	if (SFH_IsExistStmId(sfh, MWSFFRM_VID_STMID, &exist) == 0 || exist == 0) {
		ccs = 0;
	} else if (SFH_AnlyFtrColType(sfh, MWSFFRM_VID_STMID, &coltype) == 0) {
		ccs = 0;
	} else if (coltype == 3) {
		ccs = 1;
	}
	if (SFH_AnlyMaxFrmNum(sfh, &num) == 0) {
		maxfrm = -1;
	} else {
		maxfrm = num;
	}
	MWSFFRM_CNV_FXTYPE(sfh, fxtype);
	wr = mwply->sfh_wr;
	mwply->sfhinf[wr].no = mwply->sfh_cnt - 1;
	mwply->sfhinf[wr].ccs = ccs;
	mwply->sfhinf[wr].maxfrm = maxfrm;
	mwply->sfhinf[wr].fxtype = fxtype;
	mwply->sfhinf[wr].valid = 1;
	mwply->sfh_wr++;
	mwply->sfh_wr = mwply->sfh_wr % MWSFFRM_SFHINF_NUM;
	SFH_Destroy(sfh);
}

void MWSFFRM_SetShfCbFn(MWPLY mwply)
{
	void *sfd = mwply->sfd;

	SFD_SetCond(sfd, 0x4B, (Sint32)mwsffrm_AnalySofdecHeader);
	SFD_SetCond(sfd, 0x4C, (Sint32)mwply);
}

void MWSFFRM_InitSfhInfTable(MWPLY mwply)
{
	Sint32 i;

	mwply->sfh_cnt = 0;
	mwply->sfh_cur = 0;
	mwply->sfh_wr = 0;
	for (i = 0; i < MWSFFRM_SFHINF_NUM; i++) {
		mwply->sfhinf[i].valid = 0;
		mwply->sfhinf[i].no = 0;
		mwply->sfhinf[i].ccs = 0;
		mwply->sfhinf[i].maxfrm = 0;
		mwply->sfhinf[i].fxtype = SFX_COMPO_YCC420PLN;
	}
	MWSFSFX_SetColAdj(mwply, 0);
}

Sint32 mwsffrm_AnalyFxType(SFH sfh)
{
	Sint32 fxtype;

	MWSFFRM_CNV_FXTYPE(sfh, fxtype);
	return fxtype;
}

/* header information of the current frame */
#define MWSFFRM_CUR_SFHINF_IDX(mwply) ((mwply)->sfh_cur % MWSFFRM_SFHINF_NUM)

static Sint32 mwsffrm_GetCurFxType(MWPLY mwply)
{
	Sint32 idx = MWSFFRM_CUR_SFHINF_IDX(mwply);
	Sint32 fxtype;

	if (mwply->sfhinf[idx].valid == 0) {
		fxtype = SFX_COMPO_YCC420PLN;
	} else {
		fxtype = mwply->sfhinf[idx].fxtype;
	}
	return fxtype;
}

static Bool mwsffrm_IsCurCcs(MWPLY mwply)
{
	Sint32 idx = MWSFFRM_CUR_SFHINF_IDX(mwply);
	Bool ccs;

	if (mwply->sfhinf[idx].valid == 0) {
		ccs = 0;
	} else {
		ccs = mwply->sfhinf[idx].ccs;
	}
	return ccs;
}

Sint32 mwPlyGetFxType(MWPLY mwply)
{
	Sint32 fxtype = mwsffrm_GetCurFxType(mwply);

	if (fxtype == SFX_COMPO_0x51 || fxtype == SFX_COMPO_0x61) {
		fxtype = SFX_COMPO_0x41;
	}
	return fxtype;
}

/* look for the Sofdec header in the second and third sectors of the file head.
 * M1: callee-saved numbering (original inf r31, -1 r30, sfh r29, i r28, size r27, data r26). */
void MWSFFRM_AnalyTotalFrmNum(Uint8 *data, Uint32 size, MWSFFRM_TOTINF *inf)
{
	SFH sfh;
	Sint32 i;
	Sint32 issfd;
	Sint32 nmax;
	Sint32 fxtype;
	Sint32 nvid;
	Sint32 naud;
	Sint32 ofst;

	inf->maxfrm = -1;
	if (size < MWSFFRM_SFH_SIZE || data == NULL) {
		return;
	}
	for (i = 2; i <= 3; i++) {
		ofst = (i - 1) * MWSFFRM_SFH_SIZE;
		sfh = SFH_Create(data + ofst, size - ofst);
		if (sfh == NULL) {
			continue;
		}
		if (SFH_IsSfdHeader(sfh, &issfd) == 0 || issfd == 0) {
			inf->maxfrm = -1;
			inf->fxtype = -1;
			SFH_Destroy(sfh);
			continue;
		}
		inf->maxfrm = (SFH_AnlyMaxFrmNum(sfh, &nmax) == 0) ? -1 : nmax;
		MWSFFRM_CNV_FXTYPE(sfh, fxtype);
		inf->fxtype = fxtype;
		inf->numelem_vid = (SFH_AnlyNumElemVid(sfh, &nvid) == 0) ? -1 : nvid;
		inf->numelem_aud = (SFH_AnlyNumElemAud(sfh, &naud) == 0) ? -1 : naud;
		SFH_Destroy(sfh);
		return;
	}
}

Sint32 mwPlyGetNumSkipDisp(MWPLY mwply)
{
	if (MWSFD_IsEnableHndl(mwply) == 0) {
		MWSFSVM_Error("E202231: mwPlyGetNumSkipDisp: handle is invalid.");
		return 0;
	}
	return mwply->nskipdisp;
}

/* dead */
void *mwPlyGetNextPicUsr(MWPLY mwply, Sint32 *len)
{
	if (MWSFD_IsEnableHndl(mwply) == 0) {
		MWSFSVM_Error("E3122201: mwPlyGetNextPicUsr: handle is invalid.");
		return NULL;
	}
	*len = mwply->picusr_len;
	return mwply->picusr_dat;
}

/* inlined into mwPlyGetCurFrm, standalone dead-stripped */
Sint32 mwPlyIsNextFrmReady(MWPLY mwply)
{
	if (MWSFD_IsEnableHndl(mwply) == 0) {
		MWSFSVM_Error("E1122618: mwPlyIsNextFrmReady: handle is invalid.");
		return 0;
	}
	return SFD_IsNextFrmReady(mwPlyGetSfdHn(mwply));
}

void mwPlyRelCurFrm(MWPLY mwply)
{
	void *frm;
	Sint32 ngetfrm;
	Sint32 nrelfrm;
	void *sfd;

	if (MWSFD_IsEnableHndl(mwply) == 0) {
		MWSFSVM_Error("E1122615: mwPlyRelCurFrm: handle is invalid.");
		return;
	}
	frm = mwply->curfrm;
	ngetfrm = mwply->ngetfrm;
	nrelfrm = mwply->nrelfrm;
	sfd = mwPlyGetSfdHn(mwply);
	if (ngetfrm > nrelfrm) {
		SFD_RelFrm(sfd, frm);
		mwply->nrelfrm++;
		mwply->ngetfrm = mwply->nrelfrm;
	}
}

/* dead */
Sint32 mwl_convPtypeToSFD(Sint32 ptype)
{
	Sint32 ret;

	switch (ptype) {
	case 1:
		ret = 1;
		break;
	case 2:
		ret = 2;
		break;
	case 3:
		ret = 3;
		break;
	default:
		MWSFSVM_Error("mwl_convPtypeToSFD : Invalid Ptype");
		ret = 3;
		break;
	}
	return ret;
}

void mwPlyCalcYccPlane(void *buf, Sint32 width, Sint32 height, MWSFFRM_YCC420PLN *out)
{
	MWSFFRM_YCCPLN pln;

	SFD_CalcYccPlane(buf, width, height, &pln);
	out->y = pln.y;
	out->cb = pln.cb;
	out->cr = pln.cr;
	out->ywidth = pln.ywidth;
	out->cbwidth = pln.cwidth;
	out->crwidth = pln.cwidth;
}

/* frame type from the picture structure: 0 frame, 2 field pair (out-parameter: inlined, the
 * caller's variable is assigned directly; a returned value would be copied with a `mr`) */
static void mwsffrm_DecideFrmType(MWSFFRM_VFRM *vfrm, Sint32 *ftype)
{
	*ftype = 0;
	switch (vfrm->pic_struct) {
	case 3:
		if (vfrm->x6c == 0) {
			*ftype = 2;
		}
		break;
	case 1:
	case 2:
		*ftype = 2;
		break;
	case 0:
	default:
		MWSFSVM_Error("E301271: mwsffrm_DecideFrmType() : Invalid Pstruct");
		break;
	}
}

static Sint32 mwl_convPtypeFromSFD(Sint32 ptype)
{
	Sint32 ret;

	switch (ptype) {
	case 1:
		ret = 1;
		break;
	case 2:
		ret = 2;
		break;
	case 3:
		ret = 3;
		break;
	case 4:
		ret = 4;
		break;
	default:
		MWSFSVM_Error("mwl_convPtypeFromSFD : Invalid Ptype");
		ret = 1;
		break;
	}
	return ret;
}

static Sint32 mwl_convFtypeFromSFD(Sint32 ftype)
{
	Sint32 ret;

	switch (ftype) {
	case 1:
		ret = 1;
		break;
	case 2:
		ret = 2;
		break;
	case 3:
		ret = 3;
		break;
	default:
		ret = 3;
		break;
	}
	return ret;
}

/* M1: the original ranks the ten frame-field locals above the parameters (r31..r22, then frm r21,
 * vfrm r20, mwply r19, time r18, ftype r17, pstruct r16, ...); instruction stream identical. */
void mwl_convFrmInfFromSFD(MWPLY mwply, MWSFFRM_VFRM *vfrm, MWS_FRM *frm)
{
	Sint32 x00;
	Sint32 x04;
	Sint32 x08;
	Sint32 x0c;
	Sint32 x34;
	Sint32 tunit;
	Sint32 x30;
	Sint32 x24;
	Sint32 x28;
	Sint32 frmno;
	Sint32 time;
	Sint32 ftype;
	Sint32 pstruct;
	void *sfd;
	Sint32 scale;
	void *bufadr;
	Sint32 fps;
	Sint32 time2;
	void *usrptr;
	Sint32 usrlen;
	MWSFFRM_USRDAT *usr;
	void *noptr;
	void *pptr;

	sfd = mwply->sfd;
	bufadr = vfrm->bufadr;
	ftype = mwl_convFtypeFromSFD(vfrm->ptype);
	x00 = vfrm->x00;
	x04 = vfrm->x04;
	x08 = vfrm->x08;
	x0c = vfrm->x0c;
	pstruct = mwl_convPtypeFromSFD(vfrm->pstruct);
	x34 = vfrm->x34;
	tunit = vfrm->tunit;
	x30 = vfrm->x30;
	frmno = vfrm->frmno;
	x24 = vfrm->x24;
	x28 = vfrm->x28;
	if (SFD_GetFps(sfd, &fps) != 0) {
		MWSFSVM_Error("E201301: MWSFD: GetFps failed.");
	}
	scale = tunit * 1000;
	time = UTY_MulDiv(x34, fps, scale);
	time2 = UTY_MulDiv(x30, fps, scale);
	frm->bufadr = bufadr;
	noptr = &mwply->x164;
	frm->fmt = ftype;
	frm->width = x00;
	frm->height = x04;
	frm->x10 = x08;
	frm->x14 = x0c;
	frm->pstruct = pstruct;
	frm->fps = fps;
	frm->time = time;
	frm->x24 = x34;
	frm->tunit = tunit;
	frm->frmno = frmno;
	frm->tblsrc = (void *)time2;
	frm->x34 = x30;
	frm->x38 = x24;
	frm->x3c = x28;
	usr = vfrm->usr;
	pptr = mwply->picusr_ptr;
	usrptr = usr->ptr;
	usrlen = usr->len;
	if (MWSFD_GetUsePicUsr() != 1) {
		frm->usrdat = NULL;
		frm->usrlen = 0;
	} else if (pptr == noptr) {
		frm->usrdat = NULL;
		frm->usrlen = 0;
	} else {
		if (usrptr != NULL && usrlen > 4) {
			usrptr = (Uint8 *)usrptr + 4;
			usrlen -= 4;
		}
		frm->usrdat = usrptr;
		frm->usrlen = usrlen;
	}
	memcpy(frm->ext, &vfrm->x48, sizeof(frm->ext));
}

static Bool mwsffrm_IsPicUsrDat(MWPLY mwply)
{
	return mwply->picusr_dat != NULL;
}

/* M1: original mwply r30 / frm r29, ours the reverse; instruction stream identical. */
void mwPlyGetCurFrm(MWPLY mwply, MWS_FRM *frm)
{
	MWSFFRM_VFRM *vfrm;
	Sint32 slen;
	void *sptr;
	Sint32 i;
	void *sfd;
	Sint32 nskip;
	void *usrptr;
	Sint32 usrlen;
	Sint32 coladj;
	Sint32 ftype;
	MWSFFRM_VFRM *p;

	if (MWSFD_IsEnableHndl(mwply) == 0) {
		MWSFSVM_Error("E1122614: mwPlyGetCurFrm: handle is invalid.");
		frm->bufadr = NULL;
		return;
	}
	sfd = mwPlyGetSfdHn(mwply);
	SFD_GetFrm(sfd, &vfrm);
	if (vfrm != NULL && mwply->noskip == 0) {
		nskip = mwply->prm.max_skip;
		for (i = 0; i < nskip; i++) {
			if (mwPlyIsNextFrmReady(mwply) != 1) {
				break;
			}
			SFD_RelFrm(sfd, vfrm);
			mwply->nskipdisp++;
			SFD_GetFrm(sfd, &vfrm);
		}
	}
	if (vfrm != NULL) {
		mwply->ngetfrm++;
		mwply->curfrm = vfrm;
		p = vfrm;
		mwply->pic_struct = p->pic_struct;
		mwply->chroma_format = p->chroma_format;
		mwply->x94 = p->x6c;
		mwply->x98 = p->x6d;
		mwply->x9c = p->x6e;
		mwply->chromapos_h = p->chromapos_h;
		mwply->chromapos_v = p->chromapos_v;
		mwply->xa8 = 0;
		mwl_convFrmInfFromSFD(mwply, vfrm, frm);
		usrptr = vfrm->usr->ptr;
		usrlen = vfrm->usr->len;
		if (MWSFD_GetUsePicUsr() == 1 && mwply->picusr_buf != NULL) {
			if (usrptr != NULL && usrlen > 4) {
				SUD_SearchSudDat((Uint8 *)usrptr + 4, usrlen - 4, &sptr, &slen);
			} else {
				sptr = NULL;
				slen = 0;
			}
			if (sptr != NULL && slen > 0) {
				if (slen > mwply->picusr_bsize) {
					slen = mwply->picusr_bsize;
				}
				memset(mwply->picusr_buf, 0, mwply->picusr_bsize);
				memcpy(mwply->picusr_buf, sptr, slen);
				mwply->picusr_dat = mwply->picusr_buf;
				mwply->picusr_len = slen;
			} else {
				mwply->picusr_dat = NULL;
				mwply->picusr_len = 0;
			}
			MWSFSFX_SetPicUsrDat(mwply, mwply->picusr_dat, mwply->picusr_len);
		}
		if (mwply->tag_x1a4 < frm->frmno) {
			MWSFTAG_UpdateTagInf(mwply);
		}
		mwply->tag_x1a4 = frm->frmno;
		coladj = mwsffrm_IsCurCcs(mwply) == 1;
		if (mwsffrm_IsPicUsrDat(mwply) == 1) {
			if (MWSFSFX_IsFrmCcs(mwply) == 1) {
				coladj = 1;
			} else {
				coladj = 0;
			}
		}
		MWSFD_SetColAdj(mwply, coladj);
		mwsffrm_DecideFrmType(vfrm, &ftype);
		if (MWSFD_GetUsePicUsr() == 1 && MWSFD_IsFrmDivField(mwply) == 1) {
			ftype = 2;
		}
		frm->ftype = ftype;
		mwply->sfh_cur = frm->frmno;
		MWSFSFX_SetFxType(mwply, mwsffrm_GetCurFxType(mwply));
	} else {
		frm->bufadr = NULL;
	}
}

/* dead */
Sint32 mwPlyGetFrmSync(MWPLY mwply)
{
	if (MWSFD_IsEnableHndl(mwply) == 0) {
		MWSFSVM_Error("E2010801: mwPlyGetFrmSync: handle is invalid.");
		return 0;
	}
	return mwply->noskip;
}

/* dead */
void mwPlySetFrmSync(MWPLY mwply, Sint32 sw)
{
	if (MWSFD_IsEnableHndl(mwply) == 0) {
		MWSFSVM_Error("E1122629: mwPlySetFrmSync: handle is invalid.");
		return;
	}
	mwply->noskip = sw;
}
