#ifndef SOFDEC_H
#define SOFDEC_H

#include "types.h"
#include "db_log.h"

#line 8 "D:/Bio4/Prog/sofdec.h"

// Sofdec movie player front end (game/sofdec.cpp, `Sofdec`, 0x240 bytes). Only the flag word
// and the members dvd.cpp uses are known; the inline range check emits the file-name string
// into the .rodata of every unit that includes it.
class cSofdec {
public:
    u32 flag;   // 0x00  bit0: a movie is playing
    u8 pad_4[0x240 - 0x4];

    // playing check: `if (Sofdec.flag & 1) return 1; return 0;` form (li 0 / li 1)
    int isPlay() {
        if (flag & 1) {
            return 1;
        }
        return 0;
    }
    u8* getData(u32 no) {
        if (no >= flag) {
            dbgAssert(__FILE__, __LINE__);
        }
        return pad_4 + no;
    }

    void PlayPause(int pause);
};

extern cSofdec Sofdec;

extern "C" void ADXM_ExecMain();

#endif
