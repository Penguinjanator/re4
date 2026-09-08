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

// EtcModel.cpp is C++ but exports its functions with C linkage (unmangled names in the DOL).
extern "C" {
u16* GetEtcFlgPtr(int room, int no);
int getRoomEtcItem(int room, EtcItem** out, int a);
// Model a light of parent type 3 (room etc model) hangs on; 1 = found (light.cpp)
int getRoomEtcOnLight(u32 id, class cModel** out, int flag);
}

#endif
