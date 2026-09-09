/* CRI Sofdec: stream buffers between the transfer drivers (sfd_buf.c). Buffers 0..2 are ring buffers
 * (stream joints), 3/5 video frame tables, 4/6 audio output, 7 the user output channels. */
#include "cri_xpt.h"
#include "sj.h"
#include "sfd.h"

extern Sint32 SJRBF_GetFlowCnt(SJ sj, Sint32 id, Sint32 dir);
extern Sint32 SJMEM_GetBufSize(SJ sj);
extern void SFPTS_InitPtsQue(SFPTS_QUE *que);

#define SFBUF_TR_NONE 9
#define SFBUF_PRV_XSIZE 0x800

/* buffer n is addressed as SFD + n * sizeof(SFBUF_WORK) with the offsets of buf[0] folded into the
 * displacements: a view of the SFD object shifted by n buffers */
typedef struct {
	Uint8 pad[0x1308];
	SFBUF_WORK w;
} SFBUF_HN;

#define SFBUF_GET_HN(sfd, n) ((SFBUF_HN *)((Uint8 *)(sfd) + (n) * sizeof(SFBUF_WORK)))

/* creation parameters of SFBUF_InitHn */
typedef struct {
	Sint32 x00;
	Uint32 adr;                /* 0x04 work base */
	Sint32 size[7];            /* 0x08 sizes of buffers 0..6 */
	Sint32 x24;
	Sint32 unit;               /* 0x28 ring buffer 0 alignment */
} SFBUF_PRM;

static const SJUUID *sfbuf_sjmem_uuid;
static const SJUUID *sfbuf_sjrbf_uuid;

static Bool sfbuf_IsSjmem(SJ sj)
{
	if (SJ_GetUuid(sj) == sfbuf_sjmem_uuid) {
		return 1;
	}
	return 0;
}

static Bool sfbuf_IsSjrbf(SJ sj)
{
	if (SJ_GetUuid(sj) == sfbuf_sjrbf_uuid) {
		return 1;
	}
	return 0;
}

Sint64 SFBUF_UpdateFlowCnt(Sint64 cnt, Uint32 pos)
{
	Sint64 up;
	Sint64 hi;

	if (pos < (Uint32)cnt) {
		up = 1;
	} else {
		up = 0;
	}
	hi = cnt >> 32;
	hi += up;
	hi <<= 32;
	hi |= pos;
	return hi;
}

void SFBUF_GetFlowCnt(SJ sj, Sint32 *wcnt, Sint32 *rcnt)
{
	if (sfbuf_IsSjrbf(sj)) {
		*wcnt = SJRBF_GetFlowCnt(sj, 1, 1);
		*rcnt = SJRBF_GetFlowCnt(sj, 0, 1);
	} else if (sfbuf_IsSjmem(sj)) {
		*wcnt = SJMEM_GetBufSize(sj);
		*rcnt = *wcnt - SJ_GetNumData(sj, 1);
	} else {
		*wcnt = 0;
		*rcnt = 0;
	}
}

/* a flow total counts only while it is not unknown (-1) */
static void sfbuf_AddTot(Sint32 *tot, Sint32 nbyte)
{
	if (*tot >= 0) {
		*tot += nbyte;
	}
}

/* the two chunks (before / after the wrap) that make up the data of `id` */
static void sfbuf_RingGetCk(SJ sj, Sint32 id, SJCK *ck1, SJCK *ck2)
{
	Sint32 num;

	num = SJ_GetNumData(sj, id);
	SJ_GetChunk(sj, id, 0x7FFFFFFF, ck1);
	if (ck1->len < num) {
		SJ_GetChunk(sj, id, 0x7FFFFFFF, ck2);
		SJ_UngetChunk(sj, id, ck2);
	} else {
		ck2->data = NULL;
		ck2->len = 0;
	}
	SJ_UngetChunk(sj, id, ck1);
}

Sint32 SFBUF_RingGetDataSiz(SFD sfd, Sint32 n)
{
	SFBUF_HN *hn = SFBUF_GET_HN(sfd, n);
	SJ sj;
	SJCK ck2;
	SJCK ck1;
	Sint32 len1 = 0;
	Sint32 len2 = 0;

	sj = hn->w.u.ring.sup.sj;
	if (hn->w.used != 0 && sj != NULL) {
		sfbuf_RingGetCk(sj, 1, &ck1, &ck2);
		len1 = ck1.len;
		len2 = ck2.len;
	}
	return len1 + len2;
}

