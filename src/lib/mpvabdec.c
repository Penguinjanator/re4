/* CRI Sofdec MPEG video: the block coefficient decoders (MPEG-1 dct_coeff VLC table B.5,
 * dequantisation, zigzag store into the Float32 coefficient block). The AC loop is a 256-way
 * switch on an 8-bit look-ahead of the bit stream that decodes up to two coefficients (and a
 * following EOB) per step; look-aheads of longer codes fall back to the run/level tables. */
#include "cri_xpt.h"
#include "mpv.h"
#include "mpv_bit.h"

#define MPVABDEC_ESCAPE_RUN 0x40
#define MPVABDEC_D_PICTURE 4

#define MPVABDEC_LOAD_BIT()                                                                    \
	bbuf = mpv->bbuf;                                                                      \
	nbuf = mpv->nbuf;                                                                      \
	bitpos = mpv->bitpos;                                                                  \
	ptr = mpv->bitptr

#define MPVABDEC_SAVE_BIT()                                                                    \
	mpv->bbuf = bbuf;                                                                      \
	mpv->nbuf = nbuf;                                                                      \
	mpv->bitpos = bitpos;                                                                  \
	mpv->bitptr = ptr

/* the 32-bit look-ahead window */
#define MPVABDEC_PEEK32(val)                                                                   \
	val = bbuf;                                                                            \
	if (bitpos != 0) {                                                                     \
		val |= nbuf >> (32 - bitpos);                                                  \
	}

/* dequantised coefficient (forced odd) into the block at the zigzag position prm->idx */
#define MPVABDEC_STORE(coef, sgn)                                                              \
	val = ((coef) - 1) | 1;                                                                \
	if (sgn) {                                                                             \
		val = -val;                                                                    \
	}                                                                                      \
	((Float32 *)prm->dst)[prm->idx] = (Float32)val * ((Float32 *)mpv->scale_tbl)[prm->idx]

/* constant run/level/sign coefficients of the look-ahead cases */
#define MPVABDEC_AC1(run, level, sgn, n)                                                       \
	zz += (run);                                                                           \
	prm->idx = *++zz;                                                                      \
	MPVABDEC_STORE(MPVABDEC_COEF(level), sgn);                                             \
	MPVBIT_SKIP(n)

#define MPVABDEC_AC2(run1, level1, sgn1, run2, level2, sgn2, n)                                \
	zz += (run1);                                                                          \
	prm->idx = *++zz;                                                                      \
	MPVABDEC_STORE(MPVABDEC_COEF(level1), sgn1);                                           \
	zz += (run2);                                                                          \
	prm->idx = *++zz;                                                                      \
	MPVABDEC_STORE(MPVABDEC_COEF(level2), sgn2);                                           \
	MPVBIT_SKIP(n)

#define MPVABDEC_EOB(n) MPVBIT_SKIP(n)

/* the decoded (prm->run, prm->level, prm->sign) coefficient */
#define MPVABDEC_ACRL()                                                                        \
	zz = prm->run + zz;                                                                    \
	prm->idx = *++zz;                                                                      \
	MPVABDEC_STORE(MPVABDEC_COEFRL(), prm->sign)

/* run/level tables of the 11..17-bit codes: (level << 8) | run halfwords indexed by the code bits
 * after the leading zero (`code` holds the look-ahead shifted left by one) */
#define MPVABDEC_RLTBL(tbl, sh, n)                                                             \
	prm->len = (n);                                                                        \
	x = (Uint32)code >> (sh);                                                              \
	rl = ((Sint16 *)mpv->tbl)[x >> 1]

#define MPVABDEC_RLTBL4(tbl, sh, n)                                                            \
	prm->len = (n);                                                                        \
	x = ((Uint32)code >> (sh)) & 0x1F;                                                     \
	rl = ((Sint16 *)mpv->tbl)[x >> 1]

#define MPVABDEC_RLSET()                                                                       \
	prm->run = rl & 0xFF;                                                                  \
	prm->level = (Sint8)(rl >> 8);                                                         \
	prm->sign = x & 1

