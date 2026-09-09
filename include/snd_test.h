#ifndef SND_TEST_H
#define SND_TEST_H

#include "types.h"

// Sound test of the t_movie REL (t_movie/snd_test.cpp: Snd_test_*/test_*/disp_*/aram_*; the entry
// SoundTest and the file-list helper live in t_movie.cpp). Only the members other objects touch are
// laid out so far.
struct SndTestWork {
    s8 mode;      // 0x00  Snd_test_mode state
    u8 pad_1[0xB];
    u16 xC;       // 0x0C
    u8 pad_E[0x38 - 0xE];
    u32 on;       // 0x38  Joy[0] copy taken by SoundTest every frame
    u32 old;      // 0x3C
    u32 trg;      // 0x40
    u32 rep;      // 0x44
};

extern SndTestWork Snd_test_work;

int Snd_test_mode();
const char* Snd_test_get_str_name(int type);

#endif
