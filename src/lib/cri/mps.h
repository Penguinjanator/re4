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

typedef struct MPS_OBJ {
	Uint8 pad0[0x18];
	MPS_PACKHD packhd;        /* 0x18 */
	MPS_SYSHD last_syshd;     /* 0x28 */
	MPS_SYSHD syshd[3];       /* 0x48 */
	MPS_PKETHD pkethd;        /* 0xA8 */
} MPS_OBJ;

Sint32 MPSLIB_CheckHn(MPS mps);
Sint32 MPSLIB_SetErr(MPS mps, Sint32 code);

#endif
