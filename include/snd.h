#ifndef SND_H
#define SND_H

// Capcom sound library (src/game/snd_*.c): C, compiled unoptimised. Work areas live in
// snd_ram.c. Field offsets come from the disassembly; pads mark unknown bytes. Only extend.

#include "snd_sdk.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SND_REQ_BANK_MAX 2
#define SND_REQ_MAX 64
#define SND_VOICE_MAX 64
#define SND_AXV_MAX 64
#define SND_ISS_BLK_MAX 14
#define SND_STR_BLK_MAX 2
#define SND_SEQ_MAX 8
#define SND_STR_MAX 4

// Sound information table entry (SIT, 0x18 bytes) inside an ISS block.
typedef struct {
    u16 prog;       // 0x00  bank << 8 | program
    u8 pad_2[3];
    s8 pan;         // 0x05  < 0: from the DLS
    s8 vol;         // 0x06  < 0: from the DLS
    u8 pad_7[9];
    u8 srd_type;    // 0x10
    s8 span;        // 0x11
    s8 svol;        // 0x12
    u8 pad_13[3];
    u16 flag;       // 0x16
} SND_SIT;

// Room information table entry (RIT, 0x10 bytes) inside a stream block.
typedef struct {
    s16 shd_no;     // 0x00  index into the block's stream header offset table
    u8 pad_2[0xE];
} SND_RIT;

typedef struct {
    u32 num;        // 0x00  number of SITs
    SND_SIT* sit;   // 0x04
    u8* dls;        // 0x08
    u8* xC;         // 0x0C
    u8 pad_10[0x10];
} SND_ISS_BLK;

typedef struct {
    u32 num;        // 0x00
    SND_RIT* rit;   // 0x04
    u8* shd;        // 0x08  table of offsets to the stream headers
    u8 pad_C[4];
} SND_STR_BLK;

// DLS wavetable header / instrument / region views used by get_dls_vol_pan.
typedef struct {
    u32 x0;
    u32 bank_ofs;   // 0x04
    u32 inst_ofs;   // 0x08
    u32 rgn_ofs;    // 0x0C
} SND_DLS_HDR;

typedef struct {
    u32 x0;
    s32 vol;        // 0x04  16.16
    u8 pad_8[8];
    u32 rgn;        // 0x10  region index
    u8 pad_14[4];
} SND_DLS_INST;    // 0x18

typedef struct {
    u8 pad_0[0x4F];
    s8 pan;         // 0x4F
} SND_DLS_RGN;     // 0x50

// Main control work (Snd_ctrl_work, 0xB4 bytes).
typedef struct {
    u32 frame;              // 0x00  audio frame counter
    s32 sound_mode;         // 0x04  0 mono, 1 stereo, 2 DPL2
    u32 req_id;             // 0x08  last issued sound id (never 0)
    u32 aram_base;          // 0x0C
    u32 aram_free;          // 0x10
    u8 pad_14[4];
    s32 x18;                // 0x18  request bank in use
    s32 x1C;                // 0x1C
    u16 rnd;                // 0x20  random seed
    u16 reset_flag;         // 0x22  0x1 soft reset requested, 0x10 done
    u16 x24;                // 0x24
    u16 flag_26;            // 0x26  0x80 reset pan, 0x100 reset vol
    u8 pad_28[4];
    u8 srd_type;            // 0x2C  surround type of the request being issued
    u8 pad_2D[1];
    u16 rnd_pitch;          // 0x2E
    s32 multi_req;          // 0x30  set while issuing a chained (0x2000) request
    u8 pad_34[8];
    u16 sys_vol[6];         // 0x3C  system volumes (<< 8), bit 1..0x20 selects
    u8 x48;                 // 0x48  request override parameters (copied when flag_58 bit set)
    u8 x49;                 // 0x49
    u8 x4A;                 // 0x4A
    u8 x4B;                 // 0x4B
    u8 x4C;                 // 0x4C
    u8 x4D;                 // 0x4D
    u8 x4E;                 // 0x4E
    u8 x4F;                 // 0x4F
    u8 x50;                 // 0x50  surround type override (flag_58 & 0x100)
    u8 pad_51[1];
    u16 x52;                // 0x52
    u16 x54;                // 0x54
    u16 x56;                // 0x56
    u16 flag_58;            // 0x58  which override parameters are valid
    u8 pad_5A[0x70 - 0x5A];
    u32 dsp_cycles_max;     // 0x70
    u32 dsp_cycles_peak;    // 0x74
    u32 dsp_cycles;         // 0x78
    u16 voice_peak;         // 0x7C
    u16 voice_num;          // 0x7E
    u16 axv_peak;           // 0x80
    u16 axv_num;            // 0x82
    u16 str_peak;           // 0x84
    u16 str_num;            // 0x86
    u16 seq_peak;           // 0x88
    u16 seq_num;            // 0x8A
    u16 total_peak;         // 0x8C
    u16 total_num;          // 0x8E
    ARQRequest arq;         // 0x90
    s32 dma_busy;           // 0xB0
} SND_CTRL_WORK;

