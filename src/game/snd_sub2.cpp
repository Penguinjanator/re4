#include "snd_drv.h"

int Snd_se_reset_check(SND_CTRL_WORK* ctrl)
{
    SND_AXV_WORK* axv;
    int i;

    if (ctrl->reset_flag & 0x10) {
        return 1;
    }
    if (ctrl->reset_flag & 0x1) {
        Snd_req_work_clear();
        for (i = 0; i < SND_AXV_MAX; i++) {
            axv = &Snd_axv_work[i];
            if (axv->status != 0) {
                Snd_axv_work_note_off(axv, 1);
            }
        }
        ctrl->reset_flag |= 0x10;
        ctrl->se_state = 0;
        ctrl->se_ctrl = 0;
        return 1;
    }
    return 0;
}

void Snd_req_work_clear(void)
{
    SND_REQ_WORK* req;
    u32 i;
    u32 j;
    u32 k;
    u8* p;

    for (i = 0; i < SND_REQ_BANK_MAX; i++) {
        for (j = 0; j < SND_REQ_MAX; j++) {
            req = &Snd_req_work[i][j];
            p = (u8*) req;
            for (k = 0; k < sizeof(SND_REQ_WORK); k++) {
                *p++ = 0;
            }
            req->no = j;
        }
    }
}

void Snd_req_work_copy_para(SND_CTRL_WORK* ctrl, SND_REQ_WORK* req)
{
    SND_SIT* sit;

    sit = Snd_iss_blk[req->blk_no].sit;
    sit += req->req_no;
    req->prio = -1;
    req->pan = -1;
    req->span = -1;
    req->vol = -1;
    req->svol = -1;
    req->aux_a = -1;
    req->aux_b = -1;
    req->lpf_no = -1;
    req->pitch_add = 0;
    req->pitch_ofs = 0;
    req->se_flag = 0;
    req->flag = ctrl->ovr_flag;
    if (req->flag & 0x1) {
        req->prio = ctrl->prio;
    }
    if (req->flag & 0x2) {
        req->pan = ctrl->pan;
    }
    if (req->flag & 0x4) {
        req->span = ctrl->span;
    }
    if (req->flag & 0x8) {
        req->vol = ctrl->vol;
    }
    if (req->flag & 0x10) {
        req->svol = ctrl->svol;
    }
    if (req->flag & 0x20) {
        req->aux_a = ctrl->aux_a;
    }
    if (req->flag & 0x40) {
        req->aux_b = ctrl->aux_b;
    }
    if (req->flag & 0x80) {
        req->lpf_no = ctrl->lpf_no;
    }
    if (req->flag & 0x200) {
        req->pitch_add = ctrl->pitch_add;
    }
    if (req->flag & 0x400) {
        req->pitch_ofs = ctrl->pitch_ofs;
    }
    if (req->flag & 0x800) {
        req->se_flag = ctrl->se_flag;
    }
    if (req->type != 4) {
        if (ctrl->multi_req == 1) {
            req->pitch = ctrl->rnd_pitch;
        } else {
            req->pitch = Snd_get_rnd_pitch(sit);
        }
    }
}

SND_REQ_WORK* Snd_open_req_work(void)
{
    SND_CTRL_WORK* ctrl = &Snd_ctrl_work;
    SND_REQ_WORK* req;
    int i;

    if (ctrl->reset_flag & 0x10) {
        return NULL;
    }
    for (i = 0; i < SND_REQ_MAX; i++) {
        req = &Snd_req_work[ctrl->req_bank][i];
        if (req->status == 0) {
            return req;
        }
    }
    return NULL;
}

SND_REQ_WORK* Snd_search_req_work_snd_id(u32 snd_id, u8 type)
{
    SND_CTRL_WORK* ctrl = &Snd_ctrl_work;
    SND_REQ_WORK* req;
    int i;
    int j;

    for (i = 0; i < SND_REQ_BANK_MAX; i++) {
        for (j = 0; j < SND_REQ_MAX; j++) {
            req = &Snd_req_work[i][j];
            if (req->status == 0) {
                continue;
            }
            if (req->type & 0x4) {
                continue;
            }
            if (!(req->type & type)) {
                continue;
            }
            if (req->snd_id != snd_id) {
                continue;
            }
            return req;
        }
    }
    return NULL;
}
