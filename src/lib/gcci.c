/* CRI CVFS GameCube DVD device (GCCI): sector reads through the Dolphin DVD API */
#include "cvfs.h"
#include <string.h>
#include <dolphin/os.h>
#include <dolphin/dvd.h>

#define GCCI_MAX_OBJ 40
#define GCCI_DEF_SCTLEN 0x800
#define GCCI_PATH_LEN 260
#define GCCI_CANCEL_TIMEOUT 2000

/* GCCI_OBJ.stat */
#define GCCI_STAT_STOP 0
#define GCCI_STAT_COMPLETE 1
#define GCCI_STAT_READING 2
#define GCCI_STAT_ERROR 3

typedef struct {
	Sint8 used;                /* 0x00 */
	Sint8 pad01;
	Sint8 stat;                /* 0x02 */
	Sint8 pad03;
	Sint32 pad04;
	Uint8 *buf;                /* 0x08 */
	Sint32 cbstat;             /* 0x0C last DVDGetCommandBlockStatus */
	Sint32 sctlen;             /* 0x10 */
	Sint32 fsize_byte;         /* 0x14 */
	Sint32 fsize_sct;          /* 0x18 */
	Sint32 pos_sct;            /* 0x1C */
	Sint32 numtr;              /* 0x20 bytes transferred by the last request */
	Sint32 rqsct;              /* 0x24 sectors of the last request */
	DVDFileInfo fi;            /* 0x28 */
} GCCI_OBJ;

typedef GCCI_OBJ *GCCI;

/* debug information */
typedef struct {
	Sint32 cbstat;             /* 0x00 last command block status seen */
	Sint8 stat;                /* 0x04 last handle state set */
	Sint8 pad05[3];
	Sint32 cancel;             /* 0x08 inside DVDCancel */
} GCCI_DEBUG;

extern Sint32 gcg_ci_rdmode;
extern Char8 gcg_ci_root_dir[256];

const Char8 gcg_ci_build_str[] = "\nGCCI Ver.1.09 Build:Oct  8 2004 13:31:49\n";

GCCI_DEBUG gcg_ci_debug;
void *gcg_ci_err_obj;
CVFS_ERRFUNC gcg_ci_err_func;
GCCI_OBJ gcg_ci_obj[GCCI_MAX_OBJ];
const Char8 *volatile gcg_ci_build_ptr;

void gcCiExecServer(void);
void gcCiEntryErrFunc(CVFS_ERRFUNC func, void *obj);
Sint32 gcCiGetFileSize(const Char8 *fname);
GCCI gcCiOpen(const Char8 *fname, void *dir, Sint32 rw);
void gcCiClose(GCCI gcci);
Sint32 gcCiSeek(GCCI gcci, Sint32 pos, Sint32 type);
Sint32 gcCiTell(GCCI gcci);
Sint32 gcCiReqRd(GCCI gcci, Sint32 nsct, Uint8 *buf);
void gcCiStopTr(GCCI gcci);
static Sint32 gcCiGetStat(GCCI gcci);
Sint32 gcCiGetSctLen(GCCI gcci);
void gcCiSetSctLen(GCCI gcci, Sint32 sctlen);
Sint32 gcCiGetNumTr(GCCI gcci);

CVFS_IF gcg_ci_vtbl = {
	gcCiExecServer,
	gcCiEntryErrFunc,
	gcCiGetFileSize,
	NULL,
	(void *(*)(const Char8 *, void *, Sint32))gcCiOpen,
	(void (*)(void *))gcCiClose,
	(Sint32 (*)(void *, Sint32, Sint32))gcCiSeek,
	(Sint32 (*)(void *))gcCiTell,
	(Sint32 (*)(void *, Sint32, void *))gcCiReqRd,
	NULL,
	(void (*)(void *))gcCiStopTr,
	(Sint32 (*)(void *))gcCiGetStat,
	(Sint32 (*)(void *))gcCiGetSctLen,
	(void (*)(void *, Sint32))gcCiSetSctLen,
	(Sint32 (*)(void *))gcCiGetNumTr,
};

