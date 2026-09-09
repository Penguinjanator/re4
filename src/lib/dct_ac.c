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

/* OPEN: the original addresses its four double literals individually (`lis/lfd @NNN@l`) while our
 * 2.4.7 pools >= 3 literals of a function through a `...rodata.0` base register (every build in
 * build/compilers/GC pools; 1.3.2r pools nothing, -pooldata off also unpools .bss). In the DOL,
 * literal-only functions are never pooled (adx_dcd ADX_GetCoefficient, adx_sje, mpvabdec) but a
 * function that also references a string pools literals with it (adx_tlk ADXT_GetTime/Create). */
void DCT_AcInit(void)
{
	Sint32 i;
	Sint32 j;
	Float64 c;
	Float64 v;

	dctac_version_dummy = DCT_GetVerStr();
	for (i = 0; i < 8; i++) {
		if (i == 0) {
			c = 0.3535533905932738;
		} else {
			c = 0.5;
		}
		for (j = 0; j < 8; j++) {
			v = c * cos((DCTAC_PI / 8.0) * (Float64)i * (0.5 + (Float64)j));
			dctac_i_const[i][j] = v;
			dctac_f_const[j][i] = v;
		}
	}
}
