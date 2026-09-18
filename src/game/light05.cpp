// game/light05: light type 5, path-animated brightness (D:/Bio4/Prog/light05.cpp): plays a light
// path (byte sequence 0..200 = brightness in 1/200, 0xFF = end) from the room's light path data.
#include "light.h"

struct Light05Work {
    u8 pad_0[0xC];
    u8 pathNo;   // 0x0C light path data number
    u8 pathIdx;  // 0x0D path inside the data
};

// LightFuncTbl[5]: Rno0 0 binds the path (pathNo/pathIdx from the work), 1 steps it each frame and
// scales Col by value/200 into DispCol; the light is destroyed when the path ends.
// Path light: follows a light path; the path returns the brightness (0..200) or 0xFF at the end.
void Light05_Move(cLight* l)
{
    cLightPath* path = (cLightPath*)l->work;
    Light05Work* w = (Light05Work*)l->work;
    int v;
    f32 rate;

    switch (l->Rno0) {
    case 0:
        path->setPath(LightMgr.getPathPtr(w->pathNo), w->pathIdx);
        l->Rno0 = 1;
    case 1:
        v = path->movePath();
        if (v == 0xff) {
            LightMgr.destroy(l);
            return;
        }
        rate = (f32)v * 0.005f;
        l->DispCol.r = (u8)(rate * l->Col.r);
        l->DispCol.g = (u8)(rate * l->Col.g);
        l->DispCol.b = (u8)(rate * l->Col.b);
        l->DispCol.a = (u8)(rate * l->Col.a);
        break;
    }
}
