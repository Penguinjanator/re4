/* CRI Sofdec: file header (SFD header packet) analysis and storage (sfd_hds.c). The 0x800-byte raw
 * header handed in by the input driver is parsed through the SFH analyser into SFHDS_FHD. */
#include "cri_xpt.h"
#include "sfd.h"
#include "sfh.h"

extern void MEM_Copy(void *dst, const void *src, Uint32 nbytes);

#define SFHDS_SFH_NUM 0x20
#define SFHDS_SFH_WORK_SIZE 0x204

/* SFD condition ids */
#define SFD_COND_HDRCB_FN 0x4B
#define SFD_COND_HDRCB_OBJ 0x4C

typedef void (*SFHDS_HDRCB)(void *obj, Uint8 *data, Sint32 size);

static Uint8 sfhds_sfhlib_work[SFHDS_SFH_WORK_SIZE];

/* the seek work's copy of the file header, when the seek work is attached and no concatenation
 * is going on (a macro: `#pragma dont_inline` around sfhds_SetHdrRaw would keep a static helper
 * out of line there) */
#define SFHDS_GET_SEE_FHD(sfd) \
	((sfd)->see.wk == NULL ? NULL : SFMPS_GetConcatCnt(sfd) > 0 ? NULL : &(sfd)->see.wk->fhd)

Sint32 SFHDS_GetColType(SFD sfd)
{
	SFHDS_FHD *fhd = &sfd->fhd;
	SFHDS_VID *vid = &fhd->vid;

	if (fhd->valid == 0) {
		return -1;
	}
	if (vid->ftr_eff != 0) {
		return vid->ftr_coltype;
	}
	return -1;
}

/* muxer tool version as one number (1.10 -> 110) */
static Sint32 sfhds_GetVerNum(SFHDS_FHD *fhd)
{
	return fhd->ver_major * 100 + fhd->ver_minor;
}

Sint32 SFHDS_GetMuxVerNum(SFD sfd)
{
	if (sfd->fhd.valid != 0) {
		return sfhds_GetVerNum(&sfd->fhd);
	}
	return 0;
}

/* first stream id in [first, last] present in the header, 0 when none */
static Sint32 sfhds_SearchStmId(SFH sfh, Sint32 first, Sint32 last, Sint32 *exist)
{
	Sint32 id;

	for (id = first; id <= last; id++) {
		if (SFH_IsExistStmId(sfh, id, exist) && *exist != 0) {
			return id;
		}
	}
	return 0;
}

