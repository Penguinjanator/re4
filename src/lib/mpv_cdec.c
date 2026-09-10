/* Sofdec MPEG video: macroblock coefficient decode (six 8x8 blocks) */
#include "cri_xpt.h"
#include "mpv.h"

void DCT_FsriTransCbp(Sint8 *cbp);
void DCT_FsriTrans6Blk(Sint8 *cbp);

#define MPVCDEC_NBLK 6

/* clear one 8x8 block (0x100 bytes) with 8-byte stores; fully unrolled (an `int` counter: with a
 * `Sint32`/long counter MWCC keeps the `cmpwi 0x20; bge` entry guard and reloads the 0.0) */
#define mpvcdec_ClearBlk(blk)                     \
	{                                             \
		int i;                                    \
		for (i = 0; i < 32; i++) {                \
			(blk)[i] = 0.0;                       \
		}                                         \
	}

Sint32 MPVCDEC_NintraBlocks(MPV mpv)
{
	Sint32 msk;
	Sint32 i;
	MPV_BLKPRM *prm;
	Sint8 *cbp;

	cbp = mpv->cbp;
	prm = &mpv->blkprm;
	prm->qscale = mpv->qscale;
	prm->iqm = mpv->nintra_iqm;
	prm->nintra = 1;
	msk = mpv->cbp_code << 2;
	mpv->cbp_msk = msk;
	for (i = 0; i < MPVCDEC_NBLK; i++) {
		if (msk < 0) {
			prm->dst = mpv->blk[i];
			cbp[i] = mpv->nintra_func(mpv, prm);
		}
		msk <<= 1;
	}
	DCT_FsriTransCbp(cbp);
	return 0;
}

/* COMPILER-DIFF: M1 - the original clears the six blocks with 192 unrolled stores, 83 through the
 * parameter register and the rest through a second base `mpv + 0x720` computed in the prologue;
 * ours switches to the r31 copy at store 64 (loop forms) or hoists the prm/cbp address block 12
 * instructions earlier (pinned forms). The function is an asm function (the original's
 * instructions verbatim over the named 0.0 literal); the C body it encodes is kept under #else. */
static const Float64 mpvcdec_zero = 0.0; // COMPILER-DIFF: M1

