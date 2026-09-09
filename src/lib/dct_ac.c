/* Sofdec DCT: double-precision reference (I)DCT by matrix multiplication */
#include "cri_xpt.h"
#include <math.h>

#define DCTAC_PI 3.141592653589793

const Char8 *DCT_GetVerStr(void);

Float64 dctac_i_const[8][8];
Float64 dctac_f_const[8][8];
static const Char8 *dctac_version_dummy;

static void dctac_TransDouble(Float64 *in, Float64 *out, Float64 *c)
{
	Float64 tmp[8][8];
	Float64 s;
	Sint32 i;
	Sint32 j;
	Sint32 k;

	for (i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++) {
			s = 0.0;
			for (k = 0; k < 8; k++) {
				s += c[k * 8 + j] * in[i * 8 + k];
			}
			tmp[i][j] = s;
		}
	}
	for (i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++) {
			s = 0.0;
			for (k = 0; k < 8; k++) {
				s += c[k * 8 + j] * tmp[k][i];
			}
			out[j * 8 + i] = s;
		}
	}
}

void DCT_AcIdctDouble(Float64 *in, Float64 *out)
{
	dctac_TransDouble(in, out, &dctac_i_const[0][0]);
}

/* dead-stripped by the linker */
void DCT_AcFdctDouble(Float64 *in, Float64 *out)
{
	dctac_TransDouble(in, out, &dctac_f_const[0][0]);
}

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
