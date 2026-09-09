/* CRI Sofdec MW player (MWSFD) internals shared by the mwsfd* and mwsfx_* units. */
#ifndef CRI_MWSFD_H
#define CRI_MWSFD_H

#include "cri_xpt.h"
#include "sj.h"
#include "sfx.h"

typedef struct MWPLY_OBJ MWPLY_OBJ;

/* mwPlyGetCurFrm output (0x88 bytes) */
typedef struct {
	void *bufadr;              /* 0x00 */
	Sint32 fmt;                /* 0x04 MwsfdBufFmt: 1 YCC420 planar (uphalf), 2 ..., 3 planar Y/Cb/Cr */
	Sint32 width;              /* 0x08 */
	Sint32 height;             /* 0x0C */
	Sint32 x10;                /* 0x10 (SFD frame 0x08) */
	Sint32 x14;                /* 0x14 (SFD frame 0x0C) */
	Sint32 pstruct;            /* 0x18 */
	Sint32 fps;                /* 0x1C */
	Sint32 time;               /* 0x20 SFD frame 0x34 in fps units */
	Sint32 x24;                /* 0x24 (SFD frame 0x34) */
	Sint32 tunit;              /* 0x28 (SFD frame 0x18) */
	Sint32 frmno;              /* 0x2C */
	void *tblsrc;              /* 0x30 (mwl_convFrmInfFromSFD: SFD frame 0x30 in fps units) */
	Sint32 x34;                /* 0x34 (SFD frame 0x30) */
	Sint32 x38;                /* 0x38 (SFD frame 0x24) */
	Sint32 x3c;                /* 0x3C (SFD frame 0x28) */
	void *usrdat;              /* 0x40 picture user data */
	Sint32 usrlen;             /* 0x44 */
	Sint32 ftype;              /* 0x48 frame type decided by mwPlyGetCurFrm */
	Sint32 x4c;
	Uint8 ext[0x38];           /* 0x50 copy of the SFD frame's 0x48..0x80 */
} MWS_FRM;

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

/* creation parameters kept in the player object (MWPLY_OBJ + 0x0C, 0x34 bytes) */
typedef struct {
	Sint32 mode;               /* 0x00 (2: no additional-info stream) */
	Sint32 x04;
	Sint32 x08;
	Sint32 x0c;
	Sint32 max_skip;           /* 0x10 frames mwPlyGetCurFrm may skip to catch up */
	Uint8 pad14[0x20 - 0x14];
	Sint32 compo;              /* 0x20 requested component layout (0 / 0x101: additional-info sj used) */
	Uint8 pad24[0x34 - 0x24];
} MWSFD_CRPRM;

/* Sofdec header information collected by the header callback (mwsfdfrm.c, 0x14 bytes) */
typedef struct {
	Sint32 valid;              /* 0x00 */
	Sint32 no;                 /* 0x04 header count when it was seen */
	Sint32 ccs;                /* 0x08 colour type 3 */
	Sint32 maxfrm;             /* 0x0C */
	Sint32 fxtype;             /* 0x10 SFX component layout */
} MWSFFRM_SFHINF;

#define MWSFFRM_SFHINF_NUM 8

