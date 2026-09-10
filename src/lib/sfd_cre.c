/* CRI Sofdec: creation information analysis (sfd_cre.c). Scans the head of a file for the MPEG
 * system / video / ADX / MPEG audio headers to pick the transfer drivers and the stream parameters. */
#include "cri_xpt.h"
#include "sfd.h"
#include "mps.h"
#include "mpv.h"
#include <string.h>

extern void MEM_Copy(void *dst, const void *src, Uint32 nbytes);
extern Bool SFADXT_IsHeader(Uint8 *data, Sint32 size, Sint32 *hdrsiz);

extern const SFD_TR_IF SFD_tr_sd_mps;
extern const SFD_TR_IF SFD_tr_vd_mpv;
extern const SFD_TR_IF SFD_tr_ad_adxt;

#define SFCRE_TMPBUF_SIZE 0x800

/* MPEG audio (AAU) header fields */
typedef struct {
	Uint8 layer;       /* 0x0 */
	Uint8 protect;     /* 0x1 */
	Uint8 bitrate;     /* 0x2 */
	Uint8 sfreq;       /* 0x3 */
	Uint8 padding;     /* 0x4 */
	Uint8 prv;         /* 0x5 */
	Uint8 mode;        /* 0x6 */
	Uint8 mode_ext;    /* 0x7 */
	Uint8 copyright;   /* 0x8 */
	Uint8 original;    /* 0x9 */
	Uint8 emphasis;    /* 0xA */
} SFCRE_AAUHDR;

const Sint32 sfcre_mpv_picrate[9] = {
	0, 23976, 24000, 25000, 29970, 30000, 50000, 59940, 60000,
};
const Sint32 sfcre_aau_ch[4] = { 2, 2, 2, 1 };
const Sint32 sfcre_aau_freq[4] = { 44100, 48000, 32000, 0 };
const Sint32 sfcre_aau_bitrate[4][16] = {
	{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ -1, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1 },
	{ -1, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, -1 },
	{ -1, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, -1 },
};
const Sint32 sfcre_aau_const_siz[4] = { -1, 1152, 1152, 384 };

SFCRE_AAUHDR sfcre_aauhdr;
SFHDS_FHD sfcre_fhd;
Uint8 sfcre_tmpbuf[SFCRE_TMPBUF_SIZE];

/* first MPEG system start code of kind `code` (MPS_CheckDelim class) */
static Uint8 *sfcre_SearchDelim(Uint8 *p, Sint32 n, Uint32 code)
{
	while (n >= 4) {
		if (MPS_CheckDelim(p) == code) {
			return p;
		}
		p++;
		n--;
	}
	return NULL;
}

/* first valid MPEG audio frame header */
static Uint8 *sfcre_SearchAauHdr(Uint8 *p, Sint32 n, SFCRE_AAUHDR *hdr)
{
	while (n >= 4) {
		if (p[0] == 0xFF && (p[1] & 0xF8) == 0xF8) {
			hdr->layer = (p[1] >> 1) & 3;
			hdr->protect = p[1] & 1;
			hdr->bitrate = (p[2] >> 4) & 0xF;
			hdr->sfreq = (p[2] >> 2) & 3;
			hdr->padding = (p[2] >> 1) & 1;
			hdr->prv = p[2] & 1;
			hdr->mode = (p[3] >> 6) & 3;
			hdr->mode_ext = (p[3] >> 4) & 3;
			hdr->copyright = (p[3] >> 3) & 1;
			hdr->original = (p[3] >> 2) & 1;
			hdr->emphasis = p[3] & 3;
			if (hdr->layer != 0 && hdr->bitrate != 0xF && hdr->sfreq != 3) {
				return p;
			}
		}
		p++;
		n--;
	}
	return NULL;
}

static Bool sfcre_AnalyAau(Uint8 *data, Sint32 size, SFD_CREINF *inf)
{
	SFCRE_AAUHDR hdr;
	Uint8 *p;

	p = sfcre_SearchAauHdr(data, size, &hdr);
	if (p != NULL) {
		inf->atrif = NULL;
		inf->ach = sfcre_aau_ch[hdr.mode];
		inf->afreq = sfcre_aau_freq[hdr.sfreq];
		sfcre_aauhdr = hdr;
		return 1;
	}
	return 0;
}

