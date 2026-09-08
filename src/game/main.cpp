// game/main: entry point, frame loop, system init and reset handling (D:/Bio4/Prog/main.cpp).
#include "types.h"
#include "global.h"
#include "joy.h"

// .sbss objects in the order the original lays them out (uninitialised globals are emitted in
// first-declaration order, header externs included: main.h's pRK must come after these).
void* pFrame_buff[2];
void* pCurrent_buff;
f32 ZNEAR;
f32 ZFAR;
f32 ORTHO_T;
f32 ORTHO_B;
f32 ORTHO_L;
f32 ORTHO_R;

#include "main.h"
#include "main_sub.h"
#include "main_mem.h"
#include "view.h"
#include "atari.h"
#include "light.h"
#include "ctrl.h"
#include "map_obj.h"
#include "widget.h"
#include "card.h"
#include "event.h"
#include "sofdec.h"
#include "dvd.h"
#include "mes.h"
#include "id_sys.h"
#include "block.h"
#include "datactrl.h"
#include "em.h"
#include "obj.h"
#include "cockpit.h"
#include "room_data.h"
#include "db_log.h"
#include "scheduler.h"
#include "fade.h"
#include "cinesco.h"
#include "libgpu.h"
#include "trans_ot.h"
#include "pad.h"
#include "file.h"
#include "snd.h"
#include "dbmodule.h"
#include "cloth.h"
#include "filter.h"
#include "room_tex.h"
#include "TexRender.h"
#include "camera.h"
#include "gx_sub.h"
#include "gx.h"
#include "os_vi.h"
#include "rnd.h"
#include "sscrn.h"

extern "C" {
void __main();
void OSReport(const char* fmt, ...);
void OSInit();
u32 OSGetConsoleType();
void OSInitAlarm();
void VIInit();
void VIWaitForRetrace();
void VISetPostRetraceCallback(void (*cb)());
void LCEnable();
void PPCSync();
int DBIsDebuggerPresent();
u32 OSGetResetButtonState();
void PADRecalibrate(u32 mask);
void OSResetSystem(int reset, u32 resetCode, int forceMenu);
void GXCopyDisp(void* dest, u8 clear);
// game/title.cpp
void Title_task();
// game/debug.cpp
void ProcessTickInit();
void ProcessTickGet(int no, const char* name);
void DebugControl();
void ConfigSet();
// game/trans.cpp
void Render();
void SetPrimBuffPtr();
void Trans();
// game/eprintf.cpp
void EprintfInit();
void EprintfFlush();
// game/trans_lit.cpp
void LightSetInit();
// game/id_tex.cpp
void IdTexGameInit();
// game/eff_sys.cpp
void EspInit();
// game/shadow.cpp
void ShadowInit();
// game/item_model.cpp
void ItemModelInit();
// game/EtcModel.cpp
void EtcModelInit();
// game/card.cpp
void CardInit();
void CardDbgCacheSet();
// game/read.cpp
void EmReadInit();
void ReleasePlData();
void ReleaseWepData();
// game/exception.cpp
void ExceptionInit();
// game/espgen.cpp
int EspgenInit();
}
void AllocDrawTmpBuf();   // game/TmpBuf.cpp
void DbmenuModuleInit();  // game/db_menu.cpp

#define HALT()                                                    \
    do {                                                          \
        OSReport("HALT %s(%d)\n", __FILE__, __LINE__);            \
        *(volatile u32*) 0x11111111 = 0;                          \
    } while (0)

#line 40 "D:/Bio4/Prog/main.cpp"

const Vec vecZero = {0.0f, 0.0f, 0.0f};

GlobalWork Global;
SystemSaveWork SystemSave;
JOY Joy[4];
KeyWork Key;
u32 MainOt[5];
ScreenInfo Screen;

GlobalWork* pG = &Global;
SystemWork* pSys = (SystemWork*) &SystemSave;
int vsync_cnt = 0;