/* player object (0x2B8 bytes; only the fields the matched units use are named) */
struct MWPLY_OBJ {
	Sint32 x00;
	Sint32 used;               /* 0x04 */
	Sint32 stat;               /* 0x08 */
	MWSFD_CRPRM prm;           /* 0x0C */
	void *sfd;                 /* 0x40 */
	void *stm;                 /* 0x44 ADXSTM */
	Sint32 x48;
	void *lsc;                 /* 0x4C */
	Sint32 compo_fix;          /* 0x50 component layout fixed at creation */
	Sint32 compo;              /* 0x54 */
	Sint32 noskip;             /* 0x58 never skip frames */
	Sint32 x5c;
	Sint32 sleep_bdr;          /* 0x60 sleeping at the idle border */
	Sint32 mwply_svr_flg;      /* 0x64 handle server running */
	Sint32 sfd_svr_flg;        /* 0x68 SFD_ExecOne running */
	Sint32 dec_svr_flg;        /* 0x6C */
	Sint32 x70;
	Sint8 linkstm;             /* 0x74 */
	Sint8 linkstm_req;         /* 0x75 */
	Sint8 pause_flg;           /* 0x76 */
	Sint8 pad77;
	Sint32 x78;                /* 0x78 cleared by mwSfdStop */
	void *curfrm;              /* 0x7C frame handed out by mwPlyGetCurFrm */
	Sint32 ngetfrm;            /* 0x80 frames got */
	Sint32 nrelfrm;            /* 0x84 frames released */
	Sint32 nskipdisp;          /* 0x88 frames skipped by mwPlyGetCurFrm */
	Sint32 pic_struct;         /* 0x8C */
	Sint32 chroma_format;      /* 0x90 */
	Sint32 x94;
	Sint32 x98;
	Sint32 x9c;
	Sint32 chromapos_h;        /* 0xA0 */
	Sint32 chromapos_v;        /* 0xA4 */
	Sint32 xa8;
	SFX_OBJ *sfx;              /* 0xAC */
	Sint32 xb0;
	Sint32 xb4;
	Sint32 sfh_cnt;            /* 0xB8 Sofdec headers seen */
	Sint32 sfh_cur;            /* 0xBC header of the current frame */
	Sint32 sfh_wr;             /* 0xC0 next sfhinf slot */
	MWSFFRM_SFHINF sfhinf[MWSFFRM_SFHINF_NUM]; /* 0xC4 */
	Sint32 x164;               /* 0x164 (picusr_ptr == &x164: no picture user data) */
	Uint8 pad168[0x17C - 0x168];
	void *picusr_ptr;          /* 0x17C */
	void *picusr_buf;          /* 0x180 picture user data copy */
	Sint32 picusr_bsize;       /* 0x184 */
	void *picusr_dat;          /* 0x188 */
	Sint32 picusr_len;         /* 0x18C */
	SJ ainf_sj;                /* 0x190 additional-info (tag) stream joint */
	void *ainf_buf;            /* 0x194 */
	Sint32 ainf_bsize;         /* 0x198 */
	void *addinf_buf;          /* 0x19C user buffer receiving the tag block */
	Sint32 x1a0;
	Sint32 tag_x1a4;           /* 0x1A4 (-1) */
	Sint32 tag_flg;            /* 0x1A8 tag block analysed */
	void *tag_ptr;             /* 0x1AC */
	Sint32 tag_size;           /* 0x1B0 */
	Sint32 x1b4;
	const Char8 *fname;        /* 0x1B8 */
	Sint32 x1bc;
	Sint32 stm_start_req;      /* 0x1C0 */
	void *dir;                 /* 0x1C4 */
	Sint32 ofst;               /* 0x1C8 */
	Sint32 nsct;               /* 0x1CC */
	SJ sji;                    /* 0x1D0 input stream joint of the current play */
	SJ file_sj;                /* 0x1D4 stream joint fed by the file stream */
	Sint32 x1d8;
	Sint32 flow_nsct;          /* 0x1DC (MWSFPLY_SetFlowLimit: 80% of it) */
	Sint32 x1e0;
	Sint32 x1e4;               /* 0x1E4 (2 at mwSfdStartSj) */
	Sint32 x1e8;
	Sint32 x1ec;
	Sint32 x1f0;
	SJ mem_sj;                 /* 0x1F4 memory stream joint (mwSfdStartMem) */
	void *mem_buf;             /* 0x1F8 */
	Sint32 mem_size;           /* 0x1FC */
	Uint8 pad200[0x294 - 0x200];
	MWSST_OBJ sst;             /* 0x294 */
	Sint32 x2ac;
	Sint32 x2b0;
	Sint32 x2b4;               /* 0x2B4 cleared when the decoder is stopped */
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
Sint32 mwSfdGetStat(MWPLY mwply);
Sint32 MWSFRNA_GetOutPan(MWPLY_OBJ *mwply, Sint32 ch);
void MWSFRNA_SetOutPan(MWPLY_OBJ *mwply, Sint32 ch, Sint32 pan);
Sint32 MWSFRNA_GetOutVol(MWPLY_OBJ *mwply);
void MWSFRNA_SetOutVol(MWPLY_OBJ *mwply, Sint32 vol);
void MWSTM_SetFlowLimit(void *stm, Sint32 min_nsct, Sint32 max_nsct);
void MWSFLSC_SetFlowLimit(MWPLY_OBJ *mwply, Sint32 nsct);
Sint32 MWSFLIB_SetErrCode(Sint32 code);
void MWSFLIB_SfdErrFunc(void *obj, Sint32 code);
void MWSFD_SetCond(MWPLY mwply, Sint32 id, Sint32 val);
SFX_OBJ *MWSFSFX_GetSfxHn(MWPLY_OBJ *mwply);
void MWSFSFX_CnvFrmInfToSfx(MWPLY_OBJ *mwply, MWS_FRM *frm, SFX_FRM *sfxfrm);
void SFX_CnvFrmY84C44(SFX_OBJ *sfx, SFX_FRM *frm, void *ybuf, void *cbuf);
void SFX_CnvFrmARGB8888(SFX_OBJ *sfx, SFX_FRM *frm, void *buf);

#endif
