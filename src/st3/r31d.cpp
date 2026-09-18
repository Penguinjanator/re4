#include "types.h"
#include "main_mem.h"
#include "st_room.h"
#include "flag_rsf.h"
#include "global.h"
#include "sce.h"

// Room 3-1d (D:/Bio4/Prog/r31d.cpp): one shelf with an item.

struct R31dWork {
    u8 dummy;
};

static R31dWork* r31d_work;

static void r31d_ShelfOpen(int no);
static void r31d_ShelfOpened();

void R31dInit()
{
#line 29 "D:/Bio4/Prog/r31d.cpp"
    r31d_work = (R31dWork*) MEM_CALLOC(sizeof(R31dWork), 1, 0xd);
    SceSetItemEvent(7, 0x85, 0, 2, r31d_ShelfOpen, r31d_ShelfOpened, 0x35, 0);
}

void R31dMain()
{
}

static void r31d_ShelfOpen(int no)
{
    OpenBoxMain(0, 0, 0x1A, 0x35, 0x36, -1);
}

static void r31d_ShelfOpened()
{
    OpenBoxMain(0, 1, 0x1A, 0x35, 0x36, -1);
}
