/* CRI Sofdec MPEG video: 8x8 inverse DCT of the six blocks of a macroblock ("fsri": paired-single
 * separable IDCT with the row pass into the work buffer and the column pass written as 16-bit
 * pixels through GQR7). DC-only blocks are filled directly. The index helper interleaves two rows
 * of a block into one 16-entry row (the layout of the paired-single tables). */
#include "cri_xpt.h"
#include "mpv.h"

extern const Char8 *DCT_GetVerStr(void);
extern void DCT_AcInit(void);
extern void DCT_AcIdctDouble(Float64 *in, Float64 *out);
extern void *memset(void *dst, int c, Uint32 n);

/* DCT parameter block (MPV_OBJ + 0x78) */
typedef struct {
	Sint8 cbp[6];                   /* 0x00 block coded flags (0: DC only) */
	Uint8 pad06[0x18 - 0x06];
	Sint32 cnt0;                    /* 0x18 */
	Sint32 cnt1;                    /* 0x1C */
	Uint8 pad20[0x28 - 0x20];
	Sint32 cbp_msk;                 /* 0x28 coded block pattern, MSB first */
	Float32 *blk;                   /* 0x2C six 8x8 coefficient blocks */
	Sint16 **tbl;                   /* 0x30 output block pointers */
	Uint8 pad34[0x48 - 0x34];
	Uint8 *work;                    /* 0x48 row pass work buffer */
	Uint8 pad4C[0x54 - 0x4C];
} DCT_PA;

/* paired-single constants: C4 (x2), C6/C2 ... and the +-0.5 rounding pair for ps_sel */
Float32 B0TableOrg[12] = {
	0.70710677f, 0.70710677f, 2.6131258f, 2.6131258f, 1.0823922f, 1.0823922f,
	0.76536685f, 0.76536685f, 0.5f, 0.5f, -0.5f, -0.5f,
};

Float32 PreIDCT[64][64];
Float64 sfsd_scale_tbl[64];
const Char8 *dctfsri_version_dummy;

static const Float64 scale8[8] = {
	0.3535533905932738, 0.4903926402016152, 0.46193976625564337, 0.4157348061512726,
	0.3535533905932738, 0.2777851165098011, 0.1913417161825449, 0.09754516100806414,
};

