#pragma once
#include <stdint.h>

#pragma pack(push, 1)
struct vgm_header_t {
    union {
        uint8_t _raw[256];
        struct {
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
            // lfsr width
            // - 15 sms2,gg,smd
            //- 16 bbcmicro,segacomputer3000
            uint8_t width_sn76489;

            /* [VGM 1.51 additions:] */

            // lfsr flags
            // - bit 0 frequency is 0x400
            // - bit 1 output negate flag
            // - bit 2 stereo on(0)/off(1)
            // - bit 3 /8 clock divider	on(0)/off(1)
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
    };
};
#pragma pack(pop)
