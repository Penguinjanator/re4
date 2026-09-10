/* ADX header / info decoding (adx_dcd.c): the 0x8000 header block, its AINF / loop / delay
 * extensions, the 0x8001 footer, the info code scan for linked files and the second-order
 * prediction coefficients of the encoder cut-off (MSL inline sqrt / sqrtf / fpclassify). */
#include <string.h>
#include "cri_xpt.h"

extern double cos(double x);
extern float __float_nan;

#define FP_NAN 1
#define FP_INFINITE 2
#define FP_ZERO 3
#define FP_NORMAL 4
#define FP_SUBNORMAL 5

#define ADX_LD16(p) (((p)[0] << 8) | (p)[1])
#define ADX_LD32(p) (((p)[0] << 24) | ((p)[1] << 16) | ((p)[2] << 8) | (p)[3])
#define ADX_HDR 0x8000
#define ADX_FOOTER 0x8001
#define ADX_AINF 0x41494E46

/* MSL math.h inline sqrt (double): three Newton steps on the reciprocal square root estimate */
static inline double adx_sqrt(double x)
{
	const double _half = .5;
	const double _three = 3.0;

	if (x > 0.0) {
		double guess = __frsqrte(x);
		guess = _half * guess * (_three - guess * guess * x);
		guess = _half * guess * (_three - guess * guess * x);
		guess = _half * guess * (_three - guess * guess * x);
		return x * guess;
	} else if (x == 0.0) {
		return 0.0;
	} else if (x < 0.0) {
		return __float_nan;
	}
	return x;
}

static inline long adx_fpclassifyf(float x)
{
	switch ((*(unsigned long *)&x) & 0x7f800000) {
	case 0x7f800000:
		if ((*(unsigned long *)&x) & 0x007fffff) {
			return FP_NAN;
		}
		return FP_INFINITE;
	case 0:
		if ((*(unsigned long *)&x) & 0x007fffff) {
			return FP_SUBNORMAL;
		}
		return FP_ZERO;
	default:
		return FP_NORMAL;
	}
}

static inline float adx_sqrtf(float x)
{
	const double _half = .5;
	const double _three = 3.0;

	if (x > 0.0f) {
		double guess = __frsqrte((double)x);
		guess = _half * guess * (_three - guess * guess * x);
		guess = _half * guess * (_three - guess * guess * x);
		guess = _half * guess * (_three - guess * guess * x);
		return (float)(x * guess);
	} else if (x < 0.0) {
		return __float_nan;
	} else if (adx_fpclassifyf(x) == FP_NAN) {
		return __float_nan;
	}
	return x;
}

/* header block length rounded to the block alignment (with / without the loop extension).
 * The leading constant keeps the add chain in source order (the constant becomes the trailing
 * `addi`); without it MWCC defers the leaf next to the strlen() call to the end of the chain. */
Sint32 ADX_CalcHdrInfoLen(Sint32 loop, Sint32 infolen, Sint32 ofst, Sint32 align)
{
	if (loop == 0) {
		return (Uint32)(0x1B + infolen + strlen("(c)CRI") + ofst + align) / align * align - ofst;
	}
	return (Uint32)(0x33 + infolen + strlen("(c)CRI") + ofst + align) / align * align - ofst;
}

Sint32 ADX_DecodeFooter(Uint8 *data, Sint32 len, Sint16 *ofst)
{
	if (len < 0x10) {
		return -1;
	}
	if (*(Uint16 *)data != ADX_FOOTER) {
		return -2;
	}
	*ofst = *(Sint16 *)(data + 2) + 4;
	return 0;
}

/* header check and encoder version bytes */
static Sint32 adx_GetVer(Uint8 *data, Sint32 len, Uint8 *major, Uint8 *minor)
{
	if (len < 0x14) {
		return -1;
	}
	if (*(Uint16 *)data != ADX_HDR) {
		return -2;
	}
	if (*(Sint16 *)(data + 2) < 0x10) {
		return -1;
	}
	*major = data[0x12];
	*minor = data[0x13];
	return 0;
}

