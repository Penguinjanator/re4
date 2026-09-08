#ifndef ETC_MODEL_H
#define ETC_MODEL_H

#include "types.h"
#include "vec.h"

// Room item (game/EtcModel.cpp). Only the flag word is known.
struct EtcItem {
    u32 flags;   // 0x00  0x02: taken
    u8 pad_4[0x70 - 0x04];
    Vec pos;     // 0x70
};

// One room etc model record handed to the Et*_init functions (et00.cpp).
struct EtcSetData {
    u8 pad_0[3];
    u8 type;         // 0x03  WindowData row
    u8 pad_4[0x10 - 0x4];
    Vec rot;         // 0x10
    Vec pos;         // 0x1C
};

// EtcModel.cpp is C++ but exports its functions with C linkage (unmangled names in the DOL).
// C++ linkage (sym_map: GetEtcFlgPtr__Fii, getRoomEtcItem__FiPP7EtcItemi)
u16* GetEtcFlgPtr(int no, int room);   // etc flag word of etc model `no` in `room` (stage << 8 | room), 0 when none
int getRoomEtcItem(int room, EtcItem** out, int a);

extern "C" {
void* GetEtcAddr(void* arc, const char* name);   // file `name` inside the room etc archive
// Model a light of parent type 3 (room etc model) hangs on; 1 = found (light.cpp)
int getRoomEtcOnLight(u32 id, class cModel** out, int flag);
}

#endif
