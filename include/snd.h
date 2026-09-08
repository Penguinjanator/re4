#ifndef SND_H
#define SND_H

// Game-side sound interface (D:/Bio4/Prog/snd.cpp, game/snd). Wraps the sound driver in
// include/snd_drv.h. Only the entry points whose signatures are known from the disassembly
// are declared; game/snd.cpp is not matched yet (see the notes in src/game/snd.cpp).

#include "types.h"
#include "vec.h"

extern "C" {

// SndCall(blk, no, pos, ?, flag, ?): blk 0 core, 1 player, 2 weapon, 5 foot, 6 room, 7 door,
// 8.. enemies. Returns the sound id (0 = not played).
int SndCall(u16 blk, u16 no, Vec* pos, int a, u32 flag, int b);
int EmSeCall(u16 no, Vec* pos, int a, u32 flag0, u32 flag1, int b);
int RoomSeCall(u16 no, Vec* pos, u32 flag0, u32 flag1, int b);
int PlSeCall(u16 no, Vec* pos, u32 flag0, u32 flag1, int b);
int CoreSeCall(u16 no, Vec* pos, u32 flag0, u32 flag1, int b);
int FootSeCall(u16 no, Vec* pos, u32 flag0, u32 flag1);
int DoorSeCall(u16 no);

void SndSePause(int on, s16 type);
void SndSeAbsPause(void);
void SndSePauseAll(int on);
void SndSeAbsFadeOutAll_sec(int sec);
void SndSeAbsFadeOutAll_5msec(s16 time);
void SndSeqFadeOutAll_sec(u8 type, int sec);
void SndSoftReset(void);
void SndSystemReset(void);
}

#endif
