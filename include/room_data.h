#ifndef ROOM_DATA_H
#define ROOM_DATA_H

#include "types.h"

// Per-room save data (game/room_data.cpp). Layout not yet established (0x20 bytes).
class cRoomData {
public:
    u8 pad_0[0x18];
    u8* pSave;   // 0x18  room save records, 0xD8 bytes each
    u8 pad_1C[4];

    void init();
    void initRoomSet();
    void save();
    void load();
    void clear();
    // record for room `room` (stage << 8 | room_no), or NULL when the room has none
    u8* getRoomSavePtr(u16 room);
    void execInitFunc();
};

extern cRoomData RoomData;

#endif
