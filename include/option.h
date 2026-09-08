#ifndef OPTION_H
#define OPTION_H

#include "types.h"

// game/option.cpp: chapter end screen (new'd by sce_com SceChapterEnd, 0xC bytes).
class ChapterEnd {
public:
    u8 pad_0[0xC];

    void init(void* data, u8 chapter);
    void move();
    void quit();
};

extern "C" {
// Replaces the language part of an "SS/___/..." path (name + 3).
void setLangExt3(char* name);
}

#endif
