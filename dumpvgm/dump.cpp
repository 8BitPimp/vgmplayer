#define _CRT_SECURE_NO_WARNINGS
#include <cassert>
#include <cstdio>
#include <cstring>

#include <string>
#include <vector>

#include "vgm.h"
#include "vgm_fstream.h"

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

enum {
  MARK_EOF,
  MARK_DELAY,
  MARK_WRITE,
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

#define NEW_FMT 1

struct vgm_ym3812_t : public vgm_chip_t {

    struct write_t {
      uint32_t reg;
      uint32_t data;
    };

    vgm_ym3812_t(const char* path)
        : _fd(fopen(path, "wb"))
    {
    }

    ~vgm_ym3812_t()
    {
        fclose(_fd);
    }

    bool valid()
    {
        return _fd != nullptr;
    }

    void write(uint32_t port, uint32_t reg, uint32_t data)
    {
        // todo: buffer writes so we can batch them and emit just one byte
        //       with the batch size followed by an implicit delay removing
        //       the need for any markers
#if NEW_FMT
        const write_t w = {
            reg, data
        };
        _writes.push_back(w);
#else
        _write(MARK_WRITE);
        _write(port);
        _write(reg);
        _write(data);
#endif
    };

    void delay(uint32_t samples)
    {
#if NEW_FMT
        if (samples) {
            _write(_writes.size());
            for (uint32_t i = 0; i < _writes.size(); ++i) {
                _write(_writes[i].reg);
                _write(_writes[i].data);
            }
            _writes.clear();
            _write(samples);
        }
#else
        _write(MARK_DELAY);
        _write(samples);
#endif
    }

    void eof()
    {
#if NEW_FMT
#else
        _write(MARK_EOF);
#endif
    }

protected:
    void _write(uint32_t value)
    {
        // todo: flip this to encode backwards to simplify decoder

        uint8_t out = 0;
        do {
            out = value & 0x7f;
            value >>= 7;
            if (value == 0) {
                out |= 0x80;
            }
            fwrite(&out, 1, 1, _fd);
        } while (value);
    }

    std::vector<write_t> _writes;

    FILE* _fd;
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

enum {
    RET_SUCCESS,
    RET_BAD_ARGS,
    RET_BAD_OUT_FILE,
    RET_BAD_STREAM,
    RET_BAD_LOAD,
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

int main(const int argc, char** args)
{
    if (argc < 2) {
        return RET_BAD_ARGS;
    }
    std::string out_file(args[1]);
    out_file += ".opl";

    vgm_ym3812_t chip(out_file.c_str());
    if (!chip.valid()) {
        return RET_BAD_OUT_FILE;
    }

    auto stream = std::make_unique<vgm_fstream_t>(args[1]);
    if (!stream->valid()) {
        return RET_BAD_STREAM;
    }

    struct vgm_chip_bank_t chips;
    memset(&chips, 0, sizeof(chips));
    chips.ym3812 = &chip;

    vgm_t vgm;
    if (!vgm.load(stream.get(), &chips)) {
        return RET_BAD_LOAD;
    }

    while (!vgm.finished()) {
        // find max samples we can render
        const uint32_t delay = vgm.get_delay_samples();
        if (delay) {
            chip.delay(delay);
        }
        // render any needed samples
        vgm.advance();
    }

    // write end marker
    chip.eof();

    return RET_SUCCESS;
}
