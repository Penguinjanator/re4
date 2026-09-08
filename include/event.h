#ifndef EVENT_H
#define EVENT_H

#include "types.h"
#include "db_log.h"
#include "cManager.h"

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

// Event work (game/event.cpp): a cUnit managed by EventMgr; layout unknown (event.cpp passes the
// real size to the cManager constructor).
class Event : public cUnit {
public:
};

// Event manager (game/event.cpp `EvtMgr`, 0x180 bytes): a cManager<Event> (game.cpp instantiates
// roomInit / arrayAlloc / arrayFree / dispWorkNum on it); the rest of the layout is opaque.
class EventMgr : public cManager<Event> {
public:
    u32 x34;           // 0x34  running event key (sce_com SceChapterEnd: IsAliveEvt / GetEvt)
    u8 pad_38[0x180 - 0x38];

    EventMgr();
    virtual ~EventMgr();
    virtual void* memAlloc(u32 size);
    virtual void memFree(void* p);
    virtual void memClear(Event* p, u32 size);
    virtual int construct(Event* p, u32 id);

    void init();
    void myRoomInit();          // room start (game gameRoomInit, after arrayAlloc(2))
    void SetEvs(void* evs);     // room "EVS" data (game gameRoomInit)
    void Run();
    // Looks a file of the running event up by name; 0 when it is not loaded.
    int GetBin(void** out, const char* name, int a);
    // Replaces the three window jump motions of the running event (emwindow ExeWindowEvent).
    void GetEmWindowFcv(void** a, void** b, void** c);
    int IsAliveEvt(u32* key, int a, int b);
    void GetEvt(u32* key, void** out);
    void DelEvt(void* evt, int a);
};

extern EventMgr EvtMgr;

// Event debug tool (game/event.cpp `EvtDebug`, 0xE8 bytes); layout opaque.
class EventDebug {
public:
    u8 pad_0[0xE8];

    void myRoomInit();          // room start (game gameRoomInit)
};

extern EventDebug EvtDebug;

// game/event.cpp (C linkage): streamed sound blocks of the running event
extern "C" {
int SndStrPlayBlock(int a, int no, f32 vol);
void SndStrStopBlock(int blk);
}

#endif
