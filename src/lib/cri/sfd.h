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

/* audio output (auto-play) driver work (sfd_aoap.c), at SFD_OBJ + 0x3474: user callbacks */
typedef struct {
	Sint32 x00;
	Sint32 (*SetOutPan)(SFD sfd, Sint32 ch, Sint32 pan); /* 0x04 */
	Sint32 (*GetOutPan)(SFD sfd, Sint32 ch);             /* 0x08 */
	Sint32 (*SetOutVol)(SFD sfd, Sint32 vol);            /* 0x0C */
	Sint32 (*GetOutVol)(SFD sfd);                        /* 0x10 */
	void (*SetSpeed)(SFD sfd, Sint32 speed);             /* 0x14 */
	Sint32 x18;
} SFAOAP;

/* concatenated-play work (sfd_con.c), at SFD_OBJ + 0xD28 */
typedef struct {
	Uint8 pad0[0xB8];
	Sint32 tottime;            /* 0xB8 total time of the concatenated files */
	Sint32 tunit;              /* 0xBC */
	Uint8 padc0[0x164 - 0xC0];
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

/* SFBUF work (0x74 bytes): buffer n links transfer driver in_tr -> out_tr; SFD_OBJ + 0x12E8 + n * 0x74 */
typedef struct {
	Uint8 pad00[0x24];
	Sint32 x24;
	Sint32 x28;
	Sint32 x2c;
	Sint32 x30;
	Sint32 x34;
	Uint32 ofst;               /* 0x38 ring buffer start */
	Uint32 size;               /* 0x3C ring buffer size */
	Uint8 pad40[0x50 - 0x40];
	Sint32 x50;
	Sint32 x54;
	SFPTS_QUE ptsque;          /* 0x58 */
	Sint32 in_tr;              /* 0x6C transfer driver writing this buffer */
	Sint32 out_tr;             /* 0x70 transfer driver reading this buffer */
} SFBUF_WORK;

#define SFD_BUF_NUM 8

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
#define SFD_VFRM_NUM 16

/* per-frame picture information handed to the user (0x80 bytes) */
typedef struct {
	Sint32 raw[0x20];
} SFD_VFRM_INF;

/* video frame slot (0x88 bytes), SFD_OBJ + 0x16A8 + n * 0x88; slot n pairs with SFMPV frame n */
typedef struct {
	void *frm;                 /* 0x00 */
	Sint32 x04;
	SFD_VFRM_INF inf;          /* 0x08 */
} SFD_VFRM;

/* SFMPV_FRM.stat */
#define SFMPV_FRM_FREE 0
#define SFMPV_FRM_ALLOC 1
#define SFMPV_FRM_STBY 2
#define SFMPV_FRM_DRAWN 3
#define SFMPV_FRM_REF 4

/* decoded frame object of the video driver (0xE0 bytes) */
typedef struct {
	Sint32 stat;               /* 0x00 */
	Sint32 lock;               /* 0x04 */
	Uint8 pad08[0x38 - 0x08];
	Sint32 ftime;              /* 0x38 */
	Sint32 tunit;              /* 0x3C */
	Uint8 pad40[0x48 - 0x40];
	Sint32 gopno;              /* 0x48 */
	Uint8 pad4c[0x6C - 0x4C];
	Sint32 tmpref;             /* 0x6C temporal reference (10-bit) */
	Uint8 pad70[0x88 - 0x70];
	Sint32 x88;
	Sint32 x8c;
	Uint8 pad90[0xE0 - 0x90];
} SFMPV_FRM;

/* video driver work (tr[2].hn) */
typedef struct {
	Uint8 pad0[0x7C];
	Sint32 termflg;            /* 0x7C decoder terminated */
	Sint32 gopstat;            /* 0x80 */
	Uint8 pad84[0x178 - 0x84];
	Sint32 nfrm;               /* 0x178 */
	Sint32 x17c;
	SFMPV_FRM frm[1];          /* 0x180 (nfrm entries) */
} SFMPV_WORK;

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

/* seek support work supplied by the user through SFD_EntrySeek (sfd_see.c); the header analysis
 * results of the video / audio streams are kept here, followed by the user-set totals */
typedef struct {
	Sint32 analyzed;           /* 0x000 total time known */
	Sint32 ncount;             /* 0x004 total time */
	Sint32 tscale;             /* 0x008 */
	Sint32 mps_hdr;            /* 0x00C system header analysed */
	Uint8 pad010[0x018 - 0x010];
	Sint32 mps_time;           /* 0x018 total time from the system header (ms) */
	Uint8 pad01c[0x040 - 0x01C];
	Sint32 mps_rate;           /* 0x040 byte rate from the system header */
	Uint8 pad044[0x8A0 - 0x044];
	Sint32 vhdr;               /* 0x8A0 video header analysed */
	Sint32 vncount;            /* 0x8A4 */
	Sint32 vtscale;            /* 0x8A8 */
	Uint8 pad8ac[0xAD0 - 0x8AC];
	Sint32 a1hdr;              /* 0xAD0 audio 1 header analysed */
	Sint32 a1ncount;           /* 0xAD4 */
	Sint32 a1tscale;           /* 0xAD8 */
	Uint8 padadc[0xD0C - 0xADC];
	Sint32 a2hdr;              /* 0xD0C audio 2 header analysed */
	Sint32 a2ncount;           /* 0xD10 */
	Sint32 a2tscale;           /* 0xD14 */
	Uint8 padd18[0xDA8 - 0xD18];
	Sint32 rate;               /* 0xDA8 estimated byte rate */
	Sint32 fsize_est;          /* 0xDAC file size seen by the input driver */
	Sint32 tot_est;            /* 0xDB0 total time from the concatenation work */
	Sint32 tunit_est;          /* 0xDB4 */
	Sint32 av_a;               /* 0xDB8 */
	Sint32 av_b;               /* 0xDBC */
	Sint32 paddc0;
	Sint32 fsize;              /* 0xDC4 SFD_SetFileSize */
	Sint32 tottime;            /* 0xDC8 SFD_SetTotTime */
	Sint32 tunit;              /* 0xDCC */
	Sint32 byterate;           /* 0xDD0 SFD_SetByteRate */
	Sint32 seekpos;            /* 0xDD4 SFD_SetSeekPos */
} SFSEE_WORK;

typedef struct {
	Sint32 x00;
	Sint32 pos;                /* 0x04 requested seek position (-3: none) */
	Sint32 x08;
} SFSEE_REQ;

typedef struct {
	SFSEE_WORK *wk;            /* 0x34C8 */
	SFSEE_REQ req;             /* 0x34CC */
} SFSEE_HN;

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
	Uint8 padFA4[0x12E8 - 0xD28 - sizeof(SFCON)];
	SFBUF_WORK buf[SFD_BUF_NUM]; /* 0x12E8 */
	Uint8 pad1688[0x16A8 - 0x12E8 - SFD_BUF_NUM * sizeof(SFBUF_WORK)];
	SFD_VFRM vfrm[SFD_VFRM_NUM]; /* 0x16A8 */
	SFD_TR tr[SFD_TR_NUM];     /* 0x1F28 (tr[2].hn = SFMPV_WORK *, tr[8].hn = SFUO *, tr[8].bufin = user-output SFBUF id) */
	Uint8 pad218C[0x3474 - 0x1F28 - SFD_TR_NUM * sizeof(SFD_TR)];
	SFAOAP aoap;               /* 0x3474 (tr[7].hn) */
	SFUO uo_tbl;               /* 0x3490 */
	Sint32 pad34c4;
	SFSEE_HN see;              /* 0x34C8 */
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
Sint32 UTY_MulDiv(Sint32 a, Sint32 b, Sint32 c);
Sint32 SFCON_IsEndcodeSkip(SFD sfd);
Sint32 SFHDS_GetMuxVerNum(SFD sfd);
void SFSEE_InitHn(SFSEE_HN *see);
void SFSEE_ExecServer(SFD sfd);
void SFSEE_FixAvPlay(SFD sfd, Sint32 a, Sint32 b);
Sint32 SFD_GetPlyInf(SFD sfd, SFD_PLYINF *inf);
Sint32 SFD_SetCond(SFD sfd, Sint32 id, Sint32 val);
Sint32 SFD_GetHnStat(SFD sfd);
Sint32 SFD_GetTime(SFD sfd, Sint32 *ncount, Sint32 *tscale);
Sint32 SFD_SetSpeed(SFD sfd, Sint32 speed);
Bool SFTIM_IsGetFrmTime(SFD sfd, void *frm);
Bool SFTIM_IsGetFrmTimeTunit(SFD sfd, Sint32 ftime, Sint32 tunit);
Bool SFTIM_IsVideoTerm(SFD sfd);

#endif
