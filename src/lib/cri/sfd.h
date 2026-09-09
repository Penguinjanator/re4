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

/* concatenated-play work (sfd_con.c), at SFD_OBJ + 0xD28 */
typedef struct {
	Uint8 pad0[0x164];
	Sint32 ctime;              /* 0x164 accumulated concat time */
	Sint32 ctime_idx;          /* 0x168 */
	Sint32 ctime_que[32];      /* 0x16C */
	Sint32 tot_last;           /* 0x1EC */
	Sint32 pad1F0;
	Sint32 que_wr;             /* 0x1F4 */
	Sint32 que_rd;             /* 0x1F8 */
	Sint32 tot_que[32];        /* 0x1FC */
} SFCON;

/* PTS queue entry (16 bytes, 8-byte aligned) */
typedef struct {
	Sint64 pts;
	Uint32 pos;
	Uint32 len;
} SFPTS_ENT;

typedef struct {
	SFPTS_ENT *ent;            /* 0x00 */
	Sint32 num;                /* 0x04 */
	Sint32 cnt;                /* 0x08 */
	Sint32 wr;                 /* 0x0C */
	Sint32 rd;                 /* 0x10 */
} SFPTS_QUE;

/* SFBUF work (0x74 bytes): buffer n links transfer driver in_tr -> out_tr; SFD_OBJ + 0x12E0 + n * 0x74 */
typedef struct {
	Sint32 in_tr;              /* 0x00 */
	Sint32 out_tr;             /* 0x04 */
	Uint8 pad08[0x40 - 0x08];
	Uint32 ofst;               /* 0x40 ring buffer start */
	Uint32 size;               /* 0x44 ring buffer size */
	Uint8 pad48[0x60 - 0x48];
	SFPTS_QUE ptsque;          /* 0x60 */
} SFBUF_WORK;

#define SFD_BUF_NUM 9

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

typedef Sint32 (*SFD_TR_FUNC)();

/* per-transfer-driver slot (0x44 bytes), SFD_OBJ + 0x1F28 + n * 0x44 */
typedef struct {
	Sint32 prepflg;            /* 0x00 */
	Sint32 termflg;            /* 0x04 */
	void *hn;                  /* 0x08 driver handle (tr[8]: SFUO *) */
	SFD_TR_FUNC *trif;         /* 0x0C driver function table */
	Sint32 bufin;              /* 0x10 SFBUF id read by this driver */
	Sint32 bufout;             /* 0x14 SFBUF id written by this driver */
	Sint32 bufout2;            /* 0x18 */
	Sint32 bufout3;            /* 0x1C */
	Sint32 x20;                /* 0x20 (-1) */
	Uint8 pad24[0x44 - 0x24];
} SFD_TR;

#define SFD_TR_NUM 9
#define SFD_COND_NUM 100

/* 0xA0-byte player information block returned by SFD_GetPlyInf */
typedef struct {
	Sint32 raw[0x28];
} SFD_PLYINF;

/* creation parameters passed to SFTRN_InitHn: trif_tbl[n] is driver n's function table
 * (0 input, 1 system, 2 video, 3 audio, 4 video out, 5 audio out, 6/7 output, 8 user) */
typedef struct {
	SFD_TR_FUNC **trif_tbl;    /* 0x00 */
} SFTRN_PRM;

/* the 15 driver interfaces known to the library (SFTRN_Init) */
typedef struct {
	SFD_TR_IF *tbl[15];
} SFTRN_TRIF_TBL;

/* error callback state (sfd_lib.c), per handle at SFD_OBJ + 0x9F0 and library-wide */
typedef struct {
	void (*fn)(void *obj, Sint32 code); /* 0x00 */
	void *obj;                          /* 0x04 */
	Sint32 code;                        /* 0x08 first error code */
	Sint32 x0c;
	Sint32 x10;
} SFLIB_ERRINF;

typedef struct SFD_OBJ {
	Uint8 pad0[0x44];
	Sint32 chg_flg;            /* 0x44 set after a control change */
	Sint32 stat;               /* 0x48 */
	Sint32 req;                /* 0x4C requested state (3 = standby, 4 = start) */
	Sint32 pause_sw;           /* 0x50 */
	Sint32 pause_cnt;          /* 0x54 */
	Uint8 pad58[0x950 - 0x58];
	SFD_PLYINF plyinf;         /* 0x950 */
	SFLIB_ERRINF err;          /* 0x9F0 */
	Sint32 cond[SFD_COND_NUM]; /* 0xA04 */
	Sint32 cond_def[SFD_COND_NUM]; /* 0xB94 */
	Uint8 padD24[4];
	/* 0xD28 */
	SFCON con;                 /* 0xD28 */
	Uint8 padFA4[0x12E0 - 0xD28 - sizeof(SFCON)];
	SFBUF_WORK buf[SFD_BUF_NUM]; /* 0x12E0 */
	Uint8 pad1694[0x1F28 - 0x12E0 - SFD_BUF_NUM * sizeof(SFBUF_WORK)];
	SFD_TR tr[SFD_TR_NUM];     /* 0x1F28 (tr[8].hn = SFUO *, tr[8].bufin = user-output SFBUF id) */
	Uint8 pad218C[0x3490 - 0x1F28 - SFD_TR_NUM * sizeof(SFD_TR)];
	SFUO uo_tbl;               /* 0x3490 */
} SFD_OBJ;


/* library-wide work (sfd_lib.c), 0x228 bytes */
typedef struct {
	Sint32 cond[SFD_COND_NUM];  /* 0x000 default conditions */
	SFTRN_TRIF_TBL *trif_tbl;   /* 0x190 from the init parameters */
	Sint32 prm1;                /* 0x194 */
	Sint32 x198;
	SFLIB_ERRINF err;           /* 0x19C */
	Uint8 tim[0xC];             /* 0x1B0 SFTIM work */
	Sint32 buf;                 /* 0x1BC SFBUF work */
	SFTRN_TRIF_TBL trif;        /* 0x1C0 */
	Sint32 x1fc;
	Sint32 x200;
	SFD hn[8];                  /* 0x204 created handles */
	Sint32 x224;
} SFLIB_WORK;

/* SFD_Init parameters */
typedef struct {
	SFTRN_TRIF_TBL *trif_tbl;   /* 0x00 */
	Sint32 prm1;                /* 0x04 */
} SFD_INIT_PRM;

extern SFLIB_WORK SFLIB_libwork;

Sint32 SFLIB_SetErr(SFD sfd, Sint32 code);
Sint32 SFLIB_CheckHn(SFD sfd);
void SFLIB_LockCs(Sint32 *cs);
void SFLIB_UnlockCs(Sint32 *cs);
void SFSET_SetCond(SFD sfd, Sint32 id, Sint32 val);
void SFTIM_Pause(SFD sfd, Sint32 sw);
void SFTIM_SetSpeed(SFD sfd, Sint32 speed);
void SFAOAP_SetSpeed(SFD sfd, Sint32 speed);
Sint32 SFTRN_CallTrtTrif(SFD sfd, Sint32 a, Sint32 b, Sint32 c, Sint32 d);
Sint32 SFTRN_IsSetup(SFD sfd, Sint32 id);
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
