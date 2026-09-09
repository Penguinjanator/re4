#ifndef SS_MAIN_H
#define SS_MAIN_H

#include "types.h"
#include "sscrn.h"
#include "widget.h"
#include "id_sys.h"
#include "model.h"
#include "examine.h"

// Sscrn module (the sub screen DLL, src/Sscrn/ss_*.cpp): declarations shared by its units.
// ss_main.cpp owns the task, the common id/model/light helpers and the exit/examine widgets.

class cLit;

// Scalar reference setters: a store through a reference keeps the following loads of other globals
// below it (global.h FSet/BitSet).
static inline void U8Set(u8& d, u8 v) { d = v; }
static inline void U16Set(u16& d, u16 v) { d = v; }
static inline void U32Set(u32& d, u32 v) { d = v; }
static inline void S32Set(s32& d, s32 v) { d = v; }
static inline void S16Set(s16& d, s16 v) { d = v; }
static inline void PSet(void*& d, void* v) { d = v; }

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

// The screen widgets. SubScreenTask (ss_main.cpp) creates every screen's Init/Main pair and wires
// their link tables, so all of them are declared here; each unit defines its own virtuals (its key
// function owns the vtable). The link count is the base constructor argument: the module was
// built with -fno-implement-inlines (config/G4BE08/modules.py CFLAGS), so these in-class
// constructors are inlined at the `new` and never emitted out of line. Vtables are emitted in
// reverse declaration order per unit: keep each unit's classes in this order.

// ss_main.cpp
class SsExitInit : public Widget<SUB_SCREEN> {
public:
    int state;  // 0x10

