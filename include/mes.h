#ifndef MES_H
#define MES_H

#include "types.h"

struct MesWork {
    u8 pad_0[0xE1];
    s8 result;    // 0xE1  menu selection (0 = none yet)
    s8 cursor;    // 0xE2
    u8 pad_E3[0x11FC - 0xE3];
};

// game/mes.cpp
class MessageControl {
public:
    u8 x0[4];
    MesWork work;   // 0x04

    MesWork* getWork() { return &work; }

    void setLayout(int no, int layout);
    void MesSet(int no, int x, int y, u32 attr, int a, int b, int c);
    void Delete(int no);
};

struct MesDataWork {
    void* pTable;     // 0x00
    u8* ptr[5];       // 0x04
};

extern MessageControl cMes;
extern MesDataWork MesData;

#endif
