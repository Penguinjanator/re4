#ifndef ITEM_H
#define ITEM_H

#include "types.h"

// One inventory slot (cItemMgr::pItems[], 0xE bytes).
struct ItemWork {
    u16 id;        // 0x00  item id
    u16 num;       // 0x02  count / bullets
    u8 flags;      // 0x04  bit0 in use
    u8 type;       // 0x05  inventory type (cItemMgr::type selects the visible set)
    u8 pad_6[2];
    u16 x8;        // 0x08  top 3 bits: weapon slot attribute (sscrn: pG->wep_x4FB2)
    u8 pad_A[4];
};

// itemInfo() result (game/item.cpp).
struct ItemInfo {
    u8 x0;
    u8 x1;
    u8 type;       // 0x02  3 = weapon with a magazine (sscrn: empty check)
    u8 x3;
    u16 x4;
};

// Inventory manager (game/item.cpp, 0x30 bytes). Layout partially known; only the entry points
// other units use are declared.
class cItemMgr {
public:
    u8 pad_0[0xC];
    ItemWork* pArm;             // 0x0C  equipped weapon slot (NULL = bare hands)
    u16 armId;                  // 0x10  equipped weapon item id
    u8 pad_12;
    u8 type;                    // 0x13  inventory type (num(id) / search count only this type)
    ItemWork* pItems;           // 0x14
    u8 pad_18[4];
    s32 nItems;                 // 0x1C
    u8 pad_20[0x30 - 0x20];

    int num(int id);            // 0x8001EB54: count of item `id` of this->type
    int num(int id, u8 type);   // 0x8001EAE4: count of item `id` of the given type (pl_sub: num(0xFE, 0))
    u16 bulletNum();            // 0x8001FC20: bulletNumCurrent() of the equipped weapon
    u32 bulletNumCurrent();     // 0x8001FC40
    void dump(int id);          // 0x8001E970: drop item `id`
    int get(int id, int num);
    void debugWeapon(int id);
    ItemWork* search(u16 id);   // 0x8001DED0: the in-use slot of this->type holding `id`, NULL if none
    void arm(ItemWork* p);      // 0x8001F350: equip `p` (NULL: bare hands)
    // equipped weapon (this->xC), objWep: reloadable(x, 0) / reload(x, 0) / trigger(x)
    int reloadable();           // 0x8001F470
    int reload();               // 0x8001F5B4
    int trigger();              // 0x8001F7E8
};

extern cItemMgr ItemMgr;

// weapon number/type -> item id (0xFFFF when unknown)
u16 WeaponNo2WeaponId(u8 no, u8 type);

extern "C" {
// item id -> weapon number / type (0xFF when unknown), item attributes
u8 WeaponId2WeaponNo(u16 id);
u8 WeaponId2WeaponType(u16 id);
void itemInfo(u16 id, ItemInfo* info);
}

#endif
