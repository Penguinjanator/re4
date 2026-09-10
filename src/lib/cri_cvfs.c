/* CRI CVFS (virtual file system) front end (cri_cvfs.c, "CVFS/GC Ver.2.37"): a table of named
 * devices (their xxCiGetInterface tables), a default device, file handles pairing a device table
 * with the device's handle, and the "DEV:path" name split. Most of the API (volumes, directories,
 * write, option functions, ...) was dead-stripped by the linker; the strings remain in .rodata. */
#include "cri_xpt.h"
#include <string.h>
#include <stdio.h>
#include "cvfs.h"

#define CVFS_MAX_DEV 32
#define CVFS_MAX_HN 40
#define CVFS_NAME_LEN 0x129
#define CVFS_DEVNAME_LEN 12

typedef void (*CVFS_USRERRFN)(void *obj, const Char8 *msg, void *hn);
typedef CVFS_IF *(*CVFS_GETIFFN)(void);

/* device interface table (CVFS_IF) with its option function slot */
typedef struct {
	void (*ExecServer)(void);                                       /* 0x00 */
	void (*EntryErrFunc)(CVFS_ERRFUNC func, void *obj);             /* 0x04 */
	Sint32 (*GetFileSize)(const Char8 *fname);                      /* 0x08 */
	void *x0c;
	void *(*Open)(const Char8 *fname, void *dir, Sint32 rw);        /* 0x10 */
	void (*Close)(void *hn);                                        /* 0x14 */
	Sint32 (*Seek)(void *hn, Sint32 pos, Sint32 type);              /* 0x18 */
	Sint32 (*Tell)(void *hn);                                       /* 0x1C */
	Sint32 (*ReqRd)(void *hn, Sint32 nsct, void *buf);              /* 0x20 */
	void *x24;
	void (*StopTr)(void *hn);                                       /* 0x28 */
	Sint32 (*GetStat)(void *hn);                                    /* 0x2C */
	Sint32 (*GetSctLen)(void *hn);                                  /* 0x30 */
	void (*SetSctLen)(void *hn, Sint32 sctlen);                     /* 0x34 */
	Sint32 (*GetNumTr)(void *hn);                                   /* 0x38 */
	void *x3c[9];                                                   /* 0x3C */
	Sint32 (*OptFn)(void *hn, Sint32 fnid, Sint32 a, Sint32 b);     /* 0x60 */
	void *x64;
} CVFS_DEVIF;

typedef struct {
	CVFS_DEVIF *vtbl;          /* 0x00 */
	Char8 name[CVFS_DEVNAME_LEN]; /* 0x04 */
} CVFS_DEV;

typedef struct {
	CVFS_DEVIF *vtbl;          /* 0x00 */
	void *hn;                  /* 0x04 the device's handle */
} CVFS_OBJ;

CVFS_USRERRFN cvfs_errfn;
static void *cvfs_errobj;
Sint32 cvfs_init_cnt;
Char8 add_dev_tmp[CVFS_NAME_LEN];
Char8 cvfs_defdev[9];
static CVFS_DEV cvfs_tbl[CVFS_MAX_DEV];
CVFS_OBJ cvfs_obj[CVFS_MAX_HN];

/* the build string is named so that the asm functions below can address the .rodata pool through
 * it (it is the first object of the section = the compiler's `...rodata.0` base) COMPILER-DIFF: M1 */
static const Char8 cvfs_build_str[] = "\nCVFS/GC Ver.2.37 Build:Oct  8 2004 13:31:51\n";
static const Char8 *const volatile cvfs_build = cvfs_build_str;

void cvFsCallUsrErrFn(void *obj, const Char8 *msg, void *hn);

/* dead */
void cvFsInit(void)
{
	cvfs_build;
	cvfs_errfn = NULL;
	cvfs_errobj = NULL;
	if (cvfs_init_cnt == 0) {
		add_dev_tmp[0] = '\0';
		cvfs_defdev[0] = '\0';
		memset(cvfs_tbl, 0, sizeof(cvfs_tbl));
		memset(cvfs_obj, 0, sizeof(cvfs_obj));
	}
	cvfs_init_cnt++;
}

/* dead */
void cvFsFinish(void)
{
	cvfs_init_cnt--;
}

/* the user error callback */
static void cvfs_Error(const Char8 *msg)
{
	if (cvfs_errfn != NULL) {
		cvfs_errfn(cvfs_errobj, msg, NULL);
	}
}

/* upper-case a name (in place) */
static void cvfs_StrUpr(Char8 *s)
{
	Uint32 i;
	Uint32 n;
	Char8 *p;

	n = strlen(s) + 1;
	p = s;
	for (i = 0; i < n; i++) {
		if (*p >= 'a' && *p <= 'z') {
			*p = *p - 0x20;
		}
		p++;
	}
}

/* is the name a registered device (its first `len` characters) */
static Sint32 cvfs_IsExistDev(const Char8 *name, Sint32 len)
{
	Sint32 i;
	CVFS_DEV *dev;

	for (i = 0; i < CVFS_MAX_DEV; i++) {
		dev = &cvfs_tbl[i];
		if (strncmp(name, dev->name, len) == 0) {
			return 1;
		}
	}
	return 0;
}

/* the device of a name (NULL: the default device) */
static CVFS_DEVIF *cvFsGetDevIf(CVFS_DEV *tbl, const Char8 *name)
{
	Uint32 i;
	Sint32 len;
	CVFS_DEV *dev;

	if (name == NULL) {
		name = cvfs_defdev;
	}
	len = strlen(name);
	dev = tbl;
	for (i = 0; i < CVFS_MAX_DEV; i++) {
		if (strncmp(name, dev->name, len) == 0) {
			return cvfs_tbl[i].vtbl;
		}
		dev++;
	}
	return NULL;
}

static CVFS_DEVIF *cvfs_SearchDev(CVFS_DEV *tbl, const Char8 *name)
{
	CVFS_DEV *dev;
	Uint32 i;
	Sint32 len;

	len = strlen(name);
	dev = tbl;
	for (i = 0; i < CVFS_MAX_DEV; i++) {
		if (strncmp(name, dev->name, len) == 0) {
			return cvfs_tbl[i].vtbl;
		}
		dev++;
	}
	return NULL;
}

/* the device's option function (fnid 100: the device wants the "DEV:path" form) */
static Sint32 cvfs_OptFn(CVFS_DEVIF *vtbl, void *hn, Sint32 fnid, Sint32 a, Sint32 b)
{
	Sint32 ret;

	if (vtbl == NULL) {
		ret = 0;
	} else if (vtbl->OptFn != NULL) {
		ret = vtbl->OptFn(hn, fnid, a, b);
	} else {
		ret = 0;
	}
	return ret;
}

/* "DEV:path" -> upper-cased device name and path (a one-character device is a drive letter and
 * stays in the path; no ':' means no device) */
