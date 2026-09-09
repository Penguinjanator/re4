/* CRI Sofdec MPEG video decoder (MPV) object layout, from the field offsets used by the DOL. */
#ifndef CRI_MPV_H
#define CRI_MPV_H

#include "cri_xpt.h"
#include "sj.h"

typedef struct {
	void *p;
	Sint32 n;
} MPVCMC_REF;

typedef struct {
	void (*func)(void *obj, Sint32 code);
	void *obj;
	Sint32 code;
} MPVERR_INF;

typedef struct {
	Uint8 raw[0x80];
} MPV_PICATR;

/* frame parameter block passed to MPV_DecodeFrmSj (0x30 bytes) */
typedef struct {
	Uint8 pad0[0x1C];
	Sint16 width;        /* 0x1C */
	Sint16 height;       /* 0x1E */
	Uint8 pad20[4];
	MPV_PICATR *picatr;  /* 0x24 */
	Sint32 nfrm;         /* 0x28 */
	Sint32 nbyte;        /* 0x2C */
} MPV_FRM;

typedef struct MPV_OBJ *MPV;

typedef struct MPV_OBJ {
	Uint8 pad0[0xCC];
	Uint8 mc[0x120 - 0xCC];         /* 0x0CC */
	Sint32 ccnt_rt;                 /* 0x120 */
	MPVCMC_REF oi_rt[6];            /* 0x124 */
	Sint32 ccnt;                    /* 0x154 */
	MPVCMC_REF oi[6];               /* 0x158 */
	Uint8 pad188[0x1A4 - 0x188];
	Sint32 mcflag;                  /* 0x1A4 */
	Uint8 pad1A8[0x1D0 - 0x1A8];
	MPV_PICATR picatr;              /* 0x1D0 */
	MPVERR_INF errinf;              /* 0x250 */
	Sint32 nfrm_dec;                /* 0x25C */
	Sint32 nbyte_dec;               /* 0x260 */
	MPV_FRM frm;                    /* 0x264 */
	Uint8 pad294[0x2A8 - 0x294];
	Sint32 bitrate;                 /* 0x2A8 */
	Sint32 vbv_size;                /* 0x2AC */
	Sint32 pad2B0;
	Sint32 linkflg1;                /* 0x2B4 */
	Sint32 linkflg2;                /* 0x2B8 */
	Sint32 vbv_delay;               /* 0x2BC */
	Uint8 pad2C0[0x358 - 0x2C0];
	Sint32 m2v_mode;                /* 0x358 */
	Uint8 pad35C[0xD00 - 0x35C];
	Uint8 work[1];                  /* 0xD00 */
} MPV_OBJ;

Sint32 MPVLIB_CheckHn(MPV_OBJ *mpv);
Sint32 MPVERR_SetCode(MPV_OBJ *mpv, Sint32 code);

#define MPV_ERR_INVALID_HN 0xFF030200

#endif