Sint32 ADX_DecodeInfoAinf(Uint8 *data, Sint32 len, Sint32 *ainfsiz, void *ainf, Sint16 *a, Sint16 *b)
{
	Uint8 ver;
	Uint8 minor;
	Sint32 err;
	Sint32 need;
	Sint32 ofs;
	Uint8 *p;
	/* COMPILER-DIFF: M1 -- the target gives q the freed len register r4 and the ADX_LD32 byte temporaries
	 * r3/r7/r8; ours ranks the macro's temporaries first (q r8). With the four byte loads named as
	 * asm-defined `register` locals the volatiles are numbered in declaration order. */
	register Uint8 *q;
	register Uint32 b0;
	register Uint32 b1;
	register Uint32 b2;
	register Uint32 b3;

	*ainfsiz = 0;
	err = adx_GetVer(data, len, &ver, &minor);
	if (err != 0) {
		return err;
	}
	need = (ver == 4) ? 0x48 : 0x3C;
	if (len < need) {
		return -1;
	}
	if (*(Uint16 *)data != ADX_HDR) {
		return -2;
	}
	if (*(Sint16 *)(data + 2) < need - 4) {
		return -1;
	}
	/* default + conditional store defines ofs in its callee-saved register (`li r28`); the ternary
	 * computes a temporary and lets `ofs += 4` define the variable. */
	ofs = 0x14;
	if (ver == 4) {
		ofs = 0x20;
	}
	p = (Uint8 *)((Uint32)ofs + (Uint32)data);
	ofs += 4;
	if (*(Sint16 *)(p + 2) != 0) {
		ofs += 0x14;
	}
	q = data + ofs;
	asm {
		lbz b1, 1(q)
		lbz b0, 0(q)
		lbz b2, 2(q)
		lbz b3, 3(q)
	}
	if (((b0 << 24) | (b1 << 16) | (b2 << 8) | b3) != ADX_AINF) {
		return -2;
	}
	*ainfsiz = *(Sint32 *)(q + 4);
	memcpy(ainf, q + 8, 16);
	p = (Uint8 *)((Uint32)ofs + (Uint32)data);
	*a = *(Sint16 *)(p + 0x18);
	b[0] = *(Sint16 *)(p + 0x1C);
	b[1] = *(Sint16 *)(p + 0x1E);
	return 0;
}

Sint32 ADX_DecodeInfoExLoop(Uint8 *data, Sint32 len, Sint32 *lptype, Sint16 *nloop, Sint16 *lpflg, Sint32 *lpstart,
                            Sint32 *lpstartofst, Sint32 *lpend, Sint32 *lpendofst)
{
	Uint8 ver;
	Uint8 minor;
	Sint32 err;
	Sint32 need;
	Sint32 ofs;
	Uint8 *p;

	*nloop = 0;
	err = adx_GetVer(data, len, &ver, &minor);
	if (err != 0) {
		return err;
	}
	need = (ver == 4) ? 0x3C : 0x30;
	if (len < need) {
		return -1;
	}
	if (*(Uint16 *)data != ADX_HDR) {
		return -2;
	}
	if (*(Sint16 *)(data + 2) < need - 4) {
		return -1;
	}
	ofs = (ver == 4) ? 0x20 : 0x14;
	*lptype = *(Sint16 *)(data + ofs);
	p = (Uint8 *)((Uint32)ofs + (Uint32)data);
	*nloop = *(Sint16 *)(p + 2);
	if (*nloop != 1) {
		return -2;
	}
	*lpflg = *(Sint16 *)(p + 6);
	*lpstart = *(Sint32 *)(p + 8);
	*lpstartofst = *(Sint32 *)(p + 0xC);
	*lpend = *(Sint32 *)(p + 0x10);
	*lpendofst = *(Sint32 *)(p + 0x14);
	return 0;
}