/* the Sofdec file header packet found at p */
static void sfcre_AnalySfdHdr(Uint8 *p, Sint32 n, SFD_CREINF *inf)
{
	SFHDS_FHD *fhd = &sfcre_fhd;
	Sint32 len;

	fhd->valid = 0;
	len = SFHDS_RAW_SIZE;
	if (n < SFHDS_RAW_SIZE) {
		len = n;
	}
	MEM_Copy(fhd->raw, p, len);
	fhd->rawsiz = len;
	SFHDS_ProcessHdr(fhd);
	if (fhd->valid != 0) {
		if (fhd->byterate > 0) {
			inf->bitrate = fhd->byterate;
		}
		if (fhd->vid.picw > 0) {
			inf->picw = fhd->vid.picw;
		}
		if (fhd->vid.pich > 0) {
			inf->pich = fhd->vid.pich;
		}
		if (fhd->vid.picrate > 0) {
			inf->picrate = fhd->vid.picrate;
			inf->vtrif = &SFD_tr_vd_mpv;
		}
	}
}

/* pack size from the distance of three consecutive pack start codes: 0 none, -1 irregular */
static Sint32 sfcre_AnalyPackSiz(Uint8 *data, Sint32 size, Sint32 *mux_rate)
{
	Uint8 *p1;
	Uint8 *p2;
	Uint8 *p3;
	Sint32 ofs1;
	Sint32 ofs2;
	Sint32 n1;
	Sint32 packsiz;
	MPS mps;
	Sint32 len;
	Sint32 flags;
	MPS_PACKHD packhd;

	p1 = sfcre_SearchDelim(data, size, 0x10000);
	if (p1 == NULL) {
		return 0;
	}
	ofs1 = p1 - data;
	n1 = size - ofs1;
	p2 = sfcre_SearchDelim(p1 + 1, n1 - 1, 0x10000);
	if (p2 == NULL) {
		return 0;
	}
	ofs2 = p2 - data;
	p3 = sfcre_SearchDelim(p2 + 1, size - ofs2 - 1, 0x10000);
	if (p3 == NULL) {
		return 0;
	}
	packsiz = p2 - p1;
	if (packsiz != p3 - p2) {
		return -1;
	}
	if (ofs1 % packsiz != 0) {
		return -1;
	}
	mps = MPS_Create();
	if (mps != NULL) {
		MPS_DecHd(mps, p1, n1, &len, &flags);
		if (flags & 0x10000) {
			MPS_GetPackHd(mps, &packhd);
			MPS_Destroy(mps);
			*mux_rate = packhd.mux_rate;
		}
	}
	return packsiz;
}

#pragma dont_inline on
/* COMPILER-DIFF: M1 - the original computes `ofs + 1` into a fresh r0 (ours in place) and numbers
 * the header bytes b4 r4, b7 r5, ofs r6, b6 r7, b5 r6 (reusing ofs's register); pins of the byte
 * loads re-rank the others. Asm function (the original's instructions verbatim), C body under #else. */