Sint32 SFBUF_GetTermFlg(SFD sfd, Sint32 n)
{
	return sfd->buf[n].termflg;
}

void SFBUF_SetTermFlg(SFD sfd, Sint32 n, Sint32 flg)
{
	sfd->buf[n].termflg = flg;
}

Sint32 SFBUF_GetPrepFlg(SFD sfd, Sint32 n)
{
	return sfd->buf[n].prepflg;
}

void SFBUF_SetPrepFlg(SFD sfd, Sint32 n, Sint32 flg)
{
	sfd->buf[n].prepflg = flg;
}

Sint32 SFBUF_VfrmAddRead(SFD sfd, Sint32 n, void *frm)
{
	SFBUF_HN *hn = SFBUF_GET_HN(sfd, n);
	Sint32 ret = 0;

	if (hn->w.used == 0) {
		ret = SFTRN_CallTrtTrif(sfd, hn->w.in_tr, 0xC, (Sint32)frm, 0);
	}
	sfd->chg_flg = 1;
	return ret;
}

Sint32 SFBUF_VfrmGetRead(SFD sfd, Sint32 n, void **frm)
{
	SFBUF_HN *hn = SFBUF_GET_HN(sfd, n);

	if (hn->w.used == 0) {
		return SFTRN_CallTrtTrif(sfd, hn->w.in_tr, 0xB, (Sint32)frm, 0);
	}
	return 0;
}

void SFBUF_AddRtotSj(SFD sfd, Sint32 n, Sint32 nbyte)
{
	SFBUF_HN *hn = SFBUF_GET_HN(sfd, n);

	if (hn->w.u.ring.rtot >= 0) {
		hn->w.u.ring.rtot += nbyte;
	}
}

Sint32 SFBUF_RingGetSj(SFD sfd, Sint32 n, SJ *sj)
{
	SFBUF_HN *hn = SFBUF_GET_HN(sfd, n);

	*sj = NULL;
	if (hn->w.used == 0) {
		return SFLIB_SetErr(sfd, 0xFF000401);
	}
	*sj = hn->w.u.ring.sup.sj;
	return 0;
}

Sint32 SFBUF_GetWTot(SFD sfd, Sint32 n)
{
	SFBUF_HN *hn = SFBUF_GET_HN(sfd, n);
	Sint32 cs;
	Sint32 wtot;
	Sint32 rtot;

	SFLIB_LockCs(&cs);
	wtot = hn->w.u.ring.wtot;
	rtot = hn->w.u.ring.rtot;
	if (wtot == 0 && rtot != 0) {
		wtot = rtot + SJ_GetNumData(hn->w.u.ring.sup.sj, 1);
	}
	if (wtot < 0) {
		wtot = 0x7FFFFFFF;
	}
	SFLIB_UnlockCs(&cs);
	return wtot;
}

Sint32 SFBUF_GetRTot(SFD sfd, Sint32 n)
{
	return sfd->buf[n].u.ring.rtot;
}

Sint32 SFBUF_GetRingBufSiz(SFD sfd, Sint32 n)
{
	return sfd->buf[n].u.ring.sup.size;
}

void SFBUF_RingSetDlm(SFD sfd, Sint32 n, Uint8 *pos, Sint32 len)
{
	SFBUF_HN *hn = SFBUF_GET_HN(sfd, n);
	Sint32 cs;

	SFLIB_LockCs(&cs);
	hn->w.u.ring.dlm_pos = pos;
	hn->w.u.ring.dlm_len = len;
	SFLIB_UnlockCs(&cs);
}

void SFBUF_RingGetDlm(SFD sfd, Sint32 n, Uint8 **pos, Sint32 *len)
{
	SFBUF_HN *hn = SFBUF_GET_HN(sfd, n);
	Sint32 cs;

	SFLIB_LockCs(&cs);
	*pos = hn->w.u.ring.dlm_pos;
	*len = hn->w.u.ring.dlm_len;
	SFLIB_UnlockCs(&cs);
}

