/* CRI Stream Joint (SJ) interface: a handle is a pointer to an object whose first word is its
 * interface table. Slot offsets from the callers in the DOL (adx_amp, adx_insh, ...). */
#ifndef CRI_SJ_H
#define CRI_SJ_H

#include "cri_xpt.h"

typedef struct SJ_OBJ *SJ;

typedef struct {
	Uint8 *data;
	Sint32 len;
} SJCK;

typedef struct {
	void *(*QueryInterface)(SJ sj, void *iid);       /* 0x00 */
	Uint32 (*AddRef)(SJ sj);                          /* 0x04 */
	Uint32 (*Release)(SJ sj);                         /* 0x08 */
	void (*Destroy)(SJ sj);                           /* 0x0C */
	void (*Reset)(SJ sj);                             /* 0x10 */
	void (*Reset2)(SJ sj);                            /* 0x14 */
	void (*GetChunk)(SJ sj, Sint32 id, Sint32 nbyte, SJCK *ck); /* 0x18 */
	void (*PutChunk)(SJ sj, Sint32 id, SJCK *ck);     /* 0x1C */
	void (*UngetChunk)(SJ sj, Sint32 id, SJCK *ck);   /* 0x20 */
	Sint32 (*GetNumData)(SJ sj, Sint32 id);           /* 0x24 */
	Sint32 (*IsGetChunk)(SJ sj, Sint32 id, Sint32 nbyte, Sint32 *rbyte); /* 0x28 */
	Sint32 (*IsPutChunk)(SJ sj, Sint32 id, Sint32 nbyte, Sint32 *rbyte); /* 0x2C */
	void (*EntryErrFunc)(SJ sj, void (*func)(void *obj, Char8 *msg), void *obj); /* 0x30 */
	void (*SetPrm)(SJ sj, Sint32 id, Sint32 prm);     /* 0x34 */
	Sint32 (*GetPrm)(SJ sj, Sint32 id);               /* 0x38 */
} SJ_IF;

struct SJ_OBJ {
	SJ_IF *vtbl;
};

#define SJ_Destroy(sj) (*(sj)->vtbl->Destroy)(sj)
#define SJ_Reset(sj) (*(sj)->vtbl->Reset)(sj)
#define SJ_Reset2(sj) (*(sj)->vtbl->Reset2)(sj)
#define SJ_GetChunk(sj, id, nbyte, ck) (*(sj)->vtbl->GetChunk)(sj, id, nbyte, ck)
#define SJ_PutChunk(sj, id, ck) (*(sj)->vtbl->PutChunk)(sj, id, ck)
#define SJ_UngetChunk(sj, id, ck) (*(sj)->vtbl->UngetChunk)(sj, id, ck)
#define SJ_GetNumData(sj, id) (*(sj)->vtbl->GetNumData)(sj, id)

SJ SJRBF_Create(void *buf, Sint32 bsize, Sint32 xsize);
SJ SJMEM_Create(void *buf, Sint32 bsize);
void SJCRS_Lock(void);
void SJCRS_Unlock(void);
void SJERR_CallErr(Char8 *msg);
void *SJ_SearchTag(void *inf, const Char8 *tag, const Char8 *name, void *out);

#endif
