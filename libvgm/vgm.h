#pragma once
#include <stdbool.h>
#include <stdint.h>

#pragma pack(push, 1)
struct vgm_header_t {
    // "Vgm " (0x56, 0x67, 0x6d, 0x20)
    uint32_t vgm_ident;
    // relative offset to end of file
    uint32_t offset_eof;
    uint32_t version;
    // input clock rate in hz, typical 3579545
    uint32_t clock_sn76489;
    uint32_t clock_ym2413;
    // relative offset to GD3 tag; 0 if no tag;
    uint32_t offset_gd3;
    // Total of all wait values in the file.
    uint32_t total_samples;
    // relative offset of the loop point, or 0 if no loop.
    uint32_t loop_offset;
    // number of samples in one loop, or 0 if no loop
    uint32_t loop_samples;
    // rate of recording in hz. 50 pal, 60 ntsc;
    uint32_t rate;
    // lfsr feedback pattern
    uint16_t feedback_sn76489;
    // lfsr width	- 15 sms2,gg,smd
    //				- 16 bbcmicro,segacomputer3000
    uint8_t width_sn76489;
    // lfsr flags	- bit 0 frequency is 0x400
    //				- bit 1 output negate flag
    //				- bit 2 stereo
    // on(0)/off(1)
    //				- bit 3 /8 clock divider	on(0)/off(1)
    uint8_t flags_sn76489;
    uint32_t _1;
    uint32_t _2;
    // file offset to start of vgm data
    uint32_t offset_vgmdata;
    uint32_t _3;
};
#pragma pack(pop)

struct vgm_chip_t {
    void (*set_clock)(uint32_t clock);
    void (*write)(uint32_t port, uint32_t reg, uint32_t data);
    void (*render)(int16_t* dst, uint32_t samples);
    void (*mute)();
    void* user;
};

struct vgm_chip_bank_t {
    struct vgm_chip_t* ym3812;
    struct vgm_chip_t* sn76489;
    struct vgm_chip_t* ym2612;
    struct vgm_chip_t* nes_apu;
    struct vgm_chip_t* gb_dmg;
    struct vgm_chip_t* pokey;
};

struct vgm_stream_t {
    void * user;
    uint8_t(*read8)();
    uint8_t(*peek8)();
    uint16_t(*read16)();
};

struct vgm_state_t {
    const uint8_t* raw;
    const uint8_t* stream;
    size_t size;
    uint32_t spill;
    bool finished;
};

struct vgm_context_t {
    struct vgm_header_t header;
    struct vgm_state_t state;
    struct vgm_stream_t stream;
    struct vgm_chip_bank_t chips;
};

struct vgm_context_t* vgm_load(
    const uint8_t * data,
    size_t size,
    struct vgm_chip_bank_t* chips);

void vgm_free(struct vgm_context_t* vgm);

void vgm_render(
    struct vgm_context_t* cxt,
    int16_t* dst,
    uint32_t samples);
