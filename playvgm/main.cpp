#define _CRT_SECURE_NO_WARNINGS
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>

#include <windows.h>

#define _SDL_main_h
#include <SDL.h>

#include "vgm.h"
#include "vgm_fstream.h"

#include "../libchip/mame_sn76489/Sn76496.h"
#include "../libchip/mame_ym2612/Fm2612.h"

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

static const uint32_t OSC_NTSC = 53693100;
static const uint32_t OSC_PAL = 53203424;

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

vgm_chip_t* create_dev_serial(uint16_t port);
vgm_chip_t* create_dev_opl(uint16_t port);

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

struct vgm_playback_t {

    vgm_playback_t(vgm_t& vgm)
        : _vgm(vgm)
    {
    }

    void run()
    {
        const DWORD start = GetTickCount();
        uint32_t accum = 0;
        while (!_vgm.finished() && _vgm.advance()) {
            accum += _vgm.get_delay_ms();
            while ((GetTickCount() - start) < accum) {
                _yield();
            }
        }
    }

protected:
    void _yield()
    {
        extern void __stdcall Sleep(unsigned long ms);
        Sleep(0);
    }

    vgm_t& _vgm;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

struct vgm_render_t {

    vgm_render_t(vgm_t& vgm)
        : _vgm(vgm)
        , _delay(0)
        , _finished(false)
        , _error(0)
        , _gain(0xf000)
        , _gr_time(0)
    {
    }

    bool init(uint32_t rate)
    {
        _rate = rate;
        SDL_Init(SDL_INIT_AUDIO);

        SDL_AudioSpec spec = { 0 }, spec_out = { 0 };
        // set callback
        spec.callback = _trampoline;
        spec.userdata = this;
        // setup channel
        spec.channels = 2;
        spec.freq = _rate;
        spec.samples = 1024 * 4;
        spec.format = AUDIO_S16LSB;

        if (SDL_OpenAudio(&spec, &spec_out)) {
            // error
            return false;
        }
        // unpause audio
        SDL_PauseAudio(0);
        return true;
    }

    void stop() {
      SDL_CloseAudio();
      SDL_Quit();
    }

    bool finished() const
    {
        return _finished;
    }

    void run(int16_t* out, uint32_t samples)
    {
        static const uint32_t MASK = ~0x1u;
        std::array<int32_t, 1024> mixdown;
        mixdown.fill(0);
        // while there are samples to render
        while (samples && !_finished) {
            // find max samples we can render
            const uint32_t chunk = std::min<uint32_t>({ _delay, samples, mixdown.size() }) & MASK;
            // render any needed samples
            if (chunk > 1) {
                _render(mixdown.data(), chunk);
                //
                _redux(mixdown.data(), out, chunk);
                // track samples we have rendered
                out += chunk;
                samples -= chunk;
                _delay -= chunk;

                for (uint32_t i = 0; i < chunk; ++i) {
                    mixdown[i] = 0;
                }
            }
            // advance vgm stream if we need to
            if (_delay <= 1 || chunk <= 1) {
                _vgm.advance();
                _finished = _vgm.finished();
                _delay += _vgm.get_delay_samples() * 2;
            }
        }
    }

protected:
    void _redux(const int32_t* src, int16_t* dst, uint32_t count)
    {
        for (; count; --count, ++src, ++dst) {
            // very basic automatic gain reduction
            const int32_t x = (*src * _gain) >> 12;
            _gain = std::max<int32_t>(_gain - ((x > 0x7fff) ? (1 | ((x - 0x7fff) >> 4)) : 0), 0);
            *dst = std::min<int32_t>(
                std::max<int32_t>(x, -0x7ffe),
                0x7ffe);
            if ((count & 0x3f) == 0) {
                _gain += 1;
            }
        }
    }

    void _render(int32_t* out, uint32_t samples)
    {
        if (const auto chip = _vgm.chips().ym2612) {
            chip->render(out, samples);
        }
        if (const auto chip = _vgm.chips().sn76489) {
            chip->render(out, samples);
        }
    }

    static void SDLCALL _trampoline(void* userdata, Uint8* stream, int len)
    {
        vgm_render_t* self = (vgm_render_t*)userdata;
        int16_t* buffer = (int16_t*)stream;
        len /= 2;
        self->run(buffer, len);
    }