/* initial delay of the two channels (version 4 headers only) */
Sint32 ADX_DecodeInfoExIdly(Uint8 *data, Sint32 len, Uint16 *idly, Uint16 *idly2)
{
	Uint8 ver;
	Uint8 minor;

	if (adx_GetVer(data, len, &ver, &minor) != 0) {
		return -1;
	}
	if (ver >= 4) {
		if (len < 0x20) {
			return -1;
		}
		if (*(Uint16 *)data != ADX_HDR) {
			return -2;
		}
		if (*(Sint16 *)(data + 2) < 0x1C) {
			return -1;
		}
		idly[0] = *(Uint16 *)(data + 0x18);
		idly2[0] = *(Uint16 *)(data + 0x1A);
		idly[1] = *(Uint16 *)(data + 0x1C);
		idly2[1] = *(Uint16 *)(data + 0x1E);
	} else {
		idly2[1] = 0;
		idly[1] = 0;
		idly2[0] = 0;
		idly[0] = 0;
	}
	return 0;
}

Sint32 ADX_DecodeInfoExVer(Uint8 *data, Sint32 len, Uint8 *major, Uint8 *minor)
{
	if (len < 0x14) {
		return -1;
	}
	if (*(Uint16 *)data != ADX_HDR) {
		return -2;
	}
	if (*(Sint16 *)(data + 2) < 0x10) {
		return -1;
	}
	*major = data[0x12];
	*minor = data[0x13];
	return 0;
}

Sint32 ADX_DecodeInfoExADPCM2(Uint8 *data, Sint32 len, Uint16 *cutoff)
{
	if (len < 0x12) {
		return -1;
	}
	if (*(Uint16 *)data != ADX_HDR) {
		return -2;
	}
	if (*(Sint16 *)(data + 2) < 0xE) {
		return -1;
	}
	*cutoff = *(Uint16 *)(data + 0x10);
	return 0;
}

Sint32 ADX_DecodeInfo(Uint8 *data, Sint32 len, Sint16 *hdrlen, Sint8 *fmt, Sint8 *bps, Sint8 *blksiz, Sint8 *nch,
                      Sint32 *sfreq, Sint32 *nsmpl, Sint32 *blksmpl)
{
	if (len < 0x10) {
		return -1;
	}
	if ((Uint16)ADX_LD16(data) != ADX_HDR) {
		return -2;
	}
	*hdrlen = ADX_LD16(data + 2) + 4;
	*fmt = data[4];
	*blksiz = data[5];
	*bps = data[6];
	*nch = data[7];
	*sfreq = ADX_LD32(data + 8);
	*nsmpl = ADX_LD32(data + 12);
	if (*bps == 0) {
		*blksmpl = 0;
	} else {
		*blksmpl = (*blksiz - 2) * 8 / *bps;
	}
	return 0;
}

/* offset of the first 0x8000 info code word in data (-1: none) */
Sint32 ADX_ScanInfoCode(Uint8 *data, Sint32 len, Sint16 *ofst)
{
	Sint32 i;
	Sint32 pos;

	pos = 0x7FFFFFFF;
	for (i = 0; i < len - 1; i += 2) {
		if (*(Sint16 *)(data + i) == -0x8000) {
			pos = (i < pos) ? i : pos;
			break;
		}
	}
	if (pos != 0x7FFFFFFF) {
		*ofst = pos;
		return 0;
	}
	*ofst = 0;
	return -1;
}

