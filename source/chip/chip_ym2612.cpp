#include <stdint.h>
#include <array>

#include "chip.h"
#include "../assert.h"
#include "../ym/ym2612.h"

namespace
{

/* Minimum value
**/
template <typename type_t>
type_t _min(type_t a, type_t b)
{
    return (a<b) ? a : b;
}


/* Clamp value within range
**/
template <typename type_t>
type_t _clamp(type_t lo, type_t in, type_t hi)
{
    if (in<lo) return lo;
    if (in>hi) return hi;
    return in;
}


/* Return clipped input between [-1,+1]
**/
float _clip(float in)
{
    return _clamp(-1.f, in, 1.f);
}


/* 64 bit xor shift pseudo random number generator
**/
uint64_t _rand64(uint64_t & x)
{
    x ^= x>>12;
    x ^= x<<25;
    x ^= x>>27;
    return x;
}


/* Triangular noise distribution [-1,+1] tending to 0
**/
float _dither(uint64_t & x)
{
    static const uint32_t fmask = (1<<23)-1;
    union { float f; uint32_t i; } u, v;
    u.i = (uint32_t(_rand64(x)) & fmask)|0x3f800000;
    v.i = (uint32_t(_rand64(x)) & fmask)|0x3f800000;
    float out = (u.f+v.f-3.f);
    return out;
}


/* Linear Interpolation
**/
float _lerp(float a, float b, float i)
{
    return a+(b-a) * i;
}

} // namespace {}


struct vgm_chip_2612_t: public vgm_chip_t
{
    vgm_chip_2612_t()
        : vgm_chip_t(e_chip_ym2612)
    {}

    virtual void init() override
    {
        YM2612Init();
        YM2612Config(14);
        YM2612ResetChip();
    }

    virtual void write(uint32_t reg, uint32_t data) override
    {
        YM2612Write(reg, data);
    }

    virtual void render(int16_t * dst, uint32_t len) override
    {
        const float c_gain = float(0x7000);

        std::array<int32_t, 1024> buffer_;

        while (len) {

            uint32_t count = _min<uint32_t>(buffer_.size()/2, len);
            len -= count;

            YM2612Update(&buffer_[0], count);

            for (uint32_t i = 0; i<count; ++i, ++dst) {

                *dst = int16_t(buffer_[i*2]>>16);
            }
        }
    }

    virtual void silence() override
    {
        YM2612ResetChip();
    }
};


vgm_chip_t * chip_create_ym2612(uint32_t clock)
{
    vgm_chip_2612_t * chip = new vgm_chip_2612_t();
    
    chip->init();
    return chip;
}
