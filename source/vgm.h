#pragma once
#include <stdint.h>

#include "config.h"
#include "chip/chip.h"

#pragma pack(push, 1)
struct sVGMHeader {
    uint32_t vgm_ident;         // "Vgm " (0x56, 0x67, 0x6d, 0x20)
    uint32_t offset_eof;        // relative offset to end of file
    uint32_t version;
    uint32_t clock_sn76489;     // input clock rate in hz, typical 3579545
    uint32_t clock_ym2413;
    uint32_t offset_gd3;        // relative offset to GD3 tag; 0 if no tag;
    uint32_t total_samples;     // Total of all wait values in the file.
    uint32_t loop_offset;       // relative offset of the loop point, or 0 if no loop.
    uint32_t loop_samples;      // number of samples in one loop, or 0 if no loop
    uint32_t rate;              // rate of recording in hz. 50 pal, 60 ntsc;
    uint16_t feedback_sn76489;  // lfsr feedback pattern
    uint8_t  width_sn76489;     // lfsr width	- 15 sms2,gg,smd
                                //				- 16 bbcmicro,segacomputer3000
    uint8_t flags_sn76489;      // lfsr flags	- bit 0 frequency is 0x400
                                //				- bit 1 output negate flag
                                //				- bit 2 stereo
                                // on(0)/off(1)
                                //				- bit 3 /8 clock divider	on(0)/off(1)
    uint32_t _1;
    uint32_t _2;
    uint32_t offset_vgmdata;
    uint32_t _3;
};
#pragma pack(pop)

struct sVGMFile {
    uint8_t* raw;
    sVGMHeader* header;
    uint8_t* stream;
    vgm_chip_t *chip_;
    uint32_t spill;
    bool finished;
};

sVGMFile* vgm_load(const char* path);

void vgm_free(sVGMFile* vgm);

void vgm_render(sVGMFile* vgm, int16_t* out, uint32_t samples);