/* the same on the unshifted look-ahead (single-block cases of the switch) */
#define MPVABDEC_RLDIRECT(tbl, sh, msk, n)                                                     \
	prm->len = (n);                                                                        \
	rl = ((Sint16 *)mpv->tbl)[(code >> (sh)) & (msk)];                                     \
	prm->run = rl & 0xFF;                                                                  \
	prm->level = (Sint8)(rl >> 8);                                                         \
	prm->sign = (code >> ((sh) - 1)) & 1

/* 15..17-bit codes (nine leading zeros) */
#define MPVABDEC_RL15()                                                                        \
	if ((code << 8) < 0) {                                                                 \
		MPVABDEC_RLTBL4(rl_0a, 18, 15);                                                \
	} else if ((code << 9) < 0) {                                                          \
		MPVABDEC_RLTBL4(rl_0b, 17, 16);                                                \
	} else {                                                                               \
		MPVABDEC_RLTBL4(rl_0c, 16, 17);                                                \
	}

/* escape: 6-bit run, 8-bit level, 16-bit level when the first byte is 0x00/0x80 (x = the 16
 * bits after the escape code) */
#define MPVABDEC_ESCAPE()                                                                      \
	level = (Sint32)x >> 2;                                                                \
	prm->run = (Sint8)((Uint32)level >> 8);                                                \
	prm->len = 20;                                                                         \
	level = (Sint8)level;                                                                  \
	if ((level & 0x7F) == 0) {                                                             \
		prm->len += 8;                                                                 \
		level = (level << 1) | ((code >> 5) & 0xFF);                                   \
	}                                                                                      \
	if (level < 0) {                                                                       \
		prm->sign = 1;                                                                 \
		level = -level;                                                                \
	} else {                                                                               \
		prm->sign = 0;                                                                 \
	}                                                                                      \
	prm->level = level

/* codes up to 8 bits after the leading zero through the (len << 16) | (level << 8) | run table */
#define MPVABDEC_RL8(idx)                                                                      \
	rl = mpv->rl_8[idx];                                                                   \
	prm->run = rl & 0xFF;                                                                  \
	if (prm->run != MPVABDEC_ESCAPE_RUN) {                                                 \
		prm->len = rl >> 16;                                                           \
		prm->level = (Sint8)(rl >> 8);                                                 \
		prm->sign = ((Uint32)code >> (33 - prm->len)) & 1;                             \
	} else {                                                                               \
		x = (code >> 11) & 0xFFFF;                                                     \
		MPVABDEC_ESCAPE();                                                             \
	}

/* the look-ahead cases of codes longer than 8 bits */
#define MPVABDEC_CASE_00()                                                                     \
	code <<= 1;                                                                            \
	if ((code >> 24) & 0xFF) {                                                             \
		MPVABDEC_RLTBL(rl_1, 19, 14);                                                  \
	} else {                                                                               \
		MPVABDEC_RL15();                                                               \
	}                                                                                      \
	MPVABDEC_RLSET();                                                                      \
	MPVBIT_SKIP(prm->len);                                                                 \
	MPVABDEC_ACRL()

#define MPVABDEC_CASE_01()                                                                     \
	MPVABDEC_RLDIRECT(rl_2, 20, 0x7FF, 13);                                                \
	MPVBIT_SKIP(prm->len);                                                                 \
	MPVABDEC_ACRL()

#define MPVABDEC_CASE_02()                                                                     \
	MPVABDEC_RLDIRECT(rl_4, 22, 0x1FF, 11);                                                \
	MPVBIT_SKIP(prm->len);                                                                 \
	MPVABDEC_ACRL()

#define MPVABDEC_CASE_04()                                                                     \
	x = (code >> 10) & 0xFFFF;                                                             \
	code <<= 1;                                                                            \
	MPVABDEC_ESCAPE();                                                                     \
	MPVABDEC_ACRL();                                                                       \
	MPVBIT_SKIP(prm->len)

#define MPVABDEC_CASE_20()                                                                     \
	code <<= 1;                                                                            \
	MPVABDEC_RL8((code >> 25) & 0x7F);                                                     \
	MPVBIT_SKIP(prm->len);                                                                 \
	MPVABDEC_ACRL()

/* the AC coefficient loop: a switch on the top 8 bits of the look-ahead (case bodies in source
 * order: the coefficient cases from 0xFF down, then the cases ending in an EOB, 0xFE down) */