    int32_t _gain;
    uint32_t _gr_time;
    uint32_t _rate;
    bool _finished;
    vgm_t& _vgm;
    uint32_t _delay;
    uint32_t _error;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

struct chip_ym2612_t : public vgm_chip_t {

    chip_ym2612_t()
        : _inst(nullptr)
    {
        // 7.6Mhz
        _inst = ym2612_init(OSC_PAL / 7, 44100);
    }

    ~chip_ym2612_t() override{};

    void set_clock(uint32_t clock) override{
        //
    };

    void write(uint32_t port, uint32_t reg, uint32_t data) override
    {
        //
        if (_inst) {

            // XXX: Port is not always 0
            // is this the bank switching?

            ym2612_write(_inst, port, reg, data);
        }
    };

    void render(int16_t* dst, uint32_t samples) override
    {
        if (!_inst) {
            return;
        }
        std::array<int32_t, 512> buffer;
        while (samples) {
            // samples remaining
            uint32_t todo = std::min<uint32_t>(buffer.size(), samples);
            samples -= todo;
            // render a number of samples
            ym2612_render(_inst, buffer.data(), todo / 2, false);
            // mixdown
            for (uint32_t i = 0; i < todo; ++i) {
                dst[i] += int16_t(buffer[i]);
            }
            dst += todo;
        }
    }

    void render(int32_t* dst, uint32_t samples) override
    {
        if (!_inst) {
            return;
        }
        ym2612_render(_inst, dst, samples / 2, true);
    }

    void mute() override{
        //
    };

protected:
    void* _inst;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

struct chip_sn76489_t : public vgm_chip_t {

    chip_sn76489_t()
        : _inst(nullptr)
    {
        // 3.5Mhz
        static const uint32_t OSC_NTSC = 53693100;
        static const uint32_t OSC_PAL = 53203424;
        _inst = segapsg_init(OSC_PAL / 15, 44100, false);
        segapsg_set_gain(_inst, 256);
    }

    ~chip_sn76489_t() override{};

    void set_clock(uint32_t clock) override{
        //
    };

    void write(uint32_t port, uint32_t reg, uint32_t data) override
    {
        if (_inst) {
            segapsg_write_register(_inst, data);
        }
    };

    void render(int16_t* dst, uint32_t samples) override
    {
        if (!_inst) {
            return;
        }
        std::array<int32_t, 512> buffer;
        while (samples) {
            // samples remaining
            uint32_t todo = std::min<uint32_t>(buffer.size(), samples);
            samples -= todo;
            // render a number of samples
            segapsg_render(_inst, buffer.data(), todo / 2, false);
            // mixdown
            for (uint32_t i = 0; i < todo; ++i) {
                dst[i] += int16_t(buffer[i]);
            }
            dst += todo;
        }
    }

    void render(int32_t* dst, uint32_t samples) override
    {
        if (!_inst) {
            return;
        }
        segapsg_render(_inst, dst, samples / 2, true);
    }

    void mute() override{
        //
    };

protected:
    void* _inst;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

struct chip_sn76489_serial_t : public vgm_chip_t {

    chip_sn76489_serial_t()
        : _inst(nullptr)
    {
    }

    void write(uint32_t port, uint32_t reg, uint32_t data) override
    {
        if (_inst) {
            printf("%02x\n", data);
        }
    };

protected:
    void* _inst;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

int main(const int argc, char** args)
{
    if (argc < 2) {
        // well we need a song to play
        return 1;
    }

    vgm_fstream_t stream(args[1]);
    if (!stream.valid()) {
        return 1;
    }

    bool playback = false;
    if (playback) {
        // playback

        vgm_chip_bank_t bank;
        bank.sn76489 = new chip_sn76489_serial_t;

        vgm_t vgm;
        if (!vgm.init(&stream, &bank)) {
            return 1;
        }

        vgm_playback_t playback(vgm);
        playback.run();

    } else {
        // render

        vgm_chip_bank_t bank;
        bank.ym2612 = new chip_ym2612_t;
        bank.sn76489 = new chip_sn76489_t;

        vgm_t vgm;
        if (!vgm.init(&stream, &bank)) {
            return 1;
        }

        vgm_render_t render(vgm);
        if (!render.init(44100)) {
            return 1;
        }
        while (!render.finished()) {
            SDL_Delay(1);
        }

        render.stop();
    }

    return 0;
}
