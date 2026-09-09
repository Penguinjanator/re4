#ifndef SS_MAIN_H
#define SS_MAIN_H

#include "types.h"
#include "sscrn.h"
#include "widget.h"
#include "id_sys.h"

// Sscrn module (the sub screen DLL, src/Sscrn/ss_*.cpp): declarations shared by its units.
// ss_main.cpp owns the task, the common id/model/light helpers and the exit/examine widgets.

class cLit;

// game/sscrn.cpp id systems of the sub screen (sub screen ids / number digits).
extern IDSystem IdSub;
extern IDSystem IdNum;

// Item examine screen (ss_main.cpp; ss_cap/ss_file/ss_item chain into it).
class SsItemExamine : public Widget<SUB_SCREEN> {
public:
    u8 pad_10[0x70 - 0x10];

    SsItemExamine(int n) : Widget<SUB_SCREEN>(n) {}
    virtual void init(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

extern "C" {
void IdSubErase();
void IdNumErase();
void sscrnModelClear(SUB_SCREEN* wk);
void sscrnLightClear(SUB_SCREEN* wk);
void sscrnLightCreate(SUB_SCREEN* wk, cLit* lit);
void sscrnMainMenuInit(SUB_SCREEN* wk, int no);
}

#endif