void DCT_FsriTransCore(DCT_PA *pa, Sint32 cbp)
{
	register Sint8 *flg = pa->cbp;
	register Float32 *blk;
	register Sint16 **tbl;
	register Uint8 *work;
	register Float32 *src;
	register Uint8 *dst;
	register Sint16 *o;
	register Sint32 cnt;
	Sint32 i;
	int k;
	Float32 dc;
	Sint32 n;
	Uint32 *out;

	work = pa->work;
	asm {
		lis r5, B0TableOrg@ha
		addi r5, r5, B0TableOrg@l
		mr r7, r5
		psq_lu f1, 0x0(r7), 0, 0
		psq_lu f2, 0x8(r7), 0, 0
		psq_lu f3, 0x8(r7), 0, 0
		psq_lu f4, 0x8(r7), 0, 0
		psq_lu f5, 0x8(r7), 0, 0
		psq_lu f6, 0x8(r7), 0, 0
	}
	blk = pa->blk;
	tbl = pa->tbl;
	for (i = 0; i < 6; i++) {
		if (cbp < 0) {
			if (flg[0] == 0) {
				dc = blk[0];
				out = (Uint32 *)tbl[0];
				if (dc < 0.0) {
					n = (Sint16)(dc - 0.5f);
				} else {
					n = (Sint16)(0.5f + dc);
				}
				n = (n << 16) | (Uint16)n;
				out += 32;
				for (k = 0; k < 32; k++) {
					*--out = n;
				}
			} else {
				asm {
		lwz o, 0x0(tbl)
		subi src, blk, 0x8
		subi dst, work, 0x8
		li cnt, 0x4
L_80215114:
		psq_lu f0, 0x8(src), 0, 0
		psq_lu f7, 0x8(src), 0, 0
		psq_lu f8, 0x8(src), 0, 0
		psq_lu f9, 0x8(src), 0, 0
		psq_lu f10, 0x8(src), 0, 0
		psq_lu f11, 0x8(src), 0, 0
		psq_lu f12, 0x8(src), 0, 0
		psq_lu f13, 0x8(src), 0, 0
		ps_sub f31, f8, f12
		ps_add f12, f8, f12
		ps_sub f8, f0, f10
		ps_mul f31, f31, f1
		ps_add f10, f0, f10
		ps_sub f31, f31, f12
		ps_sub f0, f10, f12
		ps_add f12, f10, f12
		ps_sub f10, f8, f31
		ps_add f31, f8, f31
		ps_sub f8, f11, f9
		ps_add f9, f11, f9
		ps_sub f11, f7, f13
		ps_add f13, f7, f13
		ps_sub f7, f13, f9
		ps_add f9, f13, f9
		ps_sub f13, f8, f11
		ps_mul f11, f11, f3
		ps_mul f8, f8, f2
		ps_mul f13, f13, f4
		ps_mul f7, f7, f1
		ps_sub f11, f11, f13
		ps_sub f8, f8, f13
		ps_sub f13, f12, f9
		ps_sub f11, f11, f9
		ps_add f9, f12, f9
		ps_sub f7, f7, f11
		ps_sub f12, f31, f11
		ps_add f11, f31, f11
		ps_sub f8, f8, f7
		ps_sub f31, f0, f8
		ps_add f8, f0, f8
		ps_sub f0, f10, f7
		ps_add f7, f10, f7
		ps_merge00 f10, f9, f11
		psq_stu f10, 0x8(dst), 0, 0
		ps_merge00 f10, f7, f8
		psq_stu f10, 0x8(dst), 0, 0
		ps_merge00 f10, f31, f0
		psq_stu f10, 0x8(dst), 0, 0
		ps_merge00 f10, f12, f13
		psq_stu f10, 0x8(dst), 0, 0
		ps_merge11 f10, f9, f11
		psq_stu f10, 0x8(dst), 0, 0
		ps_merge11 f10, f7, f8
		psq_stu f10, 0x8(dst), 0, 0
		ps_merge11 f10, f31, f0
		psq_stu f10, 0x8(dst), 0, 0
		ps_merge11 f10, f12, f13
		subic. cnt, cnt, 0x1
		psq_stu f10, 0x8(dst), 0, 0
		bgt L_80215114
		mr dst, o
		mr src, work
		li cnt, 0x4
L_80215210:
		psq_lu f0, 0x0(src), 0, 0
		psq_lu f11, 0x20(src), 0, 0
		psq_lu f12, 0x20(src), 0, 0
		psq_lu f9, 0x20(src), 0, 0
		psq_lu f31, 0x20(src), 0, 0
		psq_lu f8, 0x20(src), 0, 0
		psq_lu f13, 0x20(src), 0, 0
		psq_lu f7, 0x20(src), 0, 0
		ps_sub f10, f12, f13
		ps_add f13, f12, f13
		ps_sub f12, f0, f31
		ps_mul f10, f10, f1
		ps_add f31, f0, f31
		ps_sub f10, f10, f13
		ps_sub f0, f31, f13
		ps_add f13, f31, f13
		ps_sub f31, f12, f10
		ps_add f10, f12, f10
		ps_sub f12, f8, f9
		ps_add f9, f8, f9
		ps_sub f8, f11, f7
		ps_add f7, f11, f7
		ps_sub f11, f7, f9
		ps_add f9, f7, f9
		ps_sub f7, f12, f8
		ps_mul f8, f8, f3
		ps_mul f12, f12, f2
		ps_mul f7, f7, f4
		ps_mul f11, f11, f1
		ps_sub f8, f8, f7
		ps_sub f12, f12, f7
		ps_sub f7, f13, f9
		ps_sub f8, f8, f9
		ps_add f9, f13, f9
		ps_sub f11, f11, f8
		ps_sub f13, f10, f8
		ps_add f8, f10, f8
		ps_sub f12, f12, f11
		ps_sub f10, f0, f12
		ps_add f12, f0, f12
		ps_sub f0, f31, f11
		ps_add f11, f31, f11
		ps_sel f31, f9, f5, f6
		ps_add f9, f9, f31
		ps_sel f31, f8, f5, f6
		ps_add f8, f8, f31
		ps_sel f31, f11, f5, f6
		ps_add f11, f11, f31
		psq_stu f9, 0x0(dst), 0, 7
		ps_sel f31, f12, f5, f6
		ps_add f12, f12, f31
		psq_stu f8, 0x10(dst), 0, 7
		ps_sel f31, f10, f5, f6
		psq_stu f11, 0x10(dst), 0, 7
		ps_add f10, f10, f31
		psq_stu f12, 0x10(dst), 0, 7
		ps_sel f31, f0, f5, f6
		ps_add f0, f0, f31
		ps_sel f31, f13, f5, f6
		psq_stu f10, 0x10(dst), 0, 7
		ps_add f13, f13, f31
		psq_stu f0, 0x10(dst), 0, 7
		ps_sel f31, f7, f5, f6
		psq_stu f13, 0x10(dst), 0, 7
		ps_add f7, f7, f31
		subic. cnt, cnt, 0x1
		psq_stu f7, 0x10(dst), 0, 7
		ble done
		subi src, src, 0xd8
		subi dst, dst, 0x6c
		b L_80215210
		done:
				}
			}
		}
		cbp <<= 1;
		blk += 64;
		tbl++;
		flg++;
	}
}

