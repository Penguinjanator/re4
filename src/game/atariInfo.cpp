#include "atariInfo.h"
#include "atari.h"
#include "model.h"
#include "global.h"
#include "math_sub.h"
#include "dbmodule.h"
#include "main_mem.h"

cAtariInfo::cAtariInfo()
{
    memclr_asm(this, sizeof(cAtariInfo));
}

void cAtariInfo::init0(int parts, int hokan, int flag, f32 x, f32 y, f32 z, f32 rx, f32 rz, f32 w, f32 hh)
{
    m_offset.x = x;
    m_offset.y = y;
    m_offset.z = z;
    m_radius_n = rx;
    m_radius2_n = rz;
    m_radius = rx;
    m_radius2 = rz;
    m_radius3 = w;
    m_height = hh;
    m_parts_no = parts;
    this->m_hokan = hokan;
    m_flag = flag | 0x300;
    x48 = 0;
    m_stat = 1;
}

void cAtariInfo::init(int parts, int flag, int hokan, f32 x, f32 y, f32 z, f32 rx, f32 rz, f32 w, f32 hh)
{
    init0(parts, hokan, flag, x, y, z, rx, rz, w, hh);
    m_flag |= 1;
}

void cAtariInfo::set(int mode, f32 a, f32 b)
{
    if (mode < 0) {
        m_radius_n = a;
        m_radius2_n = b;
        m_radius2 = 100.0f;
        m_radius = 100.0f;
        mode = -mode;
    } else {
        if (mode == 0) {
            m_radius = a;
            m_radius2 = b;
        }
        m_radius_n = a;
        m_radius2_n = b;
    }
    m_hokan = mode;
}

// Dead-stripped by the original linker (only its constant pool survives in .rodata).
static f32 atariInfoRange(f32 v)
{
    if (v < 250.0f) {
        return 600.0f;
    }
    return 100000.0f;
}

void cAtariInfo::getSpeedVector(cModel* m, Vec* oldPos, Vec* newPos)
{
    Vec v;

    if (m_parts_no != 0) {
        cModel* p = m->getPartsPtr(m_parts_no - 1);
        v.x = m_offset.x;
        v.y = m_offset.y;
        v.z = m_offset.z;
        RotVector(&v, &m->ang, &v);
        PSVECAdd(&p->world, &v, newPos);
        PSVECAdd(&p->world_old, &v, oldPos);
        newPos->y = m->pos.y + m_radius;
        oldPos->y = m->pos_old.y + m_radius;
    } else {
        v.x = m_offset.x;
        v.y = m_offset.y + m_radius;
        v.z = m_offset.z;
        RotVector(&v, &m->ang, &v);
        PSVECAdd(&m->pos, &v, newPos);
        PSVECAdd(&m->pos_old, &v, oldPos);
    }
}

void cAtariInfo::move()
{
    if (m_hokan != 0) {
        f32 c = (f32) m_hokan;
        m_hokan--;
        m_radius += (m_radius_n - m_radius) / c;
        m_radius2 += (m_radius2_n - m_radius2) / c;
    }
}

void cAtariInfo::getPos(cModel* m, Vec* out)
{
    Vec v;

    RotVector(&m_offset, &m->ang, &v);
    if (m_parts_no != 0) {
        PSVECAdd(&m->getPartsPtr(m_parts_no - 1)->world, &v, out);
    } else {
        PSVECAdd(&m->pos, &v, out);
    }
}

void cAtariInfo::setPriority(int prio)
{
    m_flag &= ~0x18;
    switch (prio) {
    case 0:
        break;
    case 1:
        m_flag |= 0x8;
        break;
    case 2:
        m_flag |= 0x10;
        break;
    case 3:
        m_flag |= 0x18;
        break;
    }
}

void cAtariInfo::disp(cModel* m)
{
    if (m_flag & 2) {
        dispRect(m);
    } else {
        Vec p;
        getPos(m, &p);
        p.y -= m_height;
        Draw_cylinder(&p, m_radius2, m_height * 2.0f, 0xFFFFFFFF);
    }
}

void cAtariInfo::dispRect(cModel* m)
{
    static u8 ptbl[36] = {
        0, 2, 1, 2, 3, 1, 4, 5, 6, 5, 7, 6, 2, 6, 3, 6, 7, 3,
        0, 1, 4, 1, 5, 4, 1, 3, 5, 3, 7, 5, 2, 0, 6, 0, 4, 6,
    };
    Vec v[8];
    Vec w[8];
    Vec size;
    Mtx mat;
    int i;

    size.x = m_radius;
    size.y = m_height;
    size.z = m_radius2;
    v[0].x = -size.x;
    v[0].y = -size.y;
    v[0].z = -size.z;
    v[1].x = size.x;
    v[1].y = -size.y;
    v[1].z = -size.z;
    v[2].x = -size.x;
    v[2].y = -size.y;
    v[2].z = size.z;
    v[3].x = size.x;
    v[3].y = -size.y;
    v[3].z = size.z;
    v[4].x = -size.x;
    v[4].y = size.y;
    v[4].z = -size.z;
    v[5].x = size.x;
    v[5].y = size.y;
    v[5].z = -size.z;
    v[6].x = -size.x;
    v[6].y = size.y;
    v[6].z = size.z;
    v[7].x = size.x;
    v[7].y = size.y;
    v[7].z = size.z;
    for (i = 0; i < 8; i++) {
        PSVECAdd(&v[i], &m_offset, &v[i]);
    }
    if (m_parts_no != 0) {
        cModel* p = m->getPartsPtr(m_parts_no - 1);
        PSMTXRotRad(mat, 'y', p->ang.y);
        TransMatrix(mat, &p->world);
        PSMTXConcat(pG->Cam.v_mat, mat, mat);
    } else {
        PSMTXRotRad(mat, 'y', m->ang.y);
        TransMatrix(mat, &m->pos);
        PSMTXConcat(pG->Cam.v_mat, mat, mat);
    }
    for (i = 0; i < 12; i++) {
        u8* t = &ptbl[i * 3];
        Draw_line3d_local(&v[t[0]], &v[t[1]], mat, 0xFFFFFFFF, 0);
        Draw_line3d_local(&v[t[1]], &v[t[2]], mat, 0xFFFFFFFF, 0);
        Draw_line3d_local(&v[t[2]], &v[t[0]], mat, 0xFFFFFFFF, 0);
    }
}
