#ifndef FLR_AT_H
#define FLR_AT_H

#include "types.h"
#include "vec.h"

// Floor attribute record returned by FlrAtCheck (game/flr_at.cpp). The layout depends on the
// attribute type asked for; only the bytes game/snd.cpp reads are named.
struct FlrAt {
    u8 x0;
    u8 x1;
    u8 x2;           // 0x02  (type 2) BGM control id
    u8 pad_3[0x14 - 0x03];
    u8 area[0x30];   // 0x14  (type 1) area passed to AreaHitCheck
    u8 x44;          // 0x44  (type 0) foot SE variation, (type 2) BGM slot bits / 0x10 stream
    u8 x45;          // 0x45  (type 0) foot effect, (type 2) set volume bits / 0x10 stream set
    u8 x46[2];       // 0x46  (type 0, weapon) SE offset, (type 2) BGM volume per slot
    s32 x48[2];      // 0x48  (type 2) BGM fade time per slot
    u16 str_blk;     // 0x50  (type 2) stream block
    u16 str_no;      // 0x52  (type 2) stream number
    s32 str_vol;     // 0x54  (type 2) stream volume
};

// Floor system work (`pFlrSys`).
struct FlrSys {
    FlrAt* pAt;      // 0x00  attribute under the player
    u8 pad_4[4];
    u8 no;           // 0x08  current floor material
    u8 foot_se[0x41];// 0x09  foot SE variation per material
    u8 foot_esp[1];  // 0x4A  foot effect per material
};

extern FlrSys* pFlrSys;

FlrAt* FlrAtCheck(int type, Vec* pos, int flag);

#endif
