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

/* COMPILER-DIFF: M1 - SFXA_Create's original numbers its constants zero r8, 0x1F r5, 100 r0, 1 r7,
 * 0x7F r6, 0xFF r0 and the sfxa_work address r4/r5 (sharing the dead 0x1F register); ours put the
 * address first. Every value is a hard-register asm pin (`asm { op rN, ..; mr var, rN }`, the `mr`
 * is coalesced away); a hard register written in asm is never used for a compiler temporary
 * elsewhere in the function, so the r0/r4 temporaries of the inlined search are pinned too. */
static SFXA_OBJ *sfxa_search_free(void)
{
	register SFXA_OBJ *sfxa;
	register SFXA_WORK *w;
	register Sint32 n; // COMPILER-DIFF: M1
	register Sint32 u; // COMPILER-DIFF: M1
	Sint32 i;

	w = &sfxa_work;
	asm { lwz r0, SFXA_WORK.nobj(w); mr n, r0 } // COMPILER-DIFF: M1
	sfxa = w->obj;
	for (i = 0; i < n; i++) {
		asm { lwz r0, SFXA_OBJ.used(sfxa); mr u, r0 } // COMPILER-DIFF: M1
		if (u == 0) {
			return sfxa;
		}
		sfxa++;
	}
	return NULL;
}

SFXA_OBJ *SFXA_Create(void)
{
	register SFXA_OBJ *sfxa;
	register Sint32 z; // COMPILER-DIFF: M1 (the pinned constants and temporaries, see sfxa_search_free)
	register Sint32 k1f;
	register Sint32 k100;
	register Sint32 one;
	register Sint32 k7f;
	register Sint32 kff;
	register SFXA_WORK *wp;
	register Sint32 c;

	sfxa = sfxa_search_free();
	if (sfxa == NULL) {
		return sfxa;
	}
	asm { li r8, 0; mr z, r8 } // COMPILER-DIFF: M1
	asm { li r5, 31; mr k1f, r5 } // COMPILER-DIFF: M1
	asm { li r0, 100; mr k100, r0 } // COMPILER-DIFF: M1
	asm { li r7, 1; mr one, r7 } // COMPILER-DIFF: M1
	asm { li r6, 127; mr k7f, r6 } // COMPILER-DIFF: M1
	sfxa->lumi_min = z;
	sfxa->lumi_max = k1f;
	sfxa->lumi_rate = k100;
	sfxa->need_update = one;
	sfxa->alp0 = z;
	sfxa->alp1 = k7f;
	asm { li r0, 255; mr kff, r0 } // COMPILER-DIFF: M1
	sfxa->alp2 = kff;
	asm { lis r4, sfxa_work@ha; addi r5, r4, sfxa_work@l; mr wp, r5 } // COMPILER-DIFF: M1
	asm { lwz r4, SFXA_WORK.cnt(wp); addi r0, r4, 1; mr c, r0 } // COMPILER-DIFF: M1 (sfxa_work.cnt++)
	wp->cnt = c;
	sfxa->used = one;
	return sfxa;
}

void SFXA_Init(void)
{
	memset(&sfxa_work, 0, sizeof(sfxa_work));
	sfxa_work.nobj = 8;
}
