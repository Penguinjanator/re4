/* CRI Sofdec MW player (MWSFD) internals shared by the mwsfd* and mwsfx_* units. */
#ifndef CRI_MWSFD_H
#define CRI_MWSFD_H

#include "cri_xpt.h"
#include "sj.h"

typedef struct MWPLY_OBJ MWPLY_OBJ;
typedef struct SFX_OBJ SFX_OBJ;

/* mwPlyGetCurFrm output (0x88 bytes) */
typedef struct {
	void *bufadr;
	Uint8 pad[0x84];
} MWS_FRM;

/* SFX-side frame description filled by MWSFSFX_CnvFrmInfToSfx (0x88 bytes) */
typedef struct {
	Sint32 frmfmt;
	Uint8 pad[0x84];
} SFX_FRM;

typedef MWPLY_OBJ *MWPLY;

/* sub-stream (audio side-stream) handle (mwsfdsst.c); the player embeds one at MWPLY_OBJ + 0x294 and
 * its hn points at a second MWSST_OBJ whose hn is the core-library handle */
typedef struct MWSST_OBJ {
	Sint32 used;               /* 0x00 */
	Sint32 x04;
	Sint32 x08;
	SJ sj;                     /* 0x0C */
	void *buf;                 /* 0x10 work buffer (ring buffer at +0xC0) */
	struct MWSST_OBJ *hn;      /* 0x14 */
} MWSST_OBJ;

typedef MWSST_OBJ *MWSST;

/* core sub-stream library interface registered in mwsstmng.ifc */
typedef struct {
	void *x00;
	void (*Finish)(void);                          /* 0x04 */
	void *x08;
	void *x0c;
	void (*Destroy)(MWSST hn);                     /* 0x10 */
	void (*StartSj)(MWSST hn, SJ sj);              /* 0x14 */
	void (*Stop)(MWSST hn);                        /* 0x18 */
	Sint32 (*GetStat)(MWSST hn);                   /* 0x1C */
	void *x20;
	void (*Pause)(MWSST hn, Sint32 sw);            /* 0x24 */
	void (*SetOutVol)(MWSST hn, Sint32 vol);       /* 0x28 */
	Sint32 (*GetOutVol)(MWSST hn);                 /* 0x2C */
} MWSST_IF;

/* MWPLY_OBJ.stat */
#define MWSFD_STAT_STOP 0
#define MWSFD_STAT_PREP 1
#define MWSFD_STAT_PLAYING 2
#define MWSFD_STAT_PLAYEND 3
#define MWSFD_STAT_ERROR 4

/* player object (0x2B8 bytes; only the fields the matched units use are named) */
struct MWPLY_OBJ {
	Sint32 x00;
	Sint32 used;               /* 0x04 */
	Sint32 stat;               /* 0x08 */
	Uint8 pad0c[0x40 - 0x0C];
	void *sfd;                 /* 0x40 */
	void *stm;                 /* 0x44 ADXSTM */
	Sint32 x48;
	void *lsc;                 /* 0x4C */
	Uint8 pad50[0x60 - 0x50];
	Sint32 sleep_bdr;          /* 0x60 sleeping at the idle border */
	Sint32 mwply_svr_flg;      /* 0x64 handle server running */
	Sint32 sfd_svr_flg;        /* 0x68 SFD_ExecOne running */
	Sint32 dec_svr_flg;        /* 0x6C */
	Sint32 x70;
	Sint8 linkstm;             /* 0x74 */
	Sint8 linkstm_req;         /* 0x75 */
	Sint8 pause_flg;           /* 0x76 */
	Uint8 pad77[0x1B8 - 0x77];
	const Char8 *fname;        /* 0x1B8 */
	Sint32 x1bc;
	Sint32 stm_start_req;      /* 0x1C0 */
	void *dir;                 /* 0x1C4 */
	Sint32 ofst;               /* 0x1C8 */
	Sint32 nsct;               /* 0x1CC */
	SJ sji;                    /* 0x1D0 */
	Uint8 pad1d4[0x294 - 0x1D4];
	MWSST_OBJ sst;             /* 0x294 */
	Uint8 pad2ac[0x2B8 - 0x2AC];
};

typedef struct {
	MWSST_IF *ifc;             /* 0x00 */
	Sint32 cnt;                /* 0x04 number of created handles */
} MWSST_MNG;

#define MWSFD_MAX_HN 8