static inline void cvfs_SplitFname(const Char8 *fname, Char8 *dev, Char8 *path)
{
	Sint32 i;
	Sint32 j;
	Sint32 n;

	if (fname == NULL) {
		return;
	}
	for (i = 0; i < CVFS_NAME_LEN; i++) {
		if (fname[i] == ':' || fname[i] == '\0') {
			break;
		}
		dev[i] = fname[i];
	}
	if (fname[i] == '\0') {
		dev[i] = '\0';
		memcpy(path, dev, strlen(dev) + 1);
		dev[0] = '\0';
		return;
	}
	dev[i] = '\0';
	i++;
	if (i == 2) {
		i = 0;
		dev[0] = '\0';
	}
	n = i;
	for (j = i; j < CVFS_NAME_LEN; j++) {
		if (fname[j] == '\0') {
			break;
		}
		path[n++ - i] = fname[j];
	}
	path[n - i] = '\0';
	cvfs_StrUpr(dev);
}

/* the default device name into dev (empty when none) */
static void cvfs_GetDefDev(Char8 *dev)
{
	Sint32 len;

	len = strlen(cvfs_defdev);
	if (cvfs_defdev[0] == '\0') {
		dev[0] = '\0';
	} else {
		memcpy(dev, cvfs_defdev, len + 1);
	}
}

/* the device of a split name: the name's device, else the default device (the path is then the
 * whole name); a device asking for it gets the "DEV:path" form. (OPEN: the original computes the
 * device table base once inside the first inlined search and copies it for the later ones; the
 * search helpers' loop registers differ - M1) */
static CVFS_DEVIF *cvfs_ResolveDev(const Char8 *fname, Char8 *dev, Char8 *path)
{
	CVFS_DEVIF *vtbl;
	CVFS_DEV *tbl = cvfs_tbl;

	if (dev[0] == '\0') {
		cvfs_GetDefDev(dev);
		if (dev[0] == '\0') {
			return NULL;
		}
	}
	if (cvfs_OptFn(cvFsGetDevIf(tbl, dev), NULL, 100, 0, 0) == 1) {
		strcpy(add_dev_tmp, path);
		sprintf(path, "%s:%s", dev, add_dev_tmp);
	}
	vtbl = cvfs_SearchDev(tbl, dev);
	if (vtbl == NULL) {
		cvfs_GetDefDev(dev);
		vtbl = cvfs_SearchDev(tbl, dev);
		if (vtbl == NULL) {
			vtbl = NULL;
		} else {
			strcpy(path, fname);
		}
	}
	return vtbl;
}

/* dead */
void cvFsSetDefVol(const Char8 *devname, const Char8 *volname)
{
	CVFS_DEVIF *vtbl;

	if (devname == NULL) {
		cvfs_Error("cvFsSetDefVol #1:illegal device name");
		return;
	}
	if (volname == NULL) {
		cvfs_Error("cvFsSetDefVol #2:illegal volume name");
		return;
	}
	vtbl = cvfs_SearchDev(cvfs_tbl, devname);
	if (vtbl == NULL) {
		cvfs_Error("cvFsSetDefVol #3:device not found");
		return;
	}
	cvfs_OptFn(vtbl, NULL, 1, (Sint32)volname, 0);
}

/* dead */
Sint32 cvFsGetVolumeInfo(const Char8 *devname, const Char8 *volname, void *inf)
{
	CVFS_DEVIF *vtbl;

	if (devname == NULL) {
		cvfs_Error("cvFsGetVolumeInfo #1:illegal device name");
		return 0;
	}
	if (volname == NULL) {
		cvfs_Error("cvFsGetVolumeInfo #2:illegal volume name");
		return 0;
	}
	vtbl = cvfs_SearchDev(cvfs_tbl, devname);
	if (vtbl == NULL) {
		cvfs_Error("cvFsGetVolumeInfo #3:device not found");
		return 0;
	}
	return cvfs_OptFn(vtbl, NULL, 2, (Sint32)volname, (Sint32)inf);
}

/* dead */
void cvFsDelVolume(const Char8 *devname, const Char8 *volname)
{
	CVFS_DEVIF *vtbl;

	if (devname == NULL) {
		cvfs_Error("cvFsDelVolume #1:illegal device name");
		return;
	}
	if (volname == NULL) {
		cvfs_Error("cvFsDelVolume #2:illegal volume name");
		return;
	}
	vtbl = cvfs_SearchDev(cvfs_tbl, devname);
	if (vtbl == NULL) {
		cvfs_Error("cvFsDelVolume #3:device not found");
		return;
	}
	cvfs_OptFn(vtbl, NULL, 3, (Sint32)volname, 0);
}

/* dead */
void cvFsAddVolumeEx(const Char8 *devname, const Char8 *volname, void *img)
{
	CVFS_DEVIF *vtbl;

	if (devname == NULL) {
		cvfs_Error("cvFsAddVolumeEx #1:illegal device name");
		return;
	}
	if (volname == NULL) {
		cvfs_Error("cvFsAddVolumeEx #2:illegal volume name");
		return;
	}
	if (img == NULL) {
		cvfs_Error("cvFsAddVolumeEx #3:illegal image handle");
		return;
	}
	vtbl = cvfs_SearchDev(cvfs_tbl, devname);
	if (vtbl == NULL) {
		cvfs_Error("cvFsAddVolumeEx #3:device not found");
		return;
	}
	cvfs_OptFn(vtbl, NULL, 4, (Sint32)volname, (Sint32)img);
}

/* dead */
void cvFsSetCurVolume(const Char8 *devname, void *img)
{
	CVFS_DEVIF *vtbl;

	if (devname == NULL) {
		cvfs_Error("cvFsSetCurVolume #1:illegal device name");
		return;
	}
	if (img == NULL) {
		cvfs_Error("cvFsSetCurVolume #2:illegal image handle");
		return;
	}
	vtbl = cvfs_SearchDev(cvfs_tbl, devname);
	if (vtbl == NULL) {
		cvfs_Error("cvFsSetCurVolume #3:device not found");
		return;
	}
	cvfs_OptFn(vtbl, NULL, 5, (Sint32)img, 0);
}

/* dead */
Sint32 cvFsOptFn2(CVFS_OBJ *obj, Sint32 fnid, Sint32 a, Sint32 b)
{
	if (obj == NULL) {
		cvfs_Error("cvFsOptFn2 #1:handle error");
		return 0;
	}
	if (obj->vtbl->OptFn == NULL) {
		cvfs_Error("cvFsOptFn2 #2:vtbl error");
		return 0;
	}
	return obj->vtbl->OptFn(obj->hn, fnid, a, b);
}

/* dead */
Sint32 cvFsOptFn1(CVFS_OBJ *obj, Sint32 fnid, Sint32 a)
{
	if (obj == NULL) {
		cvfs_Error("cvFsOptFn1 #1:handle error");
		return 0;
	}
	if (obj->vtbl->OptFn == NULL) {
		cvfs_Error("cvFsOptFn1 #2:vtbl error");
		return 0;
	}
	return obj->vtbl->OptFn(obj->hn, fnid, a, 0);
}

/* dead */
const Char8 *cvFsGetDevName(CVFS_OBJ *obj)
{
	Sint32 i;

	if (obj == NULL || obj->vtbl == NULL) {
		cvfs_Error("cvFsGetDevName #1:vtbl error");
		return NULL;
	}
	for (i = 0; i < CVFS_MAX_DEV; i++) {
		if (cvfs_tbl[i].vtbl == obj->vtbl) {
			return cvfs_tbl[i].name;
		}
	}
	return NULL;
}