/* prediction coefficients (12-bit fixed point) of a high-pass with the given cut-off.
 * COMPILER-DIFF: M2 - the original loads the nine float literals of this function unpooled and
 * three of them (2.0, 3.0, 1.0f) through a materialised address (`lis; addi r5; lfd f9, 0(r5)`);
 * our 2.4.7 pools them through a `...rodata.0` base and its peephole folds any `addi`/`lfd` pair
 * (also inside inline asm blocks). The only form that reproduces the bytes is the whole function
 * as an asm function over named `static const` literals; the C body it encodes is kept under #else.
 * .rodata order = first reference of the named literals, so `adx_coef_pool_order` (dead, stripped
 * by strip_unused) references them in the original's literal order before the function. */
static const Float32 adxcoef_2pi = 6.2831855f; // COMPILER-DIFF: M2
static const Float64 adxcoef_two = 2.0;
static const Float64 adxcoef_half = 0.5;
static const Float64 adxcoef_three = 3.0;
static const Float32 adxcoef_one = 1.0f;
static const Float32 adxcoef_zerof = 0.0f;
static const Float64 adxcoef_zero = 0.0;
static const Float32 adxcoef_4096 = 4096.0f;
static const Float32 adxcoef_twof = 2.0f;
static const Float64 adxcoef_cvt = 4503601774854144.0; /* 0x43300000_80000000, the Sint32 -> float bias */

asm void adx_coef_pool_order(void) // COMPILER-DIFF: M2 (dead; fixes the .rodata order of the literals above)
{
	nofralloc
	lis r3, adxcoef_2pi@ha
	lis r3, adxcoef_two@ha
	lis r3, adxcoef_half@ha
	lis r3, adxcoef_three@ha
	lis r3, adxcoef_one@ha
	lis r3, adxcoef_zerof@ha
	lis r3, adxcoef_zero@ha
	lis r3, adxcoef_4096@ha
	lis r3, adxcoef_twof@ha
	lis r3, adxcoef_cvt@ha
	blr
}

