#include "types.h"
#include "map_obj.h"
#include "light.h"
#include "widget.h"
#include "global.h"
#include "joy.h"
#include "eprintf.h"
#include "main_sub.h"
#include "t_prim.h"
#include "t_util.h"

// Debug tool helpers for the tool modules (D:/Bio4/Prog/t_util.cpp, the same object in every t_*/Tools
// REL with a tool). The DOL's game/t_util.cpp is a different version of this file: there the menu drawer
// is out of line and the cursor helpers were dead-stripped; here ToolMenuDisp_cur is an inline nobody
// calls (its strings and statics still land in the object) and TutilMoveCursor/TutilGetScreenPos exist.
// The flag backups are globals (the relocations against them carry no section offset).

void GameStopModeEnd();
extern "C" void GXGetProjectionv(f32* p);
extern "C" void GXGetViewportv(f32* vp);
extern "C" void GXProject(f32 x, f32 y, f32 z, const f32 mtx[3][4], const f32* pm, const f32* vp, f32* sx, f32* sy, f32* sz);

// Copies of the pG flag words the tools modify, restored by TutilQuitDefault.
Camera globalCamera;
u32 debug_flg_bak[4];   // pG->flags_60 .. flags_6C
u32 stop_flg_bak;       // pG->flags_170
u32 disp_flg_bak;       // pG->flags_58
u32 system_flg_bak;     // pG->flags_54
u32 status_flg_bak[4];  // pG->flags_500C .. 0x5018

void TutilInitDefault()
{
    TprimView view;

    GameStopModeEnd();
    view.rect.x = Screen.x;
    view.rect.y = Screen.y;
    view.rect.w = Screen.width;
    view.rect.h = Screen.height;
    view.nearz = 0.0f;
    view.farz = 1.0f;
    TprimInitEnv2D3D(&view, pG->Cam.projMat, pG->Cam.viewMat);
    globalCamera = pG->Cam;
    system_flg_bak = TOOL_FLAG(OFS_SYSTEM_FLG);
    stop_flg_bak = TOOL_FLAG(OFS_STOP_FLG);
    disp_flg_bak = TOOL_FLAG(OFS_DISP_FLG);
    memcpy(debug_flg_bak, TOOL_FLAG_PTR(OFS_DEBUG_FLG), sizeof(debug_flg_bak));
    memcpy(status_flg_bak, TOOL_FLAG_PTR(OFS_STATUS_FLG), sizeof(status_flg_bak));
    TOOL_FLAG(OFS_STOP_FLG) |= 0x200;
    TOOL_FLAG(OFS_STOP_FLG) |= 0x80;
    TOOL_FLAG(OFS_DEBUG_FLG) |= 0x8000;
    TOOL_FLAG(OFS_DEBUG_FLG + 8) |= 0x800000;
    TOOL_FLAG(OFS_DEBUG_FLG + 12) &= ~0x2000;
}

void TutilQuitDefault()
{
    memcpy(TOOL_PTR(OFS_CAMERA), &globalCamera, sizeof(Camera));
    TOOL_FLAG(OFS_SYSTEM_FLG) = system_flg_bak;
    TOOL_FLAG(OFS_STOP_FLG) = stop_flg_bak;
    TOOL_FLAG(OFS_DISP_FLG) = disp_flg_bak;
    if (TOOL_FLAG(OFS_DEBUG_FLG) & 0x100) {
        debug_flg_bak[0] |= 0x100;
    }
    memcpy(TOOL_FLAG_PTR(OFS_DEBUG_FLG), debug_flg_bak, sizeof(debug_flg_bak));
    memcpy(TOOL_FLAG_PTR(OFS_STATUS_FLG), status_flg_bak, sizeof(status_flg_bak));
    TOOL_FLAG(OFS_DEBUG_FLG) &= 0x7FFFFFFF;
    TOOL_FLAG(OFS_DEBUG_FLG + 12) |= 0x2000;
}

