// Sscrn/ss_pzzl: the attache case (puzzle) screen of the sub screen DLL (D:/Bio4/Prog/ss_pzzl.cpp).
// Pieces are the pzlPlayer / pzlBoard / pzlPiece of game/puzzle.cpp; this unit draws them
// (piece models, cursor, grid) and runs the select / command / combine / case change widgets.
#include "types.h"
#include "global.h"
#include "map_obj.h"
#include "light.h"
#include "atari.h"
#include "widget.h"
#include "item.h"
#include "cockpit.h"
#include "mes.h"
#include "id_sys.h"
#include "fade.h"
#include "dvd.h"
#include "main_mem.h"
#include "main.h"
#include "joy.h"
#include "pad.h"
#include "snd.h"
#include "db_log.h"
#include "camera.h"
#include "view.h"
#include "trans_ot.h"
#include "math_sub.h"
#include "puzzle.h"
#include "pl_sub.h"
#include "sscrn.h"
#include "ss_main.h"

class cSubChar;
extern cSubChar* pSUB;
extern "C" f32 tanf(f32 x);
extern "C" f64 tan(f64 x);

#define DVD_READ_N(name, dst, a, b, c, mode) DvdReadN(name, dst, a, b, c, mode, __FILE__, __LINE__)

// ss_main.cpp
extern "C" {
void clearZbuffer();
void idMainMenuFade(SUB_SCREEN* wk, int sw);
void weaponChangeRequest(u16 no, u16 type);
}

// ss_debug.cpp: the attache case editor
class ssDbgPzzl {
public:
    void* pSave;
    u8 caseSize;
    s8 cursor;
    s8 itemSet;
    s8 bullet;

    void init(SUB_SCREEN* wk);
    void quit();
    void move(SUB_SCREEN* wk);
};

// Board cell size in world units (a template-static-like COMMON word: set by caseModelMove).
struct pzlGrid {
    static f32 size;
};
f32 pzlGrid::size;

// Cursor frame: the four corner vertices of the cell run under the cursor (drawCursor).
struct PzzlCursor {
    Vec v[4];
};

