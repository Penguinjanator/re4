#ifndef SS_MAIN_H
#define SS_MAIN_H

#include "types.h"
#include "sscrn.h"
#include "widget.h"
#include "id_sys.h"
#include "model.h"

// Sscrn module (the sub screen DLL, src/Sscrn/ss_*.cpp): declarations shared by its units.
// ss_main.cpp owns the task, the common id/model/light helpers and the exit/examine widgets.

class cLit;

// game/sscrn.cpp id systems of the sub screen (sub screen ids / number digits).
extern IDSystem IdSub;
extern IDSystem IdNum;

// Every unit's linkonce block has Widget<SUB_SCREEN>::~Widget right after the cManager<cLight>
// copies, before the unit's own (synthesized) widget destructors and before Widget::quit/init/move:
// the destructor is instantiated by this header, before any derived class is declared (the other
// three at their first use in the unit).
static inline void ssWidgetDelete(Widget<SUB_SCREEN>* w)
{
    delete w;
}

// Item examine screen (ss_main.cpp; ss_cap/ss_file/ss_item chain into it).
class SsItemExamine : public Widget<SUB_SCREEN> {
public:
    u8 pad_10[0x70 - 0x10];

    virtual void init(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

// The DLL's own model managers (ss_main.cpp; the DOL's PartsMgr/ModInfoMgr are swapped out while
// the sub screen is open).
extern cPartsMgr ssPartsMgr;
extern cModInfoMgr ssModInfoMgr;

extern "C" {
// ss_main.cpp
void IdSubErase();
void IdNumErase();
void sscrnModelClear(SUB_SCREEN* wk);
void sscrnLightClear(SUB_SCREEN* wk);
void sscrnLightCreate(SUB_SCREEN* wk, cLit* lit);
void sscrnMainMenuInit(SUB_SCREEN* wk, int no);
// ss_debug.cpp
void SscrnDebugMenu(SUB_SCREEN* wk);
// ss_pzzl.cpp
void pieceModelInit(SUB_SCREEN* wk);
}

#endif
