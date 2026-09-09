#ifndef DB_TOOLBASE_H
#define DB_TOOLBASE_H

#include "types.h"
#include "db_log.h"

extern "C" unsigned int strlen(const char* s);
extern "C" char* strcpy(char* dst, const char* src);

// Debug tool window/button base (D:/Bio4/Prog/db_toolbase.h; bodies in Tools/db_toolbase.cpp, the same
// object is t_event's db_filelist.cpp and most of Sscrn's ss_term.cpp). The string-returning inlines
// reproduce the header's parse-time strings (none of the four polymorphic classes may own them: every
// in-class inline of a vtable-owning class is emitted out of line).

u32 MakeCol(f32 r, f32 g, f32 b, f32 a);
void DbgDrawBox(f32 x, f32 y, f32 w, f32 h, f32 r, f32 g, f32 b, f32 a);
void DbgDrawBoxFill(f32 x, f32 y, f32 w, f32 h, f32 r, f32 g, f32 b, f32 a);

class cDbgButtonBase {
public:
    u32 x;          // 0x00  column inside the window
    u32 y;          // 0x04  row inside the window
    int cx;         // 0x08  cursor column
    int cy;         // 0x0C  cursor row
    char* pName;    // 0x10  own copy of the label
    u32 nameLen;    // 0x14  strlen + 1
    // 0x18 vptr

    virtual ~cDbgButtonBase() { delete pName; }

    int Init(int x_, int y_, const char* name, int cx_, int cy_) {
        int len;

        x = x_;
        y = y_;
        cx = cx_;
        cy = cy_;
        len = strlen(name) + 1;
        nameLen = len;
        pName = new char[len];
        if (pName == 0) {
            pLog->err(0, 0, "cDbgButtonBase::Init(): new failed.");
            return 0;
        }
        strcpy(pName, name);
        return 1;
    }
};

// Strings of the header's display helpers (parse order = .rodata order).
struct cDbgStr {
    static const char* cursor() { return ">"; }
    static const char* nameHeader() { return "Name:                 "; }
    static const char* nameBlank() { return "               "; }
    static const char* noHeader() { return "No  :"; }
    static const char* noBlank() { return " xx "; }
    static const char* ok() { return "[OK]"; }
    static const char* fmtNo() { return "%s%02d%s"; }
    static const char* fmtName() { return "%s%s%02d%s"; }
    static const char* fmtNum() { return " %02d"; }
    static const char* okButton() { return " [OK] "; }
    static const char* cancelButton() { return "[CANCEL]"; }
};

class cDbgWindowBase {
public:
    int x;              // 0x00  window column (8 px units)
    int y;              // 0x04  window row (14 px units)
    u32 w;              // 0x08  width in columns
    u32 h;              // 0x0C  height in rows
    int cxMax;          // 0x10  largest button cursor column
    int cyMax;          // 0x14  largest button cursor row
    const char* pName;  // 0x18  title
    int x1C;
    int x20;
    // 0x24 vptr

    virtual ~cDbgWindowBase() {}
    virtual int GetCx() { return 0; }
    virtual int GetCy() { return 0; }
    virtual void SetCurrentBottomButton() {}
    virtual void ButtonAllUpdate() = 0;
    virtual int LocalUpdate() = 0;
    virtual void LocalDisp() = 0;
};

class cDbgButton : public cDbgButtonBase {
public:
    void (*pFunc)(cDbgButton*);    // 0x1C  pushed
    void (*pUpdate)(cDbgButton*);  // 0x20  called every frame by ButtonAllUpdate

    // inlined into cDbgWindow::AddButton; the tool modules are compiled with -fno-implement-inlines,
    // so no out-of-line body exists although cDbgButton's vtable is emitted there
    cDbgButton(int x_, int y_, const char* name, int cx_, int cy_) { Init(x_, y_, name, cx_, cy_); }
    virtual ~cDbgButton();
};

class cDbgWindow : public cDbgWindowBase {
public:
    u32 num;                   // 0x28
    cDbgButton* pButton[128];  // 0x2C
    cDbgButton* pCur;          // 0x22C
    cDbgButton* pTop;          // 0x230
    cDbgButton* pBottom;       // 0x234

    virtual ~cDbgWindow();
    virtual int GetCx();
    virtual int GetCy();
    virtual void SetCurrentBottomButton();
    virtual void ButtonAllUpdate();
    virtual int LocalUpdate();
    virtual void LocalDisp();
    virtual void SetCurrentTopButton();

    void Init(int x, int y, const char* name);
    void AddButton(int x, int y, const char* name, int cx, int cy, void (*func)(cDbgButton*),
                   void (*update)(cDbgButton*));
    int FindButton(int cx, int cy, cDbgButton** out);
};

#endif