#if 1 // COMPILER-DIFF: M2
asm void ADX_GetCoefficient(Sint32 cutoff, Sint32 sfreq, Sint16 *c1, Sint16 *c2)
{
	nofralloc
	stwu r1, -48(r1)
	mflr r0
	lis r7, 17200
	stw r0, 52(r1)
	xoris r0, r3, 32768
	lis r3, adxcoef_cvt@ha
	stw r0, 20(r1)
	xoris r0, r4, 32768
	lfd f2, adxcoef_cvt@l(r3)
	lis r4, adxcoef_2pi@ha
	stw r7, 16(r1)
	lfs f3, adxcoef_2pi@l(r4)
	lfd f0, 16(r1)
	stw r0, 28(r1)
	fsubs f1, f0, f2
	stw r7, 24(r1)
	lfd f0, 24(r1)
	fmuls f1, f3, f1
	stw r31, 44(r1)
	mr r31, r6
	fsubs f0, f0, f2
	stw r30, 40(r1)
	mr r30, r5
	fdivs f1, f1, f0
	bl cos
	lis r3, adxcoef_two@ha
	lis r4, adxcoef_half@ha
	addi r5, r3, adxcoef_two@l
	lfd f2, adxcoef_half@l(r4)
	lfd f9, 0(r5)
	lis r3, adxcoef_three@ha
	addi r5, r3, adxcoef_three@l
	frsqrte f7, f9
	lis r3, adxcoef_one@ha
	frsqrte f10, f9
	addi r4, r3, adxcoef_one@l
	lis r3, adxcoef_zerof@ha
	lfd f0, 0(r5)
	fmul f5, f10, f10
	lfs f4, 0(r4)
	lfs f3, adxcoef_zerof@l(r3)
	fmul f6, f7, f7
	fmul f8, f2, f7
	fnmsub f7, f9, f6, f0
	fmul f6, f2, f10
	fnmsub f5, f9, f5, f0
	fmul f8, f8, f7
	fmul f6, f6, f5
	fmul f7, f8, f8
	fmul f5, f6, f6
	fmul f8, f2, f8
	fnmsub f7, f9, f7, f0
	fmul f6, f2, f6
	fnmsub f5, f9, f5, f0
	fmul f8, f8, f7
	fmul f6, f6, f5
	fmul f7, f8, f8
	fmul f5, f6, f6
	fmul f8, f2, f8
	fnmsub f7, f9, f7, f0
	fmul f6, f2, f6
	fnmsub f5, f9, f5, f0
	fmul f7, f8, f7
	fmul f5, f6, f5
	fmul f6, f9, f7
	fmul f5, f9, f5
	frsp f7, f1
	frsp f6, f6
	frsp f1, f5
	fsubs f5, f6, f7
	fsubs f6, f1, f4
	fadds f4, f5, f6
	fsubs f1, f5, f6
	fmuls f4, f4, f1
	fcmpo cr0, f4, f3
	ble L7b4
	frsqrte f3, f4
	fmul f1, f3, f3
	fmul f3, f2, f3
	fnmsub f1, f4, f1, f0
	fmul f3, f3, f1
	fmul f1, f3, f3
	fmul f3, f2, f3
	fnmsub f1, f4, f1, f0
	fmul f3, f3, f1
	fmul f1, f3, f3
	fmul f2, f2, f3
	fnmsub f0, f4, f1, f0
	fmul f0, f2, f0
	fmul f4, f4, f0
	frsp f4, f4
	b L83c
L7b4:
	lis r3, adxcoef_zero@ha
	lfd f0, adxcoef_zero@l(r3)
	fcmpo cr0, f4, f0
	bge L7d0
	lis r3, __float_nan@ha
	lfs f4, __float_nan@l(r3)
	b L83c
L7d0:
	stfs f4, 8(r1)
	lis r0, 32640
	lwz r4, 8(r1)
	rlwinm r3, r4, 0, 1, 8
	cmpw r3, r0
	beq L7f8
	bge L828
	cmpwi r3, 0
	beq L810
	b L828
L7f8:
	clrlwi. r0, r4, 9
	beq L808
	li r0, 1
	b L82c
L808:
	li r0, 2
	b L82c
L810:
	clrlwi. r0, r4, 9
	beq L820
	li r0, 5
	b L82c
L820:
	li r0, 3
	b L82c
L828:
	li r0, 4
L82c:
	cmpwi r0, 1
	bne L83c
	lis r3, __float_nan@ha
	lfs f4, __float_nan@l(r3)
L83c:
	fsubs f0, f5, f4
	lis r3, adxcoef_twof@ha
	lis r4, adxcoef_4096@ha
	lfs f1, adxcoef_twof@l(r3)
	lfs f2, adxcoef_4096@l(r4)
	fdivs f3, f0, f6
	fneg f0, f3
	fmuls f1, f1, f3
	fmuls f0, f0, f3
	fmuls f1, f2, f1
	fmuls f0, f2, f0
	fctiwz f1, f1
	fctiwz f0, f0
	stfd f1, 24(r1)
	stfd f0, 16(r1)
	lwz r3, 28(r1)
	lwz r0, 20(r1)
	sth r3, 0(r30)
	sth r0, 0(r31)
	lwz r0, 52(r1)
	lwz r31, 44(r1)
	lwz r30, 40(r1)
	mtlr r0
	addi r1, r1, 48
	blr
}
#else
void ADX_GetCoefficient(Sint32 cutoff, Sint32 sfreq, Sint16 *c1, Sint16 *c2)
{
	Float32 z;
	Float32 a;
	Float32 b;
	Float32 d;
	Float32 c;

	z = (Float32)cos(6.2831855f * (Float32)cutoff / (Float32)sfreq);
	a = (Float32)adx_sqrt(2.0) - z;
	b = (Float32)adx_sqrt(2.0) - 1.0f;
	d = adx_sqrtf((a + b) * (a - b));
	c = (a - d) / b;
	*c1 = (Sint16)(4096.0f * (2.0f * c));
	*c2 = (Sint16)(4096.0f * (-c * c));
}
#endif
