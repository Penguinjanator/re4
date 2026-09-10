/* CRI RNA (renderer) ARAM resource handles */
#include "cri_xpt.h"
#include <string.h>
#include <dolphin/ar.h>

extern void RNAERR_CallErrFunc(Char8 *msg);

#define RNARES_MAX_OBJ 32
#define RNARES_DEF_ARAM_SIZE 0x40000
#define RNARES_BUF_SIZE 0x2000

typedef struct {
	Sint32 used;   /* 0x0 */
	Uint32 buf;    /* 0x4 ARAM address / 2 */
	Uint32 size;   /* 0x8 */
} RNARES_OBJ;

typedef RNARES_OBJ *RNARES;

static Uint32 rnares_init_cnt = 0;
Uint32 rnares_setup_fg = 0;
static Uint32 rnares_nbuf = 0;
Uint32 rnares_aram_size = 0;
Uint32 rnares_aram_ptr = 0;
static RNARES_OBJ rnares_obj[RNARES_MAX_OBJ];

Sint32 RNARES_GetBufSize(RNARES res)
{
	if (res == NULL) {
		return 0;
	}
	return res->size;
}

Uint32 RNARES_GetBuf(RNARES res)
{
	if (res == NULL) {
		return 0;
	}
	return res->buf;
}

void RNARES_Destroy(RNARES res)
{
	if (res != NULL) {
		res->used = 0;
	}
}

RNARES RNARES_Create(void)
{
	RNARES res;
	Sint32 i;

	for (i = 0; i < RNARES_MAX_OBJ; i++) {
		if (rnares_obj[i].used == 0) {
			break;
		}
	}
	if (i == RNARES_MAX_OBJ) {
		RNAERR_CallErrFunc("E1070313:Not enough RNARES handle.\n");
		return NULL;
	}
	res = &rnares_obj[i];
	res->used = 1;
	return res;
}

void RNARES_Finish(void)
{
	Sint32 i;
	Uint32 size;

	if (--rnares_init_cnt == 0) {
		for (i = 0; i < RNARES_MAX_OBJ; i++) {
			if (rnares_obj[i].used == 1) {
				RNARES_Destroy(&rnares_obj[i]);
			}
		}
		memset(rnares_obj, 0, sizeof(rnares_obj));
		if (rnares_setup_fg == 0) {
			ARFree(&size);
			if (size != rnares_aram_size) {
				RNAERR_CallErrFunc("E1090601:Free area other than ADX buffer.\n");
			}
			rnares_nbuf = 0;
			rnares_aram_size = 0;
			rnares_aram_ptr = 0;
		}
	}
}

/* dead-stripped by the linker */
void RNARES_Setup(Uint32 aram_ptr, Sint32 aram_size)
{
	if (aram_ptr == 0) {
		RNAERR_CallErrFunc("E1070310:Illigal parameter(aram_ptr=null).\n");
		return;
	}
	if (aram_size <= 0) {
		RNAERR_CallErrFunc("E1070311:Illigal parameter(aram_size<=0).\n");
		return;
	}
	if (aram_size < RNARES_BUF_SIZE) {
		RNAERR_CallErrFunc("E1070312:Not enough aram_size.\n");
		return;
	}
	rnares_setup_fg = 1;
	rnares_aram_ptr = aram_ptr;
	rnares_aram_size = aram_size;
	rnares_nbuf = aram_size / RNARES_BUF_SIZE;
	if (rnares_nbuf > RNARES_MAX_OBJ) {
		rnares_nbuf = RNARES_MAX_OBJ;
	}
}

void RNARES_Init(void)
{
	/* COMPILER-DIFF: M1 -- the target numbers the loop's volatiles ofs r4, 0x1000 r5, ptr+ofs r6, ptr r7,
	 * res r8; with the two compiler temporaries (the hoisted constant and the sum) named as asm-defined
	 * `register` locals every volatile is numbered in declaration order from r4. The zero-code form
	 * (`res->size = RNARES_BUF_SIZE / 2; res->buf = (ptr + ofs) >> 1`) ranks the temporaries first. */
	register Uint32 ofs;
	register Uint32 half;
	register Uint32 sum;
	register Uint32 ptr;
	Uint32 i;
	Uint32 n;
	RNARES_OBJ *res;

	if (rnares_init_cnt == 0) {
		if (rnares_setup_fg == 0) {
			rnares_nbuf = RNARES_MAX_OBJ;
			rnares_aram_size = RNARES_DEF_ARAM_SIZE;
			rnares_aram_ptr = ARAlloc(RNARES_DEF_ARAM_SIZE);
		}
		memset(rnares_obj, 0, sizeof(rnares_obj));
		n = rnares_nbuf;
		res = rnares_obj;
		ptr = rnares_aram_ptr;
		ofs = 0;
		asm { li half, RNARES_BUF_SIZE / 2 }
		for (i = 0; i < n; i++, res++) {
			asm { add sum, ptr, ofs }
			res->buf = sum >> 1;
			res->size = half;
			ofs += RNARES_BUF_SIZE;
		}
	}
	rnares_init_cnt++;
}