Sint32 SFBUF_RingAddRead(SFD sfd, Sint32 n, Sint32 nbyte)
{
	SFBUF_HN *hn = SFBUF_GET_HN(sfd, n);
	SFBUF_RING *ring = &hn->w.u.ring;
	SJ sj;
	SJCK ck;
	SJCK ck2;
	Sint32 rest;
	Sint32 ret = 0;
	SJCK ckw;
	SJCK ck1;
	Uint8 *pos;

	sj = hn->w.u.ring.sup.sj;
	do {
		if (nbyte == 0) {
			break;
		}
		if (hn->w.used == 0 || sj == NULL) {
			ret = 0;
			break;
		}
		{
			SJ_GetChunk(sj, 1, nbyte, &ck);
			SJ_PutChunk(sj, 0, &ck);
			if (ck.len < nbyte) {
				rest = nbyte - ck.len;
				SJ_GetChunk(sj, 1, rest, &ck2);
				SJ_PutChunk(sj, 0, &ck2);
				if (ck2.len < rest) {
					ret = SFLIB_SetErr(sfd, 0xFF00040B);
				}
			}
			if (n == 1) {
				sj = ring->sup.sj;
				sfbuf_RingGetCk(sj, 1, &ck1, &ckw);
				pos = ring->dlm_pos;
				if ((pos < ck1.data || pos >= ck1.data + ck1.len) && (pos < ckw.data || pos >= ckw.data + ckw.len)) {
					ring->dlm_pos = NULL;
					ring->dlm_len = 0;
				}
			}
			sfbuf_AddTot(&ring->rtot, nbyte);
			sfd->chg_flg = 1;
		}
	} while (0);
	return ret;
}

Sint32 SFBUF_RingAddWrite(SFD sfd, Sint32 n, Sint32 nbyte)
{
	SFBUF_HN *hn = SFBUF_GET_HN(sfd, n);
	SFBUF_RING *ring = &hn->w.u.ring;
	SJ sj;
	SJCK ck;
	SJCK ck2;
	Sint32 rest;
	Sint32 ret = 0;

	sj = hn->w.u.ring.sup.sj;
	do {
		if (nbyte == 0) {
			break;
		}
		if (hn->w.used == 0 || sj == NULL) {
			ret = 0;
			break;
		}
		{
			SJ_GetChunk(sj, 0, nbyte, &ck);
			SJ_PutChunk(sj, 1, &ck);
			if (ck.len < nbyte) {
				rest = nbyte - ck.len;
				SJ_GetChunk(sj, 0, rest, &ck2);
				SJ_PutChunk(sj, 1, &ck2);
				if (ck2.len < rest) {
					ret = SFLIB_SetErr(sfd, 0xFF00040B);
				}
			}
			sfbuf_AddTot(&ring->wtot, nbyte);
			sfd->chg_flg = 1;
		}
	} while (0);
	return ret;
}

Sint32 SFBUF_RingGetRead(SFD sfd, Sint32 n, SFBUF_RINF *inf)
{
	SFBUF_HN *hn = SFBUF_GET_HN(sfd, n);
	SJ sj;
	SJCK ck2;
	SJCK ck1;

	inf->ck1.data = NULL;
	inf->ck1.len = 0;
	inf->ck2.data = NULL;
	inf->ck2.len = 0;
	inf->rsv[0] = 0;
	inf->rsv[1] = 0;
	inf->rsv[2] = 0;
	sj = hn->w.u.ring.sup.sj;
	if (hn->w.used == 0 || sj == NULL) {
		return 0;
	}
	sfbuf_RingGetCk(sj, 1, &ck1, &ck2);
	inf->ck1.data = ck1.data;
	inf->ck1.len = ck1.len;
	inf->ck2.data = ck2.data;
	inf->ck2.len = ck2.len;
	return 0;
}

Sint32 SFBUF_RingGetWrite(SFD sfd, Sint32 n, SFBUF_RINF *inf)
{
	SFBUF_HN *hn = SFBUF_GET_HN(sfd, n);
	SJ sj;
	SJCK ck2;
	SJCK ck1;

	inf->ck1.data = NULL;
	inf->ck1.len = 0;
	inf->ck2.data = NULL;
	inf->ck2.len = 0;
	inf->rsv[0] = 0;
	inf->rsv[1] = 0;
	inf->rsv[2] = 0;
	sj = hn->w.u.ring.sup.sj;
	if (hn->w.used == 0 || sj == NULL) {
		return 0;
	}
	sfbuf_RingGetCk(sj, 0, &ck1, &ck2);
	inf->ck1.data = ck1.data;
	inf->ck1.len = ck1.len;
	inf->ck2.data = ck2.data;
	inf->ck2.len = ck2.len;
	return 0;
}

