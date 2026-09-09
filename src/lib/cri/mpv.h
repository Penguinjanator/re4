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

/* run/level VLC table selector (mpv_bdec.c) */
typedef struct {
	Uint32 *tbl;
	Sint32 bits;
} MPV_RUNLEVEL;

typedef struct MPV_OBJ *MPV;

/* block decoder parameters (mpv_cdec.c -> intra/nintra block decode function) */
typedef struct {
	Uint8 pad0[0x1C];
	Float64 *dst;                   /* 0x1C block coefficient buffer */
	Uint8 *iqm;                     /* 0x20 quantiser matrix */
	Sint32 qscale;                  /* 0x24 */
	Sint32 *dcpred;                 /* 0x28 intra DC predictor of the component */
	void *dctbl;                    /* 0x2C intra DC size table */
	Sint32 nintra;                  /* 0x30 */
} MPV_BLKPRM;

typedef Sint32 (*MPV_BLKDEC_FUNC)(MPV mpv, MPV_BLKPRM *prm);

typedef struct MPV_OBJ {
	Uint8 pad0[0x44];
	MPV_BLKPRM blkprm;              /* 0x044 */
	Sint8 cbp[6];                   /* 0x078 per-block coded flags */
	Uint8 pad7E[0xA0 - 0x7E];
	Sint32 cbp_msk;                 /* 0x0A0 */
	Uint8 padA4[0xCC - 0xA4];
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
	Uint8 pad2C0[0x2E8 - 0x2C0];
	Sint32 qscale;                  /* 0x2E8 */
	Uint8 pad2EC[0x348 - 0x2EC];
	Sint32 cbp_code;                /* 0x348 coded block pattern */
	Sint32 dcpred[3];               /* 0x34C */
	Sint32 m2v_mode;                /* 0x358 */
	Uint8 pad35C[0x680 - 0x35C];
	Float64 blk[6][32];             /* 0x680 six 0x100-byte coefficient blocks */
	Uint8 intra_iqm[64];            /* 0xC80 */
	Uint8 nintra_iqm[64];           /* 0xCC0 */
	Uint8 work[0x1100 - 0xD00];     /* 0xD00 */
	Uint16 bitmsk[16];              /* 0x1100 */
	Sint8 zigzag[64];               /* 0x1120 */
	Uint8 pad1160[0x1260 - 0x1160];
	Uint8 group_tbl[32];            /* 0x1260 */
	MPV_RUNLEVEL rl[6];             /* 0x1280 */
	Uint8 pad12B0[0x1318 - 0x12B0];
	MPV_BLKDEC_FUNC intra_func;     /* 0x1318 */
	MPV_BLKDEC_FUNC nintra_func;    /* 0x131C */
	Sint32 pad1320[2];
	void *dctbl_y;                  /* 0x1328 */
	void *dctbl_c;                  /* 0x132C */
} MPV_OBJ;

Sint32 MPVLIB_CheckHn(MPV_OBJ *mpv);
Sint32 MPVERR_SetCode(MPV_OBJ *mpv, Sint32 code);

#define MPV_ERR_INVALID_HN 0xFF030200

#endif