#define MPVABDEC_AC_LOOP()                                                                     \
	for (;;) {                                                                             \
		MPVABDEC_PEEK32(code);                                                         \
		switch ((code >> 24) & 0xFF) {                                                 \
		case 0xFF: case 0xFD: case 0xFC:                                               \
			MPVABDEC_AC2(0, 1, 1, 0, 1, 1, 6);                                     \
			continue;                                                              \
		case 0xFB: case 0xF9: case 0xF8:                                               \
			MPVABDEC_AC2(0, 1, 1, 0, 1, 0, 6);                                     \
			continue;                                                              \
		case 0xEF: case 0xEE:                                                          \
			MPVABDEC_AC2(0, 1, 1, 1, 1, 1, 7);                                     \
			continue;                                                              \
		case 0xED: case 0xEC:                                                          \
			MPVABDEC_AC2(0, 1, 1, 1, 1, 0, 7);                                     \
			continue;                                                              \
		case 0xEB:                                                                     \
			MPVABDEC_AC2(0, 1, 1, 2, 1, 1, 8);                                     \
			continue;                                                              \
		case 0xEA:                                                                     \
			MPVABDEC_AC2(0, 1, 1, 2, 1, 0, 8);                                     \
			continue;                                                              \
		case 0xE9:                                                                     \
			MPVABDEC_AC2(0, 1, 1, 0, 2, 1, 8);                                     \
			continue;                                                              \
		case 0xE8:                                                                     \
			MPVABDEC_AC2(0, 1, 1, 0, 2, 0, 8);                                     \
			continue;                                                              \
		case 0xE7: case 0xE6: case 0xE5: case 0xE4: case 0xE3: case 0xE2: case 0xE1: case 0xE0: \
			MPVABDEC_AC1(0, 1, 1, 3);                                              \
			continue;                                                              \
		case 0xDF: case 0xDD: case 0xDC:                                               \
			MPVABDEC_AC2(0, 1, 0, 0, 1, 1, 6);                                     \
			continue;                                                              \
		case 0xDB: case 0xD9: case 0xD8:                                               \
			MPVABDEC_AC2(0, 1, 0, 0, 1, 0, 6);                                     \
			continue;                                                              \
		case 0xCF: case 0xCE:                                                          \
			MPVABDEC_AC2(0, 1, 0, 1, 1, 1, 7);                                     \
			continue;                                                              \
		case 0xCD: case 0xCC:                                                          \
			MPVABDEC_AC2(0, 1, 0, 1, 1, 0, 7);                                     \
			continue;                                                              \
		case 0xCB:                                                                     \
			MPVABDEC_AC2(0, 1, 0, 2, 1, 1, 8);                                     \
			continue;                                                              \
		case 0xCA:                                                                     \
			MPVABDEC_AC2(0, 1, 0, 2, 1, 0, 8);                                     \
			continue;                                                              \
		case 0xC9:                                                                     \
			MPVABDEC_AC2(0, 1, 0, 0, 2, 1, 8);                                     \
			continue;                                                              \
		case 0xC8:                                                                     \
			MPVABDEC_AC2(0, 1, 0, 0, 2, 0, 8);                                     \
			continue;                                                              \
		case 0xC7: case 0xC6: case 0xC5: case 0xC4: case 0xC3: case 0xC2: case 0xC1: case 0xC0: \
			MPVABDEC_AC1(0, 1, 0, 3);                                              \
			continue;                                                              \
		case 0x7F: case 0x7E:                                                          \
			MPVABDEC_AC2(1, 1, 1, 0, 1, 1, 7);                                     \
			continue;                                                              \
		case 0x7D: case 0x7C:                                                          \
			MPVABDEC_AC2(1, 1, 1, 0, 1, 0, 7);                                     \
			continue;                                                              \
		case 0x77:                                                                     \
			MPVABDEC_AC2(1, 1, 1, 1, 1, 1, 8);                                     \
			continue;                                                              \
		case 0x76:                                                                     \
			MPVABDEC_AC2(1, 1, 1, 1, 1, 0, 8);                                     \
			continue;                                                              \
		case 0x75: case 0x74: case 0x73: case 0x72: case 0x71: case 0x70:              \
			MPVABDEC_AC1(1, 1, 1, 4);                                              \
			continue;                                                              \
		case 0x6F: case 0x6E:                                                          \
			MPVABDEC_AC2(1, 1, 0, 0, 1, 1, 7);                                     \
			continue;                                                              \
		case 0x6D: case 0x6C:                                                          \
			MPVABDEC_AC2(1, 1, 0, 0, 1, 0, 7);                                     \
			continue;                                                              \
		case 0x67:                                                                     \
			MPVABDEC_AC2(1, 1, 0, 1, 1, 1, 8);                                     \
			continue;                                                              \
		case 0x66:                                                                     \
			MPVABDEC_AC2(1, 1, 0, 1, 1, 0, 8);                                     \
			continue;                                                              \
		case 0x65: case 0x64: case 0x63: case 0x62: case 0x61: case 0x60:              \
			MPVABDEC_AC1(1, 1, 0, 4);                                              \
			continue;                                                              \
		case 0x5F:                                                                     \
			MPVABDEC_AC2(2, 1, 1, 0, 1, 1, 8);                                     \
			continue;                                                              \
		case 0x5E:                                                                     \
			MPVABDEC_AC2(2, 1, 1, 0, 1, 0, 8);                                     \
			continue;                                                              \
		case 0x5B: case 0x5A: case 0x59: case 0x58:                                    \
			MPVABDEC_AC1(2, 1, 1, 5);                                              \
			continue;                                                              \
		case 0x57:                                                                     \
			MPVABDEC_AC2(2, 1, 0, 0, 1, 1, 8);                                     \
			continue;                                                              \
		case 0x56:                                                                     \
			MPVABDEC_AC2(2, 1, 0, 0, 1, 0, 8);                                     \
			continue;                                                              \
		case 0x53: case 0x52: case 0x51: case 0x50:                                    \
			MPVABDEC_AC1(2, 1, 0, 5);                                              \
			continue;                                                              \
		case 0x4F:                                                                     \
			MPVABDEC_AC2(0, 2, 1, 0, 1, 1, 8);                                     \
			continue;                                                              \
		case 0x4E:                                                                     \
			MPVABDEC_AC2(0, 2, 1, 0, 1, 0, 8);                                     \
			continue;                                                              \
		case 0x4B: case 0x4A: case 0x49: case 0x48:                                    \
			MPVABDEC_AC1(0, 2, 1, 5);                                              \
			continue;                                                              \
		case 0x47:                                                                     \
			MPVABDEC_AC2(0, 2, 0, 0, 1, 1, 8);                                     \
			continue;                                                              \
		case 0x46:                                                                     \
			MPVABDEC_AC2(0, 2, 0, 0, 1, 0, 8);                                     \
			continue;                                                              \
		case 0x43: case 0x42: case 0x41: case 0x40:                                    \
			MPVABDEC_AC1(0, 2, 0, 5);                                              \
			continue;                                                              \
		case 0x3F: case 0x3D: case 0x3C:                                               \
			MPVABDEC_AC1(3, 1, 1, 6);                                              \
			continue;                                                              \
		case 0x3B: case 0x39: case 0x38:                                               \
			MPVABDEC_AC1(3, 1, 0, 6);                                              \
			continue;                                                              \
		case 0x37: case 0x35: case 0x34:                                               \
			MPVABDEC_AC1(4, 1, 1, 6);                                              \
			continue;                                                              \
		case 0x33: case 0x31: case 0x30:                                               \
			MPVABDEC_AC1(4, 1, 0, 6);                                              \
			continue;                                                              \
		case 0x2F: case 0x2D: case 0x2C:                                               \
			MPVABDEC_AC1(0, 3, 1, 6);                                              \
			continue;                                                              \
		case 0x2B: case 0x29: case 0x28:                                               \
			MPVABDEC_AC1(0, 3, 0, 6);                                              \
			continue;                                                              \
		case 0x27: case 0x26: case 0x25: case 0x24: case 0x23: case 0x22: case 0x21: case 0x20: \
			MPVABDEC_CASE_20();                                                    \
			continue;                                                              \
		case 0x1F: case 0x1E:                                                          \
			MPVABDEC_AC1(5, 1, 1, 7);                                              \
			continue;                                                              \
		case 0x1D: case 0x1C:                                                          \
			MPVABDEC_AC1(5, 1, 0, 7);                                              \
			continue;                                                              \
		case 0x1B: case 0x1A:                                                          \
			MPVABDEC_AC1(1, 2, 1, 7);                                              \
			continue;                                                              \
		case 0x19: case 0x18:                                                          \
			MPVABDEC_AC1(1, 2, 0, 7);                                              \
			continue;                                                              \
		case 0x17: case 0x16:                                                          \
			MPVABDEC_AC1(6, 1, 1, 7);                                              \
			continue;                                                              \
		case 0x15: case 0x14:                                                          \
			MPVABDEC_AC1(6, 1, 0, 7);                                              \
			continue;                                                              \
		case 0x13: case 0x12:                                                          \
			MPVABDEC_AC1(7, 1, 1, 7);                                              \
			continue;                                                              \
		case 0x11: case 0x10:                                                          \
			MPVABDEC_AC1(7, 1, 0, 7);                                              \
			continue;                                                              \
		case 0x0F:                                                                     \
			MPVABDEC_AC1(8, 1, 1, 8);                                              \
			continue;                                                              \
		case 0x0E:                                                                     \
			MPVABDEC_AC1(8, 1, 0, 8);                                              \
			continue;                                                              \
		case 0x0D:                                                                     \
			MPVABDEC_AC1(0, 4, 1, 8);                                              \
			continue;                                                              \
		case 0x0C:                                                                     \
			MPVABDEC_AC1(0, 4, 0, 8);                                              \
			continue;                                                              \
		case 0x0B:                                                                     \
			MPVABDEC_AC1(9, 1, 1, 8);                                              \
			continue;                                                              \
		case 0x0A:                                                                     \
			MPVABDEC_AC1(9, 1, 0, 8);                                              \
			continue;                                                              \
		case 0x09:                                                                     \
			MPVABDEC_AC1(2, 2, 1, 8);                                              \
			continue;                                                              \
		case 0x08:                                                                     \
			MPVABDEC_AC1(2, 2, 0, 8);                                              \
			continue;                                                              \
		case 0x07: case 0x06: case 0x05: case 0x04:                                    \
			MPVABDEC_CASE_04();                                                    \
			continue;                                                              \
		case 0x03: case 0x02:                                                          \
			MPVABDEC_CASE_02();                                                    \
			continue;                                                              \
		case 0x01:                                                                     \
			MPVABDEC_CASE_01();                                                    \
			continue;                                                              \
		case 0x00:                                                                     \
			MPVABDEC_CASE_00();                                                    \
			continue;                                                              \
		/* look-aheads ending in EOB */                                                \
		case 0xFE:                                                                     \
			MPVABDEC_AC2(0, 1, 1, 0, 1, 1, 8);                                     \
			break;                                                                 \
		case 0xFA:                                                                     \
			MPVABDEC_AC2(0, 1, 1, 0, 1, 0, 8);                                     \
			break;                                                                 \
		case 0xF7: case 0xF6: case 0xF5: case 0xF4: case 0xF3: case 0xF2: case 0xF1: case 0xF0: \
			MPVABDEC_AC1(0, 1, 1, 5);                                              \
			break;                                                                 \
		case 0xDE:                                                                     \
			MPVABDEC_AC2(0, 1, 0, 0, 1, 1, 8);                                     \
			break;                                                                 \
		case 0xDA:                                                                     \
			MPVABDEC_AC2(0, 1, 0, 0, 1, 0, 8);                                     \
			break;                                                                 \
		case 0xD7: case 0xD6: case 0xD5: case 0xD4: case 0xD3: case 0xD2: case 0xD1: case 0xD0: \
			MPVABDEC_AC1(0, 1, 0, 5);                                              \
			break;                                                                 \
		case 0xBF: case 0xBE: case 0xBD: case 0xBC: case 0xBB: case 0xBA: case 0xB9: case 0xB8: \
		case 0xB7: case 0xB6: case 0xB5: case 0xB4: case 0xB3: case 0xB2: case 0xB1: case 0xB0: \
		case 0xAF: case 0xAE: case 0xAD: case 0xAC: case 0xAB: case 0xAA: case 0xA9: case 0xA8: \
		case 0xA7: case 0xA6: case 0xA5: case 0xA4: case 0xA3: case 0xA2: case 0xA1: case 0xA0: \
		case 0x9F: case 0x9E: case 0x9D: case 0x9C: case 0x9B: case 0x9A: case 0x99: case 0x98: \
		case 0x97: case 0x96: case 0x95: case 0x94: case 0x93: case 0x92: case 0x91: case 0x90: \
		case 0x8F: case 0x8E: case 0x8D: case 0x8C: case 0x8B: case 0x8A: case 0x89: case 0x88: \
		case 0x87: case 0x86: case 0x85: case 0x84: case 0x83: case 0x82: case 0x81: case 0x80: \
			MPVABDEC_EOB(2);                                                       \
			break;                                                                 \
		case 0x7B: case 0x7A: case 0x79: case 0x78:                                    \
			MPVABDEC_AC1(1, 1, 1, 6);                                              \
			break;                                                                 \
		case 0x6B: case 0x6A: case 0x69: case 0x68:                                    \
			MPVABDEC_AC1(1, 1, 0, 6);                                              \
			break;                                                                 \
		case 0x5D: case 0x5C:                                                          \
			MPVABDEC_AC1(2, 1, 1, 7);                                              \
			break;                                                                 \
		case 0x55: case 0x54:                                                          \
			MPVABDEC_AC1(2, 1, 0, 7);                                              \
			break;                                                                 \
		case 0x4D: case 0x4C:                                                          \
			MPVABDEC_AC1(0, 2, 1, 7);                                              \
			break;                                                                 \
		case 0x45: case 0x44:                                                          \
			MPVABDEC_AC1(0, 2, 0, 7);                                              \
			break;                                                                 \
		case 0x3E:                                                                     \
			MPVABDEC_AC1(3, 1, 1, 8);                                              \
			break;                                                                 \
		case 0x3A:                                                                     \
			MPVABDEC_AC1(3, 1, 0, 8);                                              \
			break;                                                                 \
		case 0x36:                                                                     \
			MPVABDEC_AC1(4, 1, 1, 8);                                              \
			break;                                                                 \
		case 0x32:                                                                     \
			MPVABDEC_AC1(4, 1, 0, 8);                                              \
			break;                                                                 \
		case 0x2E:                                                                     \
			MPVABDEC_AC1(0, 3, 1, 8);                                              \
			break;                                                                 \
		case 0x2A:                                                                     \
			MPVABDEC_AC1(0, 3, 0, 8);                                              \
			break;                                                                 \
		}                                                                              \
		break;                                                                         \
	}

