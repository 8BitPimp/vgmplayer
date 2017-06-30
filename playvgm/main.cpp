#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <windows.h>

#include "vgm.h"

#define SBBASE 0x220

struct vgm_fstream_t {
    struct vgm_stream_t stream;
    FILE* fd;
};

uint8_t vgm_fstream_read8(struct vgm_stream_t* stream)
{
    uint8_t out;
    struct vgm_fstream_t* fstream = (struct vgm_fstream_t*)stream;
    assert(fstream && fstream->fd);
    fread(&out, 1, 1, fstream->fd);
    return out;
}

uint16_t vgm_fstream_read16(struct vgm_stream_t* stream)
{
    uint16_t out;
    struct vgm_fstream_t* fstream = (struct vgm_fstream_t*)stream;
    assert(fstream && fstream->fd);
    fread(&out, 1, 2, fstream->fd);
    return out;
}

uint32_t vgm_fstream_read32(struct vgm_stream_t* stream)
{
    uint32_t out;
    struct vgm_fstream_t* fstream = (struct vgm_fstream_t*)stream;
    assert(fstream && fstream->fd);
    fread(&out, 1, 4, fstream->fd);
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
    out->stream.read   = vgm_fstream_read;
    out->stream.read8  = vgm_fstream_read8;
    out->stream.read16 = vgm_fstream_read16;
    out->stream.read32 = vgm_fstream_read32;
    out->stream.skip   = vgm_fstream_skip;
    return true;
}

#if defined(YM3812_SOUNDBLASTER)
// write to the Adlib chip on ISA bus
static void _port_write(uint32_t port, uint32_t data)
{
    __asm {
        push edx
        mov  edx, port
        mov  eax, data
        out  dx,  al
        pop  edx
    };
}

// delay using ISA port read cycles
static void _vgm_delay(uint32_t cycles)
{
    const uint32_t port = SBBASE + 0x7;
    __asm {
        push edx
        push ecx
        mov edx, port
        mov ecx, cycles
    top:
        in al, dx
        dec ecx
        test ecx, ecx
        jnz top
        pop ecx
        pop edx
    };
}

static void _vgm_write_ym3812(
    struct vgm_chip_t* chip,
    uint32_t port,
    uint32_t reg,
    uint32_t val)
{

    _port_write(SBBASE + 0x8, reg);
    _vgm_delay(6);
    _port_write(SBBASE + 0x9, val);
    _vgm_delay(36);
}

static void _vgm_mute_ym3812(
    struct vgm_chip_t* chip)
{
    for (uint32_t i=0; i<0xff; ++i) {
        _port_write(SBBASE + 0x8, i);
        _vgm_delay(6);
        _port_write(SBBASE + 0x9, 0);
        _vgm_delay(36);
    }
}

#else // defined(YM3812_SOUNDBLASTER)

static void _vgm_write_ym3812(
    struct vgm_chip_t* chip,
    uint32_t port,
    uint32_t reg,
    uint32_t val)
{
}

static void _vgm_mute_ym3812(
    struct vgm_chip_t* chip)
{
}

#endif // defined(YM3812_SOUNDBLASTER)

static void _yield()
{
    extern void __stdcall Sleep(unsigned long ms);
    Sleep(0);
}

void playback_loop(struct vgm_context_t* vgm)
{
    const DWORD start = GetTickCount();
    uint32_t accum = 0;
    while (vgm_advance(vgm)) {
        accum += vgm_delay(vgm);
        while ((GetTickCount()-start) < accum) {
            _yield();
        }
    }
}

int main(const int argc, char** args)
{
    struct vgm_chip_t chip_ym3812 = {
        NULL,
        _vgm_write_ym3812,
        NULL,
        _vgm_mute_ym3812,
        NULL
    };
    _vgm_mute_ym3812(&chip_ym3812);

    if (argc < 2) {
        return 1;
    }

    struct vgm_fstream_t fstream;
    struct vgm_stream_t* stream = &(fstream.stream);

    if (!vgm_fstream_open(args[1], &fstream)) {
        return 2;
    }
    printf("playing: '%s'\n", args[1]);

    struct vgm_chip_bank_t chips;
    memset(&chips, 0, sizeof(chips));
    chips.ym3812 = &chip_ym3812;

    struct vgm_context_t* vgm = vgm_load(stream, &chips);
    if (!vgm) {
        return 3;
    }

    playback_loop(vgm);
    getchar();
    return 0;
}
