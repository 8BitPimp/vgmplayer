#include <string.h>

#include "../assert.h"
#include "../config.h"
#include "chip.h"

#include "../sound/sound.h"

namespace
{

struct sn76489_t
{
    sound_t sound_;
    blit_t pulse_[3];

    uint32_t delta[3];    // deltas
    uint16_t tone[3];     // tone register

    uint16_t noise_sr;
    uint16_t noise_div;
    uint16_t noise_rate;
    float noise_volume_;

    // mode (1: white noise) (0: periodic)
    uint8_t noise_md;

    // previous registers
    uint8_t prev_reg;

    // master clock frequency
    uint32_t clock;
};

enum
{
    _NC = 0x60,   // noise control register value
    _LB = 1<<7,   // latch bit
    _FB = 1<<2,   // noise feedback bit
    _R0 = 1<<6,   // register values
    _R1 = 1<<5,   // "
    _R2 = 1<<4,   // "
};

static const std::array<uint16_t, 16> _volTab =
{
    0x7FFF, 0x65AC, 0x50C3, 0x4026,
    0x32F5, 0x287A, 0x2027, 0x19A8,
    0x1449, 0x101D, 0x0CCD, 0x0A2B,
    0x0813, 0x066A, 0x0518, 0x0000
};

uint16_t _parity(uint16_t x)
{
    x ^= x>>8;
    x ^= x>>4;
    x ^= x>>2;
    x ^= x>>1;
    return x&1;
};

void psg_noise(float * out,
               size_t length,
               void * user,
               float volume) {

    assert((length&1)==0);

    sn76489_t * psg = (sn76489_t*)user;

    float vol = volume * psg->noise_volume_;

    while (length > 0) {

        const uint16_t c_size = 15;     // tandy = 14, other = 15
        const uint16_t c_taps = 0x0009; // genesis and game gear
#if 0
        const uint16_t c_taps = 0x0006;	// sega master system
        const uint16_t c_taps = 0x0003;	// bbc micro, colecovision
        const uint16_t c_taps = 0x0011;	// tandy 1000
#endif

        psg->noise_div += (psg->clock/SAMPLE_RATE);
        if (psg->noise_div>psg->noise_rate) {

            psg->noise_div -= psg->noise_rate;

            // get a nice ref to the shift register
            uint16_t &sr = psg->noise_sr;

            // generate white noise
            if (psg->noise_md & _FB)
                // white noise
                sr = (sr>>1)|(_parity(sr & c_taps)<<c_size);
            else
                // periodic noise
                sr = (sr>>1)|((sr&1)<<c_size);
        }

        out[0] += (psg->noise_sr&1) ? vol : -vol;
        out[1] += (psg->noise_sr&1) ? vol : -vol;

        out += 2;
        length -= 2;
    }
}

// compute oscillator delta for specific channel
void _compute_delta(sn76489_t* psg, uint8_t index)
{
    uint16_t tdat = psg->tone[index];

    float hz = 0.f;

    if (tdat>0) {
        hz = float(psg->clock)/(32.f*float(tdat));
    }
    
    psg->pulse_[index].set_freq(hz, 88200);
}

void _reset_noise(sn76489_t* psg, uint8_t data)
{
    uint16_t val[] = {0x010, 0x020, 0x040, uint16_t(psg->delta[1]>>16)};
    uint16_t div[] = {0x100, 0x200, 0x400, uint16_t(psg->delta[1]>>16)};
    // set noise shift register bits
    psg->noise_sr = val[data&0x3];
    // feedback bit
    psg->noise_md = data;
    // noise divider
    psg->noise_div = 0;
    psg->noise_rate = div[data&0x3];
}

void _set_vol(sn76489_t* psg, int32_t ix, int32_t data)
{
    float vol = float(_volTab[data&0x0F])/float(0xffff);
    if (ix<3) {
        psg->pulse_[ix].set_volume(vol);
    }
    else {
        psg->noise_volume_ = vol;
    }
}

void sn76489_write(sn76489_t* psg, uint8_t data)
{
    // check if latch bit is set
    if (data & _LB) {
        // save the previous register
        psg->prev_reg = data;

        // _R0 and _R1 form an index value
        int ix = (psg->prev_reg >> 5) & 0x3;

        // check R2 (signals attenuation)
        if (data & _R2) {
            _set_vol(psg, ix, data);
        }
        else {
            // noise control register
            if (ix == 0x03) {
                // noise register
                _reset_noise(psg, data);
            }
            // must be a tone frequency register
            else {
                // register is effectively cleared and low bits are set
                psg->tone[ix] = data & 0x0F;
                // compute the new delta value
                _compute_delta(psg, ix);

            } // \ check _NC
        } // \ check _R2
    }
    // continuation data
    else {
        // _R0 and _R1 form an index value
        int ix = (psg->prev_reg >> 5) & 0x3;

        // check _R2 (signals attenuation)
        if (psg->prev_reg & _R2) {
            _set_vol(psg, ix, data);
        }
        else {
            // noise control register
            if (ix == 0x03) {
                // noise register
                _reset_noise(psg, data);
            }
            // must be a tone frequency register
            else {
                // mask out high bits
                psg->tone[ix] &= 0x0F;
                // insert high bits
                psg->tone[ix] |= (data & 0x3F) << 4;
                // compute the new delta value
                _compute_delta(psg, ix);

            } // \ check _NC
        } // \ check _R2
    } // \latch bit
}

void sn76489_silence(sn76489_t* psg)
{
    psg->pulse_[0].set_volume(0.f);
    psg->pulse_[1].set_volume(0.f);
    psg->pulse_[2].set_volume(0.f);
}

void sn76489_init(sn76489_t* psg, uint32_t clock) 
{
    sound_init(&psg->sound_);

    psg->pulse_[0] = blit_t();
    psg->pulse_[1] = blit_t();
    psg->pulse_[2] = blit_t();
    
    psg->noise_volume_ = 0;

    psg->delta[0] = 0;
    psg->tone[0] = 0;
    psg->delta[1] = 0;
    psg->tone[1] = 0;
    psg->delta[2] = 0;
    psg->tone[2] = 0;

    psg->noise_sr = 1;
    psg->noise_div = 0;
    psg->noise_rate = 0;
    psg->noise_md = 1;
    psg->prev_reg = 0;
    psg->clock = clock;
}

void sn76489_render(struct sn76489_t* psg, int16_t* stream, int32_t samples)
{
    std::array<source_t, 5> source = {
        source_t{sound_source_blit, &psg->pulse_[0], true, .4f},
        source_t{sound_source_blit, &psg->pulse_[1], true, .4f},
        source_t{sound_source_blit, &psg->pulse_[2], true, .4f},
        source_t{psg_noise, psg, true, .3f},
        source_t{nullptr, nullptr, false},
    };
    sound_render(&psg->sound_, stream, samples, &source[0]);
    return;
}

struct vgm_chip_sn76489_t : public vgm_chip_t
{
    uint32_t clock_;
    sn76489_t psg_;

    vgm_chip_sn76489_t(uint32_t clock)
        : vgm_chip_t(e_chip_sn67489)
        , clock_(clock)
    {
        init();
    }

    virtual void init() override
    {
        sn76489_init(&psg_, clock_);
    }

    virtual void write(uint32_t reg, uint32_t data) override
    {
        sn76489_write(&psg_, data);
    }

    virtual void render(int16_t * dst, uint32_t len) override
    {
        sn76489_render(&psg_, dst, len);
    }

    virtual void silence() override
    {
        sn76489_silence(&psg_);
    }
};

} // namespace {}

vgm_chip_t * chip_create_sn76489(uint32_t clock)
{
    vgm_chip_sn76489_t * chip = new vgm_chip_sn76489_t(clock);
    chip->init();
    return chip;
}