#if 1 // COMPILER-DIFF: M1
asm Sint32 sfcre_AnalyMpv(Uint8 *data, Sint32 size, SFD_CREINF *inf)
{
	nofralloc
	stwu r1, -32(r1)
	mflr r0
	stw r0, 36(r1)
	stw r31, 28(r1)
	mr r31, r5
	stw r30, 24(r1)
	mr r30, r4
	stw r29, 20(r1)
	mr r29, r3
	b L10c
L28:
	mr r3, r29
	mr r4, r30
	li r5, 64
	bl MPV_SearchDelim
	cmplwi r3, 0
	bne L48
	li r3, 0
	b L118
L48:
	lbz r5, 7(r3)
	subf r6, r29, r3
	add r29, r6, r29
	lbz r4, 4(r3)
	rlwinm. r0, r5, 28, 28, 31
	lbz r7, 6(r3)
	addi r0, r6, 1
	lbz r6, 5(r3)
	lbz r8, 8(r3)
	addi r29, r29, 1
	lbz r9, 9(r3)
	subf r30, r0, r30
	lbz r10, 10(r3)
	lbz r11, 11(r3)
	beq L10c
	clrlwi r5, r5, 28
	cmplwi r5, 1
	blt L10c
	cmplwi r5, 8
	bgt L10c
	rlwinm. r0, r10, 27, 31, 31
	beq L10c
	rlwinm r3, r6, 28, 28, 31
	mr r0, r7
	rlwimi r3, r4, 4, 20, 27
	stw r3, 20(r31)
	rlwimi r0, r6, 8, 20, 23
	stw r0, 24(r31)
	lwz r0, 28(r31)
	cmpwi r0, 0
	bne Ld8
	rlwinm r0, r9, 2, 22, 29
	rlwimi r0, r8, 10, 14, 21
	rlwimi r0, r10, 26, 30, 31
	mulli r0, r0, 50
	stw r0, 28(r31)
Ld8:
	lis r4, sfcre_mpv_picrate@ha
	rlwinm r0, r11, 29, 27, 31
	rlwinm r5, r5, 2, 22, 29
	lis r3, SFD_tr_vd_mpv@ha
	addi r4, r4, sfcre_mpv_picrate@l
	rlwimi r0, r10, 5, 22, 26
	lwzx r5, r4, r5
	slwi r4, r0, 11
	addi r0, r3, SFD_tr_vd_mpv@l
	stw r5, 32(r31)
	stw r4, 36(r31)
	stw r0, 8(r31)
	b L114
L10c:
	cmpwi r30, 0
	bgt L28
L114:
	li r3, 1
L118:
	lwz r0, 36(r1)
	lwz r31, 28(r1)
	lwz r30, 24(r1)
	lwz r29, 20(r1)
	mtlr r0
	addi r1, r1, 32
	blr
}
#else
Sint32 sfcre_AnalyMpv(Uint8 *data, Sint32 size, SFD_CREINF *inf)
{
	Uint8 *p;
	Uint8 b4;
	Sint32 ofs;
	Uint8 b7;
	Uint8 b5;
	Uint8 b6;
	Uint8 b8;
	Uint8 b9;
	Uint8 b10;
	Uint8 b11;
	Uint8 picrate_code;

	while (size > 0) {
		p = (Uint8 *)MPV_SearchDelim((Sint8 *)data, size, 0x40);
		if (p == NULL) {
			return 0;
		}
		b4 = p[4];
		b5 = p[5];
		b6 = p[6];
		b7 = p[7];
		b8 = p[8];
		b9 = p[9];
		b10 = p[10];
		b11 = p[11];
		ofs = p - data;
		data = (Uint8 *)((Uint32)ofs + (Uint32)data);
		data++;
		size -= ofs + 1;
		if (((b7 >> 4) & 0xF) == 0) {
			continue;
		}
		picrate_code = b7 & 0xF;
		if (picrate_code < 1 || picrate_code > 8) {
			continue;
		}
		if (((b10 >> 5) & 1) == 0) {
			continue;
		}
		inf->picw = (b4 << 4) | ((b5 >> 4) & 0xF);
		inf->pich = ((b5 & 0xF) << 8) | b6;
		if (inf->bitrate == 0) {
			inf->bitrate = ((b8 << 10) | (b9 << 2) | ((b10 >> 6) & 3)) * 50;
		}
		inf->picrate = sfcre_mpv_picrate[picrate_code];
		inf->vbvsiz = (((b10 & 0x1F) << 5) | ((b11 >> 3) & 0x1F)) << 11;
		inf->vtrif = &SFD_tr_vd_mpv;
		break;
	}
	return 1;
}
#endif
#pragma dont_inline off