/* the return value: the zigzag position of the only coefficient, negative when there were more */
#define MPVABDEC_RESULT()                                                                      \
	MPVABDEC_SAVE_BIT();                                                                   \
	val = prm->idx;                                                                        \
	if (val != prm->idx0) {                                                                \
		val = -val;                                                                    \
	}                                                                                      \
	prm->idx = val;                                                                        \
	return prm->idx

/* non-intra dequantisation: (2 * level + 1) * quantiser_scale * iqm / 16 */
#define MPVABDEC_COEF(level) ((((level) * 2 + 1) * prm->qscale * prm->iqm[prm->idx]) >> 4)
#define MPVABDEC_COEFRL() ((prm->iqm[prm->idx] * ((prm->level * 2 + 1) * prm->qscale)) >> 4)

/* non-intra block: the first coefficient through the run/level tables ('1s' = run 0 level 1),
 * the rest through the look-ahead switch */

Sint32 MPVABDEC_NintraBlock(MPV mpv, MPV_BLKPRM *prm)
{
	Sint32 code;
	Sint8 *zz;
	Uint32 bbuf;
	Uint32 nbuf;
	Sint32 bitpos;
	Uint32 *ptr;
	Uint32 x;
	Uint32 rl;
	Sint32 level;
	Sint32 val;
	Uint32 n;
	Float64 *blk;

	blk = prm->dst;
	{
		int i;
		for (i = 0; i < 32; i++) {
			blk[i] = 0.0;
		}
	}
	MPVABDEC_LOAD_BIT();
	MPVABDEC_PEEK32(code);
	/* the first coefficient: '1s' is run 0 level 1 */
	if (code < 0) {
		prm->sign = (code >> 30) & 1;
		prm->level = 1;
		prm->run = 0;
		prm->len = 2;
	} else {
		code <<= 1;
		n = (code >> 24) & 0xFF;
		switch (n) {
		default:
			MPVABDEC_RL8(n >> 1);
			goto decoded;
		case 7:
		case 6:
		case 5:
		case 4:
			MPVABDEC_RLTBL(rl_4, 22, 11);
			break;
		case 3:
		case 2:
			MPVABDEC_RLTBL(rl_2, 20, 13);
			break;
		case 1:
			MPVABDEC_RLTBL(rl_1, 19, 14);
			break;
		case 0:
			MPVABDEC_RL15();
			break;
		}
		MPVABDEC_RLSET();
	}
decoded:
	MPVBIT_SKIP(prm->len);
	zz = mpv->zigzag_tbl;
	zz += prm->run;
	prm->idx = prm->idx0 = *zz;
	MPVABDEC_STORE(MPVABDEC_COEFRL(), prm->sign);
	MPVABDEC_AC_LOOP();
	MPVABDEC_RESULT();
}