/* dead: first references of the .bss objects (debug, err_obj, err_func) */
void gcCiInit(void)
{
	gcg_ci_debug.cbstat = 0;
	gcg_ci_err_obj = NULL;
	gcg_ci_err_func = NULL;
}

static void gcci_CallErr(const Char8 *msg, void *hn)
{
	if (gcg_ci_err_func != NULL) {
		gcg_ci_err_func(gcg_ci_err_obj, msg, hn);
	}
}

/* the handle is not reading */
static Bool gcci_IsIdle(GCCI gcci)
{
	Bool idle = 0;

	if (gcci->stat == GCCI_STAT_COMPLETE || gcci->stat == GCCI_STAT_STOP) {
		idle = 1;
	}
	return idle;
}

/* the last DVD command finished (done or canceled) */
static Bool gcci_IsCmdDone(GCCI gcci)
{
	Bool done = 0;

	if (gcci->cbstat == DVD_STATE_END || gcci->cbstat == DVD_STATE_CANCELED) {
		done = 1;
	}
	return done;
}

void gcci_rd_cbfn(s32 result, DVDFileInfo *fi)
{
}

Sint32 gcCiGetNumTr(GCCI gcci)
{
	if (gcci == NULL) {
		gcci_CallErr("E0092912:handl is null.", NULL);
		return 0;
	}
	return gcci->numtr;
}

/* COMPILER-DIFF: M1 -- the sector-length recomputation's temporaries are asm-defined `register` locals so the
 * volatile numbering follows the declaration order (n r5, sl r6, pos_byte r7; the C form gave r7/r5/r6). */
void gcCiSetSctLen(register GCCI gcci, Sint32 sctlen)
{
	register Sint32 n;
	register Sint32 sl;
	register Sint32 pos_byte;
	register Sint32 ps;
	register Sint32 sl0;
	register Sint32 fb;

	if (gcci == NULL) {
		gcci_CallErr("E0040302:handl is null.", NULL);
		return;
	}
	if (gcci->sctlen % 32 != 0) {
		gcci_CallErr("E0040303:invalidate size.", NULL);
		return;
	}
	asm { lwz ps, GCCI_OBJ.pos_sct(gcci); lwz sl0, GCCI_OBJ.sctlen(gcci) } // COMPILER-DIFF: M1
	gcci->sctlen = sctlen;
	asm { mullw pos_byte, ps, sl0 } // COMPILER-DIFF: M1
	asm { lwz sl, GCCI_OBJ.sctlen(gcci); lwz fb, GCCI_OBJ.fsize_byte(gcci); add n, sl, fb } // COMPILER-DIFF: M1
	gcci->fsize_sct = (n - 1) / sl;
	gcci->pos_sct = pos_byte / gcci->sctlen;
	gcci->numtr = gcci->rqsct * sctlen;
}

Sint32 gcCiGetSctLen(GCCI gcci)
{
	if (gcci == NULL) {
		gcci_CallErr("E0040301:handl is null.", NULL);
		return 0;
	}
	return gcci->sctlen;
}

static Sint32 gcCiGetStat(GCCI gcci)
{
	if (gcci == NULL) {
		gcci_CallErr("E0092912:handl is null.", NULL);
		return 0;
	}
	return gcci->stat;
}

