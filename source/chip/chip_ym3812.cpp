#include <stdint.h>
#include <array>

#include "chip.h"
#include "../assert.h"
#include "../ym/ym3812.h"

namespace
{

template <typename type_t>
type_t _min(type_t a, type_t b)
{
    return (a<b) ? a : b;
}

uint32_t ym_exp(int32_t index)
{
    /*
    * When such a table is used for calculation of the exponential, the table
    * is read at the position given by the 8 LSB's of the input. The value +
    * 1024 (the hidden bit) is then the significand of the floating point output
    * and the yet unused MSB's of the input are the exponent of the floating
    * point output. Indeed, YM3812 sends the audio to the YM3014B DAC in floating
    * point, so it is quite possible that summing of voices is done in floating
    * point also.
    **/

    static const std::array<uint16_t, 256> t_ym_exp = {
        0   , 3   , 6   , 8   , 11  , 14  , 17  , 20  ,
        22  , 25  , 28  , 31  , 34  , 37  , 40  , 42  ,
        45  , 48  , 51  , 54  , 57  , 60  , 63  , 66  ,
        69  , 72  , 75  , 78  , 81  , 84  , 87  , 90  ,
        93  , 96  , 99  , 102 , 105 , 108 , 111 , 114 ,
        117 , 120 , 123 , 126 , 130 , 133 , 136 , 139 ,
        142 , 145 , 148 , 152 , 155 , 158 , 161 , 164 ,
        168 , 171 , 174 , 177 , 181 , 184 , 187 , 190 ,
        194 , 197 , 200 , 204 , 207 , 210 , 214 , 217 ,
        220 , 224 , 227 , 231 , 234 , 237 , 241 , 244 ,
        248 , 251 , 255 , 258 , 262 , 265 , 268 , 272 ,
        276 , 279 , 283 , 286 , 290 , 293 , 297 , 300 ,
        304 , 308 , 311 , 315 , 318 , 322 , 326 , 329 ,
        333 , 337 , 340 , 344 , 348 , 352 , 355 , 359 ,
        363 , 367 , 370 , 374 , 378 , 382 , 385 , 389 ,
        393 , 397 , 401 , 405 , 409 , 412 , 416 , 420 ,
        424 , 428 , 432 , 436 , 440 , 444 , 448 , 452 ,
        456 , 460 , 464 , 468 , 472 , 476 , 480 , 484 ,
        488 , 492 , 496 , 501 , 505 , 509 , 513 , 517 ,
        521 , 526 , 530 , 534 , 538 , 542 , 547 , 551 ,
        555 , 560 , 564 , 568 , 572 , 577 , 581 , 585 ,
        590 , 594 , 599 , 603 , 607 , 612 , 616 , 621 ,
        625 , 630 , 634 , 639 , 643 , 648 , 652 , 657 ,
        661 , 666 , 670 , 675 , 680 , 684 , 689 , 693 ,
        698 , 703 , 708 , 712 , 717 , 722 , 726 , 731 ,
        736 , 741 , 745 , 750 , 755 , 760 , 765 , 770 ,
        774 , 779 , 784 , 789 , 794 , 799 , 804 , 809 ,
        814 , 819 , 824 , 829 , 834 , 839 , 844 , 849 ,
        854 , 859 , 864 , 869 , 874 , 880 , 885 , 890 ,
        895 , 900 , 906 , 911 , 916 , 921 , 927 , 932 ,
        937 , 942 , 948 , 953 , 959 , 964 , 969 , 975 ,
        980 , 986 , 991 , 996 , 1002, 1007, 1013, 1018,
    };

    return t_ym_exp[index%t_ym_exp.size()];
}

uint32_t ym_logsin(int32_t index)
{
    static const std::array<uint16_t, 256> t_ym_logsin =
    {
        2137, 1731, 1543, 1419, 1326, 1252, 1190, 1137,
        1091, 1050, 1013, 979 , 949 , 920 , 894 , 869 ,
        846 , 825 , 804 , 785 , 767 , 749 , 732 , 717 ,
        701 , 687 , 672 , 659 , 646 , 633 , 621 , 609 ,
        598 , 587 , 576 , 566 , 556 , 546 , 536 , 527 ,
        518 , 509 , 501 , 492 , 484 , 476 , 468 , 461 ,
        453 , 446 , 439 , 432 , 425 , 418 , 411 , 405 ,
        399 , 392 , 386 , 380 , 375 , 369 , 363 , 358 ,
        352 , 347 , 341 , 336 , 331 , 326 , 321 , 316 ,
        311 , 307 , 302 , 297 , 293 , 289 , 284 , 280 ,
        276 , 271 , 267 , 263 , 259 , 255 , 251 , 248 ,
        244 , 240 , 236 , 233 , 229 , 226 , 222 , 219 ,
        215 , 212 , 209 , 205 , 202 , 199 , 196 , 193 ,
        190 , 187 , 184 , 181 , 178 , 175 , 172 , 169 ,
        167 , 164 , 161 , 159 , 156 , 153 , 151 , 148 ,
        146 , 143 , 141 , 138 , 136 , 134 , 131 , 129 ,
        127 , 125 , 122 , 120 , 118 , 116 , 114 , 112 ,
        110 , 108 , 106 , 104 , 102 , 100 , 98  , 96  ,
        94  , 92  , 91  , 89  , 87  , 85  , 83  , 82  ,
        80  , 78  , 77  , 75  , 74  , 72  , 70  , 69  ,
        67  , 66  , 64  , 63  , 62  , 60  , 59  , 57  ,
        56  , 55  , 53  , 52  , 51  , 49  , 48  , 47  ,
        46  , 45  , 43  , 42  , 41  , 40  , 39  , 38  ,
        37  , 36  , 35  , 34  , 33  , 32  , 31  , 30  ,
        29  , 28  , 27  , 26  , 25  , 24  , 23  , 23  ,
        22  , 21  , 20  , 20  , 19  , 18  , 17  , 17  ,
        16  , 15  , 15  , 14  , 13  , 13  , 12  , 12  ,
        11  , 10  , 10  , 9   , 9   , 8   , 8   , 7   ,
        7   , 7   , 6   , 6   , 5   , 5   , 5   , 4   ,
        4   , 4   , 3   , 3   , 3   , 2   , 2   , 2   ,
        2   , 1   , 1   , 1   , 1   , 1   , 1   , 1   ,
        0   , 0   , 0   , 0   , 0   , 0   , 0   , 0   ,
    };
    return t_ym_logsin[index%t_ym_logsin.size()];
}


int32_t ym_lookup(uint32_t phase1, uint32_t phase2, uint32_t gain1, uint32_t gain2)
{
    // sign bit is xor between half cycles of each phase
    uint32_t sign = ((phase1&0x200)^(phase2&0x200))>>9;

    // flip bits every quarter to reverse index
    uint32_t p1i = phase1^((phase1&0x100)-1);
    // phase logsin table lookup
    uint32_t p1l = ym_logsin(p1i); // <--- this will definately overflow the logsin table ??
    // phase exponential transform
    uint32_t p1e = ym_exp(p1l)+gain1;

    // <--- sign transform p1e here?

    // flip bits every quarter to reverse index
    uint32_t p2i = phase2^((phase2&0x100)-1);
    // phase logsin table lookup
    uint32_t p2l = ym_logsin(p2i+p1e); // <--- this will definately overflow the logsin table ??
    // phase exponential transform
    uint32_t p2e = ym_exp(p2l)+gain2;

    // flip all bits depending on sign
    return p2e^(sign-2);
}


/* 
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


/*
**/
float _lerp(float a, float b, float i)
{
    return a+(b-a) * i;
}

} // namespace {}

