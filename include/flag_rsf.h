#ifndef FLAG_RSF_H
#define FLAG_RSF_H

#include "types.h"
#include "room_data.h"

// Room save flags (the original flag_rsf.h): bit `no` of the words after the room save record's
// header. Every unit that includes it carries the "HALT %s(%d)\n" / "D:/Bio4/Prog/flag_rsf.h"
// strings of the never-called setter (objRobo, sce_com, sce_at).
extern "C" void OSReport(const char* fmt, ...);

static inline u32* RsfFlags(u16 room)
{
    return (u32*) (RoomData.getRoomSavePtr(room) + 4);
}

// The masked word itself (objRobo R0Init tests it directly: `andis.; beq`); `!= 0` would give
// the `li 1 / li 0 / cmpwi` flag chain.
static inline u32 RsfCheck(u16 room, int no)
{
    return RsfFlags(room)[no >> 5] & (0x80000000 >> (no & 31));
}

static inline void RsfSet(u16 room, int no)
{
    u8* p = RoomData.getRoomSavePtr(room);

    if (p == 0) {
#line 30 "D:/Bio4/Prog/flag_rsf.h"
        OSReport("HALT %s(%d)\n", __FILE__, __LINE__);
        *(volatile u32*) 0x11111111 = 0;
    }
    ((u32*) (p + 4))[no >> 5] |= 0x80000000 >> (no & 31);
}

#endif