/* interleaved position of coefficient i: two 8-entry rows become one 16-entry row */
static Sint32 dctfsri_Idx(Sint32 i)
{
	Sint32 q;
	Sint32 r;
	Sint32 n;

	q = i / 8;
	r = i % 8;
	if (q % 2 == 0) {
		n = r * 2;
	} else {
		n = r * 2 + 1;
		q--;
	}
	n += q * 8;
	if (n < 0 || n >= 256) {
		for (;;) {
		}
	}
	return n;
}

void DCT_FsriInitScanTbl(Sint8 *seq, Sint8 *scan)
{
	Sint32 i;

	for (i = 0; i < 64; i++) {
		scan[i] = dctfsri_Idx(seq[i]);
	}
}

void DCT_FsriTransCbp(DCT_PA *pa)
{
	DCT_FsriTransCore(pa, pa->cbp_msk);
}

void DCT_FsriTrans6Blk(DCT_PA *pa)
{
	DCT_FsriTransCore(pa, -1);
}

void DCT_FsriSetGqr(void)
{
	asm {
		li r0, 0x7
		oris r0, r0, 0x7
		mtspr GQR7, r0
	}
}

void initSparseTbl(void)
{
	Float64 in[64];
	Float64 out[64];
	Sint32 n;
	Sint32 k;

	memset(PreIDCT, 0, sizeof(PreIDCT));
	DCT_AcInit();
	for (n = 0; n < 64; n++) {
		for (k = 0; k < 64; k++) {
			if (k == n) {
				in[k] = 1.0 / sfsd_scale_tbl[k];
			} else {
				in[k] = 0.0;
			}
		}
		DCT_AcIdctDouble(in, out);
		for (k = 0; k < 64; k++) {
			PreIDCT[dctfsri_Idx(n)][k] = (Float32)out[k];
		}
	}
}

void DCT_FsriInitPa(DCT_PA *pa)
{
	memset(pa, 0, sizeof(DCT_PA));
}

void DCT_FsriInitScaleTbl(Float32 *tbl)
{
	Sint32 i;

	for (i = 0; i < 64; i++) {
		tbl[dctfsri_Idx(i)] = (Float32)sfsd_scale_tbl[i];
	}
}

void DCT_FsriInit(void)
{
	Sint32 i;
	Sint32 j;

	dctfsri_version_dummy = DCT_GetVerStr();
	for (i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++) {
			sfsd_scale_tbl[i * 8 + j] = scale8[i] * scale8[j];
		}
	}
	initSparseTbl();
}
