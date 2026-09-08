#ifndef GX_H
#define GX_H

// GX types needed by game code. dolphin/gx.h pulls in the CodeWarrior libc and cannot be
// compiled by ProDG/GCC; shares the dolphin/gx/GXStruct.h guard so both can coexist.

#include "types.h"

#ifndef _DOLPHIN_GX_GXSTRUCT_H_
#define _DOLPHIN_GX_GXSTRUCT_H_

typedef struct {
    u8 r, g, b, a;
} GXColor;

typedef struct {
    u32 dummy[8];
} GXTexObj;  // 0x20

typedef struct {
    u32 dummy[3];
} GXTlutObj;  // 0x0C

typedef struct {
    int viTVmode;               // 0x00
    u16 fbWidth;                // 0x04
    u16 efbHeight;              // 0x06
    u16 xfbHeight;              // 0x08
    u16 viXOrigin;              // 0x0A
    u16 viYOrigin;              // 0x0C
    u16 viWidth;                // 0x0E
    u16 viHeight;               // 0x10
    int xFBmode;                // 0x14
    u8 field_rendering;         // 0x18
    u8 aa;                      // 0x19
    u8 sample_pattern[12][2];   // 0x1A
    u8 vfilter[7];              // 0x32
} GXRenderModeObj;              // 0x3C

#endif

// Immediate-mode vertex FIFO (dolphin/gx/GXVert.h). Writes are volatile, so GCC reloads
// everything between them.
#ifndef __GXVERT_H__
#define __GXVERT_H__
typedef union {
    u8 u8;
    u16 u16;
    u32 u32;
    u64 u64;
    s8 s8;
    s16 s16;
    s32 s32;
    s64 s64;
    f32 f32;
    f64 f64;
} WGPipe;
// The FIFO is addressed as a member of a struct at 0xCC000000: that makes GCC materialise the
// address as `lis rX, 0xCC01` + `-0x8000(rX)` (what the game code has) instead of `lis`/`ori`
// into a base register, and lets it CSE globals across the volatile stores.
struct GXHwRegs {
    u8 pad_0[0x8000];
    volatile WGPipe wgpipe;   // 0xCC008000
};
#define GXWGFifo (((GXHwRegs*) 0xCC000000)->wgpipe)

static inline void GXPosition3f32(f32 x, f32 y, f32 z)
{
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

static inline void GXPosition3s16(s16 x, s16 y, s16 z)
{
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
    GXWGFifo.s16 = z;
}

static inline void GXColor4u8(u8 r, u8 g, u8 b, u8 a)
{
    GXWGFifo.u8 = r;
    GXWGFifo.u8 = g;
    GXWGFifo.u8 = b;
    GXWGFifo.u8 = a;
}

static inline void GXNormal3s8(s8 x, s8 y, s8 z)
{
    GXWGFifo.s8 = x;
    GXWGFifo.s8 = y;
    GXWGFifo.s8 = z;
}

static inline void GXTexCoord2f32(f32 s, f32 t)
{
    GXWGFifo.f32 = s;
    GXWGFifo.f32 = t;
}

static inline void GXPosition2u16(u16 x, u16 y)
{
    GXWGFifo.u16 = x;
    GXWGFifo.u16 = y;
}
#endif

// GX API entry points used by game code. Enum parameters are declared as plain ints: the
// SDK enum headers pull in the CodeWarrior libc.
#ifdef __cplusplus
extern "C" {
#endif
void GXSetBlendMode(int type, int src_factor, int dst_factor, int op);
void GXSetColorUpdate(u8 update_enable);
void GXSetCullMode(int mode);
void GXSetZMode(u8 compare_enable, int func, u8 update_enable);
void GXSetNumTexGens(u8 n);
void GXSetNumTevStages(u8 n);
void GXSetTevOp(int id, int mode);
void GXSetTevOrder(int stage, int coord, int map, int color);
void GXSetNumChans(u8 n);
void GXSetChanMatColor(int chan, GXColor color);
void GXSetChanCtrl(int chan, u8 enable, int amb_src, int mat_src, u32 light_mask, int diff_fn, int attn_fn);
void GXSetLineWidth(u8 width, int tex_offsets);
void GXSetAlphaCompare(int comp0, u8 ref0, int op, int comp1, u8 ref1);
void GXClearVtxDesc(void);
void GXSetVtxDesc(int attr, int type);
void GXSetVtxAttrFmt(int vtxfmt, int attr, int cnt, int type, u8 frac);
void GXLoadPosMtxImm(const f32 mtx[3][4], u32 id);
void GXLoadNrmMtxImm(const f32 mtx[3][4], u32 id);
void GXSetCurrentMtx(u32 id);
void GXSetProjection(const f32 mtx[4][4], int type);
void GXBegin(int type, int vtxfmt, u16 nverts);
void GXInitTexObj(GXTexObj* obj, void* image, u16 width, u16 height, int format, int wrap_s, int wrap_t, u8 mipmap);
void GXInitTexObjCI(GXTexObj* obj, void* image, u16 width, u16 height, int format, int wrap_s, int wrap_t, u8 mipmap, u32 tlut_name);
void GXInitTexObjLOD(GXTexObj* obj, int min_filt, int mag_filt, f32 min_lod, f32 max_lod, f32 lod_bias, u8 bias_clamp, u8 do_edge_lod, int max_aniso);
void GXInitTlutObj(GXTlutObj* tlut_obj, void* lut, int fmt, u16 n_entries);
void GXLoadTlut(GXTlutObj* tlut_obj, u32 tlut_name);
// filter units
void GXLoadTexObj(GXTexObj* obj, int id);
void GXSetTevColor(int id, GXColor color);
void GXSetTevColorIn(int stage, int a, int b, int c, int d);
void GXSetTevAlphaIn(int stage, int a, int b, int c, int d);
void GXSetTevColorOp(int stage, int op, int bias, int scale, u8 clamp, int out_reg);
void GXSetTevAlphaOp(int stage, int op, int bias, int scale, u8 clamp, int out_reg);
void GXSetTexCoordGen2(int dst_coord, int func, int src_param, u32 mtx, u8 normalize, u32 pt_texmtx);
void GXSetAlphaUpdate(u8 update_enable);
void GXSetCopyFilter(u8 aa, const u8 sample_pattern[12][2], u8 vf, const u8 vfilter[7]);
void GXSetTexCopySrc(u16 left, u16 top, u16 wd, u16 ht);
void GXSetTexCopyDst(u16 wd, u16 ht, int fmt, u8 mipmap);
void GXCopyTex(void* dest, u8 clear);
void GXPixModeSync(void);
void GXInvalidateTexAll(void);
#ifdef __cplusplus
}
#endif
#define GXSetTexCoordGen(dst_coord, func, src_param, mtx) GXSetTexCoordGen2(dst_coord, func, src_param, mtx, 0, 125)

#endif
