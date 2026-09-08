#ifndef ITEM_H
#define ITEM_H

#include "types.h"

// Inventory manager (game/item.cpp, 0x30 bytes). Layout opaque; only the entry points the
// debug tools use are declared.
class cItemMgr {
public:
    u8 pad_0[0x30];

    int num(int id);            // 0x8001EB54: count of item `id` of this->x13 type
    int num(int id, u8 type);   // 0x8001EAE4: count of item `id` of the given type (pl_sub: num(0xFE, 0))
    int bulletNum();            // 0x8001FC20: bulletNumCurrent() of the equipped weapon
    u32 bulletNumCurrent();     // 0x8001FC40
    void dump(int id);          // 0x8001E970: drop item `id`
    int get(int id, int num);
    void debugWeapon(int id);
    // equipped weapon (this->xC), objWep: reloadable(x, 0) / reload(x, 0) / trigger(x)
    int reloadable();           // 0x8001F470
    int reload();               // 0x8001F5B4
    int trigger();              // 0x8001F7E8
};

extern cItemMgr ItemMgr;

// weapon number/type -> item id (0xFFFF when unknown)
u16 WeaponNo2WeaponId(u8 no, u8 type);

#endif
