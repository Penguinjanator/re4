/* CRI Sofdec MW player (MWSFD) internals shared by the mwsfd* and mwsfx_* units. */
#ifndef CRI_MWSFD_H
#define CRI_MWSFD_H

#include "cri_xpt.h"

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

/* library work (mwsfdlib.c, 0x162C bytes) */
typedef struct {
	Sint32 x00;                /* 0x00 */
	Float32 vfreq;             /* 0x04 */
	Sint32 x08;                /* 0x08 */
	Sint32 nfrm_pool;          /* 0x0C */
	Sint32 x10;                /* 0x10 */
	Uint8 pad14[0x38 - 0x14];
	Sint32 use_picusr;         /* 0x38 */
	Sint32 pause_bdr;          /* 0x3C */
	Uint8 pad40[0x5C - 0x40];
	Sint32 x5c;                /* 0x5C */
	Uint8 pad60[0x68 - 0x60];
	Sint32 errcode;            /* 0x68 */
	Uint8 pad6c[0x162C - 0x6C];
} MWSFD_LIBWORK;

extern MWSFD_LIBWORK mwsfd_libwork;
extern Sint32 mwg_vcnt;

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
