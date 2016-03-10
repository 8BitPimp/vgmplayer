#include <string.h>
#include <array>

#include "../assert.h"
#include "../sound/sound.h"

#include "chip.h"

#define BOOL(X) ((X)!=0)

namespace
{

const float C_CLOCK_NTSC = 1789773.f; // 1.79MHz
const float C_CLOCK_PAL  = 1662607.f; // 1.66MHz

const uint32_t frame_rate_ = 184;

// duty cycle lookup table
const std::array<float, 4> g_duty = {
    .12f, .25f, .5f, .75f
};

const std::array<uint8_t, 32> g_length_value = {
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

const std::array<uint16_t, 16> g_noise_period_pal = {
    4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708, 944, 1890, 3778
};

const std::array<uint16_t, 16> g_noise_period_ntsc = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

int32_t minv(int32_t a, int32_t b) {
    return (a<b) ? a : b;
}

struct nes_reg_t
{
    std::array<uint8_t, 0x18> data_;

    struct dirty_t {
        bool pulse_1_ : 1;
        bool pulse_2_ : 1;
        bool triangle_ : 1;
        bool noise_ : 1;
        bool reg_4017_ : 1;
    }
    dirty_;

    uint8_t voice_duty(uint32_t voice) const {
        assert(voice<2);
        return (data_[voice*4]&0xc0)>>6;
    }

    // pulse 1, pulse 2, noise
    uint8_t voice_volume(uint32_t voice) const {
        assert(voice<2 || voice==3);
        return (data_[voice*4]&0xf);
    }

    // pulse 1, pulse 2, noise
    bool voice_loop(uint8_t voice) const {
        assert(voice<2 || voice==3);
        return (data_[voice*4]&0x20) != 0;
    }

    // pulse 1, pulse 2, noise
    bool voice_const(uint32_t voice) const {
        assert(voice<2 || voice==3);
        return (data_[voice*4]&0x10) != 0;
    }

    // pulse 1, pulse 2
    bool voice_sweep_enable(uint32_t voice) const {
        assert(voice<2);
        return (data_[voice*4+1]&0x80) != 0;
    }

    // pulse 1, pulse 2
    uint8_t voice_sweep_period(uint32_t voice) const {
        assert(voice<2);
        return (data_[voice*4+1]&0x70)>>4;
    }

    bool voice_sweep_neg(uint32_t voice) const {
        assert(voice<2);
        return (data_[voice*4+1]&0x08) != 0;
    }

    uint8_t voice_sweep_shift(uint32_t voice) const {
        assert(voice<2);
        return data_[voice*4+1]&0x07;
    }

    // pulse 1, pulse 2, tri
    uint32_t voice_timer(uint32_t voice) const {
        assert(voice<3);
        return data_[voice*4+2]|((data_[voice*4+3]&0x7)<<8);
    }

    // pulse 1, pulse 2, tri, noise
    uint8_t voice_length(uint32_t voice) const {
        assert(voice<4);
        return (data_[voice*4+3]&0xf8)>>3;
    }

    uint8_t noise_period() const {
        return data_[0xe]&0x0f;
    }

    bool noise_loop() const {
        return (data_[0xe]&0x80) != 0;
    }

    bool frame_mode() const {
        return (data_[0x17]&0x80)!=0;
    }
};

struct vgm_envelope_t
{
    int8_t counter_;
    int8_t divider_;
    int8_t period_;     // register[4000]&0x0f

    bool start_;        // this is implicitly set for a write to register[4003]
    bool loop_;         // register[4000]&0x20

    // clock from frame counter
    void clock() {
        // if the start flag is set
        if (start_) {
            start_ = false;
            counter_ = 15;
            divider_ = period_;
        }
        else {
            // if the divider has a transition
            if (--divider_==-1) {
                // reset the divider
                divider_ = period_;
                // if the counter is non zero
                if (counter_!=0) {
                    // decrement the counter
                    --counter_;
                }
                else {
                    // if the loop flag it set
                    if (loop_) {
                        // reload the counter
                        counter_ = 15;
                    }
                }
            }
        }
    }
};

struct vgm_chip_nes_apu_t: public vgm_chip_t
{
    nes_reg_t     reg_;

    // frame clock (when 0 a frame occurs)
    int32_t frame_;
    // the frame counter
    uint32_t frame_ix_;
    
    vgm_envelope_t env_[4];

    std::array<bool, 4> lhalt_;
    std::array<uint32_t, 4> lcounter_;

    sound_t  sound_;
    blit_t   pulse_[2];
    nestri_t triangle_;
    lfsr_t   lfsr_;

    void _pulse_1()
    {
        pulse_[0].set_duty(g_duty[reg_.voice_duty(0)]);
        uint32_t timer = reg_.voice_timer(0);
        pulse_[0].set_freq(C_CLOCK_NTSC/(16.f * float(timer)), 88200);
    }

    void _pulse_2()
    {
        pulse_[1].set_duty(g_duty[reg_.voice_duty(1)]);
        uint32_t timer = reg_.voice_timer(1);
        pulse_[1].set_freq(C_CLOCK_NTSC/(16.f * float(timer)), 88200);
    }

    void _triangle()
    {
        uint32_t timer = reg_.voice_timer(2);
        if (timer>0) {
            triangle_.set_freq(C_CLOCK_NTSC/(32.f * float(timer)), 88200);
        }
    }

    void _clock_env(int32_t ix)
    {
        env_[ix].clock();
    }

    void _clock_env_tri()
    {
    }

    void _clock_len_tri()
    {
    }

    void _clock_sweep_units()
    {
    }

    void _frame()
    {
        static const std::array<uint8_t, 11> g_sequence = {
            1, 3, 1, 3,     // 4 frame cycle
            3, 1, 3, 1, 0   // 5 frame cycle
        };

        uint8_t bits = reg_.frame_mode() ?
            g_sequence[frame_ix_%5 + 4] :
            g_sequence[frame_ix_%4];

        if (bits&1) {
            _clock_env(0);      // pulse 1
            _clock_env(1);      // pulse 2
            _clock_env(3);      // noise
            _clock_env_tri();   // triangle
        }

        if (bits&2) {
            for (uint32_t i = 0; i<4; ++i) {
                lcounter_[i] -= (lcounter_[i]>0)&&(!lhalt_[i]);
            }
            _clock_sweep_units();
        }

        ++frame_ix_;
    }
    
    vgm_chip_nes_apu_t(uint32_t clock)
        : vgm_chip_t(e_chip_nes_apu)
        , frame_(frame_rate_)
    {
    }

    virtual void init() override
    {
        memset(&reg_, 0, sizeof(nes_reg_t));
        frame_ = frame_rate_;
        sound_init(&sound_);
    }

    virtual void write(uint32_t reg, uint32_t data) override
    {
        assert(reg<0x18);
        reg_.data_[reg] = data;

        printf("[%02x] %02x\n", reg, data);

        reg_.dirty_.pulse_1_  = reg>=0x00 && reg<=0x03;
        reg_.dirty_.pulse_2_  = reg>=0x04 && reg<=0x07;
        reg_.dirty_.triangle_ = reg>=0x08 && reg<=0x0f;
        reg_.dirty_.reg_4017_ = reg==0x17;

        // ---- ---- ---- ---- PULSE 1 CHANNEL

        if (reg==0x00) {
            lhalt_[0]       = BOOL(data&0x10);
            env_[0].loop_   = BOOL(data&0x20);
            env_[0].period_ = (reg_.data_[0]&0xf)+1;
        }
        if (reg==0x03) {
            env_[0].start_  = true;
            lcounter_[0]    = g_length_value[reg_.voice_length(0)];
        }

        // ---- ---- ---- ---- PULSE 2 CHANNEL

        if (reg==0x04) {
            lhalt_[1]       = BOOL(data&0x10);
            env_[1].loop_   = BOOL(data&0x20);
            env_[1].period_ = (reg_.data_[4]&0xf)+1;
        }
        if (reg==0x07) {
            env_[1].start_  = true;
            lcounter_[1]    = g_length_value[reg_.voice_length(1)];
        }

        // ---- ---- ---- ---- TRIANGLE CHANNEL

        if (reg==0x08) {
            lhalt_[2]       = BOOL(data&0x80);
        }
        if (reg==0x0b) {
            lcounter_[2]    = g_length_value[reg_.voice_length(2)];
        }

        // ---- ---- ---- ---- NOISE CHANNEL

        if (reg==0x0c) {
            // triangle halt bit
            lhalt_[3]       = BOOL(data&0x10);
            env_[3].loop_   = BOOL(data&0x20);
        }
        if (reg==0x0e) {
            lfsr_.set_period(g_noise_period_pal[data&0xf]);
        }
        if (reg==0x0f) {
            env_[3].start_  = true;
            lcounter_[3] = g_length_value[reg_.voice_length(3)];
        }
        

        if (reg==0x17) {
            frame_ix_ = 0;
        }

        if (reg_.dirty_.pulse_1_) {
            _pulse_1();
            reg_.dirty_.pulse_1_ = true;
        }

        if (reg_.dirty_.pulse_2_) {
            _pulse_2();
            reg_.dirty_.pulse_2_ = false;
        }

        if (reg_.dirty_.triangle_) {
            _triangle();
            reg_.dirty_.triangle_ = false;
        }
    }

    virtual void render(int16_t * dst, uint32_t len) override
    {
        float p1 = 0.f, p2 = 0.f, tr = 0.f, nz = 0;

        if (reg_.data_[0x15]&0x01) {
            if (lcounter_[0]>0) {
                uint8_t vol = reg_.voice_const(0) ?
                    (reg_.data_[0x00]&0x0f) :
                    (env_[0].counter_);
                p1 = float(vol)/32.f;
            }
        }

        if (reg_.data_[0x15]&0x02) {
            if (lcounter_[1]>0) {
                uint8_t vol = reg_.voice_const(1) ?
                    (reg_.data_[0x04]&0x0f) :
                    (env_[1].counter_);
                p2 = float(vol)/32.f;
            }
        }        

        if (reg_.data_[0x15]&0x04) {
            if (lcounter_[2]>0) {
                tr = .5f;
            }
        }

        if (reg_.data_[0x15]&0x08) {
            if (lcounter_[3]>0) {
                uint8_t vol = reg_.voice_const(1) ?
                    (reg_.data_[0x0C]&0x0f) :
                    (env_[3].counter_);
                nz = float(vol)/32.f;
            }
        }

        pulse_[0].set_volume(p1);
        pulse_[1].set_volume(p2);
        triangle_.set_volume(tr);
        lfsr_.set_volume(nz);

        while (len) {

            uint32_t count = minv(frame_, len);
            len -= count;

            if (count > 0) {

                std::array<source_t, 5> source = {
                    source_t{sound_source_blit,   &pulse_[0], true, .6f},
                    source_t{sound_source_blit,   &pulse_[1], true, .6f},
                    source_t{sound_source_nestri, &triangle_, true, .8f},
                    source_t{sound_source_lfsr,   &lfsr_,     true, .1f},
                    source_t{nullptr, nullptr, false},
                };
                sound_render(&sound_, dst, count, &source[0]);
                dst += count;
            }

            if ((frame_ -= count) <= 0) {
                frame_ += frame_rate_;
                _frame();
            }
        }
    }

    virtual void silence() override
    {
    }
};

} // namespace {}

vgm_chip_t * chip_create_nes_apu(uint32_t clock)
{
    vgm_chip_nes_apu_t * chip = new vgm_chip_nes_apu_t(clock);
    chip->init();
    return chip;
}