RK* pRK;
char* pUser_name;
void* roomInfoAddr;
s8 system_vcnt;

static void systemScreenInit();

int main()
{
    int i;

    systemStartInit();
    for (;;) {
        systemRestartInit();
        if (pRK->valid) {
            pSys->flags = pRK->sys_flags;
            pSys->language = pRK->language;
            pSys->region = pRK->region;
            pG->x4F93 = pRK->x16;
            pSys->x4 = pRK->sys_x4;
            for (i = 0; i < 4; i++) {
                pSys->x10[i] = pRK->sys_x10[i];
            }
            for (i = 0; i < 2; i++) {
                pSys->x20[i] = pRK->sys_x20[i];
            }
            if ((s32) pRK->g_flags_54 < 0) {
                pG->flags_54 |= 0x80000000;
            }
            if (pRK->g_flags_54 & 0x40000000) {
                pG->flags_54 |= 0x40000000;
            }
        }
        if (pG->flags_54 & 0x8) {
            pG->room_id = 0x120;
            pG->x4F9F = 0;
            pSys->language = 1;
            pG->debug_mode = 0;
            pG->x4FB8 = 0;
            pG->flags_6C &= ~0x2000;
        }
        TaskExec(0, Title_task, 0);
        for (;;) {
            StopwatchInit();
            ProcessTickInit();
            Render_before();
            ClearOTagR(MainOt, 5);
            PadRead();
            DebugControl();
            Render();
            ProcessTickGet(4, "RENDER SETUP");
            SetPrimBuffPtr();
            ClearOt();
            pG->flags_51E4++;
            TaskScheduler();
            ProcessTickGet(5, "TaskScheduler");
            if (!(pG->flags_54 & 0x100000) || !(pG->flags_500C & 0x40000)) {
                IdSys.move();
            }
            if (!(pG->flags_54 & 0x100000) || !(pG->flags_500C & 0x40000)) {
                IdSys.trans();
            }
            if (!(pG->flags_54 & 0x100000)) {
                Trans();
            }
            Dvd.Watcher();
            SndWatcher();
            ProcessTickGet(5, "SndWatcher");
            FadeControl(0);
            cMes.Move();
            cMes.Trans();
            CinescoMove();
            if (!(pG->flags_60 & 0x8000)) {
                Draw_cinesco();
            }
            FadeControl(1);
            DrawOTag(&MainOt[4]);
            if (pG->debug_mode != 7) {
                pLog->disp();
            }
            EprintfFlush();
            ProcessTickGet(2, "PROCESS CPU");
            iTaskScheduler();
            Render_done();
            Block.checkCommand();
            DC.check();
            Block.checkCondition();
            ProcessTickGet(3, "DRAW REMAIN");
            while (vsync_cnt < GetSystemVcnt() - 1) {}
            Render_swap();
            PPCSync();
            while (vsync_cnt < GetSystemVcnt()) {}
            vsync_cnt = 0;
            systemVSyncPost();
            pG->flags_54 &= ~0x10000000;
            ProcessTickGet(0, "PROCESS TOTAL");
            if (systemResetCheck() == 1) {
                break;
            }
        }
    }
    OSPanic(__FILE__, __LINE__, "End of biohazard4");
    return 0;
}

void systemVSyncPost()
{
}

void postVSyncCallback()
{
    vsync_cnt++;
    if (vsync_cnt >= GetSystemVcnt()) {
        iTaskSuspend();
    }
    if (!(pG->flags_54 & 0x20000000)) {
        haltExecCheck();
    }
}

void haltExecCheck()
{
    int dbg = DBIsDebuggerPresent();
    if (dbg == 0 && vsync_cnt > 3599) {
#line 548 "D:/Bio4/Prog/main.cpp"
        HALT();
    }
}

