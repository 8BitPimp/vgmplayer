#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <vgm.h>

struct vgm_fstream_t {
    struct vgm_stream_t stream;
    FILE* fd;
};

uint8_t vgm_fstream_read8(struct vgm_stream_t* stream)
{
    uint8_t out;
    struct vgm_fstream_t* fstream = (struct vgm_fstream_t*)stream;
    assert(fstream && fstream->fd);
    fread(&out, 1, sizeof(out), fstream->fd);
    return out;
}

uint16_t vgm_fstream_read16(struct vgm_stream_t* stream)
{
    uint16_t out;
    struct vgm_fstream_t* fstream = (struct vgm_fstream_t*)stream;
    assert(fstream && fstream->fd);
    fread(&out, 1, sizeof(out), fstream->fd);
    return out;
}

uint32_t vgm_fstream_read32(struct vgm_stream_t* stream)
{
    uint32_t out;
    struct vgm_fstream_t* fstream = (struct vgm_fstream_t*)stream;
    assert(fstream && fstream->fd);
    fread(&out, 1, sizeof(out), fstream->fd);
    return out;
}

void vgm_fstream_read(struct vgm_stream_t* stream, void* dst, uint32_t size)
{
    struct vgm_fstream_t* fstream = (struct vgm_fstream_t*)stream;
    assert(fstream && fstream->fd);
    fread(dst, 1, size, fstream->fd);
}

void vgm_fstream_skip(struct vgm_stream_t* stream, uint32_t size)
{
    struct vgm_fstream_t* fstream = (struct vgm_fstream_t*)stream;
    assert(fstream && fstream->fd);
    fseek(fstream->fd, size, SEEK_CUR);
}

bool vgm_fstream_open(const char* path, struct vgm_fstream_t* out)
{
    assert(path && out);
    out->fd = fopen(path, "rb");
    if (!out->fd) {
        return false;
    }
    out->stream.read = vgm_fstream_read;
    out->stream.read8 = vgm_fstream_read8;
    out->stream.read16 = vgm_fstream_read16;
    out->stream.read32 = vgm_fstream_read32;
    out->stream.skip = vgm_fstream_skip;
    return true;
}

static void _vgm_write_ym3812(
    struct vgm_chip_t* chip,
    uint32_t port,
    uint32_t reg,
    uint32_t val)
{
    printf("%02x %02x %02x\n", port, reg, val);
}

static void _vgm_mute_ym3812(
    struct vgm_chip_t* chip)
{
}

void delay_ms(uint32_t ms) {
    extern void __stdcall Sleep(unsigned long ms);
    Sleep(ms);
}

int main(const int argc, char** args)
{
    if (argc < 2) {
        return 1;
    }

    struct vgm_fstream_t fstream;
    struct vgm_stream_t* stream = &(fstream.stream);

    if (!vgm_fstream_open(args[1], &fstream)) {
        return 2;
    }

    struct vgm_chip_t chip_ym3812 = {
        NULL,
        _vgm_write_ym3812,
        NULL,
        _vgm_mute_ym3812,
        NULL
    };

    struct vgm_chip_bank_t chips;
    memset(&chips, 0, sizeof(chips));
    chips.ym3812 = &chip_ym3812;

    struct vgm_context_t* vgm = vgm_load(stream, &chips);
    if (!vgm) {
        return 3;
    }

    while (vgm_advance(vgm)) {
        const uint32_t delay = vgm_delay(vgm);
        if (delay > 0) {
            delay_ms(delay);
        }
    }
    return 0;
}
