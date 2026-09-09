/* CRI ADXT (ADX talk: stream playback handle) internals shared by the adx_*.c units.
 * ADXT_OBJ is 0xC0 bytes (adxt_obj[16] = 0xC00); only the fields seen so far are named. */
#ifndef CRI_ADX_T_H
#define CRI_ADX_T_H

#include "cri_xpt.h"
#include "sj.h"

#define ADXT_MAX_OBJ 16
#define ADXT_MAX_NCH 8

/* ADXT_OBJ.stat (internal) */
#define ADXT_ISTAT_STOP 0
#define ADXT_ISTAT_DECINFO 1
#define ADXT_ISTAT_PREP 2
#define ADXT_ISTAT_PLAYING 3
#define ADXT_ISTAT_PLAYEND_WAIT 4
#define ADXT_ISTAT_PLAYEND 5
#define ADXT_ISTAT_ERROR 6

typedef struct {
	Sint8 used;           /* 0x00 */
	Sint8 stat;           /* 0x01 ADXT_ISTAT_* */
	Sint8 mode;           /* 0x02 input kind (0/1: file stream, 2: memory, 3: stream joint) */
	Sint8 maxnch;         /* 0x03 */
	void *sjd;            /* 0x04 ADXSJD decoder */
	void *stm;            /* 0x08 ADXSTM stream controller */
	void *rna;            /* 0x0C ADXRNA renderer (SFADXT_Create copies it into SFAOAP.x00) */
	SJ sjf;               /* 0x10 file stream joint */
	SJ sji;               /* 0x14 decoder input stream joint */
	SJ sjo[ADXT_MAX_NCH]; /* 0x18 decoder output stream joints (one per channel) */
	Sint32 svrfreq;       /* 0x38 server calls per second */
	Sint32 x3c;
	Sint16 outvol;        /* 0x40 */
	Sint16 outpan[2];     /* 0x42 */
	Sint16 x46;
	Sint32 maxdecsmpl;    /* 0x48 samples decoded per server call */
	Sint32 lpcnt;         /* 0x4C loops done */
	Sint32 lpendmod;      /* 0x50 loop end offset within its sector */
	Uint8 pad54[0x60 - 0x54];
	Sint16 errcode;       /* 0x60 */
	Uint8 pad62[0x6C - 0x62];
	Sint8 lpsw;           /* 0x6C loop switch */
	Uint8 pad6d[0x70 - 0x6D];
	Sint8 pausesw;        /* 0x70 */
	Sint8 x71;
	Sint8 x72;
	Sint8 x73;
	void *amp;            /* 0x74 ADXAMP */
	Uint8 pad78[0x8C - 0x78];
	Sint32 lpendsct;      /* 0x8C loop end in sectors */
	Sint32 trapnsmpl;     /* 0x90 */
	void *lsc;            /* 0x94 load scheduler handle */
	Sint8 lnksw;          /* 0x98 link (concatenated file) switch */
	Uint8 pad99[0x9C - 0x99];
	Sint32 x9c;
	Sint32 startvsync;    /* 0xA0 adxt_vsync_cnt at start */
	Sint32 decsmpl;       /* 0xA4 samples decoded before the current (linked) file */
	Sint8 stmstart;       /* 0xA8 start the stream when the decoder is ready */
	Uint8 pada9[0xB0 - 0xA9];
	void *stm_fname;      /* 0xB0 pending adxt_start_stm arguments */
	void *stm_dir;        /* 0xB4 */
	Sint32 stm_ofst;      /* 0xB8 */
	Sint32 stm_nsct;      /* 0xBC */
} ADXT_OBJ;

typedef ADXT_OBJ *ADXT;

extern ADXT_OBJ adxt_obj[ADXT_MAX_OBJ];
extern const Char8 adxt_build[];
extern Sint32 adxt_init_cnt;
extern Sint32 adxt_svr_main_id;
extern Sint32 adxt_output_mono_flag;
extern Sint32 adxt_svr_fs_id;
extern Sint32 adxt_vsync_svr_flag;
extern Sint32 adxt_vsync_cnt;

void ADXT_Init(void);
void ADXT_Finish(void);
void ADXT_ExecServer(void);
void ADXT_ExecFsSvr(void);
void ADXT_DestroyAll(void);
void ADXT_SetDefSvrFreq(Sint32 freq);

#endif