void systemStartInit()
{
    memclr_asm(pG, sizeof(GlobalWork));
    OSInit();
    setLanguage();
    RomFontSetting();
    if (OSGetConsoleType() & 0xF0000000) {
        pG->dev_mode = 1;
    }
    if (pG->dev_mode == 1) {
        InitFile();
        if (file_path("d:\\bio4") == 0) {
            file_path("c:/");
        }
    }
    ExceptionInit();
    OSInitAlarm();
    VIInit();
    systemVISetBlack(1);
    LCEnable();
    VIWaitForRetrace();
    VISetPostRetraceCallback(postVSyncCallback);
    Dvd.Init();
    CardInit();
    asm("li 3, 4\n"
        "oris 3, 3, 4\n"
        "mtspr 914, 3\n"
        "li 3, 5\n"
        "oris 3, 3, 5\n"
        "mtspr 915, 3\n"
        "li 3, 6\n"
        "oris 3, 3, 6\n"
        "mtspr 916, 3\n"
        "li 3, 7\n"
        "oris 3, 3, 7\n"
        "mtspr 917, 3"
        :
        :
        : "r3");
    SystemMemInit();
    if (pG->dev_mode == 1) {
        CardDbgCacheSet();
    }
    TaskSchedulerInit();
    systemScreenInit();
    Render_init();
    LightSetInit();
    RndInit(0xD37);
    InitOt();
    SndInit();
    SofdecInit();
    IdSys.gameInit(0x80);
    IdTexGameInit();
    EprintfInit();
    LogInit();
    init_dbmodule();
    Dvd.SizeTableRead();
    cMes.init();
}

void systemRestartInit()
{
    int i;
    int ret;

    PadInit();
    memclr_asm(&pG->x20, sizeof(GlobalWork) - 0x20);
    MemReplaceHeap(0, 1);
    MemSetCurrentHeap(1);
    systemWorkInit();
    InitOt();
    FadeInit();
    CinescoInit();
    IdSys.roomInit();
    for (i = 0; i < 16; i++) {
        cMes.Delete(i);
    }
    DC.init();
    AllocDrawTmpBuf();
    EmMgr.roomInit();
    ObjMgr.roomInit();
    LightMgr.init(0);
    EspInit();
    RoomTexInit();
    EspgenInit();
    CtrlMgr.roomInit();
    ShadowInit();
    FilterInit();
    ClothInit();
    Cckpt.gameInit();
    RoomData.init();
    TexRenderMgrInit();
    ItemModelInit();
    EtcModelInit();
    EvtMgr.init();
    CameraGameInit();
    View.gameInit(&pG->Cam);
    SndInit2();
    bio4_GXSetCopyClear(g_sysBgColor, 0xFFFFFF);
    GXCopyDisp(pCurrent_buff, 1);
    ConfigSet();
    if (DBIsDebuggerPresent() == 0) {
        BitOff(pG->flags_54, 0x20000);
        BitOff(pG->flags_54, 0x10000);
    }
    if (pRK->brightness == 0) {
        pRK->brightness = 0x40;
    }
    pSys->brightness = pRK->brightness;
#line 752 "D:/Bio4/Prog/main.cpp"
    ret = DvdReadN("debug/roomInfo.dat", 0, 0, 0, 0, 5, __FILE__, __LINE__);
    if (Dvd.ReadCheck(ret, 0, 0, &roomInfoAddr) < 0) {
        roomInfoAddr = 0;
    }
    pLog->modeSet(200, 0, 150, 4);
    SelfScreenShotInit();
}

static void systemScreenInit()
{
    ZNEAR = 100.0f;
    ZFAR = 1000000.0f;
    ORTHO_T = 240.0f;
    ORTHO_B = -240.0f;
    ORTHO_L = -320.0f;
    ORTHO_R = 320.0f;
    SetSystemVcnt(2);
    Screen.x = 0.0f;
    Screen.y = 0.0f;
    Screen.width = (f32) Rmode.fbWidth;
    Screen.height = (f32) Rmode.efbHeight;
    pSys->brightness = 0x40;
    OSReport("width = %d\n", Rmode.fbWidth);
    OSReport("height = %d\n", Rmode.efbHeight);
}