    virtual void init(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

class SsExitMain : public Widget<SUB_SCREEN> {
public:
    SsExitMain() : Widget<SUB_SCREEN>(0) {}
    virtual void move(SUB_SCREEN* wk);
};

// Item examine screen (ss_main.cpp; ss_cap/ss_file/ss_item chain into it).
class SsItemExamine : public Widget<SUB_SCREEN> {
public:
    u8 state;          // 0x10
    u8 pad_11[3];
    ItemExamine exam;  // 0x14

    virtual void init(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

// ss_cap.cpp (attache case selection)
class SsCapInit : public Widget<SUB_SCREEN> {
public:
    int state;  // 0x10

    virtual void init(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

class CapSelect;

class SsCapMain : public Widget<SUB_SCREEN> {
public:
    int state;                 // 0x10
    CapSelect* sel;            // 0x14
    SsItemExamine* exam;       // 0x18
    Widget<SUB_SCREEN>* cur;   // 0x1C
    Widget<SUB_SCREEN>* next;  // 0x20

    SsCapMain() : Widget<SUB_SCREEN>(2) {}
    virtual void init(SUB_SCREEN* wk);
    virtual void quit(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

class CapSelect : public Widget<SUB_SCREEN> {
public:
    int state;  // 0x10  0 none, 1 back, 2 exit

    virtual void init(SUB_SCREEN* wk);
    virtual void quit(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

// ss_file.cpp (files)
class SsFileInit : public Widget<SUB_SCREEN> {
public:
    int state;  // 0x10

    virtual void init(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

class FileSelect;
class MessageDisplay;

class SsFileMain : public Widget<SUB_SCREEN> {
public:
    int state;                 // 0x10
    int sndWait;               // 0x14
    int sndCnt;                // 0x18
    FileSelect* sel;           // 0x1C
    MessageDisplay* disp;      // 0x20
    Widget<SUB_SCREEN>* cur;   // 0x24
    Widget<SUB_SCREEN>* next;  // 0x28

    SsFileMain() : Widget<SUB_SCREEN>(5) {}
    virtual void init(SUB_SCREEN* wk);
    virtual void quit(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

class FileSelect : public Widget<SUB_SCREEN> {
public:
    int state;  // 0x10  0 none, 1 back to the game, 2 main menu

    virtual void init(SUB_SCREEN* wk);
    virtual void quit(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

class MessageDisplay : public Widget<SUB_SCREEN> {
public:
    u8 state;     // 0x10  0 reading, 1 closing, 2 wait for the close animation
    u8 tplState;  // 0x11  picture: 0 shown, 1 request, 2 reading
    u8 tplFirst;  // 0x12  1 until the first picture was read
    u8 pad_13;
    s16 x;        // 0x14
    s16 y;        // 0x16

    virtual void init(SUB_SCREEN* wk);
    virtual void quit(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

// ss_item.cpp (inventory)
class SsItemInit : public Widget<SUB_SCREEN> {
public:
    int state;  // 0x10

    virtual void init(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

class SsItemMain : public Widget<SUB_SCREEN> {
public:
    u8 pad_10[0x2C - 0x10];  // members: ss_item.cpp

    SsItemMain() : Widget<SUB_SCREEN>(6) {}
    virtual void init(SUB_SCREEN* wk);
    virtual void quit(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

// ss_map.cpp (map)
class SsMapInit : public Widget<SUB_SCREEN> {
public:
    int state;  // 0x10

    virtual void init(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

class SsMapMain : public Widget<SUB_SCREEN> {
public:
    u8 pad_10[0x38 - 0x10];  // members: ss_map.cpp

    SsMapMain() : Widget<SUB_SCREEN>(5) {}
    virtual void init(SUB_SCREEN* wk);
    virtual void quit(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

// ss_pzzl.cpp (attache case puzzle)
class SsPzzlInit : public Widget<SUB_SCREEN> {
public:
    int state;  // 0x10

    virtual void init(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

class SsPzzlMain : public Widget<SUB_SCREEN> {
public:
    u8 pad_10[0x40 - 0x10];  // members: ss_pzzl.cpp

    SsPzzlMain() : Widget<SUB_SCREEN>(6) {}
    virtual void init(SUB_SCREEN* wk);
    virtual void quit(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

// ss_shop.cpp (merchant)
class SsShopInit : public Widget<SUB_SCREEN> {
public:
    int state;  // 0x10

    virtual void init(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

class SsShopMain : public Widget<SUB_SCREEN> {
public:
    u8 pad_10[0x58 - 0x10];  // members: ss_shop.cpp

    virtual void init(SUB_SCREEN* wk);
    virtual void quit(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

// ss_term.cpp (typewriter / terminal)
class SsTermInit : public Widget<SUB_SCREEN> {
public:
    u8 pad_10[0x18 - 0x10];  // members: ss_term.cpp

    virtual void init(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

class SsTermMain : public Widget<SUB_SCREEN> {
public:
    u8 pad_10[0x8C - 0x10];  // members: ss_term.cpp

    virtual void init(SUB_SCREEN* wk);
    virtual void quit(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

// The DLL's own model managers (ss_main.cpp; the DOL's PartsMgr/ModInfoMgr are swapped out while
// the sub screen is open). They are constructed by the DOL's cPartsMgr/cModInfoMgr constructors,
// but the module's view of the classes has no virtual destructor: the static destructor inlines
// cManager<T>::~cManager (stores the cManager vtable) instead of calling _._9cPartsMgr.
class cSsPartsMgr : public cManager<cParts> {
public:
    cSsPartsMgr() asm("__9cPartsMgr");
    virtual void* memAlloc(u32 size);
    virtual void memFree(void* p);
    virtual void memClear(cParts* p, u32 size);
    virtual void log(const char* fmt, ...);
    virtual int construct(cParts* p, u32 id);
};
class cSsModInfoMgr : public cManager<cModelInfo> {
public:
    cSsModInfoMgr() asm("__12cModInfoMgr");
    virtual void* memAlloc(u32 size);
    virtual void memFree(void* p);
    virtual void memClear(cModelInfo* p, u32 size);
    virtual void log(const char* fmt, ...);
    virtual int construct(cModelInfo* p, u32 id);
};
extern cSsPartsMgr ssPartsMgr;
extern cSsModInfoMgr ssModInfoMgr;

// ss_map.cpp: the sub screen's character model (MapMgr work 0), its weapon model (work 1), the
// motion-driven flag of the character and the optional second weapon model (both .data, zeroed at
// map init; the weapon change task fades / animates them).
extern cModel* ssPlModel;
extern cModel* ssWepModel;
extern cModel* ssPlMotion;
extern cModel* ssWepModel2;

extern "C" {
// ss_main.cpp
void IdSubErase();
void IdNumErase();
void sscrnModelClear(SUB_SCREEN* wk);
void sscrnLightClear(SUB_SCREEN* wk);
void sscrnLightCreate(SUB_SCREEN* wk, cLit* lit);
void sscrnMainMenuInit(SUB_SCREEN* wk, int no);
void numDisp(int id, int num, Vec* pos, u32 flags);
// ss_debug.cpp
void SscrnDebugMenu(SUB_SCREEN* wk);
// ss_pzzl.cpp
void pieceModelInit(SUB_SCREEN* wk);
// ss_model.cpp
void weaponFilename(char* name, u16 no);
void playerModelInit();
void leonModelInit(u16 no, u16 type);
void ashleyModelInit();
void adaModelInit(u16 no, u16 type);
void klauserModelInit(u16 no, u16 type);
void hunkModelInit(u16 no, u16 type);
void weskerModelInit(u16 no, u16 type);
}

#endif