/* ADX header at a 4-byte phase of the data copied to the temporary buffer */
static Bool sfcre_AnalyAdxSub(Uint8 *data, Sint32 size, SFD_CREINF *inf)
{
	Sint32 n;
	Uint8 *p;
	Sint32 hdrsiz;

	n = SFCRE_TMPBUF_SIZE;
	if (size < SFCRE_TMPBUF_SIZE) {
		n = size;
	}
	memcpy(sfcre_tmpbuf, data, n);
	for (p = sfcre_tmpbuf; n > 0; p += 4, n -= 4) {
		if (SFADXT_IsHeader(p, n, &hdrsiz)) {
			inf->atrif = &SFD_tr_ad_adxt;
			inf->ach = p[7];
			inf->afreq = (p[8] << 24) | (p[9] << 16) | (p[10] << 8) | p[11];
			return 1;
		}
	}
	return 0;
}

Sint32 sfcre_AnalyAdx(Uint8 *data, Sint32 size, SFD_CREINF *inf)
{
	if (sfcre_AnalyAdxSub(data, size, inf)) {
		return 1;
	}
	if (sfcre_AnalyAdxSub(data + 2, size - 2, inf)) {
		return 1;
	}
	if (sfcre_AnalyAdxSub(data + 1, size - 1, inf)) {
		return 1;
	}
	if (sfcre_AnalyAdxSub(data + 3, size - 3, inf)) {
		return 1;
	}
	return 0;
}

/* audio: the payload of the first audio packet (stream id 0xC0..0xDF) */
/* COMPILER-DIFF: M1 - parameters r24..r26 below the locals (pins fix them) but the `end - p <= 6`
 * diamond (r3/r0) and the inlined sfcre_AnalyAau's fifteen field temporaries stay permuted. Asm
 * function (the original's instructions verbatim), C body under #else. */
