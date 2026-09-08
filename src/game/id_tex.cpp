#include "light.h"
#include "atari.h"
#include "id_sys.h"
#include "texture.h"
#include "main_mem.h"
#include "db_log.h"
#include "gx.h"

cTexSys* g_pIdTexSys;

void IdTexGameInit()
{
    cTexSys* sys;

#line 46 "D:/Bio4/Prog/id_tex.cpp"
    sys = (cTexSys*) MEM_ALLOC(sizeof(cTexSys), 1, 13);
    g_pIdTexSys = sys;
    sys->Init("IdTex", 0x200);
    IdTexRoomInit();
}

void IdTexRoomInit()
{
    g_pIdTexSys->Clear();
}

void IdTexRelease(int id)
{
    g_pIdTexSys->TexRelease(id);
}

// Effect texture pack: version 0xB, id table at 0x04, TPL table at 0x18, animation table at 0x1C.
struct IdTexData {
    u32 version;  // 0x00  == 0xB
    u32 ofsId;    // 0x04  -> TexIdTbl
    u8 pad_8[0x18 - 0x8];
    u32 ofsTpl;   // 0x18  -> TexOfsTbl of TPLs
    u32 ofsAnm;   // 0x1C  -> TexOfsTbl of TexAnms
};

int IdTexDataLoad(void* data, int id)
{
    IdTexData* d = (IdTexData*) data;
    TexIdTbl* idTbl;
    TexOfsTbl* tplTbl;
    TexOfsTbl* anmTbl;
    u32 i;

    if (d->version != 0xB) {
        pLog->err(0, 0, "IdDataLoad():EffData [0x%x] Invalid.", data);
        return 0;
    }
    idTbl = (TexIdTbl*) ((u8*) data + d->ofsId);
    tplTbl = (TexOfsTbl*) ((u8*) data + d->ofsTpl);
    anmTbl = (TexOfsTbl*) ((u8*) data + d->ofsAnm);
    for (i = 0; i < idTbl->num; i++) {
        u8 texId = idTbl->ent[i].id;
        int check = 1;

        if (texId == 0x80) {
            check = 0;
        }
        g_pIdTexSys->TexRegist((TEXPalette*) ((u8*) tplTbl + tplTbl->ofs[i]),
                               (TexAnm*) ((u8*) anmTbl + anmTbl->ofs[i]), texId, id, 0, check);
    }
    return 1;
}

void IdTexSet(u8 id, u8 no)
{
    Mtx m;
    GXTexObj* tex;
    GXTlutObj* tlut;

    if (g_pIdTexSys->GetTexObj(id, no, &tex) == 0) {
        pLog->err(0, 0, "IdTexSet: TexId[%x] no data", id);
        return;
    }
    GXLoadTexObj(tex, 0);
    if (g_pIdTexSys->GetTlutObj(id, &tlut) != 0) {
        GXLoadTlut(tlut, 0);
    }
    GXSetZMode(0, 3, 0);
    GXSetNumTevStages(1);
    GXSetTevOp(0, 0);
    GXSetTevOrder(0, 0, 0, 4);
    GXSetNumTexGens(1);
    PSMTXIdentity(m);
    GXLoadTexMtxImm(m, 0x1E, 1);
    GXSetTexCoordGen(0, 1, 4, 0x1E);
}

int IdGetAnmAddr(u8 id, TexAnm** out)
{
    return g_pIdTexSys->GetAnmAddr(id, out);
}

void IdChannelSet(IdUnit* u)
{
    GXColor c;

    GXSetTevOp(0, 0);
    GXSetNumChans(1);
    GXSetChanCtrl(4, 0, 0, 0, 0, 0, 2);
    c.r = (u8) u->col[0];
    c.g = (u8) u->col[1];
    c.b = (u8) u->col[2];
    c.a = (u8) u->col[3];
    GXSetChanMatColor(4, c);
}

TexWk* IdGetTexWk(u8 id, int quiet)
{
    return g_pIdTexSys->GetTexWk(id, quiet);
}