// Sound request (Snd_req_work[2][64], 0x2C bytes).
typedef struct {
    u16 status;     // 0x00
    u16 no;         // 0x02
    u8 type;        // 0x04  1 / 2, bit 0x4 = pending
    u8 srd_type;    // 0x05
    u8 pad_6[2];
    u16 blk_no;     // 0x08
    u16 req_no;     // 0x0A
    u32 snd_id;     // 0x0C
    SND_SIT* sit;   // 0x10
    u16 flag;       // 0x14  override flags (from ctrl->flag_58)
    u8 pad_16[4];
    s8 x1A;         // 0x1A
    s8 x1B;         // 0x1B
    s8 x1C;         // 0x1C
    s8 x1D;         // 0x1D
    s8 x1E;         // 0x1E
    s8 x1F;         // 0x1F
    s8 x20;         // 0x20
    s8 x21;         // 0x21
    u16 x22;        // 0x22
    u16 x24;        // 0x24
    u16 x26;        // 0x26
    u16 pitch;      // 0x28
    u8 pad_2A[2];
} SND_REQ_WORK;

struct SND_AXV_WORK_;

// Voice slot (Snd_voice_work[64], 0x20 bytes).
typedef struct {
    u16 status;     // 0x00
    u16 no;         // 0x02
    u32 snd_id;     // 0x04
    u8 x8;          // 0x08
    u8 out_mode;    // 0x09  1 / 2
    s8 type;        // 0x0A  1 iss, 2 seq, 3 str
    u8 pad_B[1];
    u32 count;      // 0x0C
    u32 x10;        // 0x10
    struct SND_AXV_WORK_* axv;  // 0x14
    s16 x18;        // 0x18
    s16 x1A;        // 0x1A
    s8 prio;        // 0x1C
    s8 seq_no;      // 0x1D
    s8 seq_ch;      // 0x1E
    s8 seq_note;    // 0x1F
} SND_VOICE_WORK;

// AX voice work (Snd_axv_work[64], 0x80 bytes).
typedef struct SND_AXV_WORK_ {
    u16 status;     // 0x00
    u8 pad_2[0x7E];
} SND_AXV_WORK;

// MIDI sequence work (Snd_seq_work[8], 0x328C bytes).
typedef struct {
    u8 pad_0[0x24];
    SYNSYNTH synth;         // 0x24 .. 0x3164
    s32 x3164;              // 0x3164  active
    u8 pad_3168[4];
    void* x316C;            // 0x316C  voice allocation info (Snd_voice_work_open_ck)
    u8 pad_3170[0x328C - 0x3170];
} SND_SEQ_WORK;

// Stream work (Snd_str_work[4], 0x14C bytes).
typedef struct {
    u8 pad_0[0xC8];
    s32 xC8;        // 0xC8  voice L in use
    s32 xCC;        // 0xCC  voice R in use
    u8 pad_D0[0x14C - 0xD0];
} SND_STR_WORK;

// Sound test / debug work (Snd_test_work, 0x7C0 bytes, 32-aligned).
typedef struct {
    u8 x0;
    u8 x1;
    u8 x2;
    u8 x3;
    u8 pad_4[8];
    u16 xC;             // 0x0C
    u16 xE;             // 0x0E
    u8 pad_10[8];
    u16 x18;            // 0x18
    u16 x1A;            // 0x1A
    u8 pad_1C[0x9C - 0x1C];
    char path0[0x100];  // 0x9C
    char path1[0x380];  // 0x19C
    u32 x51C[14];       // 0x51C
    u32 x554[2];        // 0x554
    u8 pad_55C[0x6B8 - 0x55C];
    u32 aram_base;      // 0x6B8
    u8 pad_6BC[0x7C0 - 0x6BC];
} SND_TEST_WORK;

// Effect work (Snd_efx_work, 0x4F0 bytes).
typedef struct {
    u8 pad_0[0x4F0];
} SND_EFX_WORK;

// Low-pass filter table entry (Snd_lpf_tbl[24]).
typedef struct {
    u16 a0;
    u16 b0;
    const char* name;
} SND_LPF;

// snd_ram.c
extern SND_CTRL_WORK Snd_ctrl_work;
extern SND_EFX_WORK Snd_efx_work;
extern SND_REQ_WORK Snd_req_work[SND_REQ_BANK_MAX][SND_REQ_MAX];
extern SND_VOICE_WORK Snd_voice_work[SND_VOICE_MAX];
extern SND_AXV_WORK Snd_axv_work[SND_AXV_MAX];
extern SND_ISS_BLK Snd_iss_blk[SND_ISS_BLK_MAX];
extern SND_STR_BLK Snd_str_blk[SND_STR_BLK_MAX];
extern SND_SEQ_WORK Snd_seq_work[SND_SEQ_MAX];
extern SND_STR_WORK Snd_str_work[SND_STR_MAX];
extern void* Snd_str_buff[4];
extern SND_TEST_WORK Snd_test_work;

