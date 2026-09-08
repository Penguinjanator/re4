#ifndef ESPGEN_H
#define ESPGEN_H

#include "types.h"
#include "vec.h"
#include "model.h"
#include "esp.h"

// Optional 0x1C byte parameter block handed down the sequence calls (copied into the generator work).
struct EspSeqOpt {
    u32 x0;            // 0x00
    u32 x4;            // 0x04
    u32 x8;            // 0x08
    u32 xC;            // 0x0C
    u32 x10;           // 0x10
    u32 x14;           // 0x14
    u32 x18;           // 0x18
};

// Effect system work (game/eff_sys.cpp cEspSystem, g_pEspSys). Partial layout.
// Room effect (sst) table entry: an effect list and the offsets of its EspSeqData blocks (game/est.cpp SstSet).
struct SstList {
    u32 num;           // 0x00
    struct {
        union {
            u16 no;    // 0x00 room number range key
            struct {
                u8 x0;
                u8 id; // 0x01 display flag bit (GetSstDispFlag)
            } b;
        };
        u16 type;      // 0x02
        u32 x4;        // 0x04
    } ent[1];          // 0x04, 8 bytes each
};
struct SstData {
    u32 x0;            // 0x00
    u32 ofs[1];        // 0x04 byte offsets of the EspSeqData blocks from this header
};
struct SstTbl {
    SstData* data;     // 0x00
    SstList* list;     // 0x04
    int owner;         // 0x08 0xD2 = unused entry
};

// Area list for the room effect display flags (game/est.cpp AreaSstSet): 0x10 header, 0x98 byte entries.
struct SstAreaEnt {
    u8 x0;
    u8 x1;
    u8 bit;            // 0x02 display flag bit set while the player stands in the area
    u8 x3;
    u8 area[0x30];     // 0x04 AreaHitCheck data (AreaData)
    u32 flags;         // 0x34 bit0: the area counts as "in room" (esp_app EffAreaCheckInRoom)
    u8 pad_38[0x98 - 0x38];
};
struct SstArea {
    u32 num;           // 0x00
    u8 pad_4[0x10 - 4];
    SstAreaEnt ent[1]; // 0x10
};

struct cEspSystem {
    u8 pad_0[0x71E4];
    SstTbl sstTbl[0xD3];   // 0x71E4 room effect tables by owner id
    u8 pad_7BC8[0x8680 - 0x7BC8];
    SstArea* pSstArea;     // 0x8680
    u8 pad_8684[0xC548 - 0x8684];
    u32 xC548;         // 0xC548
    u8* pEspBuf;       // 0xC54C esp pool (0x150 bytes per cEsp)
    u8 pad_C550[4];
    u32 xC554;         // 0xC554 number of esp slots
    u8 pad_C558[0xC5A0 - 0xC558];
    u32 sstDispFlag;   // 0xC5A0 room effect display flags (bit per id)
    u32 sstAddAreaFlag;  // 0xC5A4
};
extern cEspSystem* g_pEspSys;

// One effect generator instance (game/espgen.cpp array, stride 0xC8). Bytes 0x14.. are the
// per-generator work (Espgen00Work, Espgen10Work, Espgen44Work, ...).
struct EspgenWork {
    EspInfo info;      // 0x00 owner info (copied from the parent by SetEspCore)
    u8 flag;           // 0x0C bit0: in use, bit1: delete requested
    u8 id;             // 0x0D generator id (index into the Espgen*Tbl tables)
    u8 xE;             // 0x0E
    u8 xF;             // 0x0F
    u8 step;           // 0x10 move step (Espgen*MoveTbl index)
    u8 pad_11[3];
    u8 work[0xC8 - 0x14];  // 0x14
};

// Effect controller 10 work (game/espgen10.cpp): plays an effect sequence (EspSeqData) record by
// record. est.cpp EstSet fills it directly.
struct Espgen10Work {
    EspSeqData* head;  // 0x14
    cModel* model;     // 0x18
    u32 serial;        // 0x1C model serial the controller was set up with
    u16 cnt;           // 0x20 frame counter
    u8 no;             // 0x22 next record
    u8 flags;          // 0x23 bit0: parts matrix fixed, bit1: pass the rotation on
    u16 parts;         // 0x24 parts number (0xFE: free position, 0xFF: none)
    u8 pad_26[2];
    u32 seed;          // 0x28
    Mtx mtx;           // 0x2C
    Vec pos;           // 0x5C
    Vec rot;           // 0x68
    EspSeqOpt opt;     // 0x74 copy of the option block p8 points at
    EspSeqOpt* p8;     // 0x90
};