struct vgm_chip_3812_t : public vgm_chip_t
{
    OPLEmul * opl_;
    uint64_t dither_;
    float dc_;

    vgm_chip_3812_t()
        : vgm_chip_t(e_chip_ym3812)
        , opl_(nullptr)
        , dither_(0.f)
        , dc_(0.f)
    {}

    virtual void init() override
    {
        opl_->Reset();
        dither_ = 1;
        dc_ = 0.f;
    }

    virtual void write(uint32_t reg, uint32_t data) override
    {
        opl_->WriteReg(reg, data);
    }

    virtual void render(int16_t * dst, uint32_t len) override
    {
        const float c_gain = float(0x7000);

        std::array<float, 1024> buffer_;

        while (len) {

            uint32_t count = _min<uint32_t>(buffer_.size(), len);
            len -= count;

            opl_->Update(&buffer_[0], count);

            for (uint32_t i = 0; i<count; ++i, ++dst) {

                float a = buffer_[i] * .8f;
                // remove any DC offset
                dc_ = _lerp(dc_, a, 0.0005f);
                // prepare for bit conversion
                float b = _clip(a-dc_);
                float c = b * c_gain;
                float d = c+_dither(dither_);
                // convert to integer
                *dst = int16_t(d-dc_);
            }
        }
    }

    virtual void silence() override
    {
        opl_->Reset();
    }
};


vgm_chip_t * chip_create_ym3812(uint32_t clock)
{
    vgm_chip_3812_t * chip = new vgm_chip_3812_t();

    chip->opl_ = JavaOPLCreate(false);
    assert(chip->opl_);

    chip->init();
    return chip;
}