void SFBUF_GetUoch(SFD sfd, Sint32 n, Sint32 chno, SFUO_CH *ch)
{
	*ch = sfd->buf[n].u.uoch[chno];
}

void SFBUF_SetUoch(SFD sfd, Sint32 n, Sint32 chno, SFUO_CH *ch)
{
	sfd->buf[n].u.uoch[chno] = *ch;
}

static Sint32 sfbuf_CheckSup(SFBUF_SUP *sup)
{
	if (sup->sj == NULL) {
		return -1;
	}
	if (sup->kind == 0) {
		if (sup->ofst == 0) {
			return -1;
		}
		if (sup->size <= 0) {
			return -1;
		}
		if (sup->x14 > 0) {
			return -1;
		}
	}
	return 0;
}

static void sfbuf_SetSup(SFBUF_WORK *wk, SFBUF_SUP *sup, Sint32 used)
{
	SFBUF_RING *ring = &wk->u.ring;
	Sint32 cs;

	SFLIB_LockCs(&cs);
	wk->used = used;
	ring->sup = *sup;
	ring->dlm_pos = NULL;
	ring->dlm_len = 0;
	ring->wtot = 0;
	ring->rtot = 0;
	SFPTS_InitPtsQue(&ring->ptsque);
	SFLIB_UnlockCs(&cs);
}

Sint32 SFBUF_SetSupplySj(SFD sfd, SFBUF_SUP *sup)
{
	Sint32 n;
	SFBUF_HN *hn;

	if (sfbuf_CheckSup(sup) != 0) {
		return SFLIB_SetErr(sfd, 0xFF000408);
	}
	if (SFTRN_IsSetup(sfd, 1)) {
		n = 0;
	} else if (SFTRN_IsSetup(sfd, 2)) {
		n = 1;
	} else if (SFTRN_IsSetup(sfd, 3)) {
		n = 2;
	} else {
		n = 0;
	}
	hn = SFBUF_GET_HN(sfd, n);
	if (hn->w.mode != SFBUF_MODE_NONE) {
		return SFLIB_SetErr(sfd, 0xFF000409);
	}
	sfbuf_SetSup(&hn->w, sup, sup->sj != NULL);
	return 0;
}

static void sfbuf_DestroySup(SFBUF_SUP *sup)
{
	if (sup->sj != NULL) {
		SJ_Destroy(sup->sj);
		sup->sj = NULL;
	}
}

void SFBUF_DestroySj(SFD sfd)
{
	if (sfd->buf[0].mode == SFBUF_MODE_RING) {
		sfbuf_DestroySup(&sfd->buf[0].u.ring.sup);
	}
	if (sfd->buf[1].mode == SFBUF_MODE_RING) {
		sfbuf_DestroySup(&sfd->buf[1].u.ring.sup);
	}
	if (sfd->buf[2].mode == SFBUF_MODE_RING) {
		sfbuf_DestroySup(&sfd->buf[2].u.ring.sup);
	}
}

/* the stream joint of a ring buffer of bsize bytes at adr (xsize of them the wrap area) */
static Sint32 sfbuf_CreateSj(SFBUF_SUP *sup, Uint32 adr, Sint32 bsize, Sint32 xsize)
{
	if (bsize <= 0) {
		return SFLIB_SetErr(NULL, 0xFF00040C);
	}
	sup->xsize = xsize;
	sup->x14 = 0;
	sup->sj = SJRBF_Create((void *)adr, bsize, xsize);
	if (sup->sj == NULL) {
		return SFLIB_SetErr(NULL, 0xFF00040A);
	}
	return 0;
}