void gcCiStopTr(GCCI gcci)
{
	Sint32 ret;
	Uint32 t0;
	Uint32 t;
	Uint32 dt;

	if (gcci == NULL) {
		gcci_CallErr("E0092912:handl is null.", NULL);
		return;
	}
	if (gcci->stat == GCCI_STAT_COMPLETE || gcci->stat == GCCI_STAT_STOP) {
		return;
	}
	DVDGetCommandBlockStatus(&gcci->fi.cb);
	DVDGetDriveStatus();
	gcg_ci_debug.cancel = 1;
	ret = DVDCancel(&gcci->fi.cb);
	gcg_ci_debug.cancel = 0;
	if (ret < 0) {
		gcci_CallErr("E0092917:DVDCancel failed.", gcci);
		return;
	}
	t0 = OSTicksToMilliseconds(OSGetTick());
	while (!gcci_IsCmdDone(gcci)) {
		gcci->cbstat = DVDGetCommandBlockStatus(&gcci->fi.cb);
		gcg_ci_debug.cbstat = gcci->cbstat;
		t = OSTicksToMilliseconds(OSGetTick());
		dt = (t >= t0) ? (t - t0) : (0xFFFFFFFF - t0 + t);
		if (dt > GCCI_CANCEL_TIMEOUT) {
			gcci_CallErr("E0092918:DVDCancel time out.", gcci);
			break;
		}
	}
	gcci->stat = GCCI_STAT_STOP;
	gcg_ci_debug.stat = GCCI_STAT_STOP;
	DVDGetCommandBlockStatus(&gcci->fi.cb);
	DVDGetDriveStatus();
}

static GCCI gcci_GetFreeHn(void)
{
	GCCI gcci = NULL;
	Sint32 i;

	for (i = 0; i < GCCI_MAX_OBJ; i++) {
		if (gcg_ci_obj[i].used == 0) {
			gcci = &gcg_ci_obj[i];
			break;
		}
	}
	return gcci;
}

/* a handle of the table has a read in flight */
static Bool gcci_IsBusy(GCCI tbl)
{
	Sint32 i;

	for (i = 0; i < GCCI_MAX_OBJ; i++) {
		if (tbl[i].used == 1 && tbl[i].stat == GCCI_STAT_READING) {
			return 1;
		}
	}
	return 0;
}

/* finish the reads of the table that completed. M1-like: the original steps the caller's table
 * pointer itself (no `mr` copy of the inlined parameter) and numbers i/gcci r29/r30 the other way
 * round in gcCiExecServer; instruction stream identical. */
static inline void gcci_ExecServer(GCCI gcci)
{
	Sint32 i;
	Sint32 over;
	Sint32 nbyte;
	Uint8 *p;

	for (i = 0; i < GCCI_MAX_OBJ; i++, gcci++) {
		if (gcci->used == 1 && gcci->stat == GCCI_STAT_READING) {
			gcci->cbstat = DVDGetCommandBlockStatus(&gcci->fi.cb);
			gcg_ci_debug.cbstat = gcci->cbstat;
			switch (gcci->cbstat) {
			case DVD_STATE_FATAL_ERROR:
				gcci->stat = GCCI_STAT_ERROR;
				gcg_ci_debug.stat = GCCI_STAT_ERROR;
				break;
			case DVD_STATE_END:
				nbyte = gcci->rqsct * gcci->sctlen;
				DCInvalidateRange(gcci->buf, nbyte);
				gcci->numtr = nbyte;
				gcci->pos_sct += gcci->rqsct;
				if (gcci->pos_sct * gcci->sctlen > gcci->fsize_byte) {
					over = gcci->pos_sct * gcci->sctlen - gcci->fsize_byte;
					p = gcci->buf + gcci->numtr - over;
					memset(p, 0, over);
					DCStoreRange(p, over);
				}
				gcci->stat = GCCI_STAT_COMPLETE;
				gcg_ci_debug.stat = GCCI_STAT_COMPLETE;
				break;
			case DVD_STATE_CANCELED:
				over = DVDGetTransferredSize(&gcci->fi);
				DCInvalidateRange(gcci->buf, over);
				gcci->numtr = (over / gcci->sctlen) * gcci->sctlen;
				gcci->pos_sct += over / gcci->sctlen;
				gcci->stat = GCCI_STAT_STOP;
				gcg_ci_debug.stat = GCCI_STAT_STOP;
				break;
			}
		}
	}
}

