#ifndef SSCRN_H
#define SSCRN_H

#include "types.h"
#include "vec.h"
#include "camera.h"
#include "main_sub.h"

// Sub screen (inventory / map / files / puzzle) front end, game/sscrn.cpp. The screen itself is
// the Sscrn.rel DLL, linked into the ARAM-swapped area while it is open.
class cObjWep;

// Sub screen data archive (ss_cmmn.dat / ss_pzzl.dat): a table of byte offsets to its sub-files.
struct SsArc {
    u32 ofs[0x12];
};
#define SS_ARC_PTR(arc, no) ((void*) ((arc)->ofs[no] + (u32) (arc)))

struct SubScreenWork {
    char path[0x28];          // 0x000  "SS/<lang>/<file>" (sscrnSetLanguage / sscrnDataFilename)
    u8 x28;                   // 0x028
    u8 pad_29[3];
    s32 type;                 // 0x02C  open type: 1 inventory, 2, 0x10, 0x20 puzzle, 0x40, 0x80
    s32 flags;                // 0x030  bit0 event, bit1 (flags_5010 bit21 at open), bit3 no sound
    s32 x34;                  // 0x034
    s32 x38;
    s32 wait;                 // 0x03C  frames left before SubScreenCall may open (SubScreenWait)
    s32 x40;
    s32 x44;
    s32 x48;
    s32 x4C;
    u32 save170;              // 0x050  pG->flags_170 while open
    u32 save58;               // 0x054  pG->flags_58 while open
    Camera cam;               // 0x058  pG->Cam while open
    Mtx plMat;                // 0x150  player matrix at open
    Mtx subMat;               // 0x180  partner matrix at open
    u8 stage;                 // 0x1B0  sscrnStageNo()
    u8 pad_1B1;
    u16 room;                 // 0x1B2  sscrnRoomNo()
    u8 scope;                 // 0x1B4  1: the scope was up, 2: and flags_5010 bit 26
    u8 bino;                  // 0x1B5  the binocular was up
    u8 x1B6;                  // 0x1B6  flags_5010 bit 28 (always 0: the mask is stored as a byte)
    u8 noBullet;              // 0x1B7  the equipped weapon (type 3) was empty
    u8 x1B8;                  // 0x1B8  item 0xFE owned
    u8 pad_1B9[3];
    s32 healing;              // 0x1BC  SubCharCheckHealing()
    void* pBuf;               // 0x1C0  MRAM area swapped with the ARAM copy (pG->pStageFont)
    u32 aramSize;             // 0x1C4  bytes read to ARAM (SubScreenAramRead)
    u32 heapOfs;              // 0x1C8  heap 12 starts at pBuf + heapOfs
    u32 relOfs;               // 0x1CC  Sscrn.rel offset in the area
    u32 cmmnOfs;              // 0x1D0  ss_cmmn.dat offset
    u32 pzzlOfs;              // 0x1D4  ss_pzzl.dat offset
    s32 relAddr;              // 0x1D8  Sscrn.rel address (0 while unlinked)
    SsArc* pCmmn;             // 0x1DC
    SsArc* pPzzl;             // 0x1E0
    u8 pad_1E4[0x200 - 0x1E4];
    SsArc* pExam;             // 0x200  item examine id data archive (examine ItemExamine::idSet)
    u8 pad_204[0x214 - 0x204];
    void* binoA;              // 0x214  CameraControl::GetBinocularIDAddr
    void* binoB;              // 0x218
    u32 x21C[8];              // 0x21C
    void* x23C;               // 0x23C  0x3E800-byte buffer
    u8 pad_240[0x264 - 0x240];
    u8 x264;                  // 0x264  2 for type 2, else 1
    u8 x265;                  // 0x265
    u8 pad_266[3];
    u8 x269;                  // 0x269
    u16 x26A;                 // 0x26A
    u8 pad_26C[0x2AE - 0x26C];
    u8 x2AE;                  // 0x2AE  item 0x7C..0x7F owned -> 0..3
    u8 x2AF;                  // 0x2AF
    u8 pad_2B0[0x2FA - 0x2B0];
    u16 x2FA;                 // 0x2FA  item id handed to the opened sub screen (sce_at sceAtGetItem)
    u16 x2FC;                 // 0x2FC  its count
    u8 pad_2FE[0x31C - 0x2FE];
    s32 mdtNo;                // 0x31C  OpeSetOpenTerm number
    s32 strBlk;               // 0x320  SndStrPlayBlock handle
    cObjWep* pObj;            // 0x324  OpeSetOpenTerm weapon object
    Vec savePos;              // 0x328  player position before OpeSetOpenTerm
    Vec saveRot;              // 0x334
    s32 cancel;               // 0x340  OpeSetOpenTermCancel
    OSModuleHeader* pModule;  // 0x344  linked sub screen DLL
    union {
        u32 save;             // 0x348  SscrnDataSave/Load word
        struct {
            u8 x348;          // 0x348  (SubScreenGameInit clears it)
            u8 x349;
            u8 x34A;
            u8 x34B;
        };
    };
    u8 pad_34C[4];
    s32 debugMode;            // 0x350  pG->debug_mode while open
    s32 x354;                 // 0x354  pG->flags_68 bit 30 while open
    u8 pad_358[0x366 - 0x358];
    u8 x366;                  // 0x366
    u8 pad_367[0x374 - 0x367];
};

extern SubScreenWork SubScreenWk;

extern "C" {
int SscrnDataSize();
void SscrnDataSave(u32* dst);
void SscrnDataLoad(u32* src);
void SubScreenAramRead();
void sscrnSetLanguage(SubScreenWork* wk, int lang);
void sscrnDataFilename(SubScreenWork* wk, const char* name);
void SubScreenGameInit();
void SubScreenRoomInit();
void SubScreenWait(int frames);
void SubScreenCall();
int sscrnStageNo();
u16 sscrnRoomNo(u16 room);
int SubScreenOpen(int type, int flags);
void SubScreenMiss();
void SubScreenExec();
void SubScreenExitCore(SubScreenWork* wk);
void SubScreenExit();
int OpeGetMdtNo();
void OpeSetMdtNo(u32 no);
int OpeMdtSetInit();
void OpeOwTypeSet(u8 type);
void OpeSetOpenTerm(int no, f32 x, f32 y, f32 z, f32 ang);
void OpeSetOpenTermCancel();
void OpeSetOpenTermEnd();
}

#endif
