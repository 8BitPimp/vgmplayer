#pragma once

#if HAS_STDINT
#include <stdint.h>
#else
typedef unsigned long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

typedef signed int int32_t;
typedef signed short int16_t;
#endif // HAS_STDINT

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

    /* [VGM 1.01 additions:] */

    // rate of recording in hz. 50 pal, 60 ntsc;
    uint32_t rate;

    /* [VGM 1.10 additions:] */

    // lfsr feedback pattern
    uint16_t feedback_sn76489;
    // lfsr width	- 15 sms2,gg,smd
    //				- 16 bbcmicro,segacomputer3000
    uint8_t width_sn76489;

    /* [VGM 1.51 additions:] */

    // lfsr flags	- bit 0 frequency is 0x400
    //				- bit 1 output negate flag
    //				- bit 2 stereo
    // on(0)/off(1)
    //				- bit 3 /8 clock divider	on(0)/off(1)
    uint8_t flags_sn76489;

    /* [VGM 1.10 additions:] */

    uint32_t clock_ym2612;
    uint32_t clock_ym2151;

    /* [VGM 1.50 additions:] */

    // file offset to start of vgm data
    uint32_t offset_vgmdata;

    /* [VGM 1.51 additions:] */

    uint32_t clock_sega_pcm;
};
#pragma pack(pop)

typedef void (*vgm_chip_write_t)(struct vgm_chip_t*, uint32_t port, uint32_t reg, uint32_t data);
typedef void (*vgm_chip_mute_t)(struct vgm_chip_t* chip);


struct vgm_chip_t {
    void (*set_clock)(uint32_t clock);
    vgm_chip_write_t write;
    void (*render)(int16_t* dst, uint32_t samples);
    vgm_chip_mute_t mute;
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
    uint8_t (*read8)(struct vgm_stream_t*);
    uint16_t (*read16)(struct vgm_stream_t*);
    uint32_t (*read32)(struct vgm_stream_t*);
    void (*read)(struct vgm_stream_t*, void* dst, uint32_t size);
    void (*skip)(struct vgm_stream_t*, uint32_t size);
};

struct vgm_state_t {
    int32_t delay;
    bool finished;
};

struct vgm_context_t {
    struct vgm_header_t header;
    struct vgm_state_t state;
    struct vgm_stream_t* stream;
    struct vgm_chip_bank_t chips;
};

struct vgm_context_t* vgm_load(
    struct vgm_stream_t* stream,
    struct vgm_chip_bank_t* chips);

void vgm_free(struct vgm_context_t* vgm);

bool vgm_advance(
    struct vgm_context_t* vgm);

uint32_t vgm_delay(
    struct vgm_context_t* vgm);

void vgm_mute(
    struct vgm_context_t* vgm);