/* dead */
Sint32 cvFsDeleteFile(const Char8 *fname)
{
	Char8 dev[CVFS_NAME_LEN];
	Char8 path[CVFS_NAME_LEN];
	CVFS_DEVIF *vtbl;

	if (fname == NULL) {
		cvfs_Error("cvFsDeleteFile #1:illegal file name");
		return 0;
	}
	cvfs_SplitFname(fname, dev, path);
	if (dev == NULL) {
		cvfs_Error("cvFsDeleteFile #2:illegal device name");
		return 0;
	}
	vtbl = cvfs_ResolveDev(fname, dev, path);
	if (vtbl == NULL) {
		cvfs_Error("cvFsDeleteFile #3:device not found");
		return 0;
	}
	if (vtbl->x64 == NULL) {
		cvfs_Error("cvFsDeleteFile #4:vtbl error");
		return 0;
	}
	return cvfs_OptFn(vtbl, NULL, 10, (Sint32)path, 0);
}

/* dead */
Sint32 cvFsRemoveDir(const Char8 *dname)
{
	Char8 dev[CVFS_NAME_LEN];
	Char8 path[CVFS_NAME_LEN];
	CVFS_DEVIF *vtbl;

	if (dname == NULL) {
		cvfs_Error("cvFsRemoveDir #1:illegal directory name");
		return 0;
	}
	cvfs_SplitFname(dname, dev, path);
	if (dev == NULL) {
		cvfs_Error("cvFsRemoveDir #2:illegal device name");
		return 0;
	}
	vtbl = cvfs_ResolveDev(dname, dev, path);
	if (vtbl == NULL) {
		cvfs_Error("cvFsRemoveDir #3:device not found");
		return 0;
	}
	if (vtbl->x64 == NULL) {
		cvfs_Error("cvFsRemoveDir #4:vtbl error");
		return 0;
	}
	return cvfs_OptFn(vtbl, NULL, 11, (Sint32)path, 0);
}

/* dead */
Sint32 cvFsMakeDir(const Char8 *dname)
{
	Char8 dev[CVFS_NAME_LEN];
	Char8 path[CVFS_NAME_LEN];
	CVFS_DEVIF *vtbl;

	if (dname == NULL) {
		cvfs_Error("cvFsMakeDir #1:illegal directory name");
		return 0;
	}
	cvfs_SplitFname(dname, dev, path);
	if (dev == NULL) {
		cvfs_Error("cvFsMakeDir #2:illegal device name");
		return 0;
	}
	vtbl = cvfs_ResolveDev(dname, dev, path);
	if (vtbl == NULL) {
		cvfs_Error("cvFsMakeDir #3:device not found");
		return 0;
	}
	if (vtbl->x64 == NULL) {
		cvfs_Error("cvFsMakeDir #4:vtbl error");
		return 0;
	}
	return cvfs_OptFn(vtbl, NULL, 12, (Sint32)path, 0);
}

/* dead */
Sint32 cvFsGetMaxByteRate(CVFS_OBJ *obj)
{
	if (obj == NULL) {
		cvfs_Error("cvFsGetMaxByteRate #1:handle error");
		return 0;
	}
	if (obj->vtbl->OptFn == NULL) {
		cvfs_Error("cvFsGetMaxByteRate #2:vtbl error");
		return 0;
	}
	return obj->vtbl->OptFn(obj->hn, 13, 0, 0);
}

/* dead */
Sint32 cvFsIsExistFile(const Char8 *fname)
{
	Char8 dev[CVFS_NAME_LEN];
	Char8 path[CVFS_NAME_LEN];
	CVFS_DEVIF *vtbl;

	cvfs_SplitFname(fname, dev, path);
	if (dev == NULL) {
		cvfs_Error("cvFsIsExistFile #2:illegal device name");
		return 0;
	}
	if (fname == NULL) {
		cvfs_Error("cvFsIsExistFile #1:illegal file name");
		return 0;
	}
	vtbl = cvfs_ResolveDev(fname, dev, path);
	if (vtbl == NULL) {
		cvfs_Error("cvFsIsExistFile #3:device not found");
		return 0;
	}
	if (vtbl->x64 == NULL) {
		cvfs_Error("cvFsIsExistFile #4:vtbl error");
		return 0;
	}
	return cvfs_OptFn(vtbl, NULL, 14, (Sint32)path, 0);
}

/* dead */
Sint32 cvFsChangeDir(const Char8 *dname)
{
	Char8 dev[CVFS_NAME_LEN];
	Char8 path[CVFS_NAME_LEN];
	CVFS_DEVIF *vtbl;

	if (dname == NULL) {
		cvfs_Error("cvFsChangeDir #1:illegal directory name");
		return 0;
	}
	cvfs_SplitFname(dname, dev, path);
	if (dev == NULL) {
		cvfs_Error("cvFsChangeDir #2:illegal device name");
		return 0;
	}
	vtbl = cvfs_ResolveDev(dname, dev, path);
	if (vtbl == NULL) {
		cvfs_Error("cvFsChangeDir #3:device not found");
		return 0;
	}
	if (vtbl->x64 == NULL) {
		cvfs_Error("cvFsChangeDir #4:vtbl error");
		return 0;
	}
	return cvfs_OptFn(vtbl, NULL, 15, (Sint32)path, 0);
}

/* dead */
Sint32 cvFsGetNumTr(CVFS_OBJ *obj)
{
	if (obj == NULL) {
		cvfs_Error("cvFsGetNumTr #1:handle error");
		return 0;
	}
	if (obj->vtbl->GetNumTr == NULL) {
		cvfs_Error("cvFsGetNumTr #2:vtbl error");
		return 0;
	}
	return obj->vtbl->GetNumTr(obj->hn);
}

/* dead */
void cvFsSetSctLen(CVFS_OBJ *obj, Sint32 sctlen)
{
	if (obj == NULL) {
		cvfs_Error("cvFsSetSctLen #3:handle error");
		return;
	}
	if (obj->vtbl->SetSctLen == NULL) {
		cvfs_Error("cvFsSetSctLen #4:vtbl error");
		return;
	}
	obj->vtbl->SetSctLen(obj->hn, sctlen);
}

/* dead */
Sint32 cvFsGetSctLen(CVFS_OBJ *obj)
{
	if (obj == NULL) {
		cvfs_Error("cvFsGetSctLen #1:handle error");
		return 0;
	}
	if (obj->vtbl->GetSctLen == NULL) {
		cvfs_Error("cvFsGetSctLen #2:vtbl error");
		return 0;
	}
	return obj->vtbl->GetSctLen(obj->hn);
}

/* dead */
Sint32 cvFsGetFreeSize(const Char8 *devname)
{
	CVFS_DEVIF *vtbl;

	vtbl = cvFsGetDevIf(cvfs_tbl, devname);
	if (vtbl == NULL) {
		cvfs_Error("cvFsGetFreeSize #5:device not found");
		return 0;
	}
	if (vtbl->x64 == NULL) {
		cvfs_Error("cvFsGetFreeSize #6:vtbl error");
		return 0;
	}
	return cvfs_OptFn(vtbl, NULL, 16, 0, 0);
}

