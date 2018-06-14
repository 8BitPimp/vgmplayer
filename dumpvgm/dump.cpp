#define _CRT_SECURE_NO_WARNINGS
#include <cassert>
#include <cstdio>
#include <cstring>

#include <string>

#include "vgm.h"
#include "vgm_fstream.h"

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

enum {
  MARK_EOF,
  MARK_DELAY,
  MARK_WRITE,
};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

struct vgm_ym3812_t : public vgm_chip_t {

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
        fputc(MARK_WRITE, _fd);
        fwrite(&port, 4, 1, _fd);
        fwrite(&reg, 4, 1, _fd);
        fwrite(&data, 4, 1, _fd);
    };

    void delay(uint32_t data)
    {
        fputc(MARK_DELAY, _fd);
        fwrite(&data, 4, 1, _fd);
    }

    void eof()
    {
        fputc(MARK_EOF, _fd);
    }

protected:
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