// The puzzle screen widgets (SsPzzlMain::init creates them).
class PzzlThinking : public Widget<SUB_SCREEN> {
public:
    virtual void init(SUB_SCREEN* wk);
    virtual void quit(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

class PiecePopUp : public Widget<SUB_SCREEN> {
public:
    int count;  // 0x10

    virtual void init(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

class PiecePopDown : public Widget<SUB_SCREEN> {
public:
    int count;  // 0x10

    virtual void init(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

class PieceSelect : public Widget<SUB_SCREEN> {
public:
    int state;  // 0x10  0 select, 1 message, 2 message closed
    int mode;   // 0x14  bit3: message open; 1 exit, 2 main menu, 4 case change

    PieceSelect() : Widget<SUB_SCREEN>(4) {}
    virtual void init(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

class PieceCombine : public Widget<SUB_SCREEN> {
public:
    int state;  // 0x10

    PieceCombine() : Widget<SUB_SCREEN>(2) {}
    virtual void init(SUB_SCREEN* wk);
    virtual void quit(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

class PieceCommand : public Widget<SUB_SCREEN> {
public:
    IdUnit* id[16];   // 0x10
    IdUnit* sub[11];  // 0x50
    u8 pad_7C[0x90 - 0x7C];
    int mode;         // 0x90  0 command, 1 open sub menu, 2 sub menu, 3 message
    s8 num;           // 0x94
    u8 cursorOld;     // 0x95
    s8 subSel;        // 0x96
    s8 onCase;        // 0x97  the piece is on the case board
    u8 lower;         // 0x98  the piece is in the lower half (menu above it)

    PieceCommand() : Widget<SUB_SCREEN>(6) {}
    virtual void init(SUB_SCREEN* wk);
    virtual void quit(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

class CaseChange : public Widget<SUB_SCREEN> {
public:
    IdUnit* a;  // 0x10
    IdUnit* b;  // 0x14

    virtual void init(SUB_SCREEN* wk);
    virtual void quit(SUB_SCREEN* wk);
    virtual void move(SUB_SCREEN* wk);
};

extern "C" {
void pzzlClearZ(SUB_SCREEN* wk);
u32 colorRRGGBBAA(u32 r, u32 g, u32 b, u32 a);
int back2PieceSelect(SUB_SCREEN* wk);
void pzzlEquipDisp(SUB_SCREEN* wk, int sw);
void pzzlCursorDisp(SUB_SCREEN* wk, int sw);
void drawCursorInit(SUB_SCREEN* wk, PzzlCursor* c);
void cmpVer(PzzlCursor* c, Vec* v);
void drawCursor(SUB_SCREEN* wk, pzlBoard* b, int x, int y, PzzlCursor* c, int line);
void drawGridLine(SUB_SCREEN* wk);
void puzzleCameraInit(SUB_SCREEN* wk, Camera* cam);
int puzzlePos2screenPos(Vec* pos, Vec* out);
void screenPos2puzzlePos(Vec* pos, Vec* out);
void pieceTblInit(SUB_SCREEN* wk);
void pieceModelOrientation(SUB_SCREEN* wk, pzlPiece* p);
void pieceFrameDisp(cModel* m, u32 color, int type);
void getPieceVertex(pzlPiece* p, Vec* out, int corner);
void pieceModelDisp(SUB_SCREEN* wk);
void pieceModelSet(pzlPiece* p);
void caseModelMove(int sw);
void tempSpaceDisp(int sw);
int checkWeaponChange(int id, int bullets);
void sscrn_pzzl_out_init(SUB_SCREEN* wk);
void sscrn_pzzl_in_init(SUB_SCREEN* wk);
int isTerminable(SUB_SCREEN* wk);
int checkMsgWindow(SUB_SCREEN* wk);
void closeMsgWindow(SUB_SCREEN* wk);
void openMsgWindow(SUB_SCREEN* wk, int no);
int remarkMsgCombine(int a, int b, int* no);
int itemCommandType(ItemWork* item);
}
static void setCommandId(u8 type, IdUnit** tbl, s8* num, int lang);

static int sscrn_pzzl_out(SUB_SCREEN* wk);

static int msg_open = 0;
int pzzlDbgNo = -12;
f32 pzzlDbgPos = -300.0f;
static u16 cursor_line_w[2] = {0xF, 0};
static u32 cursor_line_col = 0x707040FF;
static int cursor_line_blend = 10;
static u16 grid_line_w0[2] = {0xD, 0};
static u16 grid_line_w1[2] = {0xF, 0};
static u32 grid_line_col0 = 0x404040FF;
static u32 grid_line_col1 = 0x505050FF;
static u16 frame_line_w[2] = {0xD, 0};
static u32 frame_line_col = 0x5F4B37FF;
static int frame_line_blend = 12;
static f32 frame_line_len = 40.0f;
static u16 frame_tile_w1[2] = {0xD, 0};
static u32 frame_tile_col1 = 0;
static int frame_tile_blend1 = 2;
static u16 frame_tile_w2[2] = {0xF, 0};
static u32 frame_tile_col2 = 0;
static u16 frame_tile_w3[2] = {0xF, 0};
static u32 frame_tile_col3 = 0;
static u16 frame_tile_w4[2] = {0xF, 0};
static u32 frame_tile_col4 = 0x30503080;
static int frame_tile_blend4 = 0;
static f32 piece_hand_scale = 30.0f;
static Vec case_rot = {-0.57f, 0.0f, 0.0f};
static int pzzl_wait = 0;
static s16 pzzl_font_w[2] = {0, 0x12};
static s16 pzzl_font_h[2] = {0, 0x18};
static s8 pzzl_font_space[4] = {-1, -1, -1, -1};
static int msg_x = 100;
static int msg_y = 160;
static int command_id = 4;
static int pzzl_dbg_step = 0;
u8 pzzl_tbl_AD0[20] = {0xF6, 0xF6, 0xF6, 0xF6, 0x00, 0x00, 0x27, 0x10, 0x05, 0x0A, 0x0A, 0x0A,
                       0x14, 0x1E, 0x46, 0x1E, 0x0A, 0x00, 0x00, 0x00};

static void* pzzl_clear_z;
static int pzzl_read_req;
static PzzlCursor pzzl_cursor;
static pzlPiece* pzzl_sel;
static ssDbgPzzl pzzl_dbg;

// COMPILER-DIFF: item 4 (narrow-argument truncation): s16 view of MessageControl::setFontSize.
class MessageControlS : public MessageControl {
public:
    void setFontSizeS(int no, s16 w, s16 h) asm("setFontSize__14MessageControliScSc");
};
#define cMesS (*(MessageControlS*) &cMes)

#define CMES_FLAGS (*(u32*) ((u8*) &cMes + 0xFC))
#define CMES_RESULT (*(s8*) ((u8*) &cMes + 0x1D1))

void pzzlClearZ(SUB_SCREEN* wk)
{
    AddOtDirect(0xF, &pzzl_clear_z, (void (*)()) clearZbuffer, 2, 0x1000, 0, 0.0f);
}

u32 colorRRGGBBAA(u32 r, u32 g, u32 b, u32 a)
{
    return (r << 24) | (g << 16) | (b << 8) | a;
}

int back2PieceSelect(SUB_SCREEN* wk)
{
    if (wk->x2B0->spaceBoard->getPieceNum() != 0) {
        return 0;
    }
    Cckpt.life.frameIn();
    IdSub.unitPtr(0, 2)->dir &= 0xF0;
    tempSpaceDisp(0);
    idMainMenuFade(wk, 1);
    return 1;
}

void pzzlEquipDisp(SUB_SCREEN* wk, int sw)
{
    pzlPlayer* pl = wk->x2B0;
    IdUnit* id[3];
    IdUnit* id2[3];
    Vec pos;
    Vec scr;
    ItemInfo info;
    pzlPiece* arm;
    int i;

    id[0] = IdSub.unitPtr(0x30, 4);
    id2[0] = IdSub.unitPtr(0x31, 4);
    id2[0]->flags &= ~8;
    id[1] = IdSub.unitPtr(0x40, 4);
    id2[1] = IdSub.unitPtr(0x41, 4);
    id2[1]->flags &= ~8;
    id[2] = IdSub.unitPtr(0x50, 4);
    id2[2] = IdSub.unitPtr(0x51, 4);
    id2[2]->flags &= ~8;
    itemInfo(ItemMgr.armId, &info);
    switch (info.type) {
    case 1:
        arm = pl->piecePtr(ItemMgr.pArm);
        break;
    case 3:
    case 6: {
        ItemWork* w;

        if (ItemMgr.num(ItemMgr.pArm) != 0 && ItemMgr.armId == ItemMgr.pArm->id) {
            w = ItemMgr.pArm;
        } else {
            w = ItemMgr.minimumSearch(ItemMgr.armId);
        }
        arm = pl->piecePtr(w);
        break;
    }
    default:
        arm = 0;
        break;
    }
    if (sw && arm) {
        if (arm == pl->hand) {
            id[0]->flags &= ~8;
        } else {
            getPieceVertex(arm, &pos, 1);
            puzzlePos2screenPos(&pos, &scr);
            id[0]->flags |= 8;
            id[0]->scr = scr;
            pieceFrameDisp(arm->model, colorRRGGBBAA(id[0]->col0[0], id[0]->col0[1], id[0]->col0[2], 0x20), 3);
        }
        id[1]->flags &= ~8;
        id[2]->flags &= ~8;
        for (i = 0; i < 2; i++) {
            ItemWork* w = ItemMgr.weaponParts(ItemMgr.pArm, i);
            pzlPiece* p;

            if (w == 0) {
                break;
            }
            p = pl->piecePtr(w);
            if (p != pl->hand) {
                getPieceVertex(p, &pos, 1);
                puzzlePos2screenPos(&pos, &scr);
                id[i + 1]->flags |= 8;
                id[i + 1]->scr = scr;
                pieceFrameDisp(p->model, colorRRGGBBAA(id[i + 1]->col0[0], id[i + 1]->col0[1], id[i + 1]->col0[2], 0x20), 3);
            }
        }
    } else {
        id[0]->flags &= ~8;
        id2[0]->flags &= ~8;
        id[1]->flags &= ~8;
        id2[1]->flags &= ~8;
        id[2]->flags &= ~8;
        id2[2]->flags &= ~8;
    }
}

void pzzlCursorDisp(SUB_SCREEN* wk, int sw)
{
    IdUnit* u0 = IdSub.unitPtr(0x20, 4);
    IdUnit* u1 = IdSub.unitPtr(0x21, 4);
    IdUnit* u2 = IdSub.unitPtr(0x22, 4);
    pzlBoard* b = wk->x2B0->cur;
    int x = b->curX;
    int y = b->curY;
    pzlPiece* p;
    u32 col;

    u0->flags &= ~8;
    u1->flags &= ~8;
    u2->flags &= ~8;
    if (!sw) {
        wk->x268 = sw;
        return;
    }
    drawCursorInit(wk, &pzzl_cursor);
    drawCursor(wk, wk->x2B0->cur, x, y, &pzzl_cursor, 0);
    switch (wk->x267) {
    case 1:
        if (wk->x268 & 3) {
            IdSub.setTime(u0, 0);
        }
        pzzlEquipDisp(wk, 1);
        drawCursorInit(wk, &pzzl_cursor);
        drawCursor(wk, wk->x2B0->cur, x, y, &pzzl_cursor, 1);
        col = colorRRGGBBAA((u8) u0->col[0], (u8) u0->col[1], (u8) u0->col[2], (u8) u0->col[3]);
        p = wk->x2B0->ptrPiece(wk->x2B0->cur);
        if (p) {
            pieceFrameDisp(p->model, col, 1);
        }
        break;
    case 2:
        if (wk->x268 & 3) {
            IdSub.setTime(u1, 0);
        }
        if (wk->x268 & 2) {
            IdSub.setTime(u2, 0);
        }
        pzzlEquipDisp(wk, 1);
        drawCursorInit(wk, &pzzl_cursor);
        drawCursor(wk, wk->x2B0->cur, x, y, &pzzl_cursor, 1);
        col = colorRRGGBBAA((u8) u1->col[0], (u8) u1->col[1], (u8) u1->col[2], (u8) u1->col[3]);
        p = wk->x2B0->ptrPiece(wk->x2B0->cur);
        if (p) {
            pieceFrameDisp(p->model, col, 1);
        }
        col = colorRRGGBBAA((u8) u2->col[0], (u8) u2->col[1], (u8) u2->col[2], (u8) u2->col[3]);
        if (pzzl_sel) {
            pieceFrameDisp(pzzl_sel->model, col, 2);
        }
        break;
    default:
        u0->flags &= ~8;
        u1->flags &= ~8;
        u2->flags &= ~8;
        pzzlEquipDisp(wk, 1);
        drawCursorInit(wk, &pzzl_cursor);
        drawCursor(wk, wk->x2B0->cur, x, y, &pzzl_cursor, 0);
        break;
    }
    wk->x268 = 0;
    drawGridLine(wk);
}

void drawCursorInit(SUB_SCREEN* wk, PzzlCursor* c)
{
    wk->x2B0->caseBoard->clearState(0x80);
    wk->x2B0->spaceBoard->clearState(0x80);
    memclr_asm(c, sizeof(PzzlCursor));
    c->v[3].y = 1000.0f;
    c->v[2].x = -1000.0f;
    c->v[0].x = 1000.0f;
    c->v[0].y = -1000.0f;
    c->v[1].x = -1000.0f;
    c->v[1].y = -1000.0f;
    c->v[2].y = 1000.0f;
    c->v[3].x = 1000.0f;
}

// Extends the cursor frame corners by a vertex.
void cmpVer(PzzlCursor* c, Vec* v)
{
    if (v->x < c->v[0].x) {
        c->v[0].x = v->x;
    }
    if (v->y > c->v[0].y) {
        c->v[0].y = v->y;
    }
    if (v->x > c->v[1].x) {
        c->v[1].x = v->x;
    }
    if (v->y > c->v[1].y) {
        c->v[1].y = v->y;
    }
    if (v->x > c->v[2].x) {
        c->v[2].x = v->x;
    }
    if (v->y < c->v[2].y) {
        c->v[2].y = v->y;
    }
    if (v->x < c->v[3].x) {
        c->v[3].x = v->x;
    }
    if (v->y < c->v[3].y) {
        c->v[3].y = v->y;
    }
}

// Draws the cursor cell run (the cells of the piece under (x, y), or the cell itself): recursive
// flood over the neighbouring cells of the same piece, one edge line per outer side.
void drawCursor(SUB_SCREEN* wk, pzlBoard* b, int x, int y, PzzlCursor* c, int line)
{
    Vec p;
    Vec a;
    Vec d;
    Vec wa;
    Vec wd;
    Mtx mat;
    ItemWork* item = 0;
    f32 g = pzlGrid::size;
    pzlPiece* piece;
    Vec e;

    PSMTXCopy(b->mat, mat);
    if (b->cellState(x, y) & 0x82) {
        return;
    }
    *b->cell(x, y) |= 0x80;
    piece = b->getPiece(x, y);
    if (piece) {
        item = piece->item;
    }
    p.x = g * (f32) x;
    p.y = -g * (f32) y;
    p.z = 0.0f;
    piece = b->getPiece(x, y + 1);
    if (piece && item == piece->item) {
        drawCursor(wk, b, x, y + 1, c, line);
    } else {
        a = p;
        a.y -= g;
        d = p;
        d.x += g;
        d.y -= g;
        PSMTXMultVec(mat, &a, &wa);
        PSMTXMultVec(mat, &d, &wd);
        if (line) {
            ss_Draw_line3d(&wa, &wd, cursor_line_col, cursor_line_blend, 0, 0, cursor_line_w[0], cursor_line_w[1]);
        }
        e = wa;
        cmpVer(c, &e);
        e = wd;
        cmpVer(c, &e);
    }
    piece = b->getPiece(x, y - 1);
    if (piece && item == piece->item) {
        drawCursor(wk, b, x, y - 1, c, line);
    } else {
        a = p;
        d = p;
        d.x += g;
        PSMTXMultVec(mat, &a, &wa);
        PSMTXMultVec(mat, &d, &wd);
        if (line) {
            ss_Draw_line3d(&wa, &wd, cursor_line_col, cursor_line_blend, 0, 0, cursor_line_w[0], cursor_line_w[1]);
        }
        e = wa;
        cmpVer(c, &e);
        e = wd;
        cmpVer(c, &e);
    }
    piece = b->getPiece(x - 1, y);
    if (piece && item == piece->item) {
        drawCursor(wk, b, x - 1, y, c, line);
    } else {
        a = p;
        d = p;
        d.y -= g;
        PSMTXMultVec(mat, &a, &wa);
        PSMTXMultVec(mat, &d, &wd);
        if (line) {
            ss_Draw_line3d(&wa, &wd, cursor_line_col, cursor_line_blend, 0, 0, cursor_line_w[0], cursor_line_w[1]);
        }
        e = wa;
        cmpVer(c, &e);
        e = wd;
        cmpVer(c, &e);
    }
    piece = b->getPiece(x + 1, y);
    if (piece && item == piece->item) {
        drawCursor(wk, b, x + 1, y, c, line);
    } else {
        a = p;
        a.x += g;
        d = p;
        d.x += g;
        d.y -= g;
        PSMTXMultVec(mat, &a, &wa);
        PSMTXMultVec(mat, &d, &wd);
        if (line) {
            ss_Draw_line3d(&wa, &wd, cursor_line_col, cursor_line_blend, 0, 0, cursor_line_w[0], cursor_line_w[1]);
        }
        e = wa;
        cmpVer(c, &e);
        e = wd;
        cmpVer(c, &e);
    }
}

void drawGridLine(SUB_SCREEN* wk)
{
    Mtx mat;
    Vec a;
    Vec b;
    Vec wa;
    Vec wb;
    Vec wc;
    f32 g = pzlGrid::size;
    pzlBoard* bd;
    int w;
    int h;
    int i;

    PSMTXCopy(wk->x2B0->caseBoard->mat, mat);
    bd = wk->x2B0->caseBoard;
    h = bd->h;
    w = bd->w;
    a.x = 0.0f;
    a.z = 0.0f;
    a.y = 0.0f;
    b = a;
    b.y -= (f32) h * g;
    for (i = 0; i <= w; i++) {
        PSMTXMultVec(mat, &a, &wa);
        PSMTXMultVec(mat, &b, &wb);
        ss_Draw_line3d(&wa, &wb, grid_line_col0, 6, 0, 1, grid_line_w0[0], grid_line_w0[1]);
        a.x += g;
        b.x += g;
    }
    a.x = 0.0f;
    a.z = 0.0f;
    a.y = 0.0f;
    b = a;
    b.x += (f32) w * g;
    for (i = 0; i <= h; i++) {
        PSMTXMultVec(mat, &a, &wa);
        PSMTXMultVec(mat, &b, &wc);
        ss_Draw_line3d(&wa, &wc, grid_line_col0, 6, 0, 1, grid_line_w0[0], grid_line_w0[1]);
        a.y -= g;
        b.y -= g;
    }
    PSMTXCopy(wk->x2B0->spaceBoard->mat, mat);
    bd = wk->x2B0->spaceBoard;
    h = bd->h;
    w = bd->w;
    a.x = 0.0f;
    a.z = 0.0f;
    a.y = 0.0f;
    b = a;
    b.y -= (f32) h * g;
    for (i = 0; i <= w; i++) {
        PSMTXMultVec(mat, &a, &wa);
        PSMTXMultVec(mat, &b, &wb);
        ss_Draw_line3d(&wa, &wb, grid_line_col1, 6, 0, 1, grid_line_w1[0], grid_line_w1[1]);
        a.x += g;
        b.x += g;
    }
    a.x = 0.0f;
    a.z = 0.0f;
    a.y = 0.0f;
    b = a;
    b.x += (f32) w * g;
    for (i = 0; i <= h; i++) {
        PSMTXMultVec(mat, &a, &wa);
        PSMTXMultVec(mat, &b, &wb);
        ss_Draw_line3d(&wa, &wb, grid_line_col1, 6, 0, 1, grid_line_w1[0], grid_line_w1[1]);
        a.y -= g;
        b.y -= g;
    }
}

void puzzleCameraInit(SUB_SCREEN* wk, Camera* cam)
{
    sscrnCameraInit(wk, cam);
}

int puzzlePos2screenPos(Vec* pos, Vec* out)
{
    Mtx inv;
    f32 az;
    f32 h;
    f32 w;

    PSMTXInverse(pG->Cam.mat, inv);
    PSMTXMultVec(inv, pos, out);
    if (out->z > -fabsf(ZNEAR)) {
        return 0;
    }
    az = fabsf(out->z);
    h = az * tanf(pG->Cam.param.fovy * 0.5f * 0.017453292f);
    w = h * 1.3333334f;
    out->x = out->x * (320.0f / w);
    out->y = out->y * (240.0f / h);
    out->z = out->z * 0.0f;
    return 1;
}

void screenPos2puzzlePos(Vec* pos, Vec* out)
{
    Camera* cam = &pG->Cam;
    f32 h = fabsf((f32) (cam->param.pos.y * tan(cam->param.fovy * 0.5f * 3.1415927f / 180.0f)));

    out->x = pos->x * h / 240.0f;
    out->y = pos->y * h / 240.0f;
}

// Relocates the piece_info model / texture offsets of ss_pzzl.dat into pointers.
void pieceTblInit(SUB_SCREEN* wk)
{
    PieceInfo* p;

    for (p = piece_info; p->id != 0xFFFF; p++) {
        int mdl;
        int tex;

        switch (p->id) {
        case 0x40:
            mdl = 0x21;
            break;
        case 0x17:
        case 0x35:
        case 0x6D:
            mdl = 0x35;
            break;
        case 0x12 ... 0x16:
        case 0xA8:
            mdl = 0x12;
            break;
        case 1 ... 2:
        case 0xB ... 0xE:
            mdl = 1;
            break;
        case 8 ... 0xA:
            mdl = 8;
            break;
        case 6:
        case 0x19:
        case 0x1C:
            mdl = 6;
            break;
        case 0:
        case 4:
        case 7:
        case 0x18:
        case 0x1A:
        case 0x20:
        case 0x6A:
        case 0xA0:
            mdl = 0;
            break;
        default:
            mdl = p->id;
            break;
        }
        switch (p->id) {
        case 0x40:
            tex = 0x21;
            break;
        case 0x95:
        case 0x97:
            tex = 0x95;
            break;
        case 0x6D:
            tex = 0x35;
            break;
        default:
            tex = p->id;
            break;
        }
        *(void**) &p->model[0] = SS_ARC_PTR(wk->x1E4, mdl * 2 + 4);
        *(void**) &p->model[4] = SS_ARC_PTR(wk->x1E4, tex * 2 + 5);
    }
}

void pieceModelOrientation(SUB_SCREEN* wk, pzlPiece* p)
{
    cModel* m;
    pzlBoard* b;

    if (p == 0) {
        return;
    }
    m = p->model;
    switch (p->orient) {
    case 0:
        m->rot.x = 0.0f;
        m->rot.y = 0.0f;
        m->rot.z = 0.0f;
        break;
    case 1:
        m->rot.y = 0.0f;
        m->rot.z = -1.5707964f;
        m->rot.x = 0.0f;
        break;
    case 2:
        m->rot.y = 0.0f;
        m->rot.z = -3.1415927f;
        m->rot.x = 0.0f;
        break;
    case 3:
        m->rot.y = 0.0f;
        m->rot.z = -4.712389f;
        m->rot.x = 0.0f;
        break;
    case 4:
        m->rot.z = 3.1415927f;
        m->rot.y = 0.0f;
        m->rot.x = 3.1415927f;
        break;
    case 5:
        m->rot.x = 3.1415927f;
        m->rot.y = 0.0f;
        m->rot.z = 1.5707964f;
        break;
    case 6:
        m->rot.z = 0.0f;
        m->rot.x = 3.1415927f;
        m->rot.y = 0.0f;
        break;
    case 7:
        m->rot.x = 3.1415927f;
        m->rot.y = 0.0f;
        m->rot.z = -1.5707964f;
        break;
    }
    if (!(p->state & 2)) {
        if (wk->x2B0->caseBoard->search(p)) {
            b = wk->x2B0->caseBoard;
        } else {
            if (!wk->x2B0->spaceBoard->search(p)) {
                pLog->err(0, 0, "pieceModelOrientation(): lost piece");
            }
            b = wk->x2B0->spaceBoard;
        }
    } else {
        b = wk->x2B0->cur;
    }
    m->pos.x = pzlGrid::size * (p->x + 0.5f);
    m->pos.y = -pzlGrid::size * (p->y + 0.5f);
    m->matUpdate();
    PSMTXConcat(b->mat, m->mat, m->mat);
    m->partsWorldCalc();
}

// Frame around a piece model: type 0 corner lines, 1..3 tiles, 4 the whole piece (scaled).
void pieceFrameDisp(cModel* m, u32 color, int type)
{
    Vec size;
    Vec ofs;
    Vec v[4];
    Vec c;
    Vec d;
    Vec* p;
    int i;

    if (m == 0 || m->pInfo == 0) {
        return;
    }
    size.x = m->pInfo->bound.size.x;
    size.y = m->pInfo->bound.size.y;
    size.z = 0.0f;
    for (p = v; p <= &v[3]; p++) {
        if (p == &v[0]) {
            size.x = fabsf(size.x);
            size.y = fabsf(size.y);
        } else if (p == &v[1]) {
            size.x = -fabsf(size.x);
            size.y = fabsf(size.y);
        } else if (p == &v[2]) {
            size.x = -fabsf(size.x);
            size.y = -fabsf(size.y);
        } else if (p == &v[3]) {
            size.x = fabsf(size.x);
            size.y = -fabsf(size.y);
        }
        PSMTXMultVecSR(m->mat, &size, &ofs);
        c.x = m->mat[0][3];
        c.y = m->mat[1][3];
        c.z = m->mat[2][3];
        PSVECAdd(&c, &ofs, p);
    }
    switch (type) {
    case 0:
        for (i = 0; i < 4; i++) {
            int prev = i - 1;
            int next = i + 1;
            int j;

            if (prev < 0) {
                prev = i + 3;
            }
            if (next > 3) {
                next = i - 3;
            }
            for (j = 0; j < 2; j++) {
                int k = j == 0 ? next : prev;

                PSVECSubtract(&v[k], &v[i], &c);
                if (c.x == 0.0f && c.y == 0.0f && c.z == 0.0f) {
                    pLog->err(0, 0, "VECNormalize:[%s/%d]", __FILE__, 0x486);
                    c.x = 0.0f;
                    c.z = 0.0f;
                    c.y = 0.0f;
                } else {
                    PSVECNormalize(&c, &c);
                }
                PSVECScale(&c, &c, frame_line_len);
                PSVECAdd(&v[i], &c, &c);
                ss_Draw_line3d(&v[i], &c, frame_line_col, frame_line_blend, 0, 1, frame_line_w[0], frame_line_w[1]);
            }
        }
        break;
    case 1:
        ss_Draw_tile3d(&v[0], &v[1], &v[3], &v[2], color, frame_tile_col1, frame_tile_blend1, frame_tile_w1[0], frame_tile_w1[1]);
        break;
    case 2:
        ss_Draw_tile3d(&v[0], &v[1], &v[3], &v[2], color, frame_tile_col2, 1, frame_tile_w2[0], frame_tile_w2[1]);
        break;
    case 3:
        ss_Draw_tile3d(&v[0], &v[1], &v[3], &v[2], color, frame_tile_col3, 1, frame_tile_w3[0], frame_tile_w3[1]);
        break;
    case 4:
        c.x = m->mat[0][2];
        c.y = m->mat[1][2];
        c.z = m->mat[2][2];
        PSVECScale(&c, &c, m->scale.z);
        for (p = v; p <= &v[3]; p++) {
            PSVECSubtract(p, &c, p);
        }
        ss_Draw_tile3d(&v[0], &v[1], &v[3], &v[2], frame_tile_col4, frame_tile_blend4, 1, frame_tile_w4[0], frame_tile_w4[1]);
        break;
    }
}

// World position of a piece corner (0 upper right, 1 upper left).
void getPieceVertex(pzlPiece* p, Vec* out, int corner)
{
    cModel* m = p->model;
    Vec c;

    out->x = m->pInfo->bound.size.x;
    out->y = m->pInfo->bound.size.y;
    out->z = 0.0f;
    PSMTXMultVecSR(m->mat, out, out);
    switch (corner) {
    case 0:
        out->x = fabsf(out->x);
        out->y = -fabsf(out->y);
        out->z = fabsf(out->z);
        break;
    case 1:
        out->x = -fabsf(out->x);
        out->y = fabsf(out->y);
        out->z = fabsf(out->z);
        break;
    }
    c.x = m->mat[0][3];
    c.y = m->mat[1][3];
    c.z = m->mat[2][3];
    PSVECAdd(&c, out, out);
}

void pieceModelDisp(SUB_SCREEN* wk)
{
    pzlPlayer* pl;
    pzlPiece* hand;
    int no = 0;
    int i;

    for (i = 0; i < 0x3E; i++) {
        numDisp(0x40 + i, 0, 0, 0);
    }
    pl = wk->x2B0;
    hand = pl->hand;
    for (i = 0; i < wk->x2B0->pieceNum(); i++) {
        pzlPiece* p = wk->x2B0->piecePtr(i);
        cModel* m = p->model;
        Vec pos;
        Vec scr;
        ItemInfo info;
        ItemWork* item;
        int id;

        if (wk->x2B0->spaceBoard->search(p)) {
            m->x12F = 3;
        } else {
            m->x12F = 1;
        }
        if ((p->state & 1) || (hand && hand == p)) {
            m->be_flag |= 2;
            id = 0x41 + no;
            if (hand && p == hand) {
                m->x12F = 1;
                id = 0x40;
                m->scale.z = piece_hand_scale;
            } else {
                m->scale.z = 0.0f;
            }
            pieceModelOrientation(wk, p);
            getPieceVertex(p, &pos, 0);
            puzzlePos2screenPos(&pos, &scr);
            item = p->item;
            itemInfo(item->id, &info);
            if (info.type == 1) {
                if ((item->x8 >> 13) == 1) {
                    numDisp(id, item->x8 & 0x1FFF, &scr, 3);
                } else {
                    numDisp(id, item->x8 & 0x1FFF, &scr, 1);
                }
                no++;
            } else {
                itemInfo(item->id, &info);
                if (info.type != 9) {
                    itemInfo(item->id, &info);
                    if (info.x4 != 1 || item->num != 1) {
                        numDisp(id, item->num, &scr, 1);
                        no++;
                    }
                }
            }
            if (hand && hand == p) {
                pieceFrameDisp(m, 0, 4);
            } else {
                pieceFrameDisp(m, 0, 0);
            }
        } else {
            m->be_flag &= ~2;
        }
    }
    MapMgr.move();
}

void pieceModelInit(SUB_SCREEN* wk)
{
    pzlPlayer* pl = wk->x2B0;
    cModel* m;
    int i;

    pieceTblInit(wk);
    for (i = 0; i < pl->pieceNum_; i++) {
        pl->pieces[i].model = MapMgr.getWork(i + 4);
        pl->pieces[i].model->be_flag &= ~2;
    }
    for (i = 0; i < pl->pieceNum(); i++) {
        pieceModelSet(pl->piecePtr(i));
    }
    m = MapMgr.getWork(3);
    switch ((s8) wk->x2AE) {
    case 0:
        m->modelInit(SS_ARC_PTR(wk->x1E4, 0x1A6), SS_ARC_PTR(wk->x1E4, 0x1A5));
        break;
    case 1:
        m->modelInit(SS_ARC_PTR(wk->x1E4, 0x1A7), SS_ARC_PTR(wk->x1E4, 0x1A5));
        break;
    case 2:
        m->modelInit(SS_ARC_PTR(wk->x1E4, 0x1A8), SS_ARC_PTR(wk->x1E4, 0x1A5));
        break;
    case 3:
        m->modelInit(SS_ARC_PTR(wk->x1E4, 0x1A9), SS_ARC_PTR(wk->x1E4, 0x1A5));
        break;
    }
    {
        static const Vec ofs = {0.0f, 0.0f, 0.0f};
        static const Vec size = {1000.0f, 1000.0f, 0.0f};

        m->lightInfo.init2(0, 0, &ofs, &size, 0x10);
    }
    m->x135 = 2;
    m->rot = case_rot;
    m->matUpdate();
    m->x12F = 3;
    caseModelMove(1);
    pzzlClearZ(wk);
    pieceModelDisp(wk);
    pzzlCursorDisp(wk, 1);
    m->be_flag |= 2;
}

void pieceModelSet(pzlPiece* p)
{
    void** data = (void**) searchItemModelData(p->item->id, piece_info);
    cModel* m;

    if (data == 0) {
        return;
    }
    m = p->model;
    m->modelInit(data[0], data[1]);
    {
        static const Vec ofs = {0.0f, 0.0f, 0.0f};
        static const Vec size = {1000.0f, 1000.0f, 0.0f};

        m->lightInfo.init2(0, 0, &ofs, &size, 0x10);
    }
    m->x135 = 2;
    if (m->pInfo->be_flag & 2) {
        m->pInfo->be_flag &= ~2;
        pLog->warn(0, 0, "pieceModelSet(): 0x%02x flag SHAPE_MODEL clear", p->item->id);
    }
    m->x12F = 3;
}

// Places the case model under the case id unit, sets the board matrices from it (sw: opening
// position from the id unit, else the fixed position) and the case-space cursor.
void caseModelMove(int sw)
{
    Vec ofsA = {-1280.0f, 0.0f, 0.0f};
    Vec ofsB = {1280.0f, 0.0f, 0.0f};
    SUB_SCREEN* wk = &SubScreenWk;
    IdUnit* u = IdSub.unitPtr(0xFE, 1);
    IdUnit* u2 = IdSub.unitPtr(0xFD, 1);
    cModel* m = MapMgr.getWork(3);
    cModel* parts = m->getPartsPtr(1);
    Vec scr;
    Vec p;
    Mtx mat;
    Mtx mat2;
    Mtx tmp;
    Mtx tmp2;
    Vec q;
    Vec t;
    pzlBoard* b;
    IdUnit* u3;

    if (sw) {
        scr = ofsA;
    } else {
        scr = u->pos;
    }
    scr.y += (f32) pzzlDbgNo;
    screenPos2puzzlePos(&scr, &m->pos);
    PSVECScale(&u2->rotCur, &parts->rot, 0.017453292f);
    m->pos.z = pzzlDbgPos;
    m->matUpdate();
    pzlGrid::size = 100.0f;
    b = wk->x2B0->caseBoard;
    p.x = (f32) b->w * -0.5f * pzlGrid::size;
    p.y = -0.0f;
    p.z = 0.0f;
    PSMTXMultVec(m->mat, &p, &p);
    PSMTXCopy(m->mat, tmp);
    tmp[0][3] = p.x;
    tmp[1][3] = p.y;
    tmp[2][3] = p.z;
    PSMTXCopy(tmp, b->mat);
    u3 = IdSub.unitPtr(0, 0x10);
    b = wk->x2B0->spaceBoard;
    screenPos2puzzlePos(&u3->pos, &q);
    if (sw) {
        q = ofsB;
    }
    PSMTXCopy(wk->x2B0->caseBoard->mat, mat2);
    p.x = pzlGrid::size + pzlGrid::size;
    p.y = 0.0f;
    p.z = 0.0f;
    PSMTXMultVecSR(mat2, &p, &p);
    t.x = mat2[0][3];
    t.y = mat2[1][3];
    t.z = mat2[2][3];
    PSVECAdd(&p, &t, &t);
    PSMTXCopy(mat2, mat);
    mat[0][3] = q.x;
    mat[1][3] = t.y;
    mat[2][3] = t.z;
    PSMTXCopy(mat, b->mat);
    puzzlePos2screenPos(&q, &scr);
    u3->scr.y = scr.y;
}

void SsPzzlInit::init(SUB_SCREEN* wk)
{
    state = 0;
}

void SsPzzlInit::move(SUB_SCREEN* wk)
{
    switch (state) {
    case 0:
        if (wk->x4C(wk) == 1) {
            if (wk->x266 == 2) {
                wk->x44 = 1;
            }
            IdSubErase();
            IdNumErase();
            IdFreeBuffer();
            IdSub.set(SS_ARC_PTR(wk->pCmmn, 0xB), 0xFF, 0x14, 0xC, 6, 0);
            pzzl_wait = state;
            goto NEXT;
        }
        break;
    case 1:
        if (--pzzl_wait >= 0) {
            break;
        }
    NEXT:
        state++;
        break;
    case 2:
        IdSys.dispSw(0x21, 1);
        IdSub.dispSw(2, 1);
        sscrnDataFilename(wk, "ss_pzzl.dat");
#line 1682 "D:/Bio4/Prog/ss_pzzl.cpp"
        pzzl_read_req = DVD_READ_N(wk->path, wk->pPzzl, 0, 0, 0, 0x10);
        if (pzzl_read_req <= 0) {
            break;
        }
        if (wk->x266 != 2 || wk->type == 4) {
            sscrnModelClear(wk);
        } else {
            sscrnModelFree(wk);
            generalModelAlloc(wk);
            playerModelInit();
            sscrnLightClear(wk);
            Cckpt.life.fix(1);
            Cckpt.life.frameIn();
        }
        wk->x44 = 0;
        state++;
    case 3: {
        int result;
        int size;

        if (Dvd.ReadCheck(pzzl_read_req, &result, &size, 0) != 1) {
            break;
        }
        wk->x1E4 = wk->pPzzl;
        state++;
    }
    case 4:
        if (wk->type == 4) {
            GXColor start = {0, 0, 0, 0xFF};
            GXColor end = {0, 0, 0, 0};

            FadeSet(0x80000000, &start, &end, 5, 0, 0);
        }
        transit(0, wk);
        break;
    }
}

void tempSpaceDisp(int sw)
{
    IdUnit* u = IdSub.unitPtr(0, 0x10);

    if (sw == 0) {
        u->dir |= 0xF;
    } else if (sw == 1) {
        u->flags |= 8;
        u->dir &= 0xF0;
    }
}

// 1 when the equipped weapon changed (or ran dry: unarmed).
int checkWeaponChange(int id, int bullets)
{
    if (id != ItemMgr.armId) {
        return 1;
    }
    if (bullets && ItemMgr.bulletNum() == 0) {
        ItemMgr.arm(0);
        return 1;
    }
    return 0;
}

void SsPzzlMain::init(SUB_SCREEN* wk)
{
    IdUnit* tbl[16];
    s8 num;
    int lang;
    int i;
    int j;

    thinking = new PzzlThinking;
    popUp = new PiecePopUp;
    popDown = new PiecePopDown;
    select = new PieceSelect;
    combine = new PieceCombine;
    command = new PieceCommand;
    exam = new SsItemExamine;
    caseChange = new CaseChange;
    thinking->connect(0, select);
    select->connect(0, command);
    select->connect(1, thinking);
    select->connect(2, caseChange);
    command->connect(0, select);
    command->connect(1, combine);
    command->connect(2, exam);
    combine->connect(0, select);
    combine->connect(1, command);
    exam->connect(0, select);
    caseChange->connect(0, select);
    puzzleCameraInit(wk, &pG->Cam);
    IdTexDataLoad(SS_ARC_PTR(wk->x1E4, 0x1AA), 9);
    if (!IdSub.setCk(0x14)) {
        IdSub.set(SS_ARC_PTR(wk->pCmmn, 0xC), 0xFF, 0x14, 0xC, 6, 0);
    }
    IdSub.set(SS_ARC_PTR(wk->x1E4, 0x1AB), 0xFF, 0x10, 0xF, 0, 0);
    tempSpaceDisp(0);
    idMainMenuFade(wk, 1);
    for (i = 0; i < 0x3E; i++) {
        if (i == 0) {
            IdNum.set(SS_ARC_PTR(wk->pCmmn, 7), 0xFF, 0x40, 0x13, 8, 0);
        } else {
            IdNum.setI(SS_ARC_PTR(wk->pCmmn, 7), 0xFF, 0x40 + i, 0x13, 9, 0);
        }
    }
    IdSub.set(SS_ARC_PTR(wk->pCmmn, 0xC), 0xFF, 0x1C, 0x13, 2, 0);
    IdSub.set(SS_ARC_PTR(wk->pCmmn, 0xD), 0xFF, 0x1D, 0x13, 2, 0);
    for (lang = 0; lang < 2; lang++) {
        u8 type;

        for (i = 0; i < 10; i++) {
            setCommandId(i, tbl, &num, lang);
            for (j = 0; j < num * 2 + 6; j++) {
                tbl[j]->dir |= 0xF;
                tbl[j]->flags &= ~8;
            }
        }
        type = lang == 0 ? 0x1C : 0x1D;
        for (i = 0; i < 11; i++) {
            tbl[i] = IdSub.unitPtr(0x70 + i, type);
            tbl[i]->flags &= ~8;
            tbl[i]->dir |= 0xF;
        }
        for (i = 0; i < 11; i++) {
            tbl[i] = IdSub.unitPtr(0x80 + i, type);
            tbl[i]->flags &= ~8;
            tbl[i]->dir |= 0xF;
        }
    }
    IdSub.set(SS_ARC_PTR(wk->pCmmn, 0x10), 0xFF, 0x1E, 0x13, 1, 0);
    sscrnLightCreate(wk, (cLit*) SS_ARC_PTR(wk->pCmmn, 0x12));
    if (wk->x266 == 2 && wk->type != 4) {
        wk->x269 = 0;
        wk->x26A = 10;
    }
    wk->x2B0 = new pzlPlayer;
    if (!wk->x2B0->init((s8) wk->x2AE)) {
        delete wk->x2B0;
    }
    pieceModelInit(wk);
    if (wk->type & 4) {
        pzlPlayer* pl;
        pzlPiece* p;
        int w;
        int h;
        int x;
        int y;

        ItemMgr.get(wk->x2FA, wk->x2FC);
        wk->x300 = ItemMgr.pLast;
        wk->x2B0->appendExtraPiece(ItemMgr.pLast);
        wk->x2B0->inHandExtraPiece();
        wk->x2FC = wk->x300->num;
        pl = wk->x2B0;
        p = pl->extra;
        h = pl->spaceBoard->h;
        w = pl->spaceBoard->w;
        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++) {
                p->x = (f32) x + p->cx;
                p->y = (f32) y + p->cy;
                if (pl->putPiece(pl->spaceBoard)) {
                    goto PUT;
                }
            }
        }
        p->orientation(1);
        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++) {
                p->x = (f32) x + p->cx;
                p->y = (f32) y + p->cy;
                if (pl->putPiece(pl->spaceBoard)) {
                    goto PUT;
                }
            }
        }
    PUT:
        pl->cur = pl->spaceBoard;
        pl->getPiece(pl->spaceBoard);
        pieceModelSet(wk->x2B0->extra);
        cur = thinking;
        cur->init(wk);
    } else {
        if ((wk->flags & 4) && wk->x2B0->spaceBoard->getPieceNum() != 0) {
            wk->x2B0->cur = wk->x2B0->spaceBoard;
            thinking->init(wk);
        }
        cur = select;
    }
    sscrnMainMenuInit(wk, 0);
    state = 0;
    caseMove = 1;
    sscrn_pzzl_in_init(wk);
    MesData.ptr[0] = (u8*) SS_ARC_PTR(wk->pCmmn, 4);
    MesData.ptr[1] = (u8*) SS_ARC_PTR(wk->x1E4, 0x1A4);
    pzzl_dbg.init(wk);
    SndCall(0, 0x1E, 0, 0, 0, 0);
}

void SsPzzlMain::move(SUB_SCREEN* wk)
{
    int old;
    u16 armId;
    int bullets;

    caseModelMove(caseMove);
    pzzlClearZ(wk);
    pieceModelDisp(wk);
    pzzlCursorDisp(wk, 1);
    if (cur != exam) {
        IdUnit* u = IdSub.unitPtr(1, 0x1E);
        int id = 0;
        int on = 0;
        int x;
        int y;
        pzlPlayer* pl = wk->x2B0;

        if (pl->hand) {
            id = pl->hand->item->id;
            on = 1;
        } else if (pl->ptrPiece(pl->cur)) {
            on = 1;
            id = wk->x2B0->ptrPiece(wk->x2B0->cur)->item->id;
        }
        x = (int) ((u->pos.x + 320.0f) * 0.8f);
        y = (int) ((240.0f - u->pos.y) * 0.8f);
        y -= cMes.getMes(0)->fontH / 2;
        if (on && caseMove == 0) {
            cMesS.setFontSizeS(0, pzzl_font_w[1], pzzl_font_h[1]);
            cMes.getMes(0)->lineH = 0;
            cMes.getMes(0)->charSpace = pzzl_font_space[3];
            cMes.MesSet(id, x, y, 0x20088, 0, 0, 4);
        } else {
            cMes.Delete(0);
        }
    }
    armId = ItemMgr.armId;
    bullets = ItemMgr.bulletNum();
    old = state;
    wk->x267 = 0;
    msg_open = 0;
    switch (state) {
    case 0:
        if (wk->x366 == 0) {
            cur->move(wk);
            next = cur->cur;
            wk->x2B0->save();
            if (cur == select) {
                switch (select->mode) {
                case 2:
                    state = 1;
                    wk->x34 |= 1;
                    sscrnMainMenuInit(wk, 1);
                    break;
                case 1:
                    wk->x34 |= 1;
                    transit(4, wk);
                    break;
                default:
                    if (wk->type != 4 && next == cur && wk->x2B0->spaceBoard->getPieceNum() == 0 &&
                        (Key.trg & 0x00400000)) {
                        wk->x34 = 0;
                        wk->x266 = 1;
                        transit(0, wk);
                    }
                    break;
                }
            }
        }
        cur = next;
        break;
    case 1:
        if (wk->x366 == 0) {
            if (sscrnMainMenu(wk)) {
                switch ((s8) wk->x264) {
                case 1:
                    select->mode = 0;
                    sscrnMainMenuInit(wk, 0);
                    state = 0;
                    SndCall(0, 6, 0, 0, 0, 0);
                    break;
                case 0:
                    transit(0, wk);
                    break;
                case 3:
                    transit(3, wk);
                    break;
                case 2:
                    transit(2, wk);
                    break;
                case 4:
                    transit(4, wk);
                    break;
                }
            }
            if (Key.trg & 0x03000000) {
                select->mode = 0;
                sscrnMainMenuInit(wk, 0);
                state = 0;
                wk->x2B0->cur = wk->x2B0->caseBoard;
                if (Key.trg & 0x01000000) {
                    wk->x2B0->caseBoard->curY = wk->x2B0->caseBoard->h - 1;
                } else {
                    wk->x2B0->caseBoard->curY = 0;
                }
                SndCall(0, 6, 0, 0, 0, 0);
            }
        }
        break;
    }
    if (checkWeaponChange(armId, bullets)) {
        weaponChangeRequest(WeaponId2WeaponNo(ItemMgr.armId), WeaponId2WeaponType(ItemMgr.armId));
    }
    if (old != state) {
        wk->x268 |= 2;
    }
    if (wk->x366) {
        pzzl_dbg.move(wk);
        if (Joy[0].trg & 0x200) {
            wk->x366 = wk->x366 == 0;
            if (wk->x366) {
                pG->debug_mode = 1;
            } else {
                pG->debug_mode = *((u8*) &wk->debugMode + 3);
            }
        }
    }
    if (wk->x2B0->hand == 0 && (Joy[0].trg & 0x10)) {
        if (!(pG->flags_54 & 8) || PadCheckStatus(&Joy[1]) == 1) {
            wk->x366 = wk->x366 == 0;
        }
        if (wk->x366) {
            pG->debug_mode = 1;
        } else {
            pG->debug_mode = *((u8*) &wk->debugMode + 3);
        }
    }
    if (caseMove) {
        caseMove = 0;
    }
}

void SsPzzlMain::quit(SUB_SCREEN* wk)
{
    pzzlCursorDisp(wk, 0);
    pzzlEquipDisp(wk, 0);
    if (wk->type & 4) {
        pzlPlayer* pl = wk->x2B0;

        if (pl->extra && pl->spaceBoard->search(pl->extra)) {
            wk->x2B0->removeExtraPiece();
            ItemMgr.dumpAll(wk->x300);
            if (wk->x300 == ItemMgr.pArm) {
                ItemMgr.arm(0);
            }
            wk->x40 = 0;
            wk->x300 = 0;
        } else {
            wk->x2B0->save();
            wk->x40 = 1;
        }
    }
    pzzl_dbg.quit();
    ssWidgetDelete(thinking);
    ssWidgetDelete(popUp);
    ssWidgetDelete(popDown);
    ssWidgetDelete(select);
    ssWidgetDelete(combine);
    ssWidgetDelete(command);
    ssWidgetDelete(exam);
    ssWidgetDelete(caseChange);
    wk->x2B0->quit();
    delete wk->x2B0;
    sscrn_pzzl_out_init(wk);
    wk->x4C = sscrn_pzzl_out;
}

void sscrn_pzzl_out_init(SUB_SCREEN* wk)
{
    IdSub.unitPtr(0xFE, 1)->dir |= 0xF;
    IdSub.unitPtr(0xFD, 1)->dir |= 0xF;
    IdSub.unitPtr(1, 0x1E)->dir |= 1;
    if (wk->x265 == 2) {
        Cckpt.life.frameOut();
        wk->x269 = 1;
    }
}

static int sscrn_pzzl_out(SUB_SCREEN* wk)
{
    IdUnit* u;
    IdUnit* u2;

    caseModelMove(0);
    pzzlClearZ(wk);
    pieceModelDisp(wk);
    pzzlCursorDisp(wk, 1);
    u = IdSub.unitPtr(0xFE, 1);
    u2 = IdSub.unitPtr(0xFD, 1);
    if ((u->end & 1) && (u2->end & 4)) {
        return 1;
    }
    return 0;
}

void sscrn_pzzl_in_init(SUB_SCREEN* wk)
{
    IdSub.unitPtr(0xFE, 1)->dir &= 0xF0;
    IdSub.unitPtr(0xFD, 1)->dir &= 0xF0;
}

void PiecePopUp::init(SUB_SCREEN* wk)
{
    count = 0;
}

void PiecePopUp::move(SUB_SCREEN* wk)
{
    if (count++ > 0) {
        transit(0, wk);
    }
}

void PiecePopDown::init(SUB_SCREEN* wk)
{
    count = 0;
}

void PiecePopDown::move(SUB_SCREEN* wk)
{
    if (count++ > 0) {
        transit(0, wk);
    }
}

void PzzlThinking::init(SUB_SCREEN* wk)
{
    Cckpt.life.frameOut();
    IdSub.unitPtr(0, 2)->dir |= 0xF;
    tempSpaceDisp(1);
    idMainMenuFade(wk, 0);
}

void PzzlThinking::move(SUB_SCREEN* wk)
{
    pzlPlayer* pl;

    if (Key.trg & 0x80020000) {
        if (wk->x2B0->putPiece(wk->x2B0->cur)) {
            transit(0, wk);
            SndCall(0, 0xC, 0, 0, 0, 0);
        } else {
            pzlPiece* p = wk->x2B0->cmbPiece(wk->x2B0->cur);

            if (p) {
                ItemInfo info;

                pieceModelSet(p);
                pieceModelOrientation(wk, p);
                wk->x2B0->rehash();
                if (wk->x2B0->hand == 0) {
                    transit(0, wk);
                }
                itemInfo(p->item->id, &info);
                if (info.type == 2) {
                    SndCall(0, 0x29, 0, 0, 0, 0);
                } else if (info.type == 6) {
                    SndCall(0, 0x27, 0, 0, 0, 0);
                } else {
                    SndCall(0, 0x28, 0, 0, 0, 0);
                }
            } else if (wk->x2B0->chgPiece(wk->x2B0->cur)) {
                SndCall(0, 0xD, 0, 0, 0, 0);
            }
        }
    } else if (Key.trg & 0x40000000) {
        if (wk->x2B0->relPiece(wk->x2B0->cur)) {
            transit(0, wk);
            SndCall(0, 0xC, 0, 0, 0, 0);
        }
    } else {
        switch (wk->x2B0->movePiece()) {
        case 1:
            SndCall(0, 0xA, 0, 0, 0, 0);
            break;
        case 2:
            SndCall(0, 0xB, 0, 0, 0, 0);
            break;
        }
    }
}

void PzzlThinking::quit(SUB_SCREEN* wk)
{
    back2PieceSelect(wk);
}

// 1 when the screen may close: nothing (or only the extra piece) on the space board.
int isTerminable(SUB_SCREEN* wk)
{
    pzlPlayer* pl = wk->x2B0;
    int n = pl->spaceBoard->getPieceNum();

    if (n == 0) {
        return 1;
    }
    if (n == 1 && pl->spaceBoard->search(pl->extra)) {
        return 1;
    }
    return 0;
}

int checkMsgWindow(SUB_SCREEN* wk)
{
    if (CMES_FLAGS & 2) {
        return 1;
    }
    return 0;
}

void closeMsgWindow(SUB_SCREEN* wk)
{
    IdSub.kill(0xFF, 3);
    cMes.Delete(1);
}

void openMsgWindow(SUB_SCREEN* wk, int no)
{
    msg_open = 1;
    cMes.Delete(1);
    cMes.Delete(2);
    cMes.setLayout(1, 2);
    cMes.MesSet(no, msg_x, msg_y, 0x11, 1, 0, 3);
    IdSub.set(SS_ARC_PTR(wk->pCmmn, 0xA), 0xFF, 3, 0x13, 0, 0);
}

void PieceSelect::init(SUB_SCREEN* wk)
{
    state = 0;
}

void PieceSelect::move(SUB_SCREEN* wk)
{
    pzlPlayer* pl = wk->x2B0;
    pzlBoard* b = pl->cur;
    pzlBoard* other = pl->caseBoard;
    pzlBoard* space = pl->spaceBoard;

    wk->x267 = 1;
    if (b == other) {
        other = pl->spaceBoard;
    }
    if (wk->x2AE != wk->x2AF) {
        transit(2, wk);
        return;
    }
    switch (state) {
    case 0:
        mode = state;
        if (Key.trg & 0x00100000) {
            mode = 1;
            if (!isTerminable(wk)) {
                mode |= 8;
                openMsgWindow(wk, 0);
                state = 1;
                SndCall(0, 0x2A, 0, 0, 0, 0);
            }
        } else if (Key.trg & 0x40000000) {
            if (wk->type & 4) {
                mode = 1;
                if (!isTerminable(wk)) {
                    mode |= 8;
                    openMsgWindow(wk, 0);
                    state = 1;
                    SndCall(0, 0x2A, 0, 0, 0, 0);
                } else {
                    Cckpt.life.frameIn();
                    IdSub.unitPtr(0, 2)->dir &= 0xF0;
                }
            } else if (link[3] == 0) {
                mode = 2;
                if (!isTerminable(wk)) {
                    mode |= 8;
                    openMsgWindow(wk, 0);
                    state = 1;
                    SndCall(0, 0x2A, 0, 0, 0, 0);
                } else {
                    wk->x264 = 4;
                    wk->x265 = 4;
                    SndCall(0, 0xA, 0, 0, 0, 0);
                }
            } else {
                mode = 4;
                if (!isTerminable(wk)) {
                    mode |= 8;
                    openMsgWindow(wk, 0);
                    state = 1;
                    SndCall(0, 9, 0, 0, 0, 0);
                } else {
                    transit(3, wk);
                }
            }
        } else if (Key.trg & 0x80000000) {
            if (pl->ptrPiece(pl->cur) && link[0]) {
                pzzl_sel = wk->x2B0->ptrPiece(wk->x2B0->cur);
                transit(0, wk);
            }
        } else if (Key.trg & 0x00020000) {
            if (pl->ptrPiece(pl->cur)) {
                pl->getPiece(pl->cur);
                transit(1, wk);
                SndCall(0, 0xD, 0, 0, 0, 0);
            }
        } else {
            int r = pl->selPiece(pl->cur);

            if (r == 5) {
                wk->x268 |= 1;
                SndCall(0, 6, 0, 0, 0, 0);
            }
            switch (r) {
            case 1:
                if (link[3] == 0 && space->getPieceNum() == 0 && wk->type != 4) {
                    mode = 2;
                    wk->x264 = 0;
                    wk->x265 = 0;
                    SndCall(0, 0xA, 0, 0, 0, 0);
                } else {
                    b->curY = b->h - 1;
                    SndCall(0, 6, 0, 0, 0, 0);
                }
                break;
            case 2:
                if (link[3] == 0 && space->getPieceNum() == 0 && wk->type != 4) {
                    mode = r;
                    wk->x264 = state;
                    wk->x265 = state;
                    SndCall(0, 0xA, 0, 0, 0, 0);
                } else {
                    b->curY = 0;
                    SndCall(0, 6, 0, 0, 0, 0);
                }
                break;
            case 3:
                if (other->getPieceNum() != 0) {
                    pl->cur = other;
                    b = wk->x2B0->cur;
                }
                b->curX = b->w - 1;
                SndCall(0, 6, 0, 0, 0, 0);
                break;
            case 4:
                if (other->getPieceNum() != 0) {
                    pl->cur = other;
                    b = wk->x2B0->cur;
                }
                b->curX = 0;
                SndCall(0, 6, 0, 0, 0, 0);
                break;
            }
        }
        break;
    case 1: {
        int r;

        msg_open = state;
        r = CMES_RESULT;
        if (r == 0) {
            break;
        }
        closeMsgWindow(wk);
        if (r == 1) {
            if (wk->type & 4) {
                if (wk->x2B0->extra) {
                    ItemMgr.offboardDump(wk->x300);
                } else {
                    ItemMgr.offboardDump(0);
                }
            } else {
                ItemMgr.offboardDump(0);
            }
            wk->x2B0->rehash();
            back2PieceSelect(wk);
            state = 2;
            SndCall(0, 0xD, 0, 0, 0, 0);
        } else if (r == 2) {
            state = 0;
            SndCall(0, 5, 0, 0, 0, 0);
        }
        break;
    }
    case 2:
        mode &= ~8;
        switch (mode) {
        case 2:
            wk->x264 = 4;
            wk->x265 = 4;
            break;
        case 4:
            transit(3, wk);
            break;
        }
        state = 0;
        break;
    }
}

// Message number for a failed combine of the two item ids (0 when there is none).
int remarkMsgCombine(int a, int b, int* no)
{
    int i;

    for (i = 0; i < 2; i++) {
        switch (i == 0 ? a : b) {
        case 0x12:
            *no = 0x21;
            return 1;
        case 0x14:
            *no = 0x22;
            return 1;
        case 0x16:
            *no = 0x23;
            return 1;
        case 0xA8:
            *no = 0x24;
            return 1;
        }
    }
    return 0;
}

void PieceCombine::init(SUB_SCREEN* wk)
{
    state = 0;
}

void PieceCombine::move(SUB_SCREEN* wk)
{
    pzlPlayer* pl = wk->x2B0;
    pzlBoard* b = pl->cur;
    pzlBoard* other = pl->caseBoard;

    wk->x267 = 2;
    if (b == other) {
        other = pl->spaceBoard;
    }
    switch (state) {
    case 0:
        if (Key.trg & 0x40000000) {
            pl->loadCursor();
            pzzl_sel = wk->x2B0->ptrPiece(wk->x2B0->cur);
            transit(1, wk);
        } else if (Key.trg & 0x80000000) {
            if (pl->ptrPiece(b)) {
                pzlPiece* p = wk->x2B0->ptrPiece(wk->x2B0->cur);
                pzlPiece* sel = pzzl_sel;
                int extra = p == wk->x2B0->extra;
                u16 idA;
                u16 idB;

                if (sel == wk->x2B0->extra) {
                    extra = 1;
                }
                idA = p->item->id;
                idB = sel->item->id;
                if (p != sel) {
                    if (ItemMgr.combine(p->item, sel->item, 0)) {
                        ItemInfo info;

                        if (extra == 1) {
                            wk->x2B0->giveupExtraPiece();
                        }
                        if (idA != p->item->id) {
                            pieceModelSet(p);
                            pieceModelOrientation(wk, p);
                        }
                        if (idB != pzzl_sel->item->id) {
                            pieceModelSet(pzzl_sel);
                            pieceModelOrientation(wk, pzzl_sel);
                        }
                        wk->x2B0->rehash();
                        transit(0, wk);
                        itemInfo(idB, &info);
                        if (info.type == 2) {
                            SndCall(0, 0x29, 0, 0, 0, 0);
                        } else if (info.type == 6) {
                            SndCall(0, 0x27, 0, 0, 0, 0);
                        } else {
                            SndCall(0, 0x28, 0, 0, 0, 0);
                        }
                    } else {
                        int no;

                        if (remarkMsgCombine(p->item->id, pzzl_sel->item->id, &no)) {
                            openMsgWindow(wk, no);
                            state = 1;
                        }
                        SndCall(0, 7, 0, 0, 0, 0);
                    }
                } else {
                    SndCall(0, 7, 0, 0, 0, 0);
                }
            } else {
                SndCall(0, 7, 0, 0, 0, 0);
            }
        } else {
            int r = pl->selPiece(b);

            if (r == 5) {
                wk->x268 |= 1;
                SndCall(0, 6, 0, 0, 0, 0);
            }
            switch (r) {
            case 1:
                b->curY = b->h - 1;
                break;
            case 2:
                b->curY = 0;
                break;
            case 3:
                if (other->getPieceNum() != 0) {
                    pl->cur = other;
                    b = wk->x2B0->cur;
                }
                b->curX = b->w - 1;
                break;
            case 4:
                if (other->getPieceNum() != 0) {
                    pl->cur = other;
                    b = wk->x2B0->cur;
                }
                b->curX = 0;
                break;
            }
        }
        break;
    case 1:
        if (checkMsgWindow(wk)) {
            closeMsgWindow(wk);
            state = 0;
        }
        break;
    }
}

void PieceCombine::quit(SUB_SCREEN* wk)
{
    back2PieceSelect(wk);
}

void PieceCommand::init(SUB_SCREEN* wk)
{
    u8 type = itemCommandType(pzzl_sel->item);
    pzlPlayer* pl = wk->x2B0;
    Vec pos;
    Vec half;
    Vec pc;
    int i;
    int corner;

    half.x = (f32) pl->cur->w * 0.5f;
    half.y = (f32) pl->cur->h * 0.5f;
    pc.x = pzzl_sel->x;
    pc.y = pzzl_sel->y;
    onCase = pl->caseBoard->search(pzzl_sel) != 0;
    lower = pc.y > half.y - 0.5f;
    setCommandId(type, id, &num, onCase);
    for (i = 0; i < num * 2 + 6; i++) {
        id[i]->dir &= 0xF0;
        id[i]->flags |= 8;
    }
    if (onCase) {
        corner = lower ? 3 : 0;
    } else {
        corner = lower ? 2 : 1;
    }
    pos = pzzl_cursor.v[corner];
    puzzlePos2screenPos(&pos, &id[0]->scr);
    if (lower) {
        id[0]->scr.y += id[2]->sizeY;
    }
    cursorOld = 1;
    mode = 0;
    wk->x26C = 0;
    SndCall(0, 4, 0, 0, 0, 0);
}

void PieceCommand::move(SUB_SCREEN* wk)
{
    Vec pos;
    int corner;
    int i;

    wk->x267 = 1;
    if (onCase) {
        corner = lower ? 3 : 0;
    } else {
        corner = lower ? 2 : 1;
    }
    pos = pzzl_cursor.v[corner];
    puzzlePos2screenPos(&pos, &id[0]->scr);
    if (lower) {
        id[0]->scr.y += id[2]->sizeY;
    }
    switch (mode) {
    case 0:
        if (Key.trg & 0x40000000) {
            for (i = 0; i < num * 2 + 6; i++) {
                id[i]->dir |= 0xF;
            }
            transit(0, wk);
            SndCall(0, 5, 0, 0, 0, 0);
        } else if (Key.trg & 0x80000000) {
            int used = 0;
            int type = itemCommandType(pzzl_sel->item);

            switch (type) {
            case 0:
                switch (wk->x26C) {
                case 0:
                    command_id = used;
                    break;
                case 1:
                    command_id = 5;
                    break;
                case 2:
                    command_id = 6;
                    break;
                }
                break;
            case 1:
                switch (wk->x26C) {
                case 0:
                    command_id = used;
                    break;
                case 1:
                    command_id = type;
                    break;
                case 2:
                    command_id = 5;
                    break;
                case 3:
                    command_id = 6;
                    break;
                }
                break;
            case 2:
                switch (wk->x26C) {
                case 0:
                    command_id = used;
                    break;
                case 1:
                    command_id = 5;
                    break;
                case 2:
                    command_id = type;
                    break;
                case 3:
                    command_id = 5;
                    break;
                case 4:
                    command_id = 6;
                    break;
                }
                break;
            case 3:
                switch (wk->x26C) {
                case 0:
                    command_id = 2;
                    break;
                case 1:
                    command_id = 5;
                    break;
                case 2:
                    command_id = 6;
                    break;
                }
                break;
            case 6:
                switch (wk->x26C) {
                case 0:
                    command_id = 3;
                    break;
                case 1:
                    command_id = 5;
                    break;
                case 2:
                    command_id = type;
                    break;
                }
                break;
            case 4:
                switch (wk->x26C) {
                case 0:
                    command_id = type;
                    break;
                case 1:
                    command_id = 5;
                    break;
                case 2:
                    command_id = 6;
                    break;
                }
                break;
            case 7:
                switch (wk->x26C) {
                case 0:
                    command_id = 5;
                    break;
                case 1:
                    command_id = 6;
                    break;
                }
                break;
            case 8:
                switch (wk->x26C) {
                case 0:
                    command_id = 4;
                    break;
                case 1:
                    command_id = used;
                    break;
                case 2:
                    command_id = 5;
                    break;
                case 3:
                    command_id = 6;
                    break;
                }
                break;
            case 9:
                if (wk->x26C == 0) {
                    command_id = 5;
                }
                break;
            default:
                switch (wk->x26C) {
                case 0:
                    command_id = 4;
                    break;
                case 1:
                    command_id = 2;
                    break;
                case 2:
                    command_id = 5;
                    break;
                case 3:
                    command_id = 6;
                    break;
                }
                break;
            }
            if (command_id == 2 || command_id == 5) {
                for (i = 0; i < num * 2 + 6; i++) {
                    id[i]->dir |= 0xF;
                }
                wk->x2B0->saveCursor();
            }
            switch (command_id) {
            case 0:
                if (pG->x4FB8 == 1) {
                    break;
                }
                if (wk->flags & 2) {
                    for (i = 0; i < num * 2 + 6; i++) {
                        id[i]->dir |= 0xF;
                    }
                    mode = 3;
                    openMsgWindow(wk, 0x27);
                } else {
                    used = ItemMgr.arm(pzzl_sel->item) != 0;
                    if (used) {
                        wk->x2B0->rehash();
                        for (i = 0; i < num * 2 + 6; i++) {
                            id[i]->dir |= 0xF;
                        }
                    }
                }
                break;
            case 1: {
                pzlPiece* extra = 0;
                int extraNum = 0;

                if (wk->x2B0->extra) {
                    extra = wk->x2B0->extra;
                    extraNum = extra->item->num;
                }
                used = ItemMgr.reload(pzzl_sel->item, 1);
                if (wk->x2B0->extra && extraNum != extra->item->num) {
                    wk->x2B0->giveupExtraPiece();
                }
                break;
            }
            case 2:
                transit(1, wk);
                SndCall(0, 9, 0, 0, 0, 0);
                return;
            case 3: {
                ItemInfo info;

                pzzl_sel->item->x6 = 0;
                used = 1;
                itemInfo(ItemMgr.armId, &info);
                if (info.type == 1) {
                    ItemMgr.arm(ItemMgr.pArm);
                }
                break;
            }
            case 4:
                if (pSUB && ((cModel*) pSUB)->id == 3) {
                    subSel = 0;
                    mode = 1;
                    SndCall(0, 4, 0, 0, 0, 0);
                    return;
                }
                ItemMgr.x12 = 0;
                used = ItemMgr.use(pzzl_sel->item);
                if (used) {
                    PlMotionReset();
                }
                break;
            case 5:
                wk->x248 = pzzl_sel->item;
                wk->x24C = MapMgr.getWork(2);
                transit(2, wk);
                SndCall(0, 0x1A, 0, 0, 0, 0);
                return;
            case 6:
                subSel = 1;
                mode = 1;
                SndCall(0, 0x2A, 0, 0, 0, 0);
                return;
            }
            if (used == 1) {
                wk->x2B0->rehash();
                for (i = 0; i < num * 2 + 6; i++) {
                    id[i]->dir |= 0xF;
                }
                transit(0, wk);
                SndCall(0, 8, 0, 0, 0, 0);
            } else {
                SndCall(0, 7, 0, 0, 0, 0);
            }
        } else {
            int cur;

            if (Key.rep & 0x01000000) {
                wk->x26C--;
            } else if (Key.rep & 0x02000000) {
                wk->x26C++;
            }
            cur = wk->x26C;
            if (cur < 0) {
                cur = num - 1;
            } else if (cur > num - 1) {
                cur = 0;
            }
            wk->x26C = cur;
            if (Key.rep & 0x03000000) {
                SndCall(0, 0xA, 0, 0, 0, 0);
            }
            if (cursorOld != wk->x26C) {
                for (i = 0; i < num; i++) {
                    if (i == wk->x26C) {
                        id[6 + i * 2]->flags |= 8;
                    } else {
                        id[6 + i * 2]->flags &= ~8;
                    }
                }
                cursorOld = wk->x26C;
            }
        }
        break;
    case 1: {
        u8 type = 0x1C;
        int base = 0;

        if (command_id == 4) {
            base = 0x70;
        } else if (command_id == 6) {
            base = 0x80;
        }
        if (onCase == 0) {
            type = 0x1C;
        } else if (onCase == 1) {
            type = 0x1D;
        }
        for (i = 0; i < 11; i++) {
            sub[i] = IdSub.unitPtr(base + i, type);
            sub[i]->flags |= 8;
            sub[i]->dir &= 0xF0;
        }
        PSVECAdd(&id[6 + wk->x26C * 2]->scr, &id[6 + wk->x26C * 2]->parent->scr, &sub[0]->scr);
        mode = 2;
    }
    case 2:
        if (Key.trg & 0x40000000) {
            for (i = 0; i < 11; i++) {
                sub[i]->dir |= 0xF;
            }
            mode = 0;
            SndCall(0, 5, 0, 0, 0, 0);
        } else if (Key.trg & 0x80000000) {
            int used = 0;
            int msg = 0;

            switch (command_id) {
            case 4:
                if (subSel == 0) {
                    ItemMgr.x12 = 0;
                } else {
                    ItemMgr.x12 = 1;
                }
                if (ItemMgr.x12 == 1) {
                    switch (wk->healing) {
                    case -1:
                        mode = 3;
                        openMsgWindow(wk, 0x26);
                        used = 0;
                        msg = 1;
                        break;
                    case 0:
                        mode = 3;
                        openMsgWindow(wk, 0x25);
                        used = 0;
                        msg = 1;
                        break;
                    case 1:
                        used = ItemMgr.use(pzzl_sel->item);
                        break;
                    }
                } else {
                    used = ItemMgr.use(pzzl_sel->item);
                }
                if (used) {
                    if (ItemMgr.x12 == 0) {
                        PlMotionReset();
                    } else {
                        SubCharMotionReset();
                    }
                    SndCall(0, 8, 0, 0, 0, 0);
                } else {
                    SndCall(0, 7, 0, 0, 0, 0);
                }
                break;
            case 6:
                if (subSel == 0) {
                    used = ItemMgr.dumpAll(pzzl_sel->item);
                    SndCall(0, 0xD, 0, 0, 0, 0);
                } else {
                    SndCall(0, 5, 0, 0, 0, 0);
                }
                break;
            }
            for (i = 0; i < num * 2 + 6; i++) {
                id[i]->dir |= 0xF;
            }
            for (i = 0; i < 11; i++) {
                sub[i]->dir |= 0xF;
            }
            if (used) {
                wk->x2B0->rehash();
                if (wk->x2B0->spaceBoard->getPieceNum() == 0) {
                    wk->x2B0->salvCursor();
                }
            }
            if (msg == 0) {
                transit(0, wk);
            }
        } else {
            int old = subSel;
            int cur;

            if (Key.trg & 0x08000000) {
                subSel--;
            } else if (Key.trg & 0x04000000) {
                subSel++;
            }
            cur = subSel;
            if (cur < 0) {
                cur = 0;
            } else if (cur > 1) {
                cur = 1;
            }
            subSel = cur;
            if (old != (s8) cur) {
                SndCall(0, 0xA, 0, 0, 0, 0);
            }
            sub[5]->flags &= ~8;
            sub[7]->flags &= ~8;
            if (subSel == 0) {
                sub[5]->flags |= 8;
            } else {
                sub[7]->flags |= 8;
            }
        }
        break;
    case 3:
        if (checkMsgWindow(wk)) {
            closeMsgWindow(wk);
            transit(0, wk);
        }
        break;
    }
}

void PieceCommand::quit(SUB_SCREEN* wk)
{
    back2PieceSelect(wk);
}

void CaseChange::init(SUB_SCREEN* wk)
{
    a = IdSub.unitPtr(0xFE, 1);
    b = IdSub.unitPtr(0xFD, 1);
    a->dir |= 0xF;
    b->dir |= 0xF;
}

void CaseChange::move(SUB_SCREEN* wk)
{
    if ((a->end & 1) && (b->end & 4)) {
        wk->x2B0->quit();
        delete wk->x2B0;
        wk->x2AE = wk->x2AF;
        wk->x2B0 = new pzlPlayer;
        if (!wk->x2B0->init((s8) wk->x2AE)) {
            delete wk->x2B0;
        }
        pieceModelInit(wk);
        transit(0, wk);
    }
}

void CaseChange::quit(SUB_SCREEN* wk)
{
    a->dir &= 0xF0;
    b->dir &= 0xF0;
}

// Command menu type of an item: 0 weapon, 3 ammo / treasure, 4 usable, 5 combinable, 6 weapon
// part, 7 key, 8 herb, 9 file.
int itemCommandType(ItemWork* item)
{
    ItemInfo info;

    itemInfo(item->id, &info);
    switch (info.type) {
    case 1:
    case 3:
        return 0;
    case 4:
        return 7;
    case 9:
        if (item->x6 != 0) {
            return 6;
        }
        return 3;
    case 2:
    case 5:
    case 0xC:
        return 3;
    case 6:
        switch (item->id) {
        case 0x19:
        case 0x1C:
        case 0xA8:
            return 3;
        case 8 ... 0xA:
            return 8;
        }
        break;
    case 0xE:
        return 9;
    }
    if (itemCombineCheck(item->id)) {
        return 5;
    }
    return 4;
}

// The command menu id units of a command type (`lang`: 0 the case board set, 1 the space set).
static void setCommandId(u8 type, IdUnit** tbl, s8* num, int lang)
{
    u8 t = lang ? 0x1D : 0x1C;

    switch (type) {
    case 0:
        tbl[0] = IdSub.unitPtr(0, t);
        tbl[1] = IdSub.unitPtr(1, t);
        tbl[2] = IdSub.unitPtr(2, t);
        tbl[3] = IdSub.unitPtr(3, t);
        tbl[4] = IdSub.unitPtr(0xA, t);
        tbl[5] = IdSub.unitPtr(0xB, t);
        *num = 3;
        tbl[6] = IdSub.unitPtr(4, t);
        tbl[7] = IdSub.unitPtr(5, t);
        tbl[8] = IdSub.unitPtr(6, t);
        tbl[9] = IdSub.unitPtr(7, t);
        tbl[10] = IdSub.unitPtr(8, t);
        tbl[11] = IdSub.unitPtr(9, t);
        break;
    case 1:
        tbl[0] = IdSub.unitPtr(0x10, t);
        tbl[1] = IdSub.unitPtr(0x11, t);
        tbl[2] = IdSub.unitPtr(0x12, t);
        tbl[3] = IdSub.unitPtr(0x13, t);
        tbl[4] = IdSub.unitPtr(0x1C, t);
        tbl[5] = IdSub.unitPtr(0x1D, t);
        *num = 4;
        tbl[6] = IdSub.unitPtr(0x14, t);
        tbl[7] = IdSub.unitPtr(0x15, t);
        tbl[8] = IdSub.unitPtr(0x16, t);
        tbl[9] = IdSub.unitPtr(0x17, t);
        tbl[10] = IdSub.unitPtr(0x18, t);
        tbl[11] = IdSub.unitPtr(0x19, t);
        tbl[12] = IdSub.unitPtr(0x1A, t);
        tbl[13] = IdSub.unitPtr(0x1B, t);
        break;
    case 2:
        tbl[0] = IdSub.unitPtr(0x20, t);
        tbl[1] = IdSub.unitPtr(0x21, t);
        tbl[2] = IdSub.unitPtr(0x22, t);
        tbl[3] = IdSub.unitPtr(0x23, t);
        tbl[4] = IdSub.unitPtr(0x2E, t);
        tbl[5] = IdSub.unitPtr(0x2F, t);
        *num = 5;
        tbl[6] = IdSub.unitPtr(0x24, t);
        tbl[7] = IdSub.unitPtr(0x25, t);
        tbl[8] = IdSub.unitPtr(0x26, t);
        tbl[9] = IdSub.unitPtr(0x27, t);
        tbl[10] = IdSub.unitPtr(0x28, t);
        tbl[11] = IdSub.unitPtr(0x29, t);
        tbl[12] = IdSub.unitPtr(0x2A, t);
        tbl[13] = IdSub.unitPtr(0x2B, t);
        tbl[14] = IdSub.unitPtr(0x2C, t);
        tbl[15] = IdSub.unitPtr(0x2D, t);
        break;
    case 3:
        tbl[0] = IdSub.unitPtr(0x30, t);
        tbl[1] = IdSub.unitPtr(0x31, t);
        tbl[2] = IdSub.unitPtr(0x32, t);
        tbl[3] = IdSub.unitPtr(0x33, t);
        tbl[4] = IdSub.unitPtr(0x3A, t);
        tbl[5] = IdSub.unitPtr(0x3B, t);
        *num = type;
        tbl[6] = IdSub.unitPtr(0x34, t);
        tbl[7] = IdSub.unitPtr(0x35, t);
        tbl[8] = IdSub.unitPtr(0x36, t);
        tbl[9] = IdSub.unitPtr(0x37, t);
        tbl[10] = IdSub.unitPtr(0x38, t);
        tbl[11] = IdSub.unitPtr(0x39, t);
        break;
    case 4:
        tbl[0] = IdSub.unitPtr(0x40, t);
        tbl[1] = IdSub.unitPtr(0x41, t);
        tbl[2] = IdSub.unitPtr(0x42, t);
        tbl[3] = IdSub.unitPtr(0x43, t);
        tbl[4] = IdSub.unitPtr(0x4A, t);
        tbl[5] = IdSub.unitPtr(0x4B, t);
        *num = 3;
        tbl[6] = IdSub.unitPtr(0x44, t);
        tbl[7] = IdSub.unitPtr(0x45, t);
        tbl[8] = IdSub.unitPtr(0x46, t);
        tbl[9] = IdSub.unitPtr(0x47, t);
        tbl[10] = IdSub.unitPtr(0x48, t);
        tbl[11] = IdSub.unitPtr(0x49, t);
        break;
    case 5:
        tbl[0] = IdSub.unitPtr(0x50, t);
        tbl[1] = IdSub.unitPtr(0x51, t);
        tbl[2] = IdSub.unitPtr(0x52, t);
        tbl[3] = IdSub.unitPtr(0x53, t);
        tbl[4] = IdSub.unitPtr(0x5C, t);
        tbl[5] = IdSub.unitPtr(0x5D, t);
        *num = 4;
        tbl[6] = IdSub.unitPtr(0x54, t);
        tbl[7] = IdSub.unitPtr(0x55, t);
        tbl[8] = IdSub.unitPtr(0x56, t);
        tbl[9] = IdSub.unitPtr(0x57, t);
        tbl[10] = IdSub.unitPtr(0x58, t);
        tbl[11] = IdSub.unitPtr(0x59, t);
        tbl[12] = IdSub.unitPtr(0x5A, t);
        tbl[13] = IdSub.unitPtr(0x5B, t);
        break;
    case 6:
        tbl[0] = IdSub.unitPtr(0x90, t);
        tbl[1] = IdSub.unitPtr(0x91, t);
        tbl[2] = IdSub.unitPtr(0x92, t);
        tbl[3] = IdSub.unitPtr(0x93, t);
        tbl[4] = IdSub.unitPtr(0x9A, t);
        tbl[5] = IdSub.unitPtr(0x9B, t);
        *num = 3;
        tbl[6] = IdSub.unitPtr(0x94, t);
        tbl[7] = IdSub.unitPtr(0x95, t);
        tbl[8] = IdSub.unitPtr(0x96, t);
        tbl[9] = IdSub.unitPtr(0x97, t);
        tbl[10] = IdSub.unitPtr(0x98, t);
        tbl[11] = IdSub.unitPtr(0x99, t);
        break;
    case 7:
        tbl[0] = IdSub.unitPtr(0x60, t);
        tbl[1] = IdSub.unitPtr(0x61, t);
        tbl[2] = IdSub.unitPtr(0x62, t);
        tbl[3] = IdSub.unitPtr(0x63, t);
        tbl[4] = IdSub.unitPtr(0x68, t);
        tbl[5] = IdSub.unitPtr(0x69, t);
        *num = 2;
        tbl[6] = IdSub.unitPtr(0x64, t);
        tbl[7] = IdSub.unitPtr(0x65, t);
        tbl[8] = IdSub.unitPtr(0x66, t);
        tbl[9] = IdSub.unitPtr(0x67, t);
        break;
    case 8:
        tbl[0] = IdSub.unitPtr(0xA0, t);
        tbl[1] = IdSub.unitPtr(0xA1, t);
        tbl[2] = IdSub.unitPtr(0xA2, t);
        tbl[3] = IdSub.unitPtr(0xA3, t);
        tbl[4] = IdSub.unitPtr(0xAC, t);
        tbl[5] = IdSub.unitPtr(0xAD, t);
        *num = 4;
        tbl[6] = IdSub.unitPtr(0xA4, t);
        tbl[7] = IdSub.unitPtr(0xA5, t);
        tbl[8] = IdSub.unitPtr(0xA6, t);
        tbl[9] = IdSub.unitPtr(0xA7, t);
        tbl[10] = IdSub.unitPtr(0xA8, t);
        tbl[11] = IdSub.unitPtr(0xA9, t);
        tbl[12] = IdSub.unitPtr(0xAA, t);
        tbl[13] = IdSub.unitPtr(0xAB, t);
        break;
    case 9:
        tbl[0] = IdSub.unitPtr(0xB0, t);
        tbl[1] = IdSub.unitPtr(0xB1, t);
        tbl[2] = IdSub.unitPtr(0xB2, t);
        tbl[3] = IdSub.unitPtr(0xB3, t);
        tbl[4] = IdSub.unitPtr(0xB6, t);
        tbl[5] = IdSub.unitPtr(0xB7, t);
        *num = 1;
        tbl[6] = IdSub.unitPtr(0xB4, t);
        tbl[7] = IdSub.unitPtr(0xB5, t);
        break;
    }
}
