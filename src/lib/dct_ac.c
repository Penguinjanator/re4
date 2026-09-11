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

/* the cosine argument of the (i, j) coefficient: a helper defined here creates its 0.5 and int->double
 * literals before DCT_AcInit's own (0.5, cvt, 0.3535, pi/8 = the target's .rodata order) */
static Float64 dctac_Cos(Float64 w, Sint32 j)
{
	return cos(w * (0.5 + (Float64)j));
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
 * (every build in build/compilers/GC pools; 1.3.2r pools nothing, -pooldata off also unpools .bss).
 * In the DOL, literal-only functions are never pooled (adx_dcd ADX_GetCoefficient, adx_sje, mpvabdec)
 * but a function that also references a string pools literals with it (adx_tlk ADXT_GetTime/Create).
 * CRI pass 18b: `#pragma pool_data off` removes the rodata pool but also the .bss pool the target
 * keeps (dctac_i_const = the .bss start, dctac_version_dummy at +0x400, dctac_f_const at +0x200), so
 * the .bss pool base and the two table pointers are asm-emitted pool addresses (COMPILER-DIFF: M2);
 * 7 words left: the base is coloured r29 (ours ranks it below the loop temporaries), r31 in the
 * target (reused by the int->double constant after the base dies), and the second `addi` slot. */
#pragma pool_data off // COMPILER-DIFF: M2
void DCT_AcInit(void)
{
	register Uint8 *bss; // COMPILER-DIFF: M2 (.bss pool base)
	register Uint8 *hi; // COMPILER-DIFF: M2
	register Float64 *ip; // COMPILER-DIFF: M2 (dctac_i_const row pointer = pool + 0)
	register Float64 *fp; // COMPILER-DIFF: M2 (dctac_f_const column pointer = pool + 0x200)
	Sint32 i;
	Sint32 j;
	Float64 c;
	Float64 w;
	Float64 v;

	asm { lis hi, dctac_i_const@ha; addi bss, hi, dctac_i_const@l } // COMPILER-DIFF: M2
	*(const Char8 **)(bss + 0x400) = DCT_GetVerStr(); /* dctac_version_dummy through the pool */
	asm { addi ip, bss, dctac_i_const@l } // COMPILER-DIFF: M2
	asm { addi fp, bss, dctac_f_const@l } // COMPILER-DIFF: M2
	for (i = 0; i < 8; i++) {
		if (i == 0) {
			c = 0.3535533905932738;
		} else {
			c = 0.5;
		}
		w = (DCTAC_PI / 8.0) * (Float64)i;
		for (j = 0; j < 8; j++) {
			v = c * dctac_Cos(w, j);
			ip[j] = v;
			fp[j * 8] = v;
		}
		ip += 8;
		fp++;
	}
}
#pragma pool_data on
