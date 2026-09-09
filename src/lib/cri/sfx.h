/* CRI Sofdec SFX (frame conversion / effects) object layout, from the field offsets used by the
 * DOL. The matched sfx_set/sfx_cnv_to_* units carry their own partial copies of this struct. */
#ifndef CRI_SFX_H
#define CRI_SFX_H

#include "cri_xpt.h"

/* SFX_OBJ.compo: colour component layout ids */
#define SFX_COMPO_YCC420PLN 0x11
#define SFX_COMPO_YCC420PLN_UPHALF 0x21
#define SFX_COMPO_0x31 0x31
#define SFX_COMPO_0x41 0x41
#define SFX_COMPO_0x51 0x51
#define SFX_COMPO_0x61 0x61
#define SFX_COMPO_0x71 0x71
#define SFX_COMPO_0xF1 0xF1
#define SFX_COMPO_0x101 0x101
#define SFX_COMPO_0x111 0x111
#define SFX_COMPO_0x1001 0x1001

typedef struct {
	Sint32 pad0;               /* 0x00 */
	Sint32 compo;              /* 0x04 */
	Sint32 fxtype;             /* 0x08 */
	Sint32 outbuf_width;       /* 0x0C */
	Sint32 outbuf_height;      /* 0x10 */
	Sint32 unit_width;         /* 0x14 */
	Sint32 taginf_flg;         /* 0x18 */
	Sint32 tag_a;              /* 0x1C */
	Sint32 tag_b;              /* 0x20 */
	Sint32 pad24;              /* 0x24 */
	void *sfxz;                /* 0x28 */
	Sint32 pad2C[3];           /* 0x2C */
	void *coladj;              /* 0x38 */
} SFX_OBJ;

/* one colour plane of a decoded frame */
typedef struct {
	void *buf;                 /* 0x00 */
	Sint32 pitch;              /* 0x04 */
	Sint32 height;             /* 0x08 */
	Sint32 x0c;                /* 0x0C */
} SFX_PLN;

/* frame description (0x88 bytes) */
typedef struct {
	Sint32 frmfmt;             /* 0x00 */
	SFX_PLN pln[3];            /* 0x04 Y, Cb, Cr */
	Uint8 pad34[0x88 - 0x34];
} SFX_FRM;

/* CFT (colour format transform) planar source description */
typedef struct {
	void *y;                   /* 0x00 */
	void *cb;                  /* 0x04 */
	void *cr;                  /* 0x08 */
	Sint32 ypitch;             /* 0x0C */
	Sint32 cbpitch;            /* 0x10 */
	Sint32 crpitch;            /* 0x14 */
	Sint32 pad18[4];           /* 0x18 (frame size only) */
} CFT_YCC420PLN;

Sint32 sfxcnv_IsCnvUpHalf(SFX_OBJ *sfx);
void SFXLIB_Error(SFX_OBJ *sfx, SFX_FRM *frm, const Char8 *msg);
Sint32 SFXINF_GetStmInf(SFX_FRM *frm, const Char8 *tag);
void CFT_Ycc420plnToY84C44(CFT_YCC420PLN *src, void *ybuf, void *cbuf, Sint32 width, Sint32 height);

#endif
