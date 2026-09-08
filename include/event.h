#ifndef EVENT_H
#define EVENT_H

#include "types.h"
#include "db_log.h"

#line 8 "D:/Bio4/Prog/event.h"

// Event system header. Contents unknown; db_work includes it between atari.h and light.h and
// its range check emits the file-name string into that unit's .rodata.
class cEvent {
public:
    u8* pData;
    u32 nData;

    u8* getData(u32 no) {
        if (no >= nData) {
            dbgAssert(__FILE__, __LINE__);
        }
        return pData + no;
    }
    // some inline in the original event.h carries an empty string literal: 4 zero bytes follow
    // the file-name string in every including unit's .rodata
    const char* emptyName() { return ""; }
};

// Event manager (game/event.cpp `EvtMgr`, 0x180 bytes); layout opaque.
class EventMgr {
public:
    u8 pad_0[0x180];

    void init();
    void Run();
    // Looks a file of the running event up by name; 0 when it is not loaded.
    int GetBin(void** out, const char* name, int a);
};

extern EventMgr EvtMgr;

// game/event.cpp (C linkage): streamed sound blocks of the running event
extern "C" {
int SndStrPlayBlock(int a, int no, f32 vol);
void SndStrStopBlock(int blk);
}

#endif