/* intra dequantisation: 2 * level * quantiser_scale * iqm / 16 */
#undef MPVABDEC_COEF
#undef MPVABDEC_COEFRL
#define MPVABDEC_COEF(level) ((((level) * 2) * prm->qscale * prm->iqm[prm->idx]) >> 4)
#define MPVABDEC_COEFRL() ((prm->iqm[prm->idx] * ((prm->level << 1) * prm->qscale)) >> 4)

/* intra block, 8-bit DC (MPEG-1): dct_dc_size from the 7-bit look-ahead table */
Sint32 MPVABDEC_IntraBlock(MPV mpv, MPV_BLKPRM *prm)
{
	Uint32 bbuf;
	Uint32 nbuf;
	Sint32 bitpos;
	Uint32 *ptr;
	Sint8 *zz;
	Sint32 code;
	Uint32 x;
	Uint32 rl;
	Sint32 level;
	Sint32 val;
	Uint32 dcv;
	Sint32 len;
	Sint32 dc;

	MPVABDEC_LOAD_BIT();
	MPVBIT_PEEK(dcv, 16);
	x = ((Uint8 *)prm->dctbl)[dcv >> 9];
	dc = x >> 4;
	len = x & 0xF;
	if (dc != 0) {
		dcv &= ((Sint16 *)mpv->bitmsk_tbl)[len];
		len += dc;
		dcv >>= 16 - len;
		if ((dcv & (1 << (dc - 1))) == 0) {
			dcv += 1 - ((1 << (dc - 1)) << 1);
		}
		dc = dcv << 3;
	}
	MPVBIT_SKIP(len);
	dc += *prm->dcpred;
	*prm->dcpred = dc;
	((Float32 *)prm->dst)[0] = 0.125f * (Float32)dc;
	prm->idx0 = 0;
	prm->idx = 0;
	zz = mpv->zigzag_tbl;
	if (mpv->picatr.pic_type != MPVABDEC_D_PICTURE) {
		MPVABDEC_AC_LOOP();
	}
	MPVABDEC_RESULT();
}

