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
    void modeSet(int mode);
    void disp();
    void on();
    void add(int a, int b, const char* str);
};

extern cLog* pLog;

// Debug break with source location (used by the header-inline range checks).
extern void dbgAssert(const char* file, int line);

#endif
