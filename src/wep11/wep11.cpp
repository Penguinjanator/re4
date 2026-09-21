// wep11 module: the TMP machine gun. Weapon class wep/objMachinegun.cpp, routines wep/pl_machine.cpp.
// Wep11_init is the module's WeaponInitFunc (creates the cObjMachinegun, object id 0x2D, as the
// player's weapon and loads the muzzle-flash effects), PlMachineMove its WeaponMoveFunc; _prolog
// registers both and the ObjInitFunc slot.

#include "wep_mod.h"
#include "light.h"
#include "esp.h"

void PlMachineMove(cPlayer* pl);   // wep/pl_machine.cpp
void ObjMachinegun_init(cObj* obj);   // wep/objMachinegun.cpp

// WeaponInitFunc (cPlayer::weaponInit with the player): creates the cObjMachinegun as
// Wep->m_pWep, inits it on the player, installs its motions, loads the muzzle-flash effects
// (archive 0x4 as group 0x45) and points the debug preview PlWepMot at the aim idles 0x1B/0x1F/0x21.
void Wep11_init(cModel* m)
{
    cPlayer* pl = (cPlayer*) m;
    cObjWep* obj = (cObjWep*) ObjMgr.createBack(cObjMgr::ID_WEP_MACHINE);

    if (obj == 0) {
        pLog->err(0, 0, "Wep11_init() cObjWep CREATE FAILED");
    } else {
        pl->Wep->m_pWep = obj;
        obj->init(pl);
        obj->setMotion(pl);
        EspDataLoad((u32) WEP_ARC_PTR(0x4), EFF_WEP11, 1);
        PlWepMot[0] = WEP_ARC_PTR(0x1B);
        PlWepMot[1] = WEP_ARC_PTR(0x1F);
        PlWepMot[2] = WEP_ARC_PTR(0x21);
    }
}

// REL entry: registers the weapon init / move routines and the object constructor slot.
extern "C" void _prolog()
{
    WeaponInitFunc = Wep11_init;
    WeaponMoveFunc = PlMachineMove;
    ObjInitFunc[0x2D] = ObjMachinegun_init;
    OSReport("Wep11 MACHINEGUN prolog Ok\n");
}

// REL exit: frees the object constructor slot.
extern "C" void _epilog()
{
    ObjInitFunc[0x2D] = 0;
}

// Target of every unresolved cross-module branch (snmakerel patches them to `bl _unresolved`).
extern "C" void _unresolved()
{
}