/* COMPILER-DIFF: M1 - the original allocates callee-saved registers as parameters (reverse order, r31 down) then locals (fhd r31, sfh r30, ver r29); ours locals first (OPEN since the first CRI pass). Asm function (the original's instructions verbatim). */
#if 1 // COMPILER-DIFF: M1
asm void sfhds_DoProcessHdr(SFH sfh, SFHDS_FHD *fhd)
{
	nofralloc
	stwu r1, -160(r1)
	mflr r0
	stw r0, 164(r1)
	stw r31, 156(r1)
	mr r31, r4
	addi r4, r1, 140
	stw r30, 152(r1)
	mr r30, r3
	stw r29, 148(r1)
	bl SFH_IsSfdHeader
	cmpwi r3, 0
	bne L94
	li r0, 0
	stw r0, 140(r1)
L94:
	lwz r0, 140(r1)
	cmpwi r0, 0
	beq L654
	mr r3, r30
	addi r4, r1, 132
	addi r5, r1, 128
	bl SFH_AnlyHdrToolVer
	cmpwi r3, 0
	bne Lc4
	li r0, 0
	stw r0, 132(r1)
	stw r0, 128(r1)
Lc4:
	lwz r0, 132(r1)
	mr r3, r30
	addi r4, r1, 136
	stw r0, 4(r31)
	lwz r0, 128(r1)
	stw r0, 8(r31)
	lwz r0, 4(r31)
	lwz r5, 8(r31)
	mulli r0, r0, 100
	add r29, r5, r0
	bl SFH_AnlyByteRate
	cmpwi r3, 0
	bne L100
	li r0, 0
	stw r0, 136(r1)
L100:
	cmpwi r29, 110
	bge L114
	lwz r0, 136(r1)
	neg r0, r0
	stw r0, 136(r1)
L114:
	lwz r0, 136(r1)
	mr r3, r30
	addi r4, r1, 104
	stw r0, 12(r31)
	bl SFH_AnlyHdrSiz
	cmpwi r3, 0
	bne L138
	li r0, -1
	b L13c
L138:
	lwz r0, 104(r1)
L13c:
	stw r0, 16(r31)
	mr r3, r30
	addi r4, r1, 100
	bl SFH_AnlyPackType
	cmpwi r3, 0
	bne L15c
	li r0, -1
	b L160
L15c:
	lwz r0, 100(r1)
L160:
	stw r0, 20(r31)
	mr r3, r30
	addi r4, r1, 96
	bl SFH_AnlyPketSizLen
	cmpwi r3, 0
	bne L180
	li r0, -1
	b L184
L180:
	lwz r0, 96(r1)
L184:
	stw r0, 24(r31)
	lwz r0, 24(r31)
	cmpwi r0, -1
	bne L19c
	li r0, 2
	stw r0, 24(r31)
L19c:
	mr r3, r30
	addi r4, r1, 92
	bl SFH_AnlyPackSiz
	cmpwi r3, 0
	bne L1b8
	li r0, -1
	b L1bc
L1b8:
	lwz r0, 92(r1)
L1bc:
	stw r0, 28(r31)
	mr r3, r30
	addi r4, r1, 88
	bl SFH_AnlyNumElemTot
	cmpwi r3, 0
	bne L1dc
	li r0, -1
	b L1e0
L1dc:
	lwz r0, 88(r1)
L1e0:
	stw r0, 32(r31)
	mr r3, r30
	addi r4, r1, 84
	bl SFH_AnlyNumElemAud
	cmpwi r3, 0
	bne L200
	li r0, -1
	b L204
L200:
	lwz r0, 84(r1)
L204:
	stw r0, 36(r31)
	mr r3, r30
	addi r4, r1, 80
	bl SFH_AnlyNumElemVid
	cmpwi r3, 0
	bne L224
	li r0, -1
	b L228
L224:
	lwz r0, 80(r1)
L228:
	stw r0, 40(r31)
	mr r3, r30
	addi r4, r1, 76
	bl SFH_AnlyNumElemPrv
	cmpwi r3, 0
	bne L248
	li r0, -1
	b L24c
L248:
	lwz r0, 76(r1)
L24c:
	stw r0, 44(r31)
	mr r3, r30
	addi r4, r1, 72
	bl SFH_AnlyMaxPlyLenAud
	cmpwi r3, 0
	bne L26c
	li r0, -1
	b L270
L26c:
	lwz r0, 72(r1)
L270:
	stw r0, 48(r31)
	mr r3, r30
	addi r4, r1, 68
	bl SFH_AnlyMaxPlyLenVid
	cmpwi r3, 0
	bne L290
	li r0, -1
	b L294
L290:
	lwz r0, 68(r1)
L294:
	stw r0, 52(r31)
	mr r3, r30
	addi r4, r1, 64
	bl SFH_AnlyMaxFrmNum
	cmpwi r3, 0
	bne L2b4
	li r0, -1
	b L2b8
L2b4:
	lwz r0, 64(r1)
L2b8:
	stw r0, 56(r31)
	mr r3, r30
	addi r5, r1, 124
	li r4, 189
	bl SFH_IsExistStmId
	cmpwi r3, 0
	beq L2e8
	lwz r0, 124(r1)
	cmpwi r0, 0
	beq L2e8
	li r0, 189
	b L2ec
L2e8:
	li r0, 0
L2ec:
	stw r0, 60(r31)
	mr r3, r30
	addi r5, r1, 120
	li r4, 191
	bl SFH_IsExistStmId
	cmpwi r3, 0
	beq L31c
	lwz r0, 120(r1)
	cmpwi r0, 0
	beq L31c
	li r0, 191
	b L320
L31c:
	li r0, 0
L320:
	stw r0, 64(r31)
	li r29, 192
L328:
	mr r3, r30
	clrlwi r4, r29, 24
	addi r5, r1, 116
	bl SFH_IsExistStmId
	cmpwi r3, 0
	beq L350
	lwz r0, 116(r1)
	cmpwi r0, 0
	beq L350
	b L360
L350:
	addi r29, r29, 1
	cmpwi r29, 223
	ble L328
	li r29, 0
L360:
	stw r29, 68(r31)
	li r29, 224
L368:
	mr r3, r30
	clrlwi r4, r29, 24
	addi r5, r1, 112
	bl SFH_IsExistStmId
	cmpwi r3, 0
	beq L390
	lwz r0, 112(r1)
	cmpwi r0, 0
	beq L390
	b L3a0
L390:
	addi r29, r29, 1
	cmpwi r29, 239
	ble L368
	li r29, 0
L3a0:
	stw r29, 72(r31)
	lwz r29, 68(r31)
	cmpwi r29, 0
	beq L450
	mr r3, r30
	clrlwi r4, r29, 24
	addi r5, r1, 60
	bl SFH_AnlyElemCodecAud
	cmpwi r3, 0
	bne L3d0
	li r0, -1
	b L3d4
L3d0:
	lwz r0, 60(r1)
L3d4:
	stw r0, 76(r31)
	mr r3, r30
	clrlwi r4, r29, 24
	addi r5, r1, 56
	bl SFH_AnlyElemLayer
	cmpwi r3, 0
	bne L3f8
	li r0, -1
	b L3fc
L3f8:
	lwz r0, 56(r1)
L3fc:
	stw r0, 80(r31)
	mr r3, r30
	clrlwi r4, r29, 24
	addi r5, r1, 52
	bl SFH_AnlyElemChNum
	cmpwi r3, 0
	bne L420
	li r0, -1
	b L424
L420:
	lwz r0, 52(r1)
L424:
	stw r0, 84(r31)
	mr r3, r30
	clrlwi r4, r29, 24
	addi r5, r1, 48
	bl SFH_AnlyElemSmpHz
	cmpwi r3, 0
	bne L448
	li r0, -1
	b L44c
L448:
	lwz r0, 48(r1)
L44c:
	stw r0, 88(r31)
L450:
	lwz r29, 72(r31)
	mr r3, r30
	addi r5, r1, 44
	clrlwi r4, r29, 24
	bl SFH_AnlyElemCodecVid
	cmpwi r3, 0
	bne L474
	li r0, -1
	b L478
L474:
	lwz r0, 44(r1)
L478:
	stw r0, 92(r31)
	mr r3, r30
	clrlwi r4, r29, 24
	addi r5, r1, 40
	bl SFH_AnlyElemBitRate
	cmpwi r3, 0
	bne L49c
	li r0, -1
	b L4a0
L49c:
	lwz r0, 40(r1)
L4a0:
	stw r0, 96(r31)
	mr r3, r30
	clrlwi r4, r29, 24
	addi r5, r31, 100
	addi r6, r31, 104
	bl SFH_AnlyElemPicSz
	cmpwi r3, 0
	bne L4cc
	li r0, -1
	stw r0, 100(r31)
	stw r0, 104(r31)
L4cc:
	mr r3, r30
	clrlwi r4, r29, 24
	addi r5, r1, 36
	bl SFH_AnlyElemPicRate
	cmpwi r3, 0
	bne L4ec
	li r0, -1
	b L4f0
L4ec:
	lwz r0, 36(r1)
L4f0:
	stw r0, 108(r31)
	mr r3, r30
	clrlwi r4, r29, 24
	addi r5, r1, 108
	bl SFH_IsEffFtrInf
	cmpwi r3, 0
	bne L514
	li r0, 0
	stw r0, 108(r1)
L514:
	lwz r3, 108(r1)
	neg r0, r3
	or r0, r0, r3
	srwi r0, r0, 31
	stw r0, 112(r31)
	lwz r0, 108(r1)
	cmpwi r0, 0
	beq L64c
	mr r3, r30
	clrlwi r4, r29, 24
	addi r5, r1, 32
	bl SFH_AnlyFtrColType
	cmpwi r3, 0
	bne L554
	li r0, -1
	b L558
L554:
	lwz r0, 32(r1)
L558:
	stw r0, 116(r31)
	mr r3, r30
	clrlwi r4, r29, 24
	addi r5, r1, 28
	bl SFH_AnlyFtrPicType
	cmpwi r3, 0
	bne L57c
	li r0, -1
	b L580
L57c:
	lwz r0, 28(r1)
L580:
	stw r0, 120(r31)
	mr r3, r30
	clrlwi r4, r29, 24
	addi r5, r1, 24
	bl SFH_AnlyFtrFixFlg
	cmpwi r3, 0
	bne L5a4
	li r0, -1
	b L5a8
L5a4:
	lwz r0, 24(r1)
L5a8:
	stw r0, 124(r31)
	mr r3, r30
	clrlwi r4, r29, 24
	addi r5, r1, 20
	bl SFH_AnlyFtrShcFixFlg
	cmpwi r3, 0
	bne L5cc
	li r0, -1
	b L5d0
L5cc:
	lwz r0, 20(r1)
L5d0:
	stw r0, 128(r31)
	mr r3, r30
	clrlwi r4, r29, 24
	addi r5, r1, 16
	bl SFH_AnlyFtrExpand
	cmpwi r3, 0
	bne L5f4
	li r0, -1
	b L5f8
L5f4:
	lwz r0, 16(r1)
L5f8:
	stw r0, 132(r31)
	mr r3, r30
	clrlwi r4, r29, 24
	addi r5, r1, 12
	bl SFH_AnlyFtrGopN
	cmpwi r3, 0
	bne L61c
	li r0, -1
	b L620
L61c:
	lwz r0, 12(r1)
L620:
	stw r0, 136(r31)
	mr r3, r30
	clrlwi r4, r29, 24
	addi r5, r1, 8
	bl SFH_AnlyFtrGopM
	cmpwi r3, 0
	bne L644
	li r0, -1
	b L648
L644:
	lwz r0, 8(r1)
L648:
	stw r0, 140(r31)
L64c:
	li r0, 1
	stw r0, 0(r31)
L654:
	lwz r0, 164(r1)
	lwz r31, 156(r1)
	lwz r30, 152(r1)
	lwz r29, 148(r1)
	mtlr r0
	addi r1, r1, 160
	blr
}
#else
void sfhds_DoProcessHdr(SFH sfh, SFHDS_FHD *fhd)
{
	Sint32 ver;
	Sint32 is_sfd;
	Sint32 rate;
	Sint32 major;
	Sint32 minor;
	Sint32 ex_prv1;
	Sint32 ex_prv2;
	Sint32 ex_aud;
	Sint32 ex_vid;
	Sint32 eff;
	Sint32 hdrsiz;
	Sint32 packtype;
	Sint32 pketsizlen;
	Sint32 packsiz;
	Sint32 numtot;
	Sint32 numaud;
	Sint32 numvid;
	Sint32 numprv;
	Sint32 maxplyaud;
	Sint32 maxplyvid;
	Sint32 maxfrm;
	Sint32 acodec;
	Sint32 layer;
	Sint32 chnum;
	Sint32 smphz;
	Sint32 vcodec;
	Sint32 bitrate;
	Sint32 picrate;
	Sint32 coltype;
	Sint32 pictype;
	Sint32 fixflg;
	Sint32 shcfixflg;
	Sint32 expand;
	Sint32 gopn;
	Sint32 gopm;
	Sint32 id;

	if (SFH_IsSfdHeader(sfh, &is_sfd) == 0) {
		is_sfd = 0;
	}
	if (is_sfd == 0) {
		return;
	}

	if (SFH_AnlyHdrToolVer(sfh, &major, &minor) == 0) {
		major = 0;
		minor = 0;
	}
	fhd->ver_major = major;
	fhd->ver_minor = minor;
	ver = sfhds_GetVerNum(fhd);

	if (SFH_AnlyByteRate(sfh, &rate) == 0) {
		rate = 0;
	}
	if (ver < 110) {
		rate = -rate;
	}
	fhd->byterate = rate;

	fhd->hdrsiz = (SFH_AnlyHdrSiz(sfh, &hdrsiz) == 0) ? -1 : hdrsiz;
	fhd->packtype = (SFH_AnlyPackType(sfh, &packtype) == 0) ? -1 : packtype;
	fhd->pketsizlen = (SFH_AnlyPketSizLen(sfh, &pketsizlen) == 0) ? -1 : pketsizlen;
	if (fhd->pketsizlen == -1) {
		fhd->pketsizlen = 2;
	}
	fhd->packsiz = (SFH_AnlyPackSiz(sfh, &packsiz) == 0) ? -1 : packsiz;
	fhd->numelem_tot = (SFH_AnlyNumElemTot(sfh, &numtot) == 0) ? -1 : numtot;
	fhd->numelem_aud = (SFH_AnlyNumElemAud(sfh, &numaud) == 0) ? -1 : numaud;
	fhd->numelem_vid = (SFH_AnlyNumElemVid(sfh, &numvid) == 0) ? -1 : numvid;
	fhd->numelem_prv = (SFH_AnlyNumElemPrv(sfh, &numprv) == 0) ? -1 : numprv;
	fhd->maxplylen_aud = (SFH_AnlyMaxPlyLenAud(sfh, &maxplyaud) == 0) ? -1 : maxplyaud;
	fhd->maxplylen_vid = (SFH_AnlyMaxPlyLenVid(sfh, &maxplyvid) == 0) ? -1 : maxplyvid;
	fhd->maxfrmnum = (SFH_AnlyMaxFrmNum(sfh, &maxfrm) == 0) ? -1 : maxfrm;

	if (SFH_IsExistStmId(sfh, 0xBD, &ex_prv1) && ex_prv1 != 0) {
		id = 0xBD;
	} else {
		id = 0;
	}
	fhd->stmid_prv1 = id;
	if (SFH_IsExistStmId(sfh, 0xBF, &ex_prv2) && ex_prv2 != 0) {
		id = 0xBF;
	} else {
		id = 0;
	}
	fhd->stmid_prv2 = id;
	fhd->stmid_aud = sfhds_SearchStmId(sfh, 0xC0, 0xDF, &ex_aud);
	fhd->stmid_vid = sfhds_SearchStmId(sfh, 0xE0, 0xEF, &ex_vid);

	id = fhd->stmid_aud;
	if (id != 0) {
		fhd->aud.codec = (SFH_AnlyElemCodecAud(sfh, id, &acodec) == 0) ? -1 : acodec;
		fhd->aud.layer = (SFH_AnlyElemLayer(sfh, id, &layer) == 0) ? -1 : layer;
		fhd->aud.chnum = (SFH_AnlyElemChNum(sfh, id, &chnum) == 0) ? -1 : chnum;
		fhd->aud.smphz = (SFH_AnlyElemSmpHz(sfh, id, &smphz) == 0) ? -1 : smphz;
	}

	id = fhd->stmid_vid;
	fhd->vid.codec = (SFH_AnlyElemCodecVid(sfh, id, &vcodec) == 0) ? -1 : vcodec;
	fhd->vid.bitrate = (SFH_AnlyElemBitRate(sfh, id, &bitrate) == 0) ? -1 : bitrate;
	if (SFH_AnlyElemPicSz(sfh, id, &fhd->vid.picw, &fhd->vid.pich) == 0) {
		fhd->vid.picw = -1;
		fhd->vid.pich = -1;
	}
	fhd->vid.picrate = (SFH_AnlyElemPicRate(sfh, id, &picrate) == 0) ? -1 : picrate;
	if (SFH_IsEffFtrInf(sfh, id, &eff) == 0) {
		eff = 0;
	}
	fhd->vid.ftr_eff = (eff != 0);
	if (eff != 0) {
		fhd->vid.ftr_coltype = (SFH_AnlyFtrColType(sfh, id, &coltype) == 0) ? -1 : coltype;
		fhd->vid.ftr_pictype = (SFH_AnlyFtrPicType(sfh, id, &pictype) == 0) ? -1 : pictype;
		fhd->vid.ftr_fixflg = (SFH_AnlyFtrFixFlg(sfh, id, &fixflg) == 0) ? -1 : fixflg;
		fhd->vid.ftr_shcfixflg = (SFH_AnlyFtrShcFixFlg(sfh, id, &shcfixflg) == 0) ? -1 : shcfixflg;
		fhd->vid.ftr_expand = (SFH_AnlyFtrExpand(sfh, id, &expand) == 0) ? -1 : expand;
		fhd->vid.ftr_gopn = (SFH_AnlyFtrGopN(sfh, id, &gopn) == 0) ? -1 : gopn;
		fhd->vid.ftr_gopm = (SFH_AnlyFtrGopM(sfh, id, &gopm) == 0) ? -1 : gopm;
	}
	fhd->valid = 1;
}
#endif