// Moves a 2D cursor with the analog stick (scaled by `speed`) and the digital pad (by `step`).
void TutilMoveCursor(Vec* pos, f32 speed, f32 step)
{
    pos->x += (f32) Joy[0].sx * speed * 0.0078125f;
    pos->y -= (f32) Joy[0].sy * speed * 0.0078125f;
    pos->x += (Joy[0].on & JOY_RIGHT) ? step : ((Joy[0].on & JOY_LEFT) ? -step : 0.0f);
    pos->y += (Joy[0].on & JOY_DOWN) ? step : ((Joy[0].on & JOY_UP) ? -step : 0.0f);
}

// Never called in the tool modules. GCC 2.95 emits the initializer templates of local aggregates in
// inline functions at parse time; the original object has these 5 words between TutilMoveCursor's
// constant pool and ToolMenuDisp_cur's strings. Values are the original's, grouping and body a guess.
static inline void tutil_2d_env(f32* scale, Vec* size)
{
    f32 sc[2] = {0.5f, 0.0f};
    Vec sz = {1024.0f, 2048.0f, 0.0f};

    scale[0] = sc[0];
    scale[1] = sc[1];
    *size = sz;
}

TOOL_MENU* old_menu = NULL;
static int old_num = 0;  // unreferenced zero word between old_menu and the menu statics (name unknown)

// The menu drawer of the DOL's t_util (game/t_util.cpp ToolMenuDisp_cur), an inline here that nothing
// calls: only its "%s" / ">" strings and the two statics are in the object (t_util.h declares the
// out-of-line DOL function, hence the other name).
static inline int tutil_menu_disp(int x, int y, int flag, s8* cursor, TOOL_MENU* menu, int size, JOY* joy)
{
    static s8 cursor_s = 0;
    static u8 flicker = 4;
    TOOL_MENU* p = menu;
    int num;
    int i;
    int ret;
    int color;

    if (cursor != NULL) {
        cursor_s = *cursor;
    }
    num = size / sizeof(TOOL_MENU);
    if (old_menu != p) {
        if (cursor == NULL) {
            if (flag & TOOL_MENU_START_LAST) {
                cursor_s = num - 1;
            } else {
                cursor_s = 0;
            }
        }
        old_menu = p;
    }
    if (joy->rep & JOY_DOWN) {
        cursor_s++;
    }
    if (joy->rep & JOY_UP) {
        cursor_s--;
    }
    cursor_s = cursor_s < 0 ? num - 1 : (cursor_s > num - 1 ? 0 : cursor_s);
    if (joy->rep & (JOY_DOWN | JOY_UP)) {
        flicker = 8;
    }
    if ((joy->trg & JOY_B) && (flag & TOOL_MENU_B_LAST)) {
        cursor_s = num - 1;
        flicker = 8;
    }
    for (i = 0; i < num; i++) {
        color = 0x14;
        if (p->enable) {
            color = 0;
        }
        eprintf(x, y + i * 16, color, 0, "%s", p->name);
        p++;
    }
    if (flicker & 0x18) {
        eprintf(x - 8, y + cursor_s * 16, 0, 0, ">");
    }
    flicker++;
    if (cursor != NULL) {
        *cursor = cursor_s;
    }
    p = &menu[cursor_s];
    if ((joy->trg & JOY_A) && p->enable) {
        if (p->func != NULL) {
            p->func();
        }
        ret = cursor_s;
        cursor_s = 0;
        return ret;
    }
    return -1;
}

// Screen position of a world point (GXProject with the current camera); `noSetup` skips TprimDraw3D.
int TutilGetScreenPos(Vec* pos, f32* scr, int noSetup)
{
    f32 proj[7];
    f32 viewport[6];

    if (noSetup == 0) {
        TprimDraw3D(0);
    }
    GXGetProjectionv(proj);
    GXGetViewportv(viewport);
    GXProject(pos->x, pos->y, pos->z, pG->Cam.viewMat, proj, viewport, &scr[0], &scr[1], &scr[2]);
    return 1;
}