/* ring buffer of `size` bytes at adr; size 0 leaves the buffer unsupplied */
static Sint32 sfbuf_InitRing(SFBUF_WORK *wk, Uint32 *adr, Sint32 size, Sint32 xsize)
{
	Sint32 mode;
	Sint32 used;
	SFBUF_SUP sup;
	Sint32 err;

	if (size == 0) {
		used = 0;
		mode = SFBUF_MODE_NONE;
	} else {
		used = 1;
		mode = SFBUF_MODE_RING;
		sup.kind = 0;
		sup.ofst = *adr;
		sup.size = size - xsize;
		err = sfbuf_CreateSj(&sup, *adr, sup.size, xsize);
		if (err != 0) {
			return err;
		}
		sfbuf_SetSup(wk, &sup, 1);
	}
	wk->mode = mode;
	wk->used = used;
	wk->prepflg = 0;
	wk->termflg = 0;
	wk->in_tr = SFBUF_TR_NONE;
	wk->out_tr = SFBUF_TR_NONE;
	return 0;
}

static void sfbuf_InitVfrm(SFD sfd, SFBUF_WORK *wk, Uint32 *adr, Sint32 size)
{
	Sint32 i;

	wk->mode = SFBUF_MODE_VFRM;
	wk->used = (size != 0);
	wk->prepflg = 0;
	wk->termflg = 0;
	wk->in_tr = SFBUF_TR_NONE;
	wk->out_tr = SFBUF_TR_NONE;
	wk->u.vfrm.adr = *adr;
	wk->u.vfrm.size = size;
	wk->u.vfrm.x18 = 0;
	wk->u.vfrm.x1c = 0;
	wk->u.vfrm.vfrm = sfd->vfrm;
	for (i = 0; i < SFD_VFRM_NUM; i++) {
		wk->u.vfrm.vfrm[i].frm = NULL;
	}
}

static void sfbuf_InitAout(SFBUF_WORK *wk, Uint32 *adr, Sint32 size)
{
	Sint32 i;

	wk->mode = SFBUF_MODE_AOUT;
	wk->used = (size != 0);
	wk->prepflg = 0;
	wk->termflg = 0;
	wk->in_tr = SFBUF_TR_NONE;
	wk->out_tr = SFBUF_TR_NONE;
	wk->u.aout.adr = *adr;
	wk->u.aout.size = size;
	for (i = 0; i < 7; i++) {
		wk->u.aout.rsv[i] = 0;
	}
	wk->u.ring.dlm_pos = NULL;
	wk->u.ring.dlm_len = 0;
	wk->u.ring.wtot = 0;
}

Sint32 SFBUF_InitHn(SFD sfd, SFBUF_WORK *wk, SFBUF_PRM *prm)
{
	Uint32 adr[8];
	Sint32 xsize;
	Sint32 err;
	Sint32 i;

	adr[0] = prm->adr;
	for (i = 0; i < 7; i++) {
		adr[i + 1] = adr[i] + prm->size[i];
	}
	xsize = prm->size[0] % prm->unit;
	err = sfbuf_InitRing(&wk[0], &adr[0], prm->size[0], xsize);
	if (err != 0) {
		return err;
	}
	err = sfbuf_InitRing(&wk[1], &adr[1], prm->size[1], SFBUF_PRV_XSIZE);
	if (err != 0) {
		return err;
	}
	err = sfbuf_InitRing(&wk[2], &adr[2], prm->size[2], 0);
	if (err != 0) {
		return err;
	}
	sfbuf_InitVfrm(sfd, &wk[3], &adr[3], prm->size[3]);
	sfbuf_InitAout(&wk[4], &adr[4], prm->size[4]);
	sfbuf_InitVfrm(sfd, &wk[5], &adr[5], prm->size[5]);
	sfbuf_InitAout(&wk[6], &adr[6], prm->size[6]);
	wk[7].mode = SFBUF_MODE_UO;
	wk[7].used = 1;
	wk[7].prepflg = 0;
	wk[7].termflg = 0;
	wk[7].in_tr = SFBUF_TR_NONE;
	wk[7].out_tr = SFBUF_TR_NONE;
	for (i = 0; i < 12; i++) {
		((Sint32 *)wk[7].u.uoch)[i] = 0;
	}
	return 0;
}

void SFBUF_Init(void *work)
{
	Uint8 dmy[8];
	SJ sj;

	sj = SJRBF_Create(dmy, 8, 0);
	sfbuf_sjrbf_uuid = SJ_GetUuid(sj);
	SJ_Destroy(sj);
	sj = SJMEM_Create(dmy, 8);
	sfbuf_sjmem_uuid = SJ_GetUuid(sj);
	SJ_Destroy(sj);
}