/* dead */
Sint32 cvFsGetFileSizeByHndl(CVFS_OBJ *obj)
{
	if (obj == NULL) {
		cvfs_Error("cvFsGetFileSizeByHndl #1:illegal file handle");
		return 0;
	}
	return cvfs_OptFn(obj->vtbl, obj->hn, 17, 0, 0);
}

/* dead */
Sint32 cvFsGetFileSizeEx(const Char8 *fname, Sint32 *size)
{
	Char8 dev[CVFS_NAME_LEN];
	Char8 path[CVFS_NAME_LEN];
	CVFS_DEVIF *vtbl;

	if (fname == NULL) {
		cvfs_Error("cvFsGetFileSizeEx #1:illegal file name");
		return 0;
	}
	cvfs_SplitFname(fname, dev, path);
	if (dev == NULL) {
		cvfs_Error("cvFsGetFileSizeEx #2:illegal device name");
		return 0;
	}
	vtbl = cvfs_ResolveDev(fname, dev, path);
	if (vtbl == NULL) {
		cvfs_Error("cvFsGetFileSizeEx #3:device not found");
		return 0;
	}
	if (vtbl->x64 == NULL) {
		cvfs_Error("cvFsGetFileSizeEx #4:vtbl error");
		return 0;
	}
	return cvfs_OptFn(vtbl, NULL, 18, (Sint32)path, (Sint32)size);
}

void cvFsEntryErrFunc(CVFS_USRERRFN func, void *obj)
{
	if (func == NULL) {
		cvfs_errfn = NULL;
		cvfs_errobj = NULL;
	} else {
		cvfs_errfn = func;
		cvfs_errobj = obj;
	}
}

/* COMPILER-DIFF: M1 - callee-saved permutation of the inlined device-search values. Asm function
 * (the original's instructions verbatim; the string pool is addressed through cvfs_build_str), C
 * body under #else. */
asm Sint32 cvFsGetFileSize(const Char8 *fname)
{
	nofralloc
	stwu r1, -656(r1)
	mflr r0
	lis r4, cvfs_build_str@ha
	stw r0, 660(r1)
	stmw r23, 620(r1)
	mr. r29, r3
	lis r3, cvfs_errfn@ha
	addi r30, r4, cvfs_build_str@l
	addi r31, r3, cvfs_errfn@l
	bne L84
	lwz r12, 0(r31)
	cmplwi r12, 0
	beq L7c
	addi r4, r30, 2084
	lwz r3, 4(r31)
	li r5, 0
	mtctr r12
	bctrl
L7c:
	li r3, 0
	b L4c4
L84:
	beq L1e8
	li r0, 99
	mr r3, r29
	addi r4, r1, 308
	li r6, 0
	mtctr r0
L9c:
	lbz r5, 0(r3)
	cmpwi r5, 58
	beq Lfc
	extsb. r0, r5
	beq Lfc
	stb r5, 0(r4)
	addi r6, r6, 1
	lbz r5, 1(r3)
	cmpwi r5, 58
	beq Lfc
	extsb. r0, r5
	beq Lfc
	stb r5, 1(r4)
	addi r6, r6, 1
	lbz r5, 2(r3)
	cmpwi r5, 58
	beq Lfc
	extsb. r0, r5
	beq Lfc
	stb r5, 2(r4)
	addi r4, r4, 3
	addi r6, r6, 1
	addi r3, r3, 3
	bdnz L9c
Lfc:
	lbzx r0, r29, r6
	extsb. r0, r0
	bne L138
	addi r3, r1, 308
	li r0, 0
	stbx r0, r3, r6
	bl strlen
	mr r5, r3
	addi r3, r1, 8
	addi r4, r1, 308
	addi r5, r5, 1
	bl memcpy
	li r0, 0
	stb r0, 308(r1)
	b L1e8
L138:
	addi r3, r1, 308
	li r0, 0
	stbx r0, r3, r6
	addi r6, r6, 1
	cmpwi r6, 2
	bne L158
	mr r6, r0
	stb r0, 308(r1)
L158:
	subfic r0, r6, 297
	mr r5, r6
	addi r4, r1, 8
	add r3, r29, r6
	mtctr r0
	cmpwi r6, 297
	bge L194
L174:
	lbz r7, 0(r3)
	extsb. r0, r7
	beq L194
	subf r0, r6, r5
	addi r3, r3, 1
	stbx r7, r4, r0
	addi r5, r5, 1
	bdnz L174
L194:
	subf r0, r6, r5
	addi r4, r1, 8
	li r5, 0
	addi r3, r1, 308
	stbx r5, r4, r0
	bl strlen
	addi r0, r3, 1
	addi r3, r1, 308
	mtctr r0
	cmplwi r0, 0
	ble L1e8
L1c0:
	lbz r4, 0(r3)
	extsb r0, r4
	cmpwi r0, 97
	blt L1e0
	cmpwi r0, 122
	bgt L1e0
	addi r0, r4, -32
	stb r0, 0(r3)
L1e0:
	addi r3, r3, 1
	bdnz L1c0
L1e8:
	lbz r0, 8(r1)
	extsb. r0, r0
	bne L21c
	lwz r12, 0(r31)
	cmplwi r12, 0
	beq L214
	addi r4, r30, 2084
	lwz r3, 4(r31)
	li r5, 0
	mtctr r12
	bctrl
L214:
	li r3, 0
	b L4c4
L21c:
	lbz r0, 308(r1)
	addi r28, r1, 308
	extsb. r0, r0
	bne L274
	addi r3, r31, 312
	bl strlen
	lbz r0, 312(r31)
	mr r5, r3
	extsb. r0, r0
	bne L250
	li r0, 0
	stb r0, 308(r1)
	b L260
L250:
	mr r3, r28
	addi r4, r31, 312
	addi r5, r5, 1
	bl memcpy
L260:
	lbz r0, 308(r1)
	extsb. r0, r0
	bne L274
	li r26, 0
	b L434
L274:
	cmplwi r28, 0
	mr r26, r28
	bne L284
	addi r26, r31, 312
L284:
	mr r3, r26
	bl strlen
	addi r27, r31, 324
	li r25, 0
	mr r24, r27
	mr r23, r3
L29c:
	mr r3, r26
	mr r5, r23
	addi r4, r24, 4
	bl strncmp
	cmpwi r3, 0
	bne L2c4
	slwi r0, r25, 4
	addi r3, r31, 324
	lwzx r3, r3, r0
	b L2d8
L2c4:
	addi r25, r25, 1
	addi r24, r24, 16
	cmplwi r25, 32
	blt L29c
	li r3, 0
L2d8:
	cmplwi r3, 0
	bne L2e8
	li r3, 0
	b L314
L2e8:
	lwz r12, 96(r3)
	cmplwi r12, 0
	beq L310
	li r3, 0
	li r4, 100
	li r5, 0
	li r6, 0
	mtctr r12
	bctrl
	b L314
L310:
	li r3, 0
L314:
	cmpwi r3, 1
	bne L340
	addi r3, r31, 12
	addi r4, r1, 8
	bl strcpy
	mr r5, r26
	addi r3, r1, 8
	addi r4, r30, 52
	addi r6, r31, 12
	crclr 4*cr1+eq
	bl sprintf
L340:
	mr r3, r28
	bl strlen
	mr r23, r27
	li r26, 0
	mr r24, r3
L354:
	mr r3, r28
	mr r5, r24
	addi r4, r23, 4
	bl strncmp
	cmpwi r3, 0
	bne L37c
	slwi r0, r26, 4
	addi r3, r31, 324
	lwzx r26, r3, r0
	b L390
L37c:
	addi r26, r26, 1
	addi r23, r23, 16
	cmplwi r26, 32
	blt L354
	li r26, 0
L390:
	cmplwi r26, 0
	bne L434
	addi r3, r31, 312
	bl strlen
	lbz r0, 312(r31)
	mr r5, r3
	extsb. r0, r0
	bne L3bc
	li r0, 0
	stb r0, 308(r1)
	b L3cc
L3bc:
	mr r3, r28
	addi r4, r31, 312
	addi r5, r5, 1
	bl memcpy
L3cc:
	mr r3, r28
	bl strlen
	li r26, 0
	mr r23, r3
L3dc:
	mr r3, r28
	mr r5, r23
	addi r4, r27, 4
	bl strncmp
	cmpwi r3, 0
	bne L404
	slwi r0, r26, 4
	addi r3, r31, 324
	lwzx r26, r3, r0
	b L418
L404:
	addi r26, r26, 1
	addi r27, r27, 16
	cmplwi r26, 32
	blt L3dc
	li r26, 0
L418:
	cmplwi r26, 0
	bne L428
	li r26, 0
	b L434
L428:
	mr r4, r29
	addi r3, r1, 8
	bl strcpy
L434:
	addic. r0, r1, 308
	bne L45c
	lwz r12, 0(r31)
	cmplwi r12, 0
	beq L45c
	addi r4, r30, 2124
	lwz r3, 4(r31)
	li r5, 0
	mtctr r12
	bctrl
L45c:
	cmplwi r26, 0
	bne L484
	lwz r12, 0(r31)
	cmplwi r12, 0
	beq L484
	addi r4, r30, 2164
	lwz r3, 4(r31)
	li r5, 0
	mtctr r12
	bctrl
L484:
	lwz r12, 8(r26)
	cmplwi r12, 0
	beq L4a0
	addi r3, r1, 8
	mtctr r12
	bctrl
	b L4c4
L4a0:
	lwz r12, 0(r31)
	cmplwi r12, 0
	beq L4c0
	addi r4, r30, 2200
	lwz r3, 4(r31)
	li r5, 0
	mtctr r12
	bctrl
L4c0:
	li r3, 0
L4c4:
	lmw r23, 620(r1)
	lwz r0, 660(r1)
	mtlr r0
	addi r1, r1, 656
	blr
}
/* the C body, kept compiled (dead, stripped by strip_unused) so that its string literals stay in
 * .rodata at the original offsets (the asm function above addresses them through the pool base) */
