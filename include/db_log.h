#ifndef DB_LOG_H
#define DB_LOG_H

#include "types.h"

// Debug log (src/game/db_log.cpp).
class cLog {
public:
    void init();
    void mes(int a, int b, const char* fmt, ...);
    void err(int a, int b, const char* fmt, ...);
    void warn(int a, int b, const char* fmt, ...);
    void clear();
    void modeReset();
    int modeSet(int x, int y, int w, int h);   // window position/size (t_log: 0x30, 0x2A, 0xFF, 0x19)
    void disp();
    int on(int flag);
    void add(int a, int b, const char* str);
    int scrSet(s8 lines);                      // scroll by `lines`, clamped to [0, 100 - h]
    int dispLineNum(int x, int y);
};

extern cLog* pLog;

// Debug break with source location (used by the header-inline range checks).
extern void dbgAssert(const char* file, int line);

#endif
