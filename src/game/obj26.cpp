#include "atari.h"
#include "light.h"
#include "ctrl.h"
#include "obj.h"
#include "global.h"
#include "math_sub.h"

// Attachment that follows parts 2 of its parent, scales toward a target size (routine 0) and
// then shrinks/fades away (routine 1). Routine index in xFD, step in xFE.
class cObj26 : public cObj {
public:
    virtual void move();
};

extern "C" {
void MotionMove(cModel* m, int a);
void obj26_R1_Set(cObj26* obj);
void obj26_R1_Die(cObj26* obj);
void obj26MatCalc(cObj26* obj);
}

static void (*Obj26_R1_move_tbl[2])(cObj26*) = { obj26_R1_Set, obj26_R1_Die };

// Never called: the original linker dropped the body but kept its string, statics and pool.
static cObj* SetObj26(cObj* parent, Vec* scale)
{
    cObj* obj;

    obj = ObjMgr.create(0x26);
    if (obj == 0) {
        return 0;
    }
    if (obj->modelInit((void*) (pG->pArc->ofs_20 + (u32) pG->pArc),
                       (void*) (pG->pArc->ofs_24 + (u32) pG->pArc)) == 0) {
        pLog->err(0, 0, "SetObj26() failed.");
        ObjMgr.destroy(obj);
        return 0;
    }
    static const Vec p0 = { 0.0f, 0.0f, 0.0f };
    static const Vec p1 = { 1000.0f, 1000.0f, 1000.0f };

    obj->LightInfo.init2(0, 1, &p0, &p1, 0x10);
    obj->obj26.parent = parent;
    obj->obj26.tgtScale = *scale;
    obj->scale.x = obj->scale.y = obj->scale.z = 0.0f;
    obj->invisible_factor = 1.0f;
    return obj;
}

void cObj26::move()
{
    if (obj26.parent) {
        if ((obj26.parent->be_flag & 0x201) != 1) {
            ObjMgr.destroy(this);
            return;
        }
    }
    Obj26_R1_move_tbl[r_no_1](this);
    if (invisible_factor == 0.0f) {
        ObjMgr.destroy(this);
    }
}

void obj26_R1_Set(cObj26* obj)
{
    Obj26Work* w = &obj->obj26;

    switch (obj->r_no_2) {
    case 0:
        obj->r_no_2++;
    case 1:
        obj->scale.x = obj->scale.x * 0.9f + w->tgtScale.x * 0.1f;
        obj->scale.y = obj->scale.y * 0.9f + w->tgtScale.y * 0.1f;
        obj->scale.z = obj->scale.z * 0.9f + w->tgtScale.z * 0.1f;
        if (obj->pMotion) {
            MotionMove(obj, 0);
        }
        break;
    }
    obj26MatCalc(obj);
}

void obj26_R1_Die(cObj26* obj)
{
    switch (obj->r_no_2) {
    case 0:
        obj->r_no_2++;
    case 1:
        obj->scale.y = obj->scale.z = obj->scale.x = obj->scale.x * 0.9f;
        obj->invisible_factor *= 0.9f;
        if (obj->invisible_factor <= 0.01f) {
            obj->invisible_factor = 0.0f;
            obj->r_no_2++;
        } else if (obj->pMotion) {
            MotionMove(obj, 0);
        }
        break;
    case 2:
        break;
    }
    obj26MatCalc(obj);
}

void obj26MatCalc(cObj26* obj)
{
    if (obj->obj26.parent) {
        cModel* parts = obj->obj26.parent->getPartsPtr(2);
        RotMatrix(obj->mat, &obj->ang);
        TransMatrix(obj->mat, &obj->pos);
        ScaleMatrix(obj->mat, &obj->scale);
        PSMTXConcat(parts->mat, obj->mat, obj->mat);
        obj->motFlags2 |= 0x40000000;
    } else {
        RotMatrix(obj->l_mat, &obj->ang);
        TransMatrix(obj->l_mat, &obj->pos);
        ScaleMatrix(obj->l_mat, &obj->scale);
        PSMTXCopy(obj->l_mat, obj->mat);
    }
    obj->partsMatCalc();
    obj->partsWorldCalc();
}
