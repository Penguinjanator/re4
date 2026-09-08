#ifndef DATACTRL_H
#define DATACTRL_H

#include "types.h"

// One streamed data file (game/datactrl.cpp). Only the members block.cpp reads are named.
class cDataUnit {
public:
    u8 pad_0[0x10];
    void* addr;     // 0x10  MRAM address of the data
    u32 arg;        // 0x14  setCommand argument
    u8 pad_18[4];
    u32 dest;       // 0x1C
    u8 pad_20[4];
    u32 size;       // 0x24

    void setCommand(int cmd, u32 arg, u8 a);
    int getCommand();
    int getCondition();
    void setClear();
    // Inline accessors: as call arguments they make GCC precompute the values before the
    // stack argument stores (block.cpp dispDebugInfo).
    void* getAddr() { return addr; }
    u32 getArg() { return arg; }
    u32 getDest() { return dest; }
    u32 getSize() { return size; }
};

// Room data unit controller (game/datactrl.cpp, `DC`, 0xAA4 bytes). Only the members other units
// use are declared; the layout is still opaque.
class cDataCtrl {
public:
    u8 pad_0[0xAA4];

    u32 getAramFree(u32 size);
    void init();
    void check();
    cDataUnit* setData(char* name);
};
extern cDataCtrl DC;

#endif