#pragma dont_inline on
void SFHDS_ProcessHdr(SFHDS_FHD *fhd)
{
	SFH sfh;

	sfh = SFH_Create(fhd->raw, fhd->rawsiz);
	if (sfh != NULL) {
		sfhds_DoProcessHdr(sfh, fhd);
		SFH_Destroy(sfh);
	}
}
#pragma dont_inline off

void SFHDS_ReprocessHdr(SFD sfd)
{
	SFHDS_FHD *src;

	src = SFHDS_GET_SEE_FHD(sfd);
	if (src != NULL) {
		sfd->fhd = *src;
		SFHDS_ProcessHdr(&sfd->fhd);
		sfd->numelem_aud = sfd->fhd.numelem_aud;
		sfd->numelem_vid = sfd->fhd.numelem_vid;
		sfd->numelem_prv = sfd->fhd.numelem_prv;
	}
}

#pragma dont_inline on
Sint32 sfhds_SetHdrRaw(SFD sfd, Uint8 *data, Sint32 size)
{
	SFHDS_HDRCB cbfn;
	void *cbobj;
	Sint32 n;
	SFHDS_FHD *dst;

	cbfn = (SFHDS_HDRCB)SFSET_GetCond(sfd, SFD_COND_HDRCB_FN);
	cbobj = (void *)SFSET_GetCond(sfd, SFD_COND_HDRCB_OBJ);
	if (cbfn != NULL) {
		cbfn(cbobj, data, size);
	}
	if (sfd->fhd.valid != 0) {
		return 0;
	}
	n = SFHDS_RAW_SIZE;
	if (size < SFHDS_RAW_SIZE) {
		n = size;
	}
	MEM_Copy(sfd->fhd.raw, data, n);
	sfd->fhd.rawsiz = n;
	SFHDS_ProcessHdr(&sfd->fhd);
	sfd->numelem_aud = sfd->fhd.numelem_aud;
	sfd->numelem_vid = sfd->fhd.numelem_vid;
	sfd->numelem_prv = sfd->fhd.numelem_prv;
	dst = SFHDS_GET_SEE_FHD(sfd);
	if (dst != NULL) {
		*dst = sfd->fhd;
	}
	return 1;
}
#pragma dont_inline off

