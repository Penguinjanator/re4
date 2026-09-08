#ifndef MOTION_H
#define MOTION_H

// game/motion.cpp: key-frame motion playback for cModel hierarchies (Em, Obj, player, ...).

#include "types.h"
#include "vec.h"
#include "model.h"
#include "cam_ctrl.h"

// One sequence key (MotionData sequence table entry / MotionWork::key*).
struct MotionSeqKey {
    u16 frame;  // 0x00  motion frame in 10.6 fixed point
    u8 x2;      // 0x02
    u8 x3;      // 0x03
};

// Key-frame data header (the `data` given to MotionSetCore). Packed:
//   u16 maxFrame (low 14 bits), u8 nParts, u16 parts[nParts], u8 partsNo[nParts],
//   4-aligned u32 keyOfs[nParts] (relocated in place to absolute key pointers).
struct MotionData {
    u16 maxFrame;  // 0x00
    u8 nParts;     // 0x02
};

// Per-model motion work, embedded right after cModel (cModel+0x1D8, 0xDC bytes).
struct MotionWork {
    MotionData* data;     // 0x00  NULL = no motion
    u32* keyTbl;          // 0x04  per parts key data
    u16 hist[2][2][3];    // 0x08  root key history [flip][rot/pos][axis]
    f32 maxFrame;         // 0x20
    f32 frame;            // 0x24
    f32 prevFrame;        // 0x28
    f32 prevFrame2;       // 0x2C
    u8 nParts;            // 0x30
    u8 pad_31[3];
    u8* partsNo;          // 0x34  model parts index per motion parts
    u16* partsInfo;       // 0x38  low byte: kind (1 root pos, 0x40 root rot, 2/4/8/0x30 rot/pos/scale), bits 8-11: attach camera channel, bits 12-15: Fcc type
    u16 rootPosIdx;       // 0x3C  motion parts index of the root position (0xFFFF = none)
    u16 rootRotIdx;       // 0x3E  motion parts index of the root rotation
    u16 flags;            // 0x40  bit0: move the model by the root speed, bit1: reverse, bit2: loop, bit3: pause, bit6: flip, bit8, bit10: hokan speed blend, bit12: sequence reverse, bit13: blend parts, bit15: frame from seqFrame
    u16 state;            // 0x42  MotionSequenceCtrl result: 1 looped, 2 looped (reverse), 4 end, 8 end (reverse)
    u32 flags2;           // 0x44  bit26: cross frame disabled, bit27: flip hist, bit28: no IK, bit29: keep blend, bit30: no matrix, bit31
    Vec pos;              // 0x48  root position (current)
    Vec posPrev;          // 0x54
    Vec posDelta;         // 0x60  root position change over the whole motion
    Vec basePos;          // 0x6C  PartsWorldPosCalc: position the parts were computed at
    Vec speed;            // 0x78  last root speed
    Vec rot;              // 0x84  root rotation (current)
    Vec rotPrev;          // 0x90
    Vec rotDelta;         // 0x9C
    MotionSeqKey* seq;    // 0xA8  sequence table (NULL = linear)
    MotionSeqKey key0;    // 0xAC  current
    MotionSeqKey key1;    // 0xB0  previous
    MotionSeqKey key2;    // 0xB4  before previous
    f32 seqFrame;         // 0xB8  frame in sequence time
    u16 seqMax;           // 0xBC  sequence length
    u8 pad_BE[2];
    f32 speedRate;        // 0xC0  frames per game frame
    u8 hokanMax;          // 0xC4  interpolation frames from the previous pose
    u8 hokanCnt;          // 0xC5  frames left
    u8 pad_C6[2];
    f32 blendRate;        // 0xC8  weight of this work when it is another model's blend motion
    AttachCamera* cam;    // 0xCC
    MotionWork* blend;    // 0xD0  second motion blended in by MotionMove
    u16* flip;            // 0xD4  parts index remap for flipped motions
    u16* blendTbl;        // 0xD8  {count, (dst, a, b, percent)...} quaternion blended parts
};

// Every motion-driven model carries the work right after cModel.
class cMotModel : public cModel {
public:
    MotionWork mot;  // 0x1D8
};

// Parts-side motion state: a parts cModel has no light set, the cLightInfo area holds this.
struct MotionParts {
    Vec pos;         // 0x174  pose before the blend motion was applied
    Vec rot;         // 0x180
    Vec scale;       // 0x18C
    u32 x198;        // 0x198
    u16 hist[6][3];  // 0x19C  key history: rot, pos, scale; then the same for the flipped histories
    u32 flags;       // 0x1C0  bit0 / bit16: animated this frame, bit17: scale cancelled, bit24-25: skip blend, bit26: no cross frame, bit28: hokan pending, bit29: skip, bit31: hokan pending (blend)
};

#define MOTION(m) (&((cMotModel*)(m))->mot)
#define MOTION_PARTS(p) ((MotionParts*)((u8*)(p) + 0x174))
#define PARTS_BIND_MAT(p) (*(Mtx*)((u8*)(p) + 0xF8))

// HermiteInterpolation parameter block.
struct HermitePrm {
    f32 frame;     // 0x00
    f32 maxFrame;  // 0x04
    u32 flags;     // 0x08  bit0: search backwards, bit1: reverse, bit2: loop, bit3: ignore the key history
    u8 type;       // 0x0C  Fcc type
    u8 pad_D[3];
    u8* key;       // 0x10
};

extern "C" {
void PartsWorldPosCalc(cModel* m);
void MotionBlendOff(cModel* m);
void MotionPause(cModel* m);
void MotionClear(cModel* m, int flag);
u16 MotionMove(cModel* m);
u16 MotionMoveSub(cModel* m, MotionWork* w);
void MotionMoveCore(cModel* m, MotionWork* w, int flag);
void MotionHokan(cModel* m, MotionWork* w);
void MotionGetSpeed(cModel* m, MotionWork* w, int flag, Vec* pos, Vec* rot);
void MotionAddSpeed(cModel* m, MotionWork* w, Vec* pos, Vec* rot);
void MotionGetPosition(cModel* m, Vec* pos, Vec* rot);
u16 MotionSequenceCtrl(MotionWork* w);
u16 FcvGetMaxFrame(u16* data);
f32 MotionGetMaxFrame(MotionWork* w);
f32 MotionGetCurrentFrame(MotionWork* w);
int MotionCheckCrossFrame(MotionWork* w, f32 frame);
int MotionGetState(cModel* m);
int HermiteInterpolation(HermitePrm* prm, Vec* out, u16* hist);
int Fcc_next_axis_addr(int type, int n);
void IKInit(cModel* m, MotionWork* w);
void InverseKinematics(cModel* m, int flag);
void cModel_matBlend(cModel* m, f32 rate);
}
void MotionSetCore(cModel* m, void* w, void* data, int seq, int hokan, int flags, int frame);

#endif
