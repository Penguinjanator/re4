#ifndef ITEM_H
#define ITEM_H

#include "types.h"

// One inventory slot (cItemMgr::pItems[], 0xE bytes).
struct ItemWork {
    u16 id;        // 0x00  item id
    u16 num;       // 0x02  count / bullets
    u8 flags;      // 0x04  bit0 in use
    u8 type;       // 0x05  inventory type (cItemMgr::type selects the visible set)
    u16 x6;        // 0x06  weapon tune levels, one nibble each: fire << 12 | mag << 8 | speed << 4 | ex (merchant)
    u16 x8;        // 0x08  top 3 bits: weapon slot attribute (sscrn: pG->wep_x4FB2), low 13: bullets loaded
    s8 x;          // 0x0A  case position (cells * 2) and orientation (puzzle pzlPlayer::save)
    s8 y;          // 0x0B
    s8 orient;     // 0x0C
    u8 board;      // 0x0D  1 = in the case, 0 = on the spare board
};

// cItemMgr::ordering() output (cItemMgr::pOrder[], 8 bytes): the in-use slots holding one item id.
struct ItemOrder {
    ItemWork* item;  // 0x00
    u16 num;         // 0x04  copy of item->num
    u8 pad_6[2];
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
    u8 x12;                     // 0x12  (sce_at clears it before use())
    u8 type;                    // 0x13  inventory type (num(id) / search count only this type)
    ItemWork* pItems;           // 0x14
    ItemWork* pLast;            // 0x18  slot the last get() filled (puzzle PutInCase copies the piece position into it)
    s32 nItems;                 // 0x1C
    ItemOrder* pOrder;          // 0x20  ordering() result (merchant: sorted slots of one item id)
    s32 nOrder;                 // 0x24  entries in pOrder
    u32 x28;                    // 0x28  (sce_at: number shown with item 0x73)
    u32 x2C;                    // 0x2C  (sce_at: number shown with item 0x75)

    void init();                // 0x8001D3FC: title: allocate/clear the inventory
    ItemWork* at(int no);       // 0x8001DB5C: slot `no` of pItems, NULL when no >= nItems
    int searchAt(ItemWork* p);  // 0x8001DB80: slot index of `p`, -1 if not in pItems
    void ordering(u16 id);      // 0x8001DFD0: collect the in-use slots holding `id` into pOrder (qsort by order_cmp)
    int num(int id);            // 0x8001EB54: count of item `id` of this->type
    int num(int id, u8 type);   // 0x8001EAE4: count of item `id` of the given type (pl_sub: num(0xFE, 0))
    u16 bulletNum();            // 0x8001FC20: bulletNumCurrent() of the equipped weapon
    u32 bulletNumCurrent();     // 0x8001FC40
    void dump(int id);          // 0x8001E970: drop item `id`
    int get(int id, int num);
    void debugWeapon(int id);
    ItemWork* search(u16 id);   // 0x8001DED0: the in-use slot of this->type holding `id`, NULL if none
    void arm(ItemWork* p);      // 0x8001F350: equip `p` (NULL: bare hands)
    void erase(ItemWork* p);    // remove slot `p` (puzzle removeExtraPiece)
    void construct(ItemWork* out, u16 id);  // fill a slot template for item `id` (puzzle PutInCase)
    int combine(ItemWork* a, ItemWork* b, int flag);  // merge b into a (puzzle cmbPiece)
    // equipped weapon (this->xC), objWep: reloadable(x, 0) / reload(x, 0) / trigger(x)
    int reloadable();           // 0x8001F470
    int reload();               // 0x8001F5B4
    int trigger();              // 0x8001F7E8
    // sce_at: use one item of slot template `p` (id/num), item `id` available?, clear the per-frame flags
    void use(ItemWork* p);      // 0x8001E3BC
    int available(u16 id);      // 0x8001F2C4
    void flagclear();           // 0x8001F2E8
};

extern cItemMgr ItemMgr;

// weapon number/type -> item id (0xFFFF when unknown)
u16 WeaponNo2WeaponId(u8 no, u8 type);

extern "C" {
// item id -> weapon number / type (0xFF when unknown), item attributes
u8 WeaponId2WeaponNo(u16 id);
u8 WeaponId2WeaponType(u16 id);
void itemInfo(u16 id, ItemInfo* info);
// weapon item id -> its bullet item id (attr: ItemWork::x8 >> 13), charge count, max tune level per type
u16 WeaponId2BulletId(u16 id, int attr);
int WeaponId2ChargeNum(u16 id, int a);
int WeaponId2MaxLevel(u16 id, int type);
// weapon tune ratios at tune level `level` (examine: power x10 / speed, reload x100 percent)
f32 getPowerRatio(u16 id, s8 level);
f32 getSpeedRatio(u16 id, s8 level);
f32 getReloadRatio(u16 id, s8 level);
f32 getBulletRatio(u16 id, s8 level);
}

#endif
