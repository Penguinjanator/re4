#ifndef MAIN_H
#define MAIN_H

#include "types.h"
#include "vec.h"

// game/main.cpp globals that are not part of GlobalWork.

// Persistent system settings (pRK), kept across a soft reset: systemResetCommon fills it and main.cpp
// restores pSys and pG from it on the next boot.
struct RESET_KEEP_WORK {
    char head[0x10];          // 0x00
    u8 reset_flag;            // 0x10  1 = filled by systemResetCommon
    u8 tv_mode;               // 0x11  progressive scan on
    u8 tv_mode_select;        // 0x12  TV mode prompt already handled
    u8 base_brightness;       // 0x13  pSys->brightness (0x40 when unset)
    u8 language;              // 0x14  pSys->language
    u8 eff_country;           // 0x15  pSys->region
    u8 game_country;          // 0x16  pG->language
    u8 logo_skip_enable;      // 0x17  1 once the title menu has been shown (title: skips the logos on the next visit)
    u32 Config_flg;           // 0x18  pSys->flags
    u32 System_flg;           // 0x1C  pG->System_flg, of which the two SYS_OMAKE bits are restored
    u32 Extra_flg;            // 0x20  pSys->unlock_flg
    u32 MercSysRoom[4];       // 0x24  pSys->merc_stage[]
    u32 MercSysRank[2];       // 0x34  pSys->merc_rank[]
    u32 MemcardCheckDone;     // 0x3C  pG->CardStatus >> 31 (first card check done; card.cpp)
};

extern RESET_KEEP_WORK* pRK;

// Logical key state (main.cpp `Key`, 0xB8 bytes), built from Joy[0] by pad.cpp PadRead through
// Key_type_tbl. 64 logical keys, one bit each.
struct KeyWork {
    s8 stickX;     // 0x00  copies of Joy[0] (zero while the game is stopped)
    s8 stickY;     // 0x01
    s8 substickX;    // 0x02
    s8 substickY;    // 0x03
    u8 triggerLeft;  // 0x04
    u8 triggerRight;  // 0x05
    u8 analogA;     // 0x06
    u8 analogB;     // 0x07
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
    u32 unlock_flg;  // 0x04  unlock bits (mercenaries characters / stages / extra content; title charBit table); kept in pRK->sys_unlock_flg
    u8 language;   // 0x08  0 JP, 1/2/7 EN, 3 DE, 4 FR, 5 ES, 6 IT (dvd error messages)
    u8 region;     // 0x09  1 US, 2..6 EU, 7 ? (dvd: disc id game name)
    u8 brightness; // 0x0A  background brightness (Render_done -> Bg_brightness_set)
    u8 key_type;   // 0x0B  Key_type_tbl row (controller layout)
    u8 sound_mode; // 0x0C  0 mono, 1 stereo, 2 DPL2 (Snd_get_sound_mode / SndSetOutputMode)
    u8 pad_D[3];
    u32 merc_stage[4];  // 0x10  mercenaries record per stage: score / 10 | mode << 28 | new << 31 (mercenaries save/load)
    u32 merc_rank[2];   // 0x20  mercenaries rank bits: 3 per (stage, character) (flagCk MSB first)
    u8 pad_28[0x38 - 0x28];
};
extern SystemWork* pSys;

extern "C" int GetSystemVcnt();
extern "C" void SetSystemVcnt(int vcnt);

extern const Vec vecZero;  // main.cpp

// game/main.cpp (C linkage)
extern "C" {
void systemVSyncPost();
void postVSyncCallback();
void haltExecCheck();
void systemStartInit();
void systemRestartInit();
void systemWorkInit();
int checkHardReset();
int systemResetCheck();
void systemResetCommon();
void systemHardReset();
void systemSoftReset();
void setLanguage();
}
extern int vsync_cnt;
extern char* pUser_name;
extern void* roomInfoAddr;
extern u32 MainOt[5];

#endif
