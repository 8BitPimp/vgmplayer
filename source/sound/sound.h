#pragma once
#include <stdint.h>
#include <array>

#include "decimate.h"

struct sound_t
{
    decimate_9_t decimate_;
    uint64_t     dither_;
    float        dc_;

    std::array<float, 1024> data_;


    sound_t()
        : decimate_()
        , dither_(1)
    {
        for (uint32_t i = 0; i<data_.size(); ++i)
            data_[i] = 0.f;
    }
};

struct pulse_t
{
    float freq_;
    float duty_;
    float period_;
    float offset_;
    float accum_;
    float volume_;

    pulse_t() {
        duty_       = .5f;
        volume_     = .0f;
        accum_      = .0f;
        period_     = 100.f;
        set_freq(100.f, 44100.f);
    }

    void set_duty(float duty) {
        duty_ = duty;
        offset_ = period_ * duty_;
    }

    void set_volume(float volume) {
        volume_ = volume;
    }

    void set_freq(float freq, float sample_rate) {
        freq_   = freq;
        period_ = sample_rate/freq;
        offset_ = period_ * duty_;
    }
};

struct nestri_t
{
    float accum_;
    float delta_;
    float volume_;

    nestri_t() {
        volume_ = .0f;
        accum_  = .0f;
        set_freq(100.f, 44100.f);
    }

    void set_volume(float volume) {
        volume_ = volume;
    }

    void set_freq(float freq, float sample_rate) {
        delta_ = (32.f/sample_rate) * freq;
    }
};

struct lfsr_t
{
    uint32_t lfsr_;
    uint32_t period_;
    uint32_t counter_;
    float    volume_;

    lfsr_t()
        : lfsr_(1)
        , period_(100)
        , counter_(100)
        , volume_(0.f)
    {
    }

    void set_volume(float volume) {
        volume_ = volume;
    }

    void set_period(uint32_t period) {
        period_ = period;
    }
};

struct blit_t
{
    static const size_t c_ring_size = 32;
    
    float   period_;
    float   duty_;
    float   volume_;

    float    accum_;
    float    out_;
    int32_t  edge_;
    float    hcycle_[2];
    uint32_t index_;
    std::array<float, c_ring_size> ring_;

    blit_t()
        : duty_(.5f)
        , out_(0.f)
        , edge_(0)
        , index_(0)
        , volume_(0.f)
        , accum_(0.f)
        , period_(1.f)
    {
        for (uint32_t i = 0; i<ring_.size(); ++i) {
            ring_[i] = 0.f;
        }
        hcycle_[0] = 1.f;
        hcycle_[1] = 1.f;
    }

    void set_duty(float duty) {
        duty_ = duty;
        hcycle_[0] = period_ * duty_;
        hcycle_[1] = period_ - hcycle_[0];
    }

    void set_freq(float freq, float sample_rate) {
        if (freq>0.f) {
            period_ = sample_rate/freq;
        }
        else {
            period_ = 1.f;
        }
        hcycle_[0] = period_ * duty_;
        hcycle_[1] = period_-hcycle_[0];
    }

    void set_volume(float volume) {
        volume_ = volume;
    }
};

struct source_t {

    void(*render_)(float * out, size_t length, void * user, float volume);
    void *user_;
    bool enable_;
    float volume_;
};

/* 
**/
void sound_init(sound_t * buffer);

/* Render into sound buffer at 2x oversample rate
**/
void sound_render(sound_t  * buffer,
                  int16_t  * out,
                  size_t     length,
                  source_t * source);

/* Simple Pulse Wave Generator
**/
void sound_source_pulse(float * out,
                        size_t length,
                        void * user,
                        float volume);

/* NES Noise Channel
**/
void sound_source_lfsr(float * out,
                       size_t length,
                       void * user,
                       float volume);

/* Band Limited Inpulse Train Pules Generator
**/
void sound_source_blit(float * out,
                       size_t length,
                       void * user,
                       float volume);

/* NES Triangle Channel
**/
void sound_source_nestri(float * out,
                         size_t length,
                         void * user,
                         float volume);
