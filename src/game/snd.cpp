// game/snd: game-side sound interface (D:/Bio4/Prog/snd.cpp). NOT MATCHED: only the trivial
// wrappers around the sound driver are here so far (they match). The rest of the unit (97
// functions, 0x5AC4 bytes, -O2) still has to be written: SndInit / SndDriverInit (ARAM memory map
// in `SndMem`, 0xA0 bytes of pointers filled from DVD files 0x60/0x68/0x6A/0x59 read to
// 0x80370000), SndCall and the *SeCheck helpers, the room BGM/stream tables (`Snd`, 0xAE8 bytes,
// pSnd), SndWatcher and the debug display (debugDisp, SndStatDisp) that prints through pLog.
// Notes from the disassembly:
//  - Snd.x20 is a flag word (bit 0x01000000: door SEs enabled; bit words at 0x20.. are also the
//    per-block "loaded" bitmask read by sndExistCheck: word[blk >> 5] bit (0x80000000 >> (no&31))).
//  - callErr is a static u32[14][32] bitmap of already reported illegal SE numbers (sndCallErr).
//  - sndVolCalc/sndPitchCalc/sndFilterCalc interpolate tables at pSnd->x9C + ofs[i] (0xC0/0x140/0x1C0).
//  - pSys->sound_mode (u8 at 0x0C) receives Snd_get_sound_mode() in SndInit.
#include "snd.h"
#include "snd_drv.h"

extern "C" void SndDriverInit();

int EmSeCall(u16 no, Vec* pos, int a, u32 flag0, u32 flag1, int b)
{
    return SndCall(8, no, pos, a, flag0 | flag1, b);
}

int RoomSeCall(u16 no, Vec* pos, u32 flag0, u32 flag1, int b)
{
    return SndCall(6, no, pos, 0, flag1 | flag0, b);
}

int PlSeCall(u16 no, Vec* pos, u32 flag0, u32 flag1, int b)
{
    return SndCall(1, no, pos, 0, flag0 | flag1, b);
}

int CoreSeCall(u16 no, Vec* pos, u32 flag0, u32 flag1, int b)
{
    return SndCall(0, no, pos, 0, flag0 | flag1, b);
}

int FootSeCall(u16 no, Vec* pos, u32 flag0, u32 flag1)
{
    return SndCall(5, no, pos, 0, flag0 | flag1, 0);
}

void SndSystemReset()
{
    SEQQuit();
    SYNQuit();
    AXARTQuit();
    MIXQuit();
    AXQuit();
    AIReset();
    SndDriverInit();
}

void SndSePause(int on, s16 type)
{
    if (on == 1) {
        Snd_se_pause_on2(type);
    } else {
        Snd_se_pause_off2(type);
    }
}

void SndSeAbsPause()
{
    Snd_se_pause_on3();
}

void SndSePauseAll(int on)
{
    SndSePause(on, -1);
}

void SndSoftReset()
{
    Snd_soft_reset_req();
    while (Snd_soft_reset_ck() != 0) {
        Snd_iss_control();
    }
    Snd_efx_req(0, 0);
}

void SndSeAbsFadeOutAll_sec(int sec)
{
    Snd_se_fade_out_all2(sec * 200);
}

void SndSeAbsFadeOutAll_5msec(s16 time)
{
    Snd_se_fade_out_all2(time);
}

void SndSeqFadeOutAll_sec(u8 type, int sec)
{
    Snd_seq_fade_out_type(type, sec * 200);
}