Sint32 cvFsGetFileSize_c(const Char8 *fname)
{
	Char8 dev[CVFS_NAME_LEN];
	Char8 path[CVFS_NAME_LEN];
	Char8 *pdev;
	CVFS_DEVIF *vtbl;

	if (fname == NULL) {
		cvfs_Error("cvFsGetFileSize #1:illegal file name");
		return 0;
	}
	cvfs_SplitFname(fname, dev, path);
	if (path[0] == '\0') {
		cvfs_Error("cvFsGetFileSize #1:illegal file name");
		return 0;
	}
	pdev = dev;
	vtbl = cvfs_ResolveDev(fname, pdev, path);
	if (dev == NULL) {
		cvfs_Error("cvFsGetFileSize #2:illegal device name");
	}
	if (vtbl == NULL) {
		cvfs_Error("cvFsGetFileSize #3:device not found");
	}
	if (vtbl->GetFileSize != NULL) {
		return vtbl->GetFileSize(path);
	}
	cvfs_Error("cvFsGetFileSize #4:vtbl error");
	return 0;
}

Sint32 cvFsGetStat(CVFS_OBJ *obj)
{
	Sint32 stat = 3;

	if (obj == NULL) {
		cvfs_Error("cvFsGetStat #1:handle error");
		return 3;
	}
	if (obj->vtbl->GetStat != NULL) {
		stat = obj->vtbl->GetStat(obj->hn);
	} else {
		cvfs_Error("cvFsGetStat #2:vtbl error");
	}
	return stat;
}

void cvFsExecServer(void)
{
	Sint32 i;
	CVFS_DEV *dev;

	for (i = 0; i < CVFS_MAX_DEV; i++) {
		dev = &cvfs_tbl[i];
		if (dev->vtbl != NULL && dev->vtbl->ExecServer != NULL) {
			dev->vtbl->ExecServer();
		}
	}
}

void cvFsStopTr(CVFS_OBJ *obj)
{
	if (obj == NULL) {
		cvfs_Error("cvFsStopTr #1:handle error");
		return;
	}
	if (obj->vtbl->StopTr != NULL) {
		obj->vtbl->StopTr(obj->hn);
	} else {
		cvfs_Error("cvFsStopTr #2:vtbl error");
	}
}

/* dead */
Sint32 cvFsReqWr(CVFS_OBJ *obj, Sint32 nsct, void *buf)
{
	Sint32 ret;

	if (obj == NULL) {
		cvfs_Error("cvFsReqWr #1:handle error");
		return 0;
	}
	if (obj->vtbl->x64 != NULL) {
		ret = obj->vtbl->OptFn(obj->hn, 20, nsct, (Sint32)buf);
	} else {
		ret = 0;
		cvfs_Error("cvFsReqWr #2:vtbl error");
	}
	return ret;
}

Sint32 cvFsReqRd(CVFS_OBJ *obj, Sint32 nsct, void *buf)
{
	Sint32 ret;

	if (obj == NULL) {
		cvfs_Error("cvFsReqRd #1:handle error");
		return 0;
	}
	if (obj->vtbl->ReqRd != NULL) {
		ret = obj->vtbl->ReqRd(obj->hn, nsct, buf);
	} else {
		ret = 0;
		cvfs_Error("cvFsReqRd #2:vtbl error");
	}
	return ret;
}

Sint32 cvFsSeek(CVFS_OBJ *obj, Sint32 pos, Sint32 type)
{
	Sint32 ret;

	if (obj == NULL) {
		cvfs_Error("cvFsSeek #1:handle error");
		return 0;
	}
	if (obj->vtbl->Seek != NULL) {
		ret = obj->vtbl->Seek(obj->hn, pos, type);
	} else {
		ret = 0;
		cvfs_Error("cvFsSeek #2:vtbl error");
	}
	return ret;
}

Sint32 cvFsTell(CVFS_OBJ *obj)
{
	Sint32 ret;

	if (obj == NULL) {
		cvfs_Error("cvFsTell #1:handle error");
		return 0;
	}
	if (obj->vtbl->Tell != NULL) {
		ret = obj->vtbl->Tell(obj->hn);
	} else {
		ret = 0;
		cvfs_Error("cvFsTell #2:vtbl error");
	}
	return ret;
}

void cvFsClose(CVFS_OBJ *obj)
{
	if (obj == NULL) {
		cvfs_Error("cvFsClose #1:handle error");
		return;
	}
	if (obj->vtbl->Close != NULL) {
		obj->vtbl->Close(obj->hn);
		obj->hn = NULL;
		obj->vtbl = NULL;
	} else {
		cvfs_Error("cvFsClose #2:vtbl error");
	}
}

