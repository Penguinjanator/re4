#include "types.h"
#include "main_mem.h"
#include "st_room.h"
#include "atari.h"
#include "flag_rsf.h"
#include "global.h"
#include "sce.h"
#include "sce_sys.h"

// Room 3-12 (D:/Bio4/Prog/r312.cpp): one treasure chest.

struct R312Work {
    u8 dummy;
};

static R312Work* r312_work;

static void OpenBoxTreasure(int id);
static void OpenedBoxTreasure(int id);

void R312Init()
{
#line 44 "D:/Bio4/Prog/r312.cpp"
    r312_work = (R312Work*) MEM_CALLOC(sizeof(R312Work), 1, 0xd);
    SceSetItemEvent(6, 0x82, 2, 5, OpenBoxTreasure, (void (*)()) OpenedBoxTreasure, 0x82, 0);
}

void R312Main()
{
}

static void OpenedBoxTreasure(int id)
{
    if (id == 0x82) {
        OpenBoxMain(1, 1, 0x1c, 0x38, -1, -1);
    }
}

static void OpenBoxTreasure(int id)
{
    if (id == 0x82) {
        OpenBoxMain(1, 0, 0x1c, 0x38, -1, -1);
    }
}
