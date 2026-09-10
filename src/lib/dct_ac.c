/* Sofdec DCT: double-precision reference (I)DCT by matrix multiplication */
#include "cri_xpt.h"
#include <math.h>

#define DCTAC_PI 3.141592653589793

const Char8 *DCT_GetVerStr(void);

Float64 dctac_i_const[8][8];
Float64 dctac_f_const[8][8];
static const Char8 *dctac_version_dummy;

static void dctac_TransDouble(Float64 *in, Float64 *out, Float64 c[8][8])
{
	Float64 tmp[64];
	Float64 s;
	Sint32 i;
	Sint32 j;
	Sint32 k;

	for (i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++) {
			s = 0.0;
			for (k = 0; k < 8; k++) {
				s += c[k][j] * in[i * 8 + k];
			}
			tmp[i * 8 + j] = s;
		}
	}
	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			s = 0.0;
			for (k = 0; k < 8; k++) {
				s += c[k][i] * tmp[k * 8 + j];
			}
			out[i * 8 + j] = s;
		}
	}
}

void DCT_AcIdctDouble(Float64 *in, Float64 *out)
{
	dctac_TransDouble(in, out, dctac_i_const);
}

/* dead-stripped by the linker */
void DCT_AcFdctDouble(Float64 *in, Float64 *out)
{
	dctac_TransDouble(in, out, dctac_f_const);
}

/* COMPILER-DIFF: M2 - the original addresses the four double literals of DCT_AcInit individually
 * (`lis/lfd @NNN@l`) while our 2.4.7 pools >= 3 literals of a function through a `...rodata.0` base
 * (every build in build/compilers/GC pools). The only form that reproduces the bytes is the whole
 * function as an asm function over named `static const` literals (the C body it encodes is kept
 * under #else); `dctac_pool_order` (dead, stripped by strip_unused) fixes the .rodata order of the
 * literals (first reference) and keeps `dctac_version_dummy` referenced (the asm stores it through
 * `dctac_i_const + 0x400`, the original's one-base .bss addressing). */
extern void _savefpr_27(void);
extern void _restfpr_27(void);
static const Float64 dctac_half = 0.5; // COMPILER-DIFF: M2
static const Float64 dctac_cvt = 4503601774854144.0; /* 0x43300000_80000000, the Sint32 -> double bias */
static const Float64 dctac_c0 = 0.3535533905932738;
static const Float64 dctac_pi8 = DCTAC_PI / 8.0;

asm void dctac_pool_order(void) // COMPILER-DIFF: M2 (dead)
{
	nofralloc
	lis r3, dctac_half@ha
	lis r3, dctac_cvt@ha
	lis r3, dctac_c0@ha
	lis r3, dctac_pi8@ha
	lis r3, dctac_version_dummy@ha
	blr
}

#if 1 // COMPILER-DIFF: M2
asm void DCT_AcInit(void)
{
	nofralloc
	stwu r1, -96(r1)
	mflr r0
	stw r0, 100(r1)
	addi r11, r1, 96
	bl _savefpr_27
	stmw r25, 28(r1)
	lis r3, dctac_i_const@ha
	addi r31, r3, dctac_i_const@l
	bl DCT_GetVerStr
	lis r5, dctac_half@ha
	lis r4, dctac_cvt@ha
	lis r6, dctac_pi8@ha
	stw r3, 1024(r31)
	lfd f30, dctac_half@l(r5)
	addi r28, r31, 0
	addi r27, r31, 512
	lfd f31, dctac_cvt@l(r4)
	lfd f29, dctac_pi8@l(r6)
	li r26, 0
	lis r31, 17200
L32c:
	cmpwi r26, 0
	bne L340
	lis r3, dctac_c0@ha
	lfd f28, dctac_c0@l(r3)
	b L348
L340:
	lis r3, dctac_half@ha
	lfd f28, dctac_half@l(r3)
L348:
	xoris r0, r26, 32768
	stw r31, 8(r1)
	mr r30, r28
	mr r29, r27
	stw r0, 12(r1)
	li r25, 0
	lfd f0, 8(r1)
	fsub f0, f0, f31
	fmul f27, f29, f0
L36c:
	xoris r0, r25, 32768
	stw r31, 8(r1)
	stw r0, 12(r1)
	lfd f0, 8(r1)
	fsub f0, f0, f31
	fadd f0, f30, f0
	fmul f1, f27, f0
	bl cos
	fmul f0, f28, f1
	addi r25, r25, 1
	cmpwi r25, 8
	stfd f0, 0(r30)
	addi r30, r30, 8
	stfd f0, 0(r29)
	addi r29, r29, 64
	blt L36c
	addi r26, r26, 1
	addi r27, r27, 8
	cmpwi r26, 8
	addi r28, r28, 64
	blt L32c
	addi r11, r1, 96
	bl _restfpr_27
	lmw r25, 28(r1)
	lwz r0, 100(r1)
	mtlr r0
	addi r1, r1, 96
	blr
}
#else
void DCT_AcInit(void)
{
	Sint32 i;
	Sint32 j;
	Float64 c;
	Float64 w;
	Float64 v;

	dctac_version_dummy = DCT_GetVerStr();
	for (i = 0; i < 8; i++) {
		if (i == 0) {
			c = 0.3535533905932738;
		} else {
			c = 0.5;
		}
		w = (DCTAC_PI / 8.0) * (Float64)i;
		for (j = 0; j < 8; j++) {
			v = c * cos(w * (0.5 + (Float64)j));
			dctac_i_const[i][j] = v;
			dctac_f_const[j][i] = v;
		}
	}
}
#endif
