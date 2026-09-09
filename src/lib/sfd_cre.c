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
