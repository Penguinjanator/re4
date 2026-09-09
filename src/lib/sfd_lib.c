/* Sofdec: library init, handle check and error reporting */
#include "cri_xpt.h"
#include "sfd.h"

#define SFD_VERSION 0x3598

extern void SVM_Lock(void);
extern void SVM_Unlock(void);
extern void SJRBF_Init(void);
extern void UTY_MemsetDword(Uint32 *dst, Uint32 val, Uint32 ndw);
extern void MEM_Copy(void *dst, const void *src, Uint32 nbytes);
extern const Sint32 SFPLY_cond_dfl[SFD_COND_NUM];
extern void SFTIM_Init(void *work);
extern void SFBUF_Init(void *work);
extern Sint32 SFTRN_Init(SFTRN_TRIF_TBL *dst, SFTRN_TRIF_TBL *src);
extern void SFPLY_Init(void);
extern void SFHDS_Init(void);

const Char8 SFLIB_version_str[] = "\nCRI SFD/GC Ver.1.947 Build:Sep 22 2004 10:35:15\n\0Append: MW2407 GC20Apr2004Patch1\n";

static Sint32 sflib_sizeof_sfdhn = 0;
Char8 *SFD_pts_error_msg = NULL;
SFD sfd_hn_last = NULL;
static const Char8 *cri_verstr_ptr = NULL;
SFLIB_WORK SFLIB_libwork;

void SFLIB_UnlockCs(Sint32 *cs)
{
	SVM_Unlock();
}

void SFLIB_LockCs(Sint32 *cs)
{
	SVM_Lock();
}

Sint32 SFLIB_CheckHn(SFD sfd)
{
	sfd_hn_last = sfd;
	if (sfd == NULL) {
		return -1;
	}
	if (sfd->stat == 0) {
		return -1;
	}
	return 0;
}

static Sint32 sflib_SetErr(SFD sfd, Sint32 code)
{
	if (sfd == NULL) {
		if (SFLIB_libwork.err.code == 0) {
			SFLIB_libwork.err.code = code;
		}
		if (code != 0 && SFLIB_libwork.err.fn != NULL) {
			SFLIB_libwork.err.fn(SFLIB_libwork.err.obj, code);
		}
	} else {
		if (sfd->err.code == 0) {
			sfd->err.code = code;
		}
		if (code != 0 && sfd->err.fn != NULL) {
			sfd->err.fn(sfd->err.obj, code);
		}
		if (sfd->stat > 0) {
			sfd->stat = -sfd->stat;
		}
	}
	return code;
}

Sint32 SFD_SetErrFn(SFD sfd, void (*fn)(void *obj, Sint32 code), void *obj)
{
	if (sfd == NULL) {
		SFLIB_libwork.err.fn = fn;
		SFLIB_libwork.err.obj = obj;
	} else {
		if (SFLIB_CheckHn(sfd) != 0) {
			return sflib_SetErr(NULL, 0xFF000101);
		}
		sfd->err.fn = fn;
		sfd->err.obj = obj;
	}
	return 0;
}

Sint32 SFLIB_SetErr(SFD sfd, Sint32 code)
{
	if (code == 0) {
		return 0;
	}
	return sflib_SetErr(sfd, code);
}

void SFLIB_InitErrInf(SFLIB_ERRINF *err)
{
	err->fn = NULL;
	err->obj = NULL;
	err->code = 0;
	err->x0c = 0;
	err->x10 = 0;
}

static Sint32 sflib_ChkRet(Sint32 r)
{
	Sint32 ret;

	ret = 0;
	if (r != 0) {
		ret = r;
	}
	return ret;
}

Sint32 SFD_Init(SFD_INIT_PRM *prm)
{
	SFTRN_TRIF_TBL *tbl;
	Sint32 p1;
	Sint32 ret;
	Sint32 i;

	sflib_sizeof_sfdhn = SFD_VERSION;
	cri_verstr_ptr = SFLIB_version_str;
	SJRBF_Init();
	UTY_MemsetDword((Uint32 *)&SFLIB_libwork, 0, sizeof(SFLIB_libwork) / 4 - 1);
	MEM_Copy(&SFLIB_libwork, SFPLY_cond_dfl, sizeof(SFLIB_libwork.cond));
	tbl = prm->trif_tbl;
	p1 = prm->prm1;
	SFLIB_libwork.trif_tbl = tbl;
	SFLIB_libwork.prm1 = p1;
	SFLIB_libwork.x198 = 0;
	SFLIB_InitErrInf(&SFLIB_libwork.err);
	SFTIM_Init(SFLIB_libwork.tim);
	SFBUF_Init(&SFLIB_libwork.buf);
	SFLIB_libwork.x1fc = 0;
	SFLIB_libwork.x200 = 0;
	for (i = 0; i < 8; i++) {
		SFLIB_libwork.hn[i] = NULL;
	}
	ret = sflib_ChkRet(SFTRN_Init(&SFLIB_libwork.trif, prm->trif_tbl));
	if (ret != 0) {
		return ret;
	}
	SFPLY_Init();
	SFHDS_Init();
	return 0;
}

Sint32 SFD_IsVersionCompatible(Sint32 a, Sint32 ver)
{
	return ver == SFD_VERSION;
}