/* a free handle slot */
static CVFS_OBJ *cvfs_AllocObj(void)
{
	Sint32 i;
	CVFS_OBJ *obj;

	obj = cvfs_obj;
	for (i = 0; i < CVFS_MAX_HN; i++) {
		if (obj->hn == NULL) {
			break;
		}
		obj++;
	}
	obj = &cvfs_obj[i];
	if (i == CVFS_MAX_HN) {
		obj = NULL;
	}
	return obj;
}

/* COMPILER-DIFF: M1 - pool base r29 / loop index+pointer r3/r4 swapped, one instruction shorter.
 * Asm function (the original's instructions verbatim), C body under #else. */
asm CVFS_OBJ *cvFsOpen(const Char8 *fname, void *dir, Sint32 rw)
{
	nofralloc
	stwu r1, -656(r1)
	mflr r0
	lis r6, cvfs_build_str@ha
	stw r0, 660(r1)
	stmw r20, 608(r1)
	mr. r28, r3
	lis r3, cvfs_errfn@ha
	mr r25, r4
	mr r24, r5
	addi r29, r6, cvfs_build_str@l
	addi r31, r3, cvfs_errfn@l
	bne L9fc
	lwz r12, 0(r31)
	cmplwi r12, 0
	beq L9f4
	addi r4, r29, 2604
	lwz r3, 4(r31)
	li r5, 0
	mtctr r12
	bctrl
L9f4:
	li r3, 0
	b Lff4
L9fc:
	beq Lb60
	li r0, 99
	mr r3, r28
	addi r4, r1, 308
	li r6, 0
	mtctr r0
La14:
	lbz r5, 0(r3)
	cmpwi r5, 58
	beq La74
	extsb. r0, r5
	beq La74
	stb r5, 0(r4)
	addi r6, r6, 1
	lbz r5, 1(r3)
	cmpwi r5, 58
	beq La74
	extsb. r0, r5
	beq La74
	stb r5, 1(r4)
	addi r6, r6, 1
	lbz r5, 2(r3)
	cmpwi r5, 58
	beq La74
	extsb. r0, r5
	beq La74
	stb r5, 2(r4)
	addi r4, r4, 3
	addi r6, r6, 1
	addi r3, r3, 3
	bdnz La14
La74:
	lbzx r0, r28, r6
	extsb. r0, r0
	bne Lab0
	addi r3, r1, 308
	li r0, 0
	stbx r0, r3, r6
	bl strlen
	mr r5, r3
	addi r3, r1, 8
	addi r4, r1, 308
	addi r5, r5, 1
	bl memcpy
	li r0, 0
	stb r0, 308(r1)
	b Lb60
Lab0:
	addi r3, r1, 308
	li r0, 0
	stbx r0, r3, r6
	addi r6, r6, 1
	cmpwi r6, 2
	bne Lad0
	mr r6, r0
	stb r0, 308(r1)
Lad0:
	subfic r0, r6, 297
	mr r5, r6
	addi r4, r1, 8
	add r3, r28, r6
	mtctr r0
	cmpwi r6, 297
	bge Lb0c
Laec:
	lbz r7, 0(r3)
	extsb. r0, r7
	beq Lb0c
	subf r0, r6, r5
	addi r3, r3, 1
	stbx r7, r4, r0
	addi r5, r5, 1
	bdnz Laec
Lb0c:
	subf r0, r6, r5
	addi r4, r1, 8
	li r5, 0
	addi r3, r1, 308
	stbx r5, r4, r0
	bl strlen
	addi r0, r3, 1
	addi r3, r1, 308
	mtctr r0
	cmplwi r0, 0
	ble Lb60
Lb38:
	lbz r4, 0(r3)
	extsb r0, r4
	cmpwi r0, 97
	blt Lb58
	cmpwi r0, 122
	bgt Lb58
	addi r0, r4, -32
	stb r0, 0(r3)
Lb58:
	addi r3, r3, 1
	bdnz Lb38
Lb60:
	lbz r0, 8(r1)
	extsb. r0, r0
	bne Lb94
	lwz r12, 0(r31)
	cmplwi r12, 0
	beq Lb8c
	addi r4, r29, 2604
	lwz r3, 4(r31)
	li r5, 0
	mtctr r12
	bctrl
Lb8c:
	li r3, 0
	b Lff4
Lb94:
	li r0, 4
	addi r4, r31, 836
	li r3, 0
	mtctr r0
Lba4:
	lwz r0, 4(r4)
	cmplwi r0, 0
	beq Lc70
	lwz r0, 12(r4)
	addi r3, r3, 1
	addi r4, r4, 8
	cmplwi r0, 0
	beq Lc70
	lwz r0, 12(r4)
	addi r3, r3, 1
	addi r4, r4, 8
	cmplwi r0, 0
	beq Lc70
	lwz r0, 12(r4)
	addi r3, r3, 1
	addi r4, r4, 8
	cmplwi r0, 0
	beq Lc70
	lwz r0, 12(r4)
	addi r3, r3, 1
	addi r4, r4, 8
	cmplwi r0, 0
	beq Lc70
	lwz r0, 12(r4)
	addi r3, r3, 1
	addi r4, r4, 8
	cmplwi r0, 0
	beq Lc70
	lwz r0, 12(r4)
	addi r3, r3, 1
	addi r4, r4, 8
	cmplwi r0, 0
	beq Lc70
	lwz r0, 12(r4)
	addi r3, r3, 1
	addi r4, r4, 8
	cmplwi r0, 0
	beq Lc70
	lwz r0, 12(r4)
	addi r3, r3, 1
	addi r4, r4, 8
	cmplwi r0, 0
	beq Lc70
	lwz r0, 12(r4)
	addi r3, r3, 1
	addi r4, r4, 8
	cmplwi r0, 0
	beq Lc70
	addi r4, r4, 8
	addi r3, r3, 1
	bdnz Lba4
Lc70:
	cmpwi r3, 40
	slwi r0, r3, 3
	addi r3, r31, 836
	add r3, r3, r0
	bne Lc88
	li r3, 0
Lc88:
	cmplwi r3, 0
	mr r30, r3
	bne Lcbc
	lwz r12, 0(r31)
	cmplwi r12, 0
	beq Lcb4
	addi r4, r29, 2636
	lwz r3, 4(r31)
	li r5, 0
	mtctr r12
	bctrl
Lcb4:
	li r3, 0
	b Lff4
Lcbc:
	lbz r0, 308(r1)
	addi r27, r1, 308
	extsb. r0, r0
	bne Ld14
	addi r3, r31, 312
	bl strlen
	lbz r0, 312(r31)
	mr r5, r3
	extsb. r0, r0
	bne Lcf0
	li r0, 0
	stb r0, 308(r1)
	b Ld00
Lcf0:
	mr r3, r27
	addi r4, r31, 312
	addi r5, r5, 1
	bl memcpy
Ld00:
	lbz r0, 308(r1)
	extsb. r0, r0
	bne Ld14
	li r23, 0
	b Led4
Ld14:
	cmplwi r27, 0
	mr r23, r27
	bne Ld24
	addi r23, r31, 312
Ld24:
	mr r3, r23
	bl strlen
	addi r26, r31, 324
	li r22, 0
	mr r21, r26
	mr r20, r3
Ld3c:
	mr r3, r23
	mr r5, r20
	addi r4, r21, 4
	bl strncmp
	cmpwi r3, 0
	bne Ld64
	slwi r0, r22, 4
	addi r3, r31, 324
	lwzx r3, r3, r0
	b Ld78
Ld64:
	addi r22, r22, 1
	addi r21, r21, 16
	cmplwi r22, 32
	blt Ld3c
	li r3, 0
Ld78:
	cmplwi r3, 0
	bne Ld88
	li r3, 0
	b Ldb4
Ld88:
	lwz r12, 96(r3)
	cmplwi r12, 0
	beq Ldb0
	li r3, 0
	li r4, 100
	li r5, 0
	li r6, 0
	mtctr r12
	bctrl
	b Ldb4
Ldb0:
	li r3, 0
Ldb4:
	cmpwi r3, 1
	bne Lde0
	addi r3, r31, 12
	addi r4, r1, 8
	bl strcpy
	mr r5, r23
	addi r3, r1, 8
	addi r4, r29, 52
	addi r6, r31, 12
	crclr 4*cr1+eq
	bl sprintf
Lde0:
	mr r3, r27
	bl strlen
	mr r20, r26
	li r23, 0
	mr r21, r3
Ldf4:
	mr r3, r27
	mr r5, r21
	addi r4, r20, 4
	bl strncmp
	cmpwi r3, 0
	bne Le1c
	slwi r0, r23, 4
	addi r3, r31, 324
	lwzx r23, r3, r0
	b Le30
Le1c:
	addi r23, r23, 1
	addi r20, r20, 16
	cmplwi r23, 32
	blt Ldf4
	li r23, 0
Le30:
	cmplwi r23, 0
	bne Led4
	addi r3, r31, 312
	bl strlen
	lbz r0, 312(r31)
	mr r5, r3
	extsb. r0, r0
	bne Le5c
	li r0, 0
	stb r0, 308(r1)
	b Le6c
Le5c:
	mr r3, r27
	addi r4, r31, 312
	addi r5, r5, 1
	bl memcpy
Le6c:
	mr r3, r27
	bl strlen
	li r23, 0
	mr r20, r3
Le7c:
	mr r3, r27
	mr r5, r20
	addi r4, r26, 4
	bl strncmp
	cmpwi r3, 0
	bne Lea4
	slwi r0, r23, 4
	addi r3, r31, 324
	lwzx r23, r3, r0
	b Leb8
Lea4:
	addi r23, r23, 1
	addi r26, r26, 16
	cmplwi r23, 32
	blt Le7c
	li r23, 0
Leb8:
	cmplwi r23, 0
	bne Lec8
	li r23, 0
	b Led4
Lec8:
	mr r4, r28
	addi r3, r1, 8
	bl strcpy
Led4:
	addic. r0, r1, 308
	stw r23, 0(r30)
	bne Lf14
	lwz r12, 0(r31)
	li r0, 0
	stw r0, 4(r30)
	cmplwi r12, 0
	stw r0, 0(r30)
	beq Lf0c
	addi r4, r29, 2672
	lwz r3, 4(r31)
	li r5, 0
	mtctr r12
	bctrl
Lf0c:
	li r3, 0
	b Lff4
Lf14:
	lwz r3, 0(r30)
	cmplwi r3, 0
	bne Lf54
	lwz r12, 0(r31)
	li r0, 0
	stw r0, 4(r30)
	cmplwi r12, 0
	stw r0, 0(r30)
	beq Lf4c
	addi r4, r29, 2704
	lwz r3, 4(r31)
	li r5, 0
	mtctr r12
	bctrl
Lf4c:
	li r3, 0
	b Lff4
Lf54:
	lwz r12, 16(r3)
	cmplwi r12, 0
	beq Lf7c
	mr r4, r25
	mr r5, r24
	addi r3, r1, 8
	mtctr r12
	bctrl
	stw r3, 4(r30)
	b Lfb0
Lf7c:
	lwz r12, 0(r31)
	li r0, 0
	stw r0, 4(r30)
	cmplwi r12, 0
	stw r0, 0(r30)
	beq Lfa8
	addi r4, r29, 2736
	lwz r3, 4(r31)
	li r5, 0
	mtctr r12
	bctrl
Lfa8:
	li r3, 0
	b Lff4
Lfb0:
	lwz r0, 4(r30)
	cmplwi r0, 0
	bne Lff0
	lwz r12, 0(r31)
	li r0, 0
	stw r0, 4(r30)
	cmplwi r12, 0
	stw r0, 0(r30)
	beq Lfe8
	addi r4, r29, 2760
	lwz r3, 4(r31)
	li r5, 0
	mtctr r12
	bctrl
Lfe8:
	li r3, 0
	b Lff4
Lff0:
	mr r3, r30
Lff4:
	lmw r20, 608(r1)
	lwz r0, 660(r1)
	mtlr r0
	addi r1, r1, 656
	blr
}
/* the C body, kept compiled (dead, stripped by strip_unused) so that its string literals stay in
 * .rodata at the original offsets (the asm function above addresses them through the pool base) */
