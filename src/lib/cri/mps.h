/* CRI Sofdec MPEG system-stream demultiplexer (MPS) object layout, from the DOL's field offsets. */
#ifndef CRI_MPS_H
#define CRI_MPS_H

#include "cri_xpt.h"

typedef struct MPS_OBJ *MPS;

/* pack header (16 bytes, 8-byte aligned: holds the 64-bit SCR) */
typedef struct {
	Sint64 scr;
	Sint32 mux_rate;
	Sint32 rsv;
} MPS_PACKHD;

/* system header (32 bytes) */
typedef struct {
	Sint32 raw[8];
} MPS_SYSHD;

/* packet header (40 bytes, 8-byte aligned: holds the 64-bit PTS/DTS) */
typedef struct {
	Sint64 pts;
	Sint64 dts;
	Sint32 raw[6];
} MPS_PKETHD;

/* MPS_OBJ.used */
#define MPS_HN_FREE 1
#define MPS_HN_USED 2

typedef struct MPS_OBJ {
	Sint32 used;              /* 0x00 */
	void (*errfn)(void *obj); /* 0x04 */
	void *errobj;             /* 0x08 */
	Sint32 errcode;           /* 0x0C */
	Sint32 x10;               /* 0x10 (2) */
	Sint32 x14;               /* 0x14 */
	MPS_PACKHD packhd;        /* 0x18 */
	MPS_SYSHD last_syshd;     /* 0x28 */
	MPS_SYSHD syshd[3];       /* 0x48 */
	MPS_PKETHD pkethd;        /* 0xA8 */
	Sint32 xd0;               /* 0xD0 */
	Sint32 (*dechd_func)();   /* 0xD4 */
	Sint32 xd8;               /* 0xD8 */
	Sint32 xdc;               /* 0xDC */
	Sint32 xe0;               /* 0xE0 */
	Sint32 xe4;               /* 0xE4 */
	Sint32 xe8;               /* 0xE8 */
	Uint8 padEC[0x100 - 0xEC];
} MPS_OBJ;

/* library work (mps_lib.c): header + num_hn handles */
typedef struct {
	void (*errfn)(void *obj); /* 0x00 */
	void *errobj;             /* 0x04 */
	Sint32 errcode;           /* 0x08 */
	Sint32 num_hn;            /* 0x0C */
	MPS_OBJ hn[1];            /* 0x10 */
} MPSLIB_WORK;

extern MPSLIB_WORK *MPSLIB_libwork;

Sint32 MPSLIB_CheckHn(MPS mps);
Sint32 MPSLIB_SetErr(MPS mps, Sint32 code);

#endif
