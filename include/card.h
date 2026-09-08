#ifndef CARD_H
#define CARD_H

#include "types.h"
#include "db_log.h"

#line 8 "D:/Bio4/Prog/card.h"

// Memory card front end (game/card.cpp). Layout unknown; the header carries an inline range
// check whose file-name string shows up in the .rodata of every unit that includes it.
class cCard {
public:
    u8* pData;
    u32 nData;

    u8* getData(u32 no) {
        if (no >= nData) {
            dbgAssert(__FILE__, __LINE__);
        }
        return pData + no;
    }
};

extern "C" {
void CardFirstCheck();
void CardSave(int a, int b);
int CardLoad();
void CardSysSave();
}

#endif
