#include "types.h"
#include "main_mem.h"
#include "st_room.h"
#include "flag_rsf.h"

// Room 3-28 (D:/Bio4/Prog/r328.cpp): work allocation only.

struct R328Work {
    u8 dummy;
};

static R328Work* r328_work;

void R328Init()
{
#line 26 "D:/Bio4/Prog/r328.cpp"
    r328_work = (R328Work*) MEM_CALLOC(sizeof(R328Work), 1, 0xd);
}

void R328Main()
{
}
