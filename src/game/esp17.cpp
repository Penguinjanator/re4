#include "global.h"
#include "esp.h"

// Screen-space effect: keeps its own copy of the position and converts it into view space.
class cEsp17 : public cEsp {
public:
    Vec work;  // 0xF8

    virtual void move();
    virtual int SetFreeWork(EspGenWork* gen, u32* seed);
};

cEsp* Esp17_Create()
{
    return new cEsp17;
}

void cEsp17::move()
{
    Mtx inv;

    m_Pos = work;
    if (CommonMove()) {
        if (!AnmMove()) {
            PushEsp(this);
        } else {
            work = m_Pos;
            FSet(m_Pos.z, -m_Pos.z);
            PSMTXInverse(pG->Cam.v_mat, inv);
            PSMTXMultVec(inv, &m_Pos, &m_Pos);
        }
    }
}

int cEsp17::SetFreeWork(EspGenWork* gen, u32* seed)
{
    work = m_Pos;
    return 1;
}
