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

/* inlined into SFPTS_ReadPtsQue only; the hard-register asm pins (idx r4, st r3) are COMPILER-DIFF: M1 */
static Sint32 sfpts_SearchPts(SFPTS_ENT *ent, register Sint32 idx0, Sint32 cnt, Sint32 num, Uint32 pos, Uint32 ofst,
			      Uint32 size, Uint32 end)
{
	register Uint32 st;
	register SFPTS_ENT *e;
	Uint32 en;
	register Sint32 i;
	register Sint32 idx;

	asm { mr r4, idx0; mr idx, r4 } // COMPILER-DIFF: M1
	for (i = 0; i < cnt; i++) {
		e = &ent[idx];
		asm { lwz r3, SFPTS_ENT.pos(e); mr st, r3 } // COMPILER-DIFF: M1
		en = e->pos + e->len;
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
 * COMPILER-DIFF: M1 - the original ranks hn r7 (shared with the -1 constant) and i r8 before the
 * inlined loop's temporaries, idx r4, st r3 and the tail temporaries r3; every one of them is a
 * hard-register asm pin (`asm { op rN, ..; mr var, rN }`, the `mr` is coalesced away). A hard
 * register written in asm is never used for a compiler temporary anywhere else in the function, so
 * the tail's r3 values (cnt - i, &ent[idx]) had to be pinned as well. */
Sint32 SFPTS_ReadPtsQue(register SFD sfd, register Sint32 strm, Uint32 pos, SFPTS_ENT *out)
{
	register SFBUF_HN *hn; // COMPILER-DIFF: M1 (r7)
	register SFBUF_HN *h0;
	register Sint32 m1; // COMPILER-DIFF: M1 (the -1 in r7)
	register Sint32 i;
	register Sint32 c; // COMPILER-DIFF: M1 (r3)
	register SFPTS_ENT *src; // COMPILER-DIFF: M1 (r3)
	register Sint32 o;
	Uint32 end;
	SFPTS_ENT *ent;
	register Sint32 rd; // COMPILER-DIFF: M1 (r12)
	register Sint32 num;
	Uint32 ofst;
	Uint32 size;
	Sint32 cnt;
	register Sint32 idx; // COMPILER-DIFF: M1 (r4)
	register Sint32 n;

	asm { li r7, -1; mr m1, r7 } // COMPILER-DIFF: M1
	((Sint32 *)&out->pts)[1] = m1; /* out->pts = -1 */
	((Sint32 *)&out->pts)[0] = m1;
	h0 = SFBUF_GET_HN(sfd, strm);
	asm { mr r7, h0; mr hn, r7 } // COMPILER-DIFF: M1
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
		asm { lwz r12, SFBUF_HN.w.u.ring.ptsque.rd(hn); mr rd, r12 } // COMPILER-DIFF: M1
		i = sfpts_SearchPts(ent, rd, cnt, num, pos, ofst, size, end);
		if (i != -1) {
			n = rd + i; /* idx = sfpts_Wrap(rd + i, num) */
			asm { subf r4, num, n; mr idx, r4 } // COMPILER-DIFF: M1
			if (n < num) {
				idx = n;
			}
			asm { lwz r3, SFBUF_HN.w.u.ring.ptsque.cnt(hn); subf r3, i, r3; mr c, r3 } // COMPILER-DIFF: M1 (cnt -= i)
			hn->w.u.ring.ptsque.cnt = c;
			hn->w.u.ring.ptsque.rd = idx;
			o = idx << 4;
			asm { lwz r3, SFBUF_HN.w.u.ring.ptsque.ent(hn); add r3, r3, o; mr src, r3 } // COMPILER-DIFF: M1 (&ent[idx])
			*out = *src;
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