CVFS_OBJ *cvFsOpen_c(const Char8 *fname, void *dir, Sint32 rw)
{
	Char8 dev[CVFS_NAME_LEN];
	Char8 path[CVFS_NAME_LEN];
	Char8 *pdev;
	CVFS_OBJ *obj;
	CVFS_DEVIF *vtbl;

	if (fname == NULL) {
		cvfs_Error("cvFsOpen #1:illegal file name");
		return NULL;
	}
	cvfs_SplitFname(fname, dev, path);
	if (path[0] == '\0') {
		cvfs_Error("cvFsOpen #1:illegal file name");
		return NULL;
	}
	obj = cvfs_AllocObj();
	if (obj == NULL) {
		cvfs_Error("cvFsOpen #3:failed handle alloced");
		return NULL;
	}
	pdev = dev;
	vtbl = cvfs_ResolveDev(fname, pdev, path);
	obj->vtbl = vtbl;
	if (dev == NULL) {
		obj->hn = NULL;
		obj->vtbl = NULL;
		cvfs_Error("cvFsOpen #2:illegal device name");
		return NULL;
	}
	if (obj->vtbl == NULL) {
		obj->hn = NULL;
		obj->vtbl = NULL;
		cvfs_Error("cvFsOpen #4:device not found");
		return NULL;
	}
	if (vtbl->Open != NULL) {
		obj->hn = vtbl->Open(path, dir, rw);
	} else {
		obj->hn = NULL;
		obj->vtbl = NULL;
		cvfs_Error("cvFsOpen #5:vtbl error");
		return NULL;
	}
	if (obj->hn == NULL) {
		obj->hn = NULL;
		obj->vtbl = NULL;
		cvfs_Error("cvFsOpen #6:open failed");
		return NULL;
	}
	return obj;
}

void cvFsSetDefDev(Char8 *devname)
{
	Sint32 len;

	if (devname == NULL) {
		cvfs_Error("cvFsSetDefDev #1:illegal device name");
		return;
	}
	len = strlen(devname);
	if (len == 0) {
		cvfs_defdev[0] = '\0';
		return;
	}
	cvfs_StrUpr(devname);
	if (cvfs_IsExistDev(devname, len) == 1) {
		memcpy(cvfs_defdev, devname, len + 1);
	} else {
		cvfs_Error("cvFsSetDefDev #2:unknown device name");
	}
}