#if 1 // COMPILER-DIFF: M1
asm void sfcre_AnalyAudio(Uint8 *data, Sint32 size, SFD_CREINF *inf)
{
	nofralloc
	stwu r1, -64(r1)
	mflr r0
	stw r0, 68(r1)
	stmw r24, 32(r1)
	mr r24, r3
	mr r25, r4
	mr r26, r5
	add r28, r24, r25
	lwz r27, 16(r5)
	b L65c
L44c:
	mr r29, r25
	mr r31, r24
	b L478
L458:
	mr r3, r31
	bl MPS_CheckDelim
	addis r0, r3, -4
	cmplwi r0, 0
	bne L470
	b L484
L470:
	addi r31, r31, 1
	addi r29, r29, -1
L478:
	cmpwi r29, 4
	bge L458
	li r31, 0
L484:
	cmplwi r31, 0
	beq L664
	lbz r0, 3(r31)
	cmplwi r0, 192
	blt L648
	cmplwi r0, 223
	bgt L648
	bl MPS_Create
	mr. r29, r3
	bne L4c8
	subf r3, r31, r28
	li r0, 6
	cmpwi r3, 6
	bgt L4c0
	mr r0, r3
L4c0:
	add r30, r31, r0
	b L4ec
L4c8:
	mr r4, r31
	subf r5, r31, r28
	addi r6, r1, 8
	addi r7, r1, 12
	bl MPS_DecHd
	mr r3, r29
	bl MPS_Destroy
	lwz r0, 8(r1)
	add r30, r31, r0
L4ec:
	subf r0, r30, r28
	mr r29, r27
	cmpw r0, r27
	bge L500
	mr r29, r0
L500:
	mr r3, r30
	mr r4, r29
	mr r5, r26
	bl sfcre_AnalyAdx
	cmpwi r3, 0
	bne L664
	addi r0, r29, -3
	mtctr r0
	cmpwi r29, 4
	blt L5c8
L528:
	lbz r0, 0(r30)
	cmplwi r0, 255
	bne L5bc
	lbz r4, 1(r30)
	rlwinm r0, r4, 0, 24, 28
	cmpwi r0, 248
	bne L5bc
	lbz r3, 2(r30)
	rlwinm. r12, r4, 31, 30, 31
	lbz r0, 3(r30)
	clrlwi r11, r4, 31
	rlwinm r10, r3, 28, 28, 31
	rlwinm r9, r3, 30, 30, 31
	rlwinm r6, r0, 26, 30, 31
	rlwinm r5, r0, 28, 30, 31
	rlwinm r8, r3, 31, 31, 31
	clrlwi r7, r3, 31
	rlwinm r4, r0, 29, 31, 31
	rlwinm r3, r0, 30, 31, 31
	clrlwi r0, r0, 30
	stb r12, 16(r1)
	stb r11, 17(r1)
	stb r10, 18(r1)
	stb r9, 19(r1)
	stb r8, 20(r1)
	stb r7, 21(r1)
	stb r6, 22(r1)
	stb r5, 23(r1)
	stb r4, 24(r1)
	stb r3, 25(r1)
	stb r0, 26(r1)
	beq L5bc
	cmplwi r10, 15
	beq L5bc
	cmplwi r9, 3
	beq L5bc
	b L5cc
L5bc:
	addi r30, r30, 1
	addi r29, r29, -1
	bdnz L528
L5c8:
	li r30, 0
L5cc:
	cmplwi r30, 0
	beq L63c
	lbz r4, 22(r1)
	lis r3, sfcre_aau_ch@ha
	lbz r0, 19(r1)
	li r6, 0
	slwi r4, r4, 2
	addi r3, r3, sfcre_aau_ch@l
	lwzx r5, r3, r4
	lis r3, sfcre_aau_freq@ha
	stw r6, 12(r26)
	addi r4, r3, sfcre_aau_freq@l
	slwi r0, r0, 2
	lis r3, sfcre_aauhdr@ha
	stb r5, 40(r26)
	addi r6, r3, sfcre_aauhdr@l
	lwzx r0, r4, r0
	li r7, 1
	lwz r5, 16(r1)
	stw r0, 44(r26)
	lwz r4, 20(r1)
	lhz r3, 24(r1)
	lbz r0, 26(r1)
	stw r5, 0(r6)
	stw r4, 4(r6)
	sth r3, 8(r6)
	stb r0, 10(r6)
	b L640
L63c:
	li r7, 0
L640:
	cmpwi r7, 0
	bne L664
L648:
	subf r3, r24, r31
	add r24, r3, r24
	addi r0, r3, 1
	subf r25, r0, r25
	addi r24, r24, 1
L65c:
	cmpwi r25, 0
	bgt L44c
L664:
	lmw r24, 32(r1)
	lwz r0, 68(r1)
	mtlr r0
	addi r1, r1, 64
	blr
}
#else
void sfcre_AnalyAudio(Uint8 *data, Sint32 size, SFD_CREINF *inf)
{
	Uint8 *end;
	Sint32 packsiz;
	Uint8 *p;
	Uint8 *q;
	Sint32 n;
	Sint32 ofs;
	MPS mps;
	Sint32 flags;
	Sint32 len;

	end = data + size;
	packsiz = inf->packsiz;
	while (size > 0) {
		p = sfcre_SearchDelim(data, size, 0x40000);
		if (p == NULL) {
			return;
		}
		if (p[3] >= 0xC0 && p[3] <= 0xDF) {
			mps = MPS_Create();
			if (mps == NULL) {
				n = 6;
				if (end - p <= 6) {
					n = end - p;
				}
				q = p + n;
			} else {
				MPS_DecHd(mps, p, end - p, &len, &flags);
				MPS_Destroy(mps);
				q = p + len;
			}
			n = packsiz;
			if (end - q < packsiz) {
				n = end - q;
			}
			if (sfcre_AnalyAdx(q, n, inf) != 0) {
				return;
			}
			if (sfcre_AnalyAau(q, n, inf) != 0) {
				return;
			}
		}
		ofs = p - data;
		data = (Uint8 *)((Uint32)ofs + (Uint32)data);
		data++;
		size -= ofs + 1;
	}
}
#endif

/* COMPILER-DIFF: M1 - callee-saved permutation of the inlined sfcre_AnalyPackSiz values (p1 r30,
 * n1 r28, mps r22 ...). Asm function (the original's instructions verbatim), C body under #else. */