Sint32 gcCiReqRd(GCCI gcci, Sint32 nsct, Uint8 *buf)
{
	Sint32 ofst;
	Sint32 nbyte;
	Sint32 ret;
	GCCI tbl;

	if (gcci == NULL) {
		gcci_CallErr("E0092912:handl is null.", NULL);
		return 0;
	}
	if (nsct < 0) {
		gcci_CallErr("E0092913:nsct < 0.(gcCiReqRd)", gcci);
		return 0;
	}
	if (buf == NULL) {
		gcci_CallErr("E0092914:buf is null.(gcCiReqRd)", gcci);
		return 0;
	}
	if (!gcci_IsIdle(gcci)) {
		return 0;
	}
	if (!gcci_IsCmdDone(gcci)) {
		return 0;
	}
	tbl = gcg_ci_obj;
	if (gcci_IsBusy(tbl)) {
		return 0;
	}
	if (nsct == 0) {
		gcci->stat = GCCI_STAT_COMPLETE;
		gcg_ci_debug.stat = GCCI_STAT_COMPLETE;
		return 0;
	}
	gcci->numtr = 0;
	gcci->buf = buf;
	gcci->rqsct = nsct;
	gcci_ExecServer(tbl);
	ofst = gcci->pos_sct * gcci->sctlen;
	nbyte = gcci->rqsct * gcci->sctlen;
	if (ofst + nbyte > gcci->fsize_byte) {
		nbyte = gcci->fsize_byte - ofst;
		if (nbyte < 0) {
			gcci->stat = GCCI_STAT_COMPLETE;
			gcg_ci_debug.stat = GCCI_STAT_COMPLETE;
			return nsct;
		}
	}
	nbyte = (nbyte + 31) & ~31;
	DCInvalidateRange(buf, nbyte);
	if (gcg_ci_rdmode == 0) {
		ret = DVDReadAsyncPrio(&gcci->fi, buf, nbyte, ofst, gcci_rd_cbfn, 2);
	} else {
		ret = DVDReadPrio(&gcci->fi, buf, nbyte, ofst, 2);
	}
	if (ret == 0) {
		return 0;
	}
	gcci->stat = GCCI_STAT_READING;
	gcg_ci_debug.stat = GCCI_STAT_READING;
	return gcci->rqsct;
}

/* dead: the build string (referenced after gcg_ci_obj so that gcg_ci_build_ptr ends the .bss) */
const Char8 *gcCiGetVersion(void)
{
	gcg_ci_build_ptr = gcg_ci_build_str;
	return gcg_ci_build_str;
}

Sint32 gcCiTell(GCCI gcci)
{
	if (gcci == NULL) {
		gcci_CallErr("E0092912:handl is null.", NULL);
		return 0;
	}
	return gcci->pos_sct;
}

Sint32 gcCiSeek(GCCI gcci, Sint32 pos, Sint32 type)
{
	Sint32 max;

	if (gcci == NULL) {
		gcci_CallErr("E0092912:handl is null.", NULL);
		return 0;
	}
	if (type == CVFS_SEEK_SET) {
		gcci->pos_sct = pos;
	} else if (type == CVFS_SEEK_END) {
		gcci->pos_sct = gcci->fsize_sct + pos;
	} else if (type == CVFS_SEEK_CUR) {
		gcci->pos_sct += pos;
	}
	max = gcci->fsize_sct;
	if (gcci->pos_sct < max) {
		max = gcci->pos_sct;
	}
	gcci->pos_sct = max;
	gcci->pos_sct = (gcci->pos_sct > 0) ? gcci->pos_sct : 0;
	return gcci->pos_sct;
}

/* M1-like (OPEN): the original copies the handle into a second callee-saved register for the inlined
 * gcCiStopTr (`mr r29, r3` after the NULL test, stmw r24) and addresses the .bss pool through the
 * gcg_ci_debug symbol; ours coalesces the copy. void*-parameter / local-copy / nested-if forms tried. */
void gcCiClose(GCCI gcci)
{
	if (gcci == NULL) {
		return;
	}
	gcCiStopTr(gcci);
	DVDClose(&gcci->fi);
	gcci->used = 0;
	memset(gcci, 0, sizeof(GCCI_OBJ));
}

