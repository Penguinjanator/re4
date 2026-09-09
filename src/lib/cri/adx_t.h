/* CRI ADXT (ADX talk: stream playback handle) internals shared by the adx_*.c units.
 * ADXT_OBJ is 0xC0 bytes (adxt_obj[16] = 0xC00); only the fields seen so far are named. */
#ifndef CRI_ADX_T_H
#define CRI_ADX_T_H

#include "cri_xpt.h"
#include "sj.h"

#define ADXT_MAX_OBJ 16

typedef struct {
	Uint8 pad00[0x14];
	SJ sji;               /* 0x14 */
	Uint8 pad18[0xC0 - 0x18];
} ADXT_OBJ;

typedef ADXT_OBJ *ADXT;

extern ADXT_OBJ adxt_obj[ADXT_MAX_OBJ];
extern const Char8 adxt_build[];
extern Sint32 adxt_init_cnt;
extern Sint32 adxt_svr_main_id;
extern Sint32 adxt_output_mono_flag;
extern Sint32 adxt_svr_fs_id;
extern Sint32 adxt_vsync_svr_flag;

void ADXT_Init(void);
void ADXT_Finish(void);
void ADXT_ExecServer(void);
void ADXT_ExecFsSvr(void);
void ADXT_DestroyAll(void);
void ADXT_SetDefSvrFreq(Sint32 freq);

#endif
