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

    void stageInit();
    void setLayout(int no, int layout);
    void MesSet(int no, int x, int y, u32 attr, int a, int b, int c);
    void Delete(int no);
    void Move();
    void Trans();
};

// ROM font glyph renderer (game/mes.cpp), used by the dvd error screen before the message
// system is up.
class RomFont {
public:
    void* pFont;  // 0x00  OSFontHeader

    RomFont(void* font);
    void setup(void* image);
    void draw(int x, int y, int cx, int cy);
};

struct MesDataWork {
    void* pTable;     // 0x00
    u8* ptr[5];       // 0x04
};

extern MessageControl cMes;
extern MesDataWork MesData;

#endif
