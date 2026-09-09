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

Bool MWSFD_IsEnableHndl(MWPLY_OBJ *mwply);
void MWSFSVM_Error(const Char8 *fmt, ...);
SFX_OBJ *MWSFSFX_GetSfxHn(MWPLY_OBJ *mwply);
void MWSFSFX_CnvFrmInfToSfx(MWPLY_OBJ *mwply, MWS_FRM *frm, SFX_FRM *sfxfrm);
void SFX_CnvFrmY84C44(SFX_OBJ *sfx, SFX_FRM *frm, void *ybuf, void *cbuf);
void SFX_CnvFrmARGB8888(SFX_OBJ *sfx, SFX_FRM *frm, void *buf);

#endif