Bool SFHDS_IsSfdHeader(void *data, Sint32 size)
{
	SFH sfh;
	Sint32 flag;

	sfh = SFH_Create(data, size);
	if (sfh == NULL) {
		return 0;
	}
	if (SFH_IsSfdHeader(sfh, &flag) == 0) {
		flag = 0;
	}
	SFH_Destroy(sfh);
	return flag;
}

#define SFHDS_STARTCODE_PRV2 0x1BF

/* big-endian 32-bit start code at p */
static Sint32 sfhds_GetStartCode(Uint8 *p)
{
	Sint32 code;

	code = (p[0] << 8) | p[1];
	code <<= 8;
	code |= p[2];
	code <<= 8;
	code |= p[3];
	return code;
}

/* set the header from the private stream 2 packet whose start code is at p */
static Bool sfhds_SetHdrPkt(SFD sfd, Uint8 *p, Sint32 len, Sint32 *result)
{
	if (SFHDS_IsSfdHeader(p - 12, len + 12) == 0) {
		return 0;
	}
	*result = sfhds_SetHdrRaw(sfd, p - 12, len + 12);
	return 1;
}

/* the file header travels in a private stream 2 packet: `data` points at its payload */
/* COMPILER-DIFF: M1 - as sfhds_DoProcessHdr (result r30, len r29, p r28, sfd r27). Asm function (the original's instructions verbatim). */
#if 1 // COMPILER-DIFF: M1
asm Bool SFHDS_SetHdr(SFD sfd, Sint32 type, Uint8 *data, Sint32 size, Sint32 *result)
{
	nofralloc
	stwu r1, -48(r1)
	mflr r0
	cmpwi r4, 2
	stw r0, 52(r1)
	li r0, 0
	stmw r27, 28(r1)
	mr r30, r7
	mr r27, r3
	stw r0, 0(r7)
	beq L940
	li r3, 0
	b La14
L940:
	addi r28, r5, -6
	lbz r5, -6(r5)
	lbz r4, 1(r28)
	addi r29, r6, 6
	lbz r3, 2(r28)
	mr r6, r4
	lbz r0, 3(r28)
	rlwimi r6, r5, 8, 16, 23
	slwi r6, r6, 8
	or r6, r6, r3
	slwi r6, r6, 8
	or r6, r6, r0
	cmpwi r6, 447
	beq L9ac
	lbz r0, -2(r28)
	addi r29, r29, 2
	lbz r3, -1(r28)
	addi r28, r28, -2
	rlwimi r3, r0, 8, 16, 23
	slwi r3, r3, 8
	or r3, r3, r5
	slwi r3, r3, 8
	or r3, r3, r4
	cmpwi r3, 447
	beq L9ac
	li r3, 0
	b La14
L9ac:
	addi r3, r28, -12
	addi r4, r29, 12
	bl SFH_Create
	mr. r31, r3
	bne L9c8
	li r0, 0
	b L9ec
L9c8:
	addi r4, r1, 8
	bl SFH_IsSfdHeader
	cmpwi r3, 0
	bne L9e0
	li r0, 0
	stw r0, 8(r1)
L9e0:
	mr r3, r31
	bl SFH_Destroy
	lwz r0, 8(r1)
L9ec:
	cmpwi r0, 0
	bne L9fc
	li r3, 0
	b La14
L9fc:
	mr r3, r27
	addi r4, r28, -12
	addi r5, r29, 12
	bl sfhds_SetHdrRaw
	stw r3, 0(r30)
	li r3, 1
La14:
	lmw r27, 28(r1)
	lwz r0, 52(r1)
	mtlr r0
	addi r1, r1, 48
	blr
}
#else
Bool SFHDS_SetHdr(SFD sfd, Sint32 type, Uint8 *data, Sint32 size, Sint32 *result)
{
	Uint8 *p;
	Sint32 len;

	*result = 0;
	if (type != 2) {
		return 0;
	}
	p = data - 6;
	len = size + 6;
	if (sfhds_GetStartCode(p) != SFHDS_STARTCODE_PRV2) {
		p -= 2;
		len += 2;
		if (sfhds_GetStartCode(p) != SFHDS_STARTCODE_PRV2) {
			return 0;
		}
	}
	return sfhds_SetHdrPkt(sfd, p, len, result);
}
#endif

void SFHDS_FinishFhd(SFHDS_FHD *fhd)
{
	fhd->valid = 0;
	fhd->byterate = 0;
	fhd->rawsiz = 0;
}

void SFHDS_InitFhd(SFHDS_FHD *fhd)
{
	fhd->valid = 0;
	fhd->ver_major = 0;
	fhd->ver_minor = 0;
	fhd->byterate = 0;
	fhd->rawsiz = 0;
}

void SFHDS_Init(void)
{
	SFH_Init(SFHDS_SFH_NUM, sfhds_sfhlib_work);
}