// snd_sub0.c
extern s32 Snd_dls_vol_tbl[128];
extern SND_LPF Snd_lpf_tbl[24];
int Snd_pronounce_ck(void);
void Snd_soft_reset_req(void);
int Snd_soft_reset_ck(void);
void Snd_reset_pan_all(void);
void Snd_reset_vol_all(void);
void Snd_set_system_vol(u16 type, u16 vol);
s16 Snd_get_system_vol(s16 type);
s32 Snd_vol_syn_to_ax(s32 vol);
s32 Snd_vol_ax_to_syn(s32 vol);
u8 Snd_rnd(void);
s16 Snd_get_rnd_pitch(SND_SIT* sit);
void Snd_test_work_clear(void);

// snd_sub1.c
void Snd_voice_work_clear(void);
void Snd_stop_voice_work(SND_VOICE_WORK* voice);
SND_VOICE_WORK* Snd_open_voice_work_str(SND_STR_WORK* str, s8 no);
SND_VOICE_WORK* Snd_open_voice_work_seq(SND_SEQ_WORK* seq, s8 prio);
SND_VOICE_WORK* Snd_voice_work_open_ck(void* info, s8 prio);
SND_VOICE_WORK* Snd_search_voice_work_snd_id(u32 snd_id);
SND_VOICE_WORK* Snd_search_voice_work_seq(SND_SEQ_WORK* seq, u8 ch, u8 note);

// snd_sub2.c
int Snd_se_reset_check(SND_CTRL_WORK* ctrl);
void Snd_req_work_clear(void);
void Snd_req_work_copy_para(SND_CTRL_WORK* ctrl, SND_REQ_WORK* req);
SND_REQ_WORK* Snd_open_req_work(void);
SND_REQ_WORK* Snd_search_req_work_snd_id(u32 snd_id, u8 type);

// snd_sub3.c
void Snd_iss_blk_init(u32 blk_no, void* data);
void Snd_str_blk_init(u32 blk_no, void* data);
SND_ISS_BLK* Snd_get_blk_adrs(u16 blk_no, u16 req_no);
SND_SIT* Snd_get_sit_adrs(u16 blk_no, u16 req_no);
SND_RIT* Snd_get_rit_adrs(u16 blk_no, u16 req_no);
void* Snd_get_shd_adrs(u16 blk_no, u16 req_no);
u16 Snd_iss_get_sit_type(u16 blk_no, u16 req_no);
s8 Snd_iss_get_sit_vol(u16 blk_no, u16 req_no);
s8 Snd_iss_get_sit_svol(u16 blk_no, u16 req_no);
s8 Snd_iss_get_sit_pan(u16 blk_no, u16 req_no);
s8 Snd_iss_get_sit_span(u16 blk_no, u16 req_no);
s8 get_dls_vol_pan(u16 blk_no, u16 req_no, int mode);

// snd_main.c
void Snd_system_init(void);
void snd_work_clear(void);
void zero_buff_clear(void);
void cb_audio_frame(void);
void Snd_sound_mode_init(void);
s32 Snd_sound_mode_init_load(s32 mode);
s32 Snd_get_sound_mode(void);
void Snd_set_sound_mode(s32 mode);
void snd_mode_set_ax_mix(SND_CTRL_WORK* ctrl);
void Snd_iss_control(void);
void Snd_dev_voice_ck(void);

// snd_iss0.c
int Snd_iss_req_para(u16 blk_no, u16 req_no, u8* para);
int req_iss_main(u16 blk_no, u16 req_no, u8* para);
void req_set_srd_type(SND_CTRL_WORK* ctrl, SND_SIT* sit, u8* para);
int req_iss_one(SND_CTRL_WORK* ctrl, SND_SIT* sit, u16 blk_no, u16 req_no);
int req_iss_one_sub(SND_CTRL_WORK* ctrl, SND_SIT* sit, u16 blk_no, u16 req_no);
int Snd_get_play_type(u32 snd_id);

// snd_iss1.c
int Snd_se_pronounce_ck_all(void);

// snd_iss2.c
void Snd_iss_manager(void);

// snd_iss4.c
void Snd_axv_work_clear(void);
void Snd_axv_work_note_off(SND_AXV_WORK* axv, u32 mode);

// snd_efx.c
void Snd_efx_work_clear(void);

// snd_seq0.c
void Snd_seq_reset_vol_type(int type);
int Snd_seq_pronounce_ck_type(int type);

// snd_seq1.c
void Snd_midi_sequencer(void);

// snd_seq3.c
void Snd_seq_work_clear(void);
SND_SEQ_WORK* Snd_search_seq_work_snd_id(u32 snd_id);
void Snd_seq_work_close_check(void);
void Snd_send_midi(SYNSYNTH* synth, u8 status, u8 data1, u8 data2);

// snd_str0.c
void Snd_str_reset_vol_type(int type);
void Snd_str_reset_pan_type(int type);
int Snd_str_pronounce_ck_type(int type);

// snd_str1.c
void Snd_stream_player(void);

// snd_str4.c
void Snd_str_work_clear(void);
SND_STR_WORK* Snd_search_str_work_snd_id(u32 snd_id);
void Snd_str_work_close_check(void);

#ifdef __cplusplus
}
#endif

#endif