#if 1 // COMPILER-DIFF: M1
asm Sint32 sfcre_AnalyMps(Uint8 *data, Sint32 size, SFD_CREINF *inf)
{
	nofralloc
	stwu r1, -80(r1)
	mflr r0
	stw r0, 84(r1)
	stmw r22, 40(r1)
	mr r24, r3
	mr r25, r4
	mr r26, r5
	li r27, 0
	mr r30, r24
	mr r28, r25
	b L6c4
L6a4:
	mr r3, r30
	bl MPS_CheckDelim
	addis r0, r3, -1
	cmplwi r0, 0
	bne L6bc
	b L6d0
L6bc:
	addi r30, r30, 1
	addi r28, r28, -1
L6c4:
	cmpwi r28, 4
	bge L6a4
	li r30, 0
L6d0:
	cmplwi r30, 0
	bne L6e0
	li r31, 0
	b L7f4
L6e0:
	subf r29, r24, r30
	addi r23, r30, 1
	subf r28, r29, r25
	addi r31, r28, -1
	b L714
L6f4:
	mr r3, r23
	bl MPS_CheckDelim
	addis r0, r3, -1
	cmplwi r0, 0
	bne L70c
	b L720
L70c:
	addi r23, r23, 1
	addi r31, r31, -1
L714:
	cmpwi r31, 4
	bge L6f4
	li r23, 0
L720:
	cmplwi r23, 0
	bne L730
	li r31, 0
	b L7f4
L730:
	subf r0, r24, r23
	addi r22, r23, 1
	subf r3, r0, r25
	addi r31, r3, -1
	b L764
L744:
	mr r3, r22
	bl MPS_CheckDelim
	addis r0, r3, -1
	cmplwi r0, 0
	bne L75c
	b L770
L75c:
	addi r22, r22, 1
	addi r31, r31, -1
L764:
	cmpwi r31, 4
	bge L744
	li r22, 0
L770:
	cmplwi r22, 0
	bne L780
	li r31, 0
	b L7f4
L780:
	subf r31, r30, r23
	subf r0, r23, r22
	cmpw r31, r0
	beq L798
	li r31, -1
	b L7f4
L798:
	divw r0, r29, r31
	mullw r0, r0, r31
	subf. r0, r0, r29
	beq L7b0
	li r31, -1
	b L7f4
L7b0:
	bl MPS_Create
	mr. r22, r3
	beq L7f4
	mr r4, r30
	mr r5, r28
	addi r6, r1, 8
	addi r7, r1, 12
	bl MPS_DecHd
	lwz r0, 12(r1)
	rlwinm. r0, r0, 0, 15, 15
	beq L7f4
	mr r3, r22
	addi r4, r1, 16
	bl MPS_GetPackHd
	mr r3, r22
	bl MPS_Destroy
	lwz r27, 28(r1)
L7f4:
	cmpwi r31, 0
	bne L804
	li r3, 0
	b L938
L804:
	cmpwi r31, -1
	stw r31, 16(r26)
	bne L818
	li r3, 1
	b L938
L818:
	cmpwi r27, -1
	beq L830
	cmpwi r27, 0
	ble L830
	mulli r0, r27, 50
	stw r0, 28(r26)
L830:
	lis r3, SFD_tr_sd_mps@ha
	mr r30, r25
	addi r0, r3, SFD_tr_sd_mps@l
	mr r29, r24
	stw r0, 4(r26)
	li r27, 0
	lwz r28, 16(r26)
L84c:
	mr r3, r29
	mr r4, r30
	bl SFHDS_IsSfdHeader
	cmpwi r3, 0
	bne L880
	cmpwi r27, 3
	add r29, r29, r28
	subf r30, r28, r30
	bge L914
	cmpwi r30, 0
	ble L914
	addi r27, r27, 1
	b L84c
L880:
	lis r3, sfcre_fhd@ha
	cmpwi r30, 2048
	addi r28, r3, sfcre_fhd@l
	li r0, 0
	stw r0, 0(r28)
	li r27, 2048
	bge L8a0
	mr r27, r30
L8a0:
	mr r4, r29
	mr r5, r27
	addi r3, r28, 148
	bl MEM_Copy
	stw r27, 144(r28)
	mr r3, r28
	bl SFHDS_ProcessHdr
	lwz r0, 0(r28)
	cmpwi r0, 0
	beq L914
	lwz r0, 12(r28)
	cmpwi r0, 0
	ble L8d8
	stw r0, 28(r26)
L8d8:
	lwz r0, 100(r28)
	cmpwi r0, 0
	ble L8e8
	stw r0, 20(r26)
L8e8:
	lwz r0, 104(r28)
	cmpwi r0, 0
	ble L8f8
	stw r0, 24(r26)
L8f8:
	lwz r0, 108(r28)
	cmpwi r0, 0
	ble L914
	lis r3, SFD_tr_vd_mpv@ha
	stw r0, 32(r26)
	addi r0, r3, SFD_tr_vd_mpv@l
	stw r0, 8(r26)
L914:
	mr r3, r24
	mr r4, r25
	mr r5, r26
	bl sfcre_AnalyAudio
	mr r3, r24
	mr r4, r25
	mr r5, r26
	bl sfcre_AnalyMpv
	li r3, 1
L938:
	lmw r22, 40(r1)
	lwz r0, 84(r1)
	mtlr r0
	addi r1, r1, 80
	blr
}
#else
Sint32 sfcre_AnalyMps(Uint8 *data, Sint32 size, SFD_CREINF *inf)
{
	Sint32 mux_rate = 0;
	Sint32 packsiz;
	Uint8 *p;
	Sint32 n;
	Sint32 i;
	Sint32 ps;

	packsiz = sfcre_AnalyPackSiz(data, size, &mux_rate);
	if (packsiz == 0) {
		return 0;
	}
	inf->packsiz = packsiz;
	if (packsiz == -1) {
		return 1;
	}
	if (mux_rate != -1 && mux_rate > 0) {
		inf->bitrate = mux_rate * 50;
	}
	inf->strif = &SFD_tr_sd_mps;
	p = data;
	n = size;
	i = 0;
	ps = inf->packsiz;
	for (;;) {
		if (SFHDS_IsSfdHeader(p, n)) {
			goto found;
		}
		p += ps;
		n -= ps;
		if (i >= 3) {
			break;
		}
		if (n <= 0) {
			break;
		}
		i++;
	}
	goto done;
found:
	sfcre_AnalySfdHdr(p, n, inf);
done:
	sfcre_AnalyAudio(data, size, inf);
	sfcre_AnalyMpv(data, size, inf);
	return 1;
}
#endif

