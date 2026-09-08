#include "types.h"
#include "rnd.h"

static u16 Random;

static void RndInit(u16 seed)
{
    Random = seed;
}

u8 Rnd()
{
    u16 r = Random;
    u32 n = ((u8) ((r >> 1) + (r >> 8)) << 8) | (u8) (r >> 1);
    u32 m = n;

    if (m == r) {
        m = n + 0x101;
    }
    Random = m;
    return m >> 8;
}

f32 fRand0_1()
{
    u32 a = Rnd();
    u32 b = Rnd();
    u32 c = Rnd();
    u32 u = a + (b << 8) + ((c & 0x7F) << 16) + 0x3F800000;

    return *(f32*) &u - 1.0f;
}

f32 fRand1_1()
{
    u32 a = Rnd();
    u32 b = Rnd();
    u32 c = Rnd();
    u32 u = a + (b << 8) + ((c & 0x7F) << 16) + 0x3F800000;

    return *(f32*) &u * 2.0f - 3.0f;
}

f32 fRandSeed0_1(u32* seed)
{
    f32 f;

    *seed = *seed * 0x19660D + 0x3C6EF35F;
    *(u32*) &f = (*seed & 0x007FFFFF) | 0x3F800000;
    f -= 1.0f;
    return f;
}

f32 fRandSeed1_1(u32* seed)
{
    f32 f;

    *seed = *seed * 0x19660D + 0x3C6EF35F;
    *(u32*) &f = (*seed & 0x007FFFFF) | 0x3F800000;
    f = f * 2.0f - 2.0f - 1.0f;
    return f;
}