/* intra block, 11-bit DC (MPEG-2 intra_dc_precision 3): 10-bit look-ahead table */
Sint32 MPVABDEC_IntraBlockDc11(MPV mpv, MPV_BLKPRM *prm)
{
	Uint32 bbuf;
	Uint32 nbuf;
	Sint32 bitpos;
	Uint32 *ptr;
	Sint32 code;
	Sint8 *zz;
	Uint32 x;
	Uint32 rl;
	Sint32 level;
	Sint32 val;
	Sint32 len;
	Sint32 dc;
	Sint32 dcv;

	MPVABDEC_LOAD_BIT();
	MPVABDEC_PEEK32(code);
	x = ((Uint8 *)prm->dctbl)[(code >> 22) & 0x3FF];
	dc = x >> 4;
	len = x & 0xF;
	if (dc != 0) {
		code <<= len;
		len += dc;
		dcv = (code >> 1) ^ 0x80000000;
		dc = ((Uint32)dcv >> 31) + (dcv >> (31 - dc));
	}
	MPVBIT_SKIP(len);
	dc += *prm->dcpred;
	*prm->dcpred = dc;
	((Float32 *)prm->dst)[0] = 0.125f * (Float32)dc;
	prm->idx0 = 0;
	prm->idx = 0;
	zz = mpv->zigzag_tbl;
	MPVABDEC_AC_LOOP();
	MPVABDEC_RESULT();
}