void sfcre_AnalyCreInf(Uint8 *data, Sint32 size, SFD_CREINF *inf)
{
	memset(inf, 0, sizeof(SFD_CREINF));
	inf->creatable = 0;
	inf->avail = 0;
	inf->strif = NULL;
	inf->vtrif = NULL;
	inf->atrif = NULL;
	inf->packsiz = 0;
	inf->picw = 0;
	inf->pich = 0;
	inf->bitrate = 0;
	inf->picrate = 0;
	inf->vbvsiz = 0;
	inf->ach = 0;
	inf->afreq = 0;
	if (sfcre_AnalyMps(data, size, inf) != 0) {
		return;
	}
	if (sfcre_AnalyMpv(data, size, inf) != 0) {
		return;
	}
	if (sfcre_AnalyAdx(data, size, inf) != 0) {
		return;
	}
	if (sfcre_AnalyAau(data, size, inf) != 0) {
		return;
	}
}

void SFD_AnalyCreInf(void *data, Sint32 size, SFD_CREINF *inf)
{
	Sint32 cs;

	SFLIB_LockCs(&cs);
	sfcre_AnalyCreInf(data, size, inf);
	if (inf->vtrif != NULL || inf->atrif != NULL || inf->packsiz == -1) {
		inf->creatable = 1;
	}
	if (inf->vtrif != NULL || inf->atrif != NULL) {
		inf->avail = 1;
	}
	SFLIB_UnlockCs(&cs);
}
