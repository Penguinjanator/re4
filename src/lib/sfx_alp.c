#include "cri_xpt.h"
#include <string.h>

typedef struct {
	Sint32 used;         /* 0x00 */
	Sint32 need_update;  /* 0x04 */
	Sint32 lumi_min;     /* 0x08 */
	Sint32 lumi_max;     /* 0x0C */
	Sint32 lumi_rate;    /* 0x10 */
	Uint8 alp0;          /* 0x14 */
	Uint8 alp1;          /* 0x15 */
	Uint8 alp2;          /* 0x16 */
	Uint8 pad17;
} SFXA_OBJ;

typedef struct {
	Sint32 cnt;
	Sint32 nobj;
	SFXA_OBJ obj[8];
} SFXA_WORK;

extern void CFT_MakeArgb8888Alp3211Tbl(void *tbl, Uint8 a0, Uint8 a1, Uint8 a2);
extern void CFT_MakeArgb8888Alp3110Tbl(void *tbl, Uint8 a0, Uint8 a1, Uint8 a2);
extern void CFT_MakeArgb8888AlpLumiTbl(Sint32 min, Sint32 max, Sint32 rate, void *tbl);

SFXA_WORK sfxa_work;

Sint32 SFXA_IsNeedUpdateLumiTbl(SFXA_OBJ *sfxa)
{
	return sfxa->need_update;
}

void SFXA_MakeAlp3211Tbl(SFXA_OBJ *sfxa, void *frm, void *tbl)
{
	CFT_MakeArgb8888Alp3211Tbl(tbl, sfxa->alp0, sfxa->alp1, sfxa->alp2);
}

void SFXA_MakeAlp3110Tbl(SFXA_OBJ *sfxa, void *frm, void *tbl)
{
	CFT_MakeArgb8888Alp3110Tbl(tbl, sfxa->alp0, sfxa->alp1, sfxa->alp2);
}

void SFXA_MakeAlpLumiTbl(SFXA_OBJ *sfxa, void *frm, void *tbl)
{
	CFT_MakeArgb8888AlpLumiTbl(sfxa->lumi_min, sfxa->lumi_max, sfxa->lumi_rate, tbl);
	sfxa->need_update = 0;
}

void SFXA_Destroy(SFXA_OBJ *sfxa)
{
	if (sfxa == NULL) {
		return;
	}
	sfxa->used = 0;
	sfxa_work.cnt--;
}

static SFXA_OBJ *sfxa_search_free(void)
{
	SFXA_OBJ *sfxa;
	Sint32 i;

	sfxa = sfxa_work.obj;
	for (i = 0; i < sfxa_work.nobj; i++) {
		if (sfxa->used == 0) {
			return sfxa;
		}
		sfxa++;
	}
	return NULL;
}

SFXA_OBJ *SFXA_Create(void)
{
	SFXA_OBJ *sfxa;

	sfxa = sfxa_search_free();
	if (sfxa == NULL) {
		return sfxa;
	}
	sfxa->lumi_min = 0;
	sfxa->lumi_max = 0x1F;
	sfxa->lumi_rate = 100;
	sfxa->need_update = 1;
	sfxa->alp0 = 0;
	sfxa->alp1 = 0x7F;
	sfxa->alp2 = 0xFF;
	sfxa_work.cnt++;
	sfxa->used = 1;
	return sfxa;
}

void SFXA_Init(void)
{
	memset(&sfxa_work, 0, sizeof(sfxa_work));
	sfxa_work.nobj = 8;
}