void systemWorkInit()
{
    systemScreenInit();
    pG->debug_mode = 1;
    pG->debug_disp = -1;
    pG->room_id = 0x120;
    pG->next_room = pG->room_id;
    pG->x4FB8 = 0;
    pG->x8354 = 5;
    pG->costume2 = 0;
    pG->costume = 0;
#line 823 "D:/Bio4/Prog/main.cpp"
    pUser_name = (char*) mem_calloc(0x40, __FILE__, __LINE__, 1, 13);
    pSys->language = 1;
    pSys->region = 1;
    pG->x4F93 = 1;
}

void SetSystemVcnt(int vcnt)
{
    if (vcnt > 0) {
        system_vcnt = vcnt;
    } else {
        system_vcnt = 1;
    }
}

int GetSystemVcnt()
{
    return system_vcnt;
}

int checkHardReset()
{
    static u8 reset_check = 0;

    if (reset_check == 0) {
        if (OSGetResetButtonState() == 1) {
            reset_check = 1;
        }
    } else {
        if (OSGetResetButtonState() == 0) {
            reset_check = 0;
            pG->flags_54 |= 0x8000;
        }
    }
    if ((pG->flags_54 & 0x8000) && !(pG->flags_54 & 0x200)) {
        systemHardReset();
        PADRecalibrate(0xF0000000);
        OSResetSystem(0, 0, 0);
        return 1;
    }
    return 0;
}

int systemResetCheck()
{
    static u8 Soft_reset_cnt = 0;

    if ((Joy[0].on & 0x1600) == 0x1600) {
        Soft_reset_cnt += GetSystemVcnt();
        if (Soft_reset_cnt > 30) {
            if (pG->dev_mode == 1) {
                pG->flags_54 |= 0x4000000;
            } else {
                pG->flags_54 |= 0x8000;
            }
        }
    } else {
        Soft_reset_cnt = 0;
    }
    checkHardReset();
    if (!(pG->flags_54 & 0x8000) && (pG->flags_54 & 0x4000000) && !(pG->flags_54 & 0x200)) {
        systemSoftReset();
        return 1;
    }
    return 0;
}

void systemResetCommon()
{
    int i;

    SubScreenExitCore(&SubScreenWk);
    TaskAllClear();
    EmReadInit();
    ReleasePlData();
    ReleaseWepData();
    RoomData.stopRelData();
    pRK->sys_flags = pSys->flags;
    pRK->language = pSys->language;
    pRK->region = pSys->region;
    pRK->x16 = pG->x4F93;
    pRK->sys_x4 = pSys->x4;
    pRK->g_flags_54 = pG->flags_54;
    for (i = 0; i < 4; i++) {
        pRK->sys_x10[i] = pSys->x10[i];
    }
    for (i = 0; i < 2; i++) {
        pRK->sys_x20[i] = pSys->x20[i];
    }
    pRK->x3C = pG->x8 >> 31;
    pRK->valid = 1;
}

void systemHardReset()
{
    OSReport("--HARD_RESET START!!\n");
    systemVISetBlack(1);
    VIFlush();
    VIWaitForRetrace();
    systemResetCommon();
    OSReport("--HARD_RESET END!!\n");
}

void systemSoftReset()
{
    OSReport("--SOFT_RESET START!!\n");
    systemVISetBlack(1);
    VIFlush();
    VIWaitForRetrace();
    GXDrawDone();
    Dvd.ReadCancelAll();
    Aram.DmaCancelAll();
    SndSoftReset();
    DbmenuModuleInit();
    ResetDebugAlloc();
    MemClearAllHeap();
    SndSystemReset();
    systemResetCommon();
    RomFontSetting();
    pG->x1C = 0;
    OSReport("--SOFT_RESET END!!\n");
}

void setLanguage()
{
    pSys->language = 1;
    pSys->region = pSys->language;
    pG->x4F93 = pSys->language;
}
