#define _CRT_SECURE_NO_WARNINGS
#include <cassert>
#include <cstdio>
#include <cstring>

#include <windows.h>

#include "vgm.h"
#include "vgm_fstream.h"

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
    {
    }

    void render(int16_t* out, uint32_t samples)
    {
        // ...
    }

    void run(int16_t* out, uint32_t samples)
    {
        //        if (_delay <= 0) {
        //            _delay += _vgm.get_delay_samples();
        //        }
        // while there are samples to render
        while (samples) {
            // find max samples we can render
            const uint32_t chunk = min(_delay, samples);
            // render any needed samples
            if (chunk) {
                render(out, chunk);
                // track samples we have rendered
                out += chunk;
                samples -= chunk;
                _delay -= chunk;
            }
            // advance vgm stream if we need to
            if (_delay <= 0) {
                _vgm.advance();
                _delay += _vgm.get_delay_samples();
            }
        }
    }

protected:
    vgm_t& _vgm;
    uint32_t _delay;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

int main(const int argc, char** args)
{
    if (argc < 2) {
        //
    }

    vgm_fstream_t stream(args[1]);
    if (!stream.valid()) {
        //
    }

    vgm_chip_bank_t bank;
    bank.ym3812 = create_dev_serial(5);
    if (!bank.ym3812) {
        //
    }

    vgm_t vgm;
    if (!vgm.load(&stream, &bank)) {
      //
    }

    vgm_playback_t playback(vgm);
    playback.run();

    return 0;
}
