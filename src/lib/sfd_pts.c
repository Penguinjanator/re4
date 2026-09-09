/* Sofdec: PTS (presentation time stamp) queues */
#include "cri_xpt.h"
#include "sfd.h"
#include <string.h>

#define PQ(sfd, strm) ((sfd)->buf[strm].ptsque)

static Sint32 sfpts_Wrap(Sint32 n, Sint32 num)
{
	Sint32 r;

	r = n - num;
	if (n < num) {
		r = n;
	}
	return r;
}

Sint32 SFPTS_IsPtsQueFull(SFD sfd, Sint32 strm)
{
	if (PQ(sfd, strm).ent == NULL) {
		return 0;
	}
	return PQ(sfd, strm).cnt >= PQ(sfd, strm).num;
}

Sint32 SFPTS_ReadPtsQue(SFD sfd, Sint32 strm, Uint32 pos, SFPTS_ENT *out)
{
	SFBUF_WORK *bw;
	SFPTS_ENT *ent;
	Uint32 ofst;
	Uint32 size;
	Uint32 end;
	Uint32 st;
	Uint32 en;
	Sint32 cnt;
	Sint32 num;
	Sint32 idx;
	Sint32 rd;
	Sint32 i;

	out->pts = -1;
	bw = &sfd->buf[strm];
	ent = bw->ptsque.ent;
	ofst = bw->ofst;
	size = bw->size;
	if (ent == NULL) {
		return 0;
	}
	end = ofst + size;
	if (pos >= end) {
		pos -= size;
	}
	cnt = bw->ptsque.cnt;
	if (cnt != 0) {
		rd = bw->ptsque.rd;
		num = bw->ptsque.num;
		idx = rd;
		for (i = 0; i < cnt; i++) {
			st = ent[idx].pos;
			en = ent[idx].pos + ent[idx].len;
			if (en <= end) {
				if (st <= pos && pos < en) {
					goto found;
				}
			} else {
				if (st <= pos && pos < end) {
					goto found;
				}
				if (ofst <= pos && pos < en - size) {
					goto found;
				}
			}
			idx = sfpts_Wrap(idx + 1, num);
		}
		i = -1;
found:
		if (i != -1) {
			idx = sfpts_Wrap(rd + i, num);
			bw->ptsque.cnt -= i;
			bw->ptsque.rd = idx;
			*out = bw->ptsque.ent[idx];
		}
	}
	return 0;
}

Sint32 SFPTS_WritePtsQue(SFD sfd, Sint32 strm, SFPTS_ENT *in, Sint32 *full)
{
	SFPTS_ENT *ent;
	Sint32 wr;
	Sint32 ret;

	*full = 0;
	if (in->pts < 0) {
		return 0;
	}
	ent = PQ(sfd, strm).ent;
	if (ent == NULL) {
		return 0;
	}
	if (PQ(sfd, strm).cnt == PQ(sfd, strm).num) {
		*full = 1;
		ret = -1;
	} else {
		wr = PQ(sfd, strm).wr;
		ent[wr] = *in;
		wr = sfpts_Wrap(wr + 1, PQ(sfd, strm).num);
		PQ(sfd, strm).cnt++;
		PQ(sfd, strm).wr = wr;
		if (PQ(sfd, strm).cnt >= PQ(sfd, strm).num) {
			*full = 1;
		} else {
			*full = 0;
		}
		ret = 0;
	}
	if (ret == -1) {
		return SFLIB_SetErr(sfd, 0xFF000421);
	}
	return 0;
}

Sint32 SFD_SetVideoPts(SFD sfd, Uint8 *buf, Sint32 size)
{
	Uint8 *p;

	if (buf == NULL || size <= 0) {
		return 0;
	}
	if (SFLIB_CheckHn(sfd) != 0) {
		return SFLIB_SetErr(NULL, 0xFF000165);
	}
	p = (Uint8 *)(((Uint32)buf + 7) & ~7);
	size -= p - buf;
	memset(p, 0, size);
	sfd->buf[1].ptsque.ent = (SFPTS_ENT *)p;
	sfd->buf[1].ptsque.num = size / 16;
	sfd->buf[1].ptsque.cnt = 0;
	sfd->buf[1].ptsque.wr = 0;
	sfd->buf[1].ptsque.rd = 0;
	return 0;
}

void SFPTS_InitPtsQue(SFPTS_QUE *que)
{
	que->ent = NULL;
	que->num = 0;
	que->cnt = 0;
	que->wr = 0;
	que->rd = 0;
}
