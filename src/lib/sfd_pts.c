/* Sofdec: PTS (presentation time stamp) queues */
#include "cri_xpt.h"
#include "sfd.h"
#include <string.h>

#define PQ(sfd, strm) ((sfd)->buf[strm].u.ring.ptsque)

typedef struct {
	Uint8 pad[0x1308];
	SFBUF_WORK w;
} SFBUF_HN;

#define SFBUF_GET_HN(sfd, n) ((SFBUF_HN *)((Uint8 *)(sfd) + (n) * sizeof(SFBUF_WORK)))


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

static Sint32 sfpts_SearchPts(SFPTS_ENT *ent, Sint32 idx, Sint32 cnt, Sint32 num, Uint32 pos, Uint32 ofst,
			      Uint32 size, Uint32 end)
{
	Uint32 st;
	Uint32 en;
	Sint32 i;

	for (i = 0; i < cnt; i++) {
		st = ent[idx].pos;
		en = ent[idx].pos + ent[idx].len;
		if (en <= end) {
			if (st <= pos && pos < en) {
				return i;
			}
		} else {
			if ((st <= pos && pos < end) || (ofst <= pos && pos < en - size)) {
				return i;
			}
		}
		idx = sfpts_Wrap(idx + 1, num);
	}
	return -1;
}

/* the buffer is addressed through one base (sfd + strm * sizeof(SFBUF_WORK)) kept across the inlined
 * search: the shifted view (sfd_buf.c SFBUF_HN). Volatile registers follow the declaration order from
 * r9 (r7/r8 go to the inlined loop's temporaries), the locals declared after the fourth get the
 * callee-saved ones: this order gives num/ofst/size r31/r30/r29 and ent/rd r11/r12 like the target.
 * M1: the target gives hn r7 and i r8 (before the loop temporaries), ours r9/r4. */
Sint32 SFPTS_ReadPtsQue(SFD sfd, Sint32 strm, Uint32 pos, SFPTS_ENT *out)
{
	SFBUF_HN *hn;
	Sint32 i;
	Uint32 end;
	SFPTS_ENT *ent;
	Sint32 rd;
	Sint32 num;
	Uint32 ofst;
	Uint32 size;
	Sint32 cnt;
	Sint32 idx;

	out->pts = -1;
	hn = SFBUF_GET_HN(sfd, strm);
	ent = hn->w.u.ring.ptsque.ent;
	ofst = hn->w.u.ring.sup.ofst;
	size = hn->w.u.ring.sup.size;
	if (ent == NULL) {
		return 0;
	}
	end = ofst + size;
	if (pos >= end) {
		pos -= size;
	}
	cnt = hn->w.u.ring.ptsque.cnt;
	if (cnt != 0) {
		num = hn->w.u.ring.ptsque.num;
		rd = hn->w.u.ring.ptsque.rd;
		i = sfpts_SearchPts(ent, rd, cnt, num, pos, ofst, size, end);
		if (i != -1) {
			idx = sfpts_Wrap(rd + i, num);
			hn->w.u.ring.ptsque.cnt -= i;
			hn->w.u.ring.ptsque.rd = idx;
			*out = hn->w.u.ring.ptsque.ent[idx];
		}
	}
	return 0;
}

Sint32 SFPTS_WritePtsQue(SFD sfd, Sint32 strm, SFPTS_ENT *in, Sint32 *full)
{
	Sint32 wr;
	SFPTS_ENT *ent;
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
	sfd->buf[1].u.ring.ptsque.ent = (SFPTS_ENT *)p;
	sfd->buf[1].u.ring.ptsque.num = size / 16;
	sfd->buf[1].u.ring.ptsque.cnt = 0;
	sfd->buf[1].u.ring.ptsque.wr = 0;
	sfd->buf[1].u.ring.ptsque.rd = 0;
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