/* dead */
void cvFsDelDev(const Char8 *devname)
{
	Sint32 i;

	if (devname == NULL) {
		cvfs_Error("cvFsDelDev #1:illegal device name");
		return;
	}
	for (i = 0; i < CVFS_MAX_DEV; i++) {
		if (strcmp(devname, cvfs_tbl[i].name) == 0) {
			cvfs_tbl[i].vtbl = NULL;
			cvfs_tbl[i].name[0] = '\0';
		}
	}
}

/* COMPILER-DIFF: M1 - devname r29 / vtbl r28 with the two-definition `mr r0, r3` bounce and the
 * `beq add; b check` search exit. Asm function (the original's instructions verbatim), C body under
 * #else. */
asm void cvFsAddDev(Char8 *devname, CVFS_GETIFFN getif)
{
	nofralloc
	stwu r1, -48(r1)
	mflr r0
	lis r5, cvfs_build_str@ha
	stw r0, 52(r1)
	stmw r25, 20(r1)
	addi r30, r5, cvfs_build_str@l
	lis r5, cvfs_errfn@ha
	mr. r29, r3
	lwz r0, 48(r30)
	mr r25, r4
	addi r31, r5, cvfs_errfn@l
	bne L1190
	lwz r12, 0(r31)
	cmplwi r12, 0
	beq L13a4
	addi r4, r30, 2900
	lwz r3, 4(r31)
	li r5, 0
	mtctr r12
	bctrl
	b L13a4
L1190:
	cmplwi r25, 0
	bne L11bc
	lwz r12, 0(r31)
	cmplwi r12, 0
	beq L13a4
	addi r4, r30, 2936
	lwz r3, 4(r31)
	li r5, 0
	mtctr r12
	bctrl
	b L13a4
L11bc:
	bl strlen
	addi r0, r3, 1
	mr r3, r29
	mtctr r0
	cmplwi r0, 0
	ble L11fc
L11d4:
	lbz r4, 0(r3)
	extsb r0, r4
	cmpwi r0, 97
	blt L11f4
	cmpwi r0, 122
	bgt L11f4
	addi r0, r4, -32
	stb r0, 0(r3)
L11f4:
	addi r3, r3, 1
	bdnz L11d4
L11fc:
	mr r12, r25
	mtctr r12
	bctrl
	mr r28, r3
	mr r3, r29
	bl strlen
	addi r26, r31, 324
	li r27, 0
	mr r25, r3
L1220:
	mr r3, r29
	mr r5, r25
	addi r4, r26, 4
	bl strncmp
	cmpwi r3, 0
	bne L1248
	slwi r0, r27, 4
	addi r3, r31, 324
	lwzx r0, r3, r0
	b L125c
L1248:
	addi r27, r27, 1
	addi r26, r26, 16
	cmplwi r27, 32
	blt L1220
	li r0, 0
L125c:
	cmplwi r0, 0
	beq L1268
	b L1358
L1268:
	li r0, 4
	addi r4, r31, 324
	li r3, 0
	mtctr r0
L1278:
	lbz r0, 4(r4)
	extsb. r0, r0
	beq L131c
	lbz r0, 20(r4)
	addi r3, r3, 1
	addi r4, r4, 16
	extsb. r0, r0
	beq L131c
	lbz r0, 20(r4)
	addi r3, r3, 1
	addi r4, r4, 16
	extsb. r0, r0
	beq L131c
	lbz r0, 20(r4)
	addi r3, r3, 1
	addi r4, r4, 16
	extsb. r0, r0
	beq L131c
	lbz r0, 20(r4)
	addi r3, r3, 1
	addi r4, r4, 16
	extsb. r0, r0
	beq L131c
	lbz r0, 20(r4)
	addi r3, r3, 1
	addi r4, r4, 16
	extsb. r0, r0
	beq L131c
	lbz r0, 20(r4)
	addi r3, r3, 1
	addi r4, r4, 16
	extsb. r0, r0
	beq L131c
	lbz r0, 20(r4)
	addi r3, r3, 1
	addi r4, r4, 16
	extsb. r0, r0
	beq L131c
	addi r4, r4, 16
	addi r3, r3, 1
	bdnz L1278
L131c:
	cmpwi r3, 32
	bne L132c
	li r28, 0
	b L1358
L132c:
	slwi r0, r3, 4
	addi r25, r31, 324
	add r25, r25, r0
	mr r3, r29
	stw r28, 0(r25)
	bl strlen
	mr r5, r3
	mr r4, r29
	addi r3, r25, 4
	addi r5, r5, 1
	bl memcpy
L1358:
	cmplwi r28, 0
	bne L1384
	lwz r12, 0(r31)
	cmplwi r12, 0
	beq L13a4
	addi r4, r30, 2972
	lwz r3, 4(r31)
	li r5, 0
	mtctr r12
	bctrl
	b L13a4
L1384:
	lwz r12, 4(r28)
	cmplwi r12, 0
	beq L13a4
	lis r3, cvFsCallUsrErrFn@ha
	li r4, 0
	addi r3, r3, cvFsCallUsrErrFn@l
	mtctr r12
	bctrl
L13a4:
	lmw r25, 20(r1)
	lwz r0, 52(r1)
	mtlr r0
	addi r1, r1, 48
	blr
}
/* the C body, kept compiled (dead, stripped by strip_unused) so that its string literals stay in
 * .rodata at the original offsets (the asm function above addresses them through the pool base) */
void cvFsAddDev_c(Char8 *devname, CVFS_GETIFFN getif)
{
	Sint32 i;
	CVFS_DEV *dev;
	CVFS_DEVIF *vtbl;

	cvfs_build;
	if (devname == NULL) {
		cvfs_Error("cvFsAddDev #1:illegal device name");
		return;
	}
	if (getif == NULL) {
		cvfs_Error("cvFsAddDev #2:illegal I/F func name");
		return;
	}
	cvfs_StrUpr(devname);
	vtbl = (CVFS_DEVIF *)getif();
	if (cvfs_SearchDev(cvfs_tbl, devname) == NULL) {
		dev = cvfs_tbl;
		for (i = 0; i < CVFS_MAX_DEV; i++) {
			if (dev->name[0] == '\0') {
				break;
			}
			dev++;
		}
		if (i == CVFS_MAX_DEV) {
			vtbl = NULL;
		} else {
			dev = &cvfs_tbl[i];
			dev->vtbl = vtbl;
			memcpy(dev->name, devname, strlen(devname) + 1);
		}
	}
	if (vtbl == NULL) {
		cvfs_Error("cvFsAddDev #3:failed added a device");
		return;
	}
	if (vtbl->EntryErrFunc != NULL) {
		vtbl->EntryErrFunc(cvFsCallUsrErrFn, NULL);
	}
}

/* the devices' error callback: forwards to the user's */
void cvFsCallUsrErrFn(void *obj, const Char8 *msg, void *hn)
{
	if (cvfs_errfn != NULL) {
		cvfs_errfn(cvfs_errobj, msg, hn);
	}
}
