/* CRI Sofdec decoder core (SFD) object layout, from the field offsets used by the DOL. */
#ifndef CRI_SFD_H
#define CRI_SFD_H

#include "cri_xpt.h"

typedef struct SFD_OBJ *SFD;

/* user-output (usr sj) channel */
typedef struct {
	void *sj;
	void *prm;
	Sint32 rsv1;
	Sint32 rsv2;
} SFUO_CH;

typedef struct {
	Sint32 nch;
	SFUO_CH ch[3];
} SFUO;

typedef struct SFD_OBJ {
	Uint8 pad0[0x48];
	Sint32 stat;               /* 0x48 */
	Uint8 pad4C[0x1F3C - 0x4C];
	Sint32 sfbuf;              /* 0x1F3C  ring buffer (SFBUF) handle */
	Uint8 pad1F40[0x20D0 - 0x1F40];
	Sint32 vfrmbuf;            /* 0x20D0  video frame buffer (SFBUF) handle */
	Uint8 pad20D4[0x2150 - 0x20D4];
	SFUO *uo;                  /* 0x2150 */
	Sint32 pad2154;
	Sint32 uobuf;              /* 0x2158  user-output SFBUF id */
	Uint8 pad215C[0x3490 - 0x215C];
	SFUO uo_tbl;               /* 0x3490 */
} SFD_OBJ;

/* transfer/stream driver interface (SFD_tr_in_mem, SFD_tr_vo_manu, ...) */
typedef struct {
	Sint32 (*Init)();
	Sint32 (*Finish)();
	Sint32 (*ExecServer)();
	Sint32 (*Create)();
	Sint32 (*Destroy)();
	Sint32 (*Standby)();
	Sint32 (*Start)();
	Sint32 (*Stop)();
	Sint32 (*Pause)();
	Sint32 (*GetWrite)();
	Sint32 (*AddWrite)();
	Sint32 (*GetRead)();
	Sint32 (*AddRead)();
	Sint32 (*Seek)();
} SFD_TR_IF;

Sint32 SFLIB_SetErr(SFD sfd, Sint32 code);
Sint32 SFLIB_CheckHn(SFD sfd);
void SFBUF_SetUoch(SFD sfd, Sint32 buf, Sint32 chno, SFUO_CH *ch);
Sint32 SFSET_GetCond(SFD sfd, Sint32 id);
Sint32 SFTRN_GetTermFlg(SFD sfd, Sint32 id);
void SFTRN_SetTermFlg(SFD sfd, Sint32 id, Sint32 flg);
Sint32 SFTRN_GetPrepFlg(SFD sfd, Sint32 id);
void SFTRN_SetPrepFlg(SFD sfd, Sint32 id, Sint32 flg);
Sint32 SFBUF_GetTermFlg(SFD sfd, Sint32 buf);
Sint32 SFBUF_GetPrepFlg(SFD sfd, Sint32 buf);
void SFBUF_SetPrepFlg(SFD sfd, Sint32 buf, Sint32 flg);
Sint32 SFBUF_RingAddWrite(SFD sfd, Sint32 buf, Sint32 a, Sint32 b);
Sint32 SFBUF_RingGetWrite(SFD sfd, Sint32 buf, void *a);
Sint32 SFBUF_VfrmAddRead(SFD sfd, Sint32 buf, void *frm);
Sint32 SFBUF_VfrmGetRead(SFD sfd, Sint32 buf, void **frm);
Bool SFTIM_IsGetFrmTime(SFD sfd, void *frm);
Bool SFTIM_IsVideoTerm(SFD sfd);

#endif
