#ifndef MAIN_H
#define MAIN_H

#include "types.h"

// game/main.cpp globals that are not part of GlobalWork.

// Persistent system settings (pRK).
struct RK {
    u8 pad_0[0x11];
    u8 progressive;   // 0x11  progressive scan on
    u8 tv_mode_done;  // 0x12  TV mode prompt already handled
    u8 pad_13[0x40 - 0x13];
};

extern RK* pRK;

// Logical key state (main.cpp `Key`, 0xB8 bytes), built from Joy[0] by pad.cpp PadRead through
// Key_type_tbl. 64 logical keys, one bit each.
struct KeyWork {
    s8 sx;     // 0x00  copies of Joy[0] (zero while the game is stopped)
    s8 sy;     // 0x01
    s8 ssx;    // 0x02
    s8 ssy;    // 0x03
    u8 trigL;  // 0x04
    u8 trigR;  // 0x05
    u8 x6;     // 0x06
    u8 x7;     // 0x07
    u64 old;   // 0x08
    u64 on;    // 0x10
    u64 trg;   // 0x18  (bit 31 = skip TV-mode prompt)
    u64 rel;   // 0x20
    u64 rep;   // 0x28
    u64 rep2;  // 0x30
    s8 rep_timer[64];   // 0x38
    s8 rep2_timer[64];  // 0x78
};

extern KeyWork Key;

// System work (main.cpp `pSys`); only the fields other units read are named.
struct SystemWork {
    u32 flags;     // 0x00  bit 30 = progressive/60Hz screen scaling, 0x08000000 = vibration on
    u8 pad_4[4];
    u8 language;   // 0x08  0 JP, 1/2/7 EN, 3 DE, 4 FR, 5 ES, 6 IT (dvd error messages)
    u8 region;     // 0x09  1 US, 2..6 EU, 7 ? (dvd: disc id game name)
    u8 brightness; // 0x0A  background brightness (Render_done -> Bg_brightness_set)
    u8 key_type;   // 0x0B  Key_type_tbl row (controller layout)
    u8 sound_mode; // 0x0C  0 mono, 1 stereo, 2 DPL2 (Snd_get_sound_mode / SndSetOutputMode)
};
extern SystemWork* pSys;

extern "C" u32 GetSystemVcnt();
extern "C" void SetSystemVcnt(int vcnt);

#endif