/* root directory + file name, '\' -> '/' */
static void gcci_MakePath(Char8 *path, const Char8 *fname)
{
	Uint32 len;
	Uint32 i;

	strcpy(path, gcg_ci_root_dir);
	strcat(path, fname);
	len = strlen(path);
	for (i = 0; i < len; i++) {
		if (path[i] == '\\') {
			path[i] = '/';
		}
	}
}

GCCI gcCiOpen(const Char8 *fname, void *dir, Sint32 rw)
{
	Char8 path[GCCI_PATH_LEN];
	GCCI gcci;
	Uint32 len;
	Sint32 n;

	if (fname == NULL) {
		gcci_CallErr("E0092908:fname is null.(gcCiOpen)", NULL);
		return NULL;
	}
	if (rw != 0) {
		gcci_CallErr("E0092909:rw is illigal.(gcCiOpen)", NULL);
		return NULL;
	}
	gcci = gcci_GetFreeHn();
	if (gcci == NULL) {
		gcci_CallErr("E0092910:not enough handle resource.(gcCiOpen)", NULL);
		return NULL;
	}
	gcci_MakePath(path, fname);
	if (DVDOpen(path, &gcci->fi) == 0) {
		gcci_CallErr("E0092911:DVDOpen fail.(gcCiOpen)", NULL);
		memset(gcci, 0, sizeof(GCCI_OBJ));
		return NULL;
	}
	gcci->sctlen = GCCI_DEF_SCTLEN;
	len = gcci->fi.length;
	if (len & 0x80000000) {
		len = 0x7FFFFFFF;
	}
	gcci->fsize_byte = len;
	n = gcci->sctlen;
	n += gcci->fsize_byte;
	gcci->fsize_sct = (n - 1) / gcci->sctlen;
	gcci->pos_sct = 0;
	gcci->buf = NULL;
	gcci->rqsct = 0;
	gcci->numtr = 0;
	gcci->stat = GCCI_STAT_STOP;
	gcci->used = 1;
	return gcci;
}

Sint32 gcCiGetFileSize(const Char8 *fname)
{
	Char8 path[GCCI_PATH_LEN];
	DVDFileInfo fi;
	Uint32 len;

	if (fname == NULL) {
		gcci_CallErr("E0092901:fname is null.(gcCiGetFileSize)", NULL);
		return 0;
	}
	gcci_MakePath(path, fname);
	if (DVDOpen(path, &fi) == 0) {
		gcci_CallErr("E0040201:can't open a file.(gcCiGetFileSize)", NULL);
		return 0;
	}
	len = fi.length;
	if (len & 0x80000000) {
		len = 0x7FFFFFFF;
	}
	if (DVDClose(&fi) == 0) {
		gcci_CallErr("E0040202:can't close a file.(gcCiGetFileSize)", NULL);
		return 0;
	}
	return len;
}

/* 24 zero bytes end the original .rodata after the gcCiGetFileSize strings: a dead zero table */
static const Sint32 gcg_ci_rsv[6] = {0, 0, 0, 0, 0, 0};

/* dead */
Sint32 gcCiGetRsv(Sint32 i)
{
	return gcg_ci_rsv[i];
}

void gcCiEntryErrFunc(CVFS_ERRFUNC func, void *obj)
{
	gcg_ci_err_func = func;
	gcg_ci_err_obj = obj;
}

void gcCiExecServer(void)
{
	gcci_ExecServer(gcg_ci_obj);
}

CVFS_IF *gcCiGetInterface(void)
{
	gcg_ci_build_ptr = gcg_ci_build_ptr;
	memset(gcg_ci_root_dir, 0, sizeof(gcg_ci_root_dir));
	gcg_ci_err_func = NULL;
	gcg_ci_err_obj = NULL;
	memset(&gcg_ci_debug, 0, sizeof(gcg_ci_debug));
	return &gcg_ci_vtbl;
}