typedef void (*EspgenMoveFunc)(EspgenWork* w);
typedef void (*EspgenTransFunc)(EspgenWork* w);
typedef int (*EspgenSetFreeWorkFunc)(EspgenWork* w, EspGenWork* rec, EspSeqData* head, cModel* model, u16 parts,
                                     Mtx* mtx, Vec* pos, Vec* rot, EspSeqOpt* p8, int flag);
// the application generators (Espgen4x) take no flag argument
typedef int (*EspgenSetFreeWorkAppFunc)(EspgenWork* w, EspGenWork* rec, EspSeqData* head, cModel* model,
                                        u16 parts, Mtx* mtx, Vec* pos, Vec* rot, EspSeqOpt* p8);
typedef void (*EspgenDestructFunc)(EspgenWork* w);

extern "C" {
// game/espgen.cpp
u32 GetEspgenIdMax();
int PullEspgen(EspgenWork** out);
int PullEspgenFront(EspgenWork** out);
void PushEspgen(EspgenWork* w);
int EspgenSetFreeWork(EspgenWork* w, EspGenWork* rec, EspSeqData* head, cModel* model, u16 parts, Mtx* mtx,
                      Vec* pos, Vec* rot, EspSeqOpt* p8, int flag);
int EspgenSeqSet(EspSeqData* head, int no, EspInfo* info, cModel* model, u16 parts, Mtx* mtx, Vec* pos, Vec* rot,
                 EspSeqOpt* p8, int flag);
void EspgenArrayClear();
void EspgenDelete(int a, int b, int c);
void EspgenDeleteEvent();
int EspgenGetCallNo();
void EspgenIncCallNo();

// game/esp_sub.cpp
int EspSeqSet(EspGenWork* rec, EspInfo* info, u32* seed, cModel* model, Mtx* mtx, int a, cEsp** out, EspSeqOpt* p8,
              Vec* pos, f32 f);

// game/est.cpp
extern cModel* EspEvModList[0x80];

// game/espgen10.cpp
int EspgenDataSet(EspSeqData* head, int no, EspInfo* info, u32* seed, cModel* model, u16 parts, Mtx* mtx, Vec* pos,
                  Vec* rot, EspSeqOpt* p8, int flag);
void SetEspCore(EspgenWork* w, int a, u32 b, u8 c, u32 d, int e);
int PullEspEspgen(EspgenWork** out, int a, int c, u32 b, u32 d, int e, int front);
void Espgen10_Move(EspgenWork* w);

// game/espgen00.cpp
void Espgen00_Move(EspgenWork* w);
int Espgen00_SetFreeWork(EspgenWork* w, EspGenWork* rec, EspSeqData* head, cModel* model, u16 parts, Mtx* mtx,
                         Vec* pos, Vec* rot, EspSeqOpt* p8, int flag);

// game/espgen01.cpp
void Espgen01_Move(EspgenWork* w);
void Espgen01_Trans(EspgenWork* w);
int Espgen01_SetFreeWork(EspgenWork* w, EspGenWork* rec, EspSeqData* head, cModel* model, u16 parts, Mtx* mtx,
                         Vec* pos, Vec* rot, EspSeqOpt* p8, int flag);

// game/espgen02.cpp
void Espgen02_Move(EspgenWork* w);
int Espgen02_SetFreeWork(EspgenWork* w, EspGenWork* rec, EspSeqData* head, cModel* model, u16 parts, Mtx* mtx,
                         Vec* pos, Vec* rot, EspSeqOpt* p8, int flag);

// game/espgen44.cpp
void Espgen44_Move(EspgenWork* w);
void Espgen44_Trans(EspgenWork* w);
void Espgen44_Destruct(EspgenWork* w);
int Espgen44_SetFreeWork(EspgenWork* w, EspGenWork* rec, EspSeqData* head, cModel* model, u16 parts, Mtx* mtx,
                         Vec* pos, Vec* rot, EspSeqOpt* p8);
}

#endif
