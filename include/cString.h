#ifndef CSTRING_H
#define CSTRING_H

#include "types.h"

// game/cString.cpp: minimal heap string. `str` points at the shared "" literal when empty.
class cString {
public:
    char* str;  // 0x00

    cString();
    cString(const char* s);
    ~cString();
    cString& operator=(const cString& o);
    cString& operator+=(const cString& o);
    cString& operator+=(const char* s);
    char* c_str();
    u32 size();
    void clear();
    void copy(const char* s);
};

#endif