/* library work (mwsfdlib.c, 0x162C bytes) */
typedef struct {
	Sint32 x00;                /* 0x00 */
	Float32 vfreq;             /* 0x04 */
	Sint32 x08;                /* 0x08 */
	Sint32 nfrm_pool;          /* 0x0C */
	Sint32 x10;                /* 0x10 decode in the main thread (1) instead of the idle thread */
	Uint8 pad14[0x24 - 0x14];
	Sint32 svr_bdr;            /* 0x24 a handle is sleeping at the idle border */
	Uint8 pad28[0x38 - 0x28];
	Sint32 use_picusr;         /* 0x38 */
	Sint32 pause_bdr;          /* 0x3C */
	Sint32 (*pre_func)(void *obj);  /* 0x40 called before the decode server */
	void *pre_obj;             /* 0x44 */
	Sint32 (*post_func)(void *obj); /* 0x48 */
	void *post_obj;            /* 0x4C */
	Sint32 (*idle_func)(void *obj); /* 0x50 called when no handle waits */
	void *idle_obj;            /* 0x54 */
	Sint32 svr_flg;            /* 0x58 decode server running */
	Sint32 x5c;                /* 0x5C vsync server running */
	Uint8 pad60[0x68 - 0x60];
	Sint32 errcode;            /* 0x68 */
	MWPLY_OBJ hn[MWSFD_MAX_HN]; /* 0x6C (0x2B8 each) */
} MWSFD_LIBWORK;

extern MWSST_MNG mwsstmng;
extern MWSFD_LIBWORK mwsfd_libwork;
extern Sint32 mwsfd_init_flag;
extern Sint32 mwg_vcnt;
extern MWPLY mwsfd_hn_last;
MWSFD_LIBWORK *MWSFLIB_GetLibWorkPtr(void);
Sint32 MWSFSVM_TestAndSet(Sint32 *flag);

void MWSST_Destroy(MWSST sst);
void MWSST_Reset(MWPLY mwply);
Sint32 MWSST_GetOutVol(MWSST sst);
void MWSST_SetOutVol(MWSST sst, Sint32 vol);
void MWSST_Pause(MWSST sst, Sint32 sw);
Sint32 MWSST_GetStat(MWSST sst);
void MWSST_Stop(MWSST sst);
void MWSST_StartSj(MWSST sst);
void MWSFSVM_GotoIdleBorder(void);

/* mwPlyInitSfdFx creation parameters (0x20 bytes) */
typedef struct {
	Float32 vfreq;             /* 0x00 video refresh rate */
	Sint32 x04;                /* 0x04 */
	Sint32 nfrm_pool;          /* 0x08 */
	Sint32 x0c;                /* 0x0C */
	Sint32 x10;                /* 0x10 */
	Sint32 x14;                /* 0x14 */
	Sint32 x18;                /* 0x18 */
	Sint32 x1c;                /* 0x1C */
} MWSFD_INIT_PRM;



Bool MWSFD_IsEnableHndl(MWPLY_OBJ *mwply);
void MWSFSVM_Error(const Char8 *fmt, ...);
void MWSFSVM_Init(void);
void MWSFSVM_EntryIdVfunc(Sint32 id, Sint32 (*func)(void *obj), void *obj);
void MWSFSVM_EntryMainFunc(Sint32 (*func)(void *obj), void *obj);
void MWSFSVM_EntryIdleFunc(Sint32 (*func)(void *obj), void *obj);
void MWSFSVR_SetMwsfdSvrFlg(Sint32 flg);
Sint32 MWSFSVR_VsyncThrdProc(void *obj);
Sint32 MWSFSVR_MainThrdProc(void *obj);
Sint32 MWSFSVR_IdleThrdProc(void *obj);
void MWSFSFX_Init(void);
Sint32 MWSTM_InitStatic(void);
void *mwPlyGetSfdHn(MWPLY mwply);
Sint32 MWSFLIB_SetErrCode(Sint32 code);
void MWSFLIB_SfdErrFunc(void *obj, Sint32 code);
void MWSFD_SetCond(MWPLY mwply, Sint32 id, Sint32 val);
SFX_OBJ *MWSFSFX_GetSfxHn(MWPLY_OBJ *mwply);
void MWSFSFX_CnvFrmInfToSfx(MWPLY_OBJ *mwply, MWS_FRM *frm, SFX_FRM *sfxfrm);
void SFX_CnvFrmY84C44(SFX_OBJ *sfx, SFX_FRM *frm, void *ybuf, void *cbuf);
void SFX_CnvFrmARGB8888(SFX_OBJ *sfx, SFX_FRM *frm, void *buf);

#endif