#if 1 // COMPILER-DIFF: M1
asm Sint32 MPVCDEC_IntraBlocks(MPV mpv)
{
	nofralloc
	stwu r1, -32(r1)
	mflr r0
	lis r4, mpvcdec_zero@ha
	stw r0, 36(r1)
	lfd f0, mpvcdec_zero@l(r4)
	stw r31, 28(r1)
	mr r31, r3
	addi r8, r31, 1824
	stw r30, 24(r1)
	stw r29, 20(r1)
	stfd f0, 1664(r3)
	stfd f0, 1672(r3)
	stfd f0, 1680(r3)
	stfd f0, 1688(r3)
	stfd f0, 1696(r3)
	stfd f0, 1704(r3)
	stfd f0, 1712(r3)
	stfd f0, 1720(r3)
	stfd f0, 1728(r3)
	stfd f0, 1736(r3)
	stfd f0, 1744(r3)
	stfd f0, 1752(r3)
	stfd f0, 1760(r3)
	stfd f0, 1768(r3)
	stfd f0, 1776(r3)
	stfd f0, 1784(r3)
	stfd f0, 1792(r3)
	stfd f0, 1800(r3)
	stfd f0, 1808(r3)
	stfd f0, 1816(r3)
	stfd f0, 1824(r3)
	stfd f0, 1832(r3)
	stfd f0, 1840(r3)
	stfd f0, 1848(r3)
	stfd f0, 1856(r3)
	stfd f0, 1864(r3)
	stfd f0, 1872(r3)
	stfd f0, 1880(r3)
	stfd f0, 1888(r3)
	stfd f0, 1896(r3)
	stfd f0, 1904(r3)
	stfd f0, 1912(r3)
	stfd f0, 1920(r3)
	stfd f0, 1928(r3)
	stfd f0, 1936(r3)
	stfd f0, 1944(r3)
	stfd f0, 1952(r3)
	stfd f0, 1960(r3)
	stfd f0, 1968(r3)
	stfd f0, 1976(r3)
	stfd f0, 1984(r3)
	stfd f0, 1992(r3)
	stfd f0, 2000(r3)
	stfd f0, 2008(r3)
	stfd f0, 2016(r3)
	stfd f0, 2024(r3)
	stfd f0, 2032(r3)
	stfd f0, 2040(r3)
	stfd f0, 2048(r3)
	stfd f0, 2056(r3)
	stfd f0, 2064(r3)
	stfd f0, 2072(r3)
	stfd f0, 2080(r3)
	stfd f0, 2088(r3)
	stfd f0, 2096(r3)
	stfd f0, 2104(r3)
	stfd f0, 2112(r3)
	stfd f0, 2120(r3)
	stfd f0, 2128(r3)
	stfd f0, 2136(r3)
	stfd f0, 2144(r3)
	stfd f0, 2152(r3)
	stfd f0, 2160(r3)
	stfd f0, 2168(r3)
	stfd f0, 2176(r3)
	stfd f0, 2184(r3)
	stfd f0, 2192(r3)
	stfd f0, 2200(r3)
	stfd f0, 2208(r3)
	stfd f0, 2216(r3)
	stfd f0, 2224(r3)
	stfd f0, 2232(r3)
	stfd f0, 2240(r3)
	stfd f0, 2248(r3)
	stfd f0, 2256(r3)
	stfd f0, 2264(r3)
	stfd f0, 2272(r3)
	stfd f0, 2280(r3)
	stfd f0, 2288(r3)
	stfd f0, 2296(r3)
	stfd f0, 2304(r3)
	stfd f0, 2312(r3)
	stfd f0, 2320(r3)
	stfd f0, 504(r8)
	stfd f0, 512(r8)
	stfd f0, 520(r8)
	stfd f0, 528(r8)
	stfd f0, 536(r8)
	stfd f0, 544(r8)
	stfd f0, 552(r8)
	stfd f0, 560(r8)
	stfd f0, 568(r8)
	stfd f0, 576(r8)
	stfd f0, 584(r8)
	stfd f0, 592(r8)
	stfd f0, 600(r8)
	stfd f0, 608(r8)
	stfd f0, 616(r8)
	stfd f0, 624(r8)
	stfd f0, 632(r8)
	stfd f0, 640(r8)
	stfd f0, 648(r8)
	stfd f0, 656(r8)
	stfd f0, 664(r8)
	stfd f0, 672(r8)
	stfd f0, 680(r8)
	stfd f0, 688(r8)
	stfd f0, 696(r8)
	stfd f0, 704(r8)
	stfd f0, 712(r8)
	stfd f0, 720(r8)
	stfd f0, 728(r8)
	stfd f0, 736(r8)
	stfd f0, 744(r8)
	stfd f0, 752(r8)
	stfd f0, 760(r8)
	stfd f0, 768(r8)
	stfd f0, 776(r8)
	stfd f0, 784(r8)
	stfd f0, 792(r8)
	stfd f0, 800(r8)
	stfd f0, 808(r8)
	stfd f0, 816(r8)
	stfd f0, 824(r8)
	stfd f0, 832(r8)
	stfd f0, 840(r8)
	stfd f0, 848(r8)
	stfd f0, 856(r8)
	stfd f0, 864(r8)
	stfd f0, 872(r8)
	stfd f0, 880(r8)
	stfd f0, 888(r8)
	stfd f0, 896(r8)
	stfd f0, 904(r8)
	stfd f0, 912(r8)
	stfd f0, 920(r8)
	stfd f0, 928(r8)
	stfd f0, 936(r8)
	stfd f0, 944(r8)
	stfd f0, 952(r8)
	stfd f0, 960(r8)
	stfd f0, 968(r8)
	stfd f0, 976(r8)
	stfd f0, 984(r8)
	stfd f0, 992(r8)
	stfd f0, 1000(r8)
	stfd f0, 1008(r8)
	stfd f0, 1016(r8)
	stfd f0, 1024(r8)
	stfd f0, 1032(r8)
	stfd f0, 1040(r8)
	stfd f0, 1048(r8)
	stfd f0, 1056(r8)
	stfd f0, 1064(r8)
	stfd f0, 1072(r8)
	stfd f0, 1080(r8)
	stfd f0, 1088(r8)
	stfd f0, 1096(r8)
	stfd f0, 1104(r8)
	stfd f0, 1112(r8)
	stfd f0, 1120(r8)
	stfd f0, 1128(r8)
	stfd f0, 1136(r8)
	stfd f0, 1144(r8)
	stfd f0, 1152(r8)
	stfd f0, 1160(r8)
	stfd f0, 1168(r8)
	stfd f0, 1176(r8)
	addi r30, r31, 68
	addi r7, r31, 3200
	li r6, 0
	stfd f0, 1184(r8)
	addi r5, r31, 844
	addi r0, r31, 1664
	mr r4, r30
	stfd f0, 1192(r8)
	addi r29, r31, 120
	stfd f0, 1200(r8)
	stfd f0, 1208(r8)
	stfd f0, 1216(r8)
	stfd f0, 1224(r8)
	stfd f0, 1232(r8)
	stfd f0, 1240(r8)
	stfd f0, 1248(r8)
	stfd f0, 1256(r8)
	stfd f0, 1264(r8)
	stfd f0, 1272(r8)
	stfd f0, 1280(r8)
	stfd f0, 1288(r8)
	stfd f0, 1296(r8)
	stfd f0, 1304(r8)
	stfd f0, 1312(r8)
	stfd f0, 1320(r8)
	stfd f0, 1328(r8)
	stfd f0, 1336(r8)
	stfd f0, 1344(r8)
	stfd f0, 1352(r8)
	stfd f0, 1360(r8)
	stfd f0, 1368(r8)
	lwz r8, 744(r31)
	stw r8, 104(r31)
	stw r7, 100(r31)
	stw r6, 116(r31)
	lwz r6, 4904(r31)
	stw r6, 112(r31)
	stw r5, 108(r31)
	stw r0, 96(r31)
	lwz r12, 4888(r31)
	mtctr r12
	bctrl
	stb r3, 0(r29)
	addi r0, r31, 1920
	mr r3, r31
	mr r4, r30
	stw r0, 28(r30)
	lwz r12, 4888(r31)
	mtctr r12
	bctrl
	stb r3, 1(r29)
	addi r0, r31, 2176
	mr r3, r31
	mr r4, r30
	stw r0, 28(r30)
	lwz r12, 4888(r31)
	mtctr r12
	bctrl
	stb r3, 2(r29)
	addi r0, r31, 2432
	mr r3, r31
	mr r4, r30
	stw r0, 28(r30)
	lwz r12, 4888(r31)
	mtctr r12
	bctrl
	stb r3, 3(r29)
	addi r5, r31, 848
	addi r0, r31, 2688
	mr r3, r31
	lwz r6, 4908(r31)
	mr r4, r30
	stw r6, 44(r30)
	stw r5, 40(r30)
	stw r0, 28(r30)
	lwz r12, 4888(r31)
	mtctr r12
	bctrl
	stb r3, 4(r29)
	addi r3, r31, 852
	addi r0, r31, 2944
	mr r4, r30
	stw r3, 40(r30)
	mr r3, r31
	stw r0, 28(r30)
	lwz r12, 4888(r31)
	mtctr r12
	bctrl
	stb r3, 5(r29)
	mr r3, r29
	bl DCT_FsriTrans6Blk
	lwz r0, 36(r1)
	li r3, 0
	lwz r31, 28(r1)
	lwz r30, 24(r1)
	lwz r29, 20(r1)
	mtlr r0
	addi r1, r1, 32
	blr
}
#else
Sint32 MPVCDEC_IntraBlocks(MPV mpv)
{
	MPV_BLKPRM *prm;
	Sint8 *cbp;

	mpvcdec_ClearBlk(mpv->blk[0]);
	mpvcdec_ClearBlk(mpv->blk[1]);
	mpvcdec_ClearBlk(mpv->blk[2]);
	mpvcdec_ClearBlk(mpv->blk[3]);
	mpvcdec_ClearBlk(mpv->blk[4]);
	mpvcdec_ClearBlk(mpv->blk[5]);
	prm = &mpv->blkprm;
	cbp = mpv->cbp;
	prm->qscale = mpv->qscale;
	prm->iqm = mpv->intra_iqm;
	prm->nintra = 0;
	prm->dctbl = mpv->dctbl_y;
	prm->dcpred = &mpv->dcpred[0];
	prm->dst = mpv->blk[0];
	cbp[0] = mpv->intra_func(mpv, prm);
	prm->dst = mpv->blk[1];
	cbp[1] = mpv->intra_func(mpv, prm);
	prm->dst = mpv->blk[2];
	cbp[2] = mpv->intra_func(mpv, prm);
	prm->dst = mpv->blk[3];
	cbp[3] = mpv->intra_func(mpv, prm);
	prm->dctbl = mpv->dctbl_c;
	prm->dcpred = &mpv->dcpred[1];
	prm->dst = mpv->blk[4];
	cbp[4] = mpv->intra_func(mpv, prm);
	prm->dcpred = &mpv->dcpred[2];
	prm->dst = mpv->blk[5];
	cbp[5] = mpv->intra_func(mpv, prm);
	DCT_FsriTrans6Blk(cbp);
	return 0;
}
#endif

void MPVCDEC_InitFrm(MPV mpv)
{
}
