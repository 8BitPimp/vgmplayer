#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vgm.h"

const uint32_t C_VGM_SAMPLE_RATE = 44100;

#ifndef MIN
#define MIN(A, B) (((A) < (B)) ? (A) : (B))
#endif

#if 0
struct vgm_stream_t {
    void * user;
    uint8_t(*read8)();
    uint8_t(*peek8)();
    uint16_t(*read16)();
};
#endif

// read 16 bits from the input data state
static uint32_t read16(const uint8_t* data)
{
    return data[1] | (data[2] << 8);
}

// convert wait count to audio samples
static uint32_t inline _toSamples(uint32_t count)
{
    const uint64_t rate = 1 << 10;
    return (uint32_t)(((uint64_t)count * rate) >> 10);
}

// silence all output from the output chips
static void _mute(struct vgm_context_t* vgm)
{
    assert(vgm);
    if (vgm->chips.ym3812)
        vgm->chips.ym3812->mute();
    if (vgm->chips.sn76489)
        vgm->chips.sn76489->mute();
    if (vgm->chips.ym2612)
        vgm->chips.ym2612->mute();
    if (vgm->chips.nes_apu)
        vgm->chips.nes_apu->mute();
    if (vgm->chips.gb_dmg)
        vgm->chips.gb_dmg->mute();
    if (vgm->chips.pokey)
        vgm->chips.pokey->mute();
}

// delay for a number of sample
static void _vgm_render(struct vgm_context_t* vgm, const uint32_t count)
{
}

void _handle_data_block(struct vgm_context_t* vgm, const uint8_t* data, uint8_t type, uint32_t size)
{
    //XXX: need to implement data block handling
}

// return samples to render
static bool _vgm_parse(struct vgm_context_t* vgm, uint32_t* count)
{
    assert(vgm && count);
    const uint8_t* data = vgm->state.stream;
    assert(data);
    // while we have no samples to render keep parsing
    while (count == 0 && !vgm->state.finished) {
        // parse this vgm opcode
        const uint8_t opcode = data[0];
        switch (opcode) {
        case (0x4f):
        case (0x50):
            // write to sn76489
            if (vgm->chips.sn76489)
                vgm->chips.sn76489->write(0, 0, data[1]);
            data += 2;
            break;
        case (0x52):
            // write to YM2612 PORT 1
            if (vgm->chips.ym2612) {
                vgm->chips.ym2612->write(0, data[1], data[2]);
            }
            data += 3;
            break;
        case (0x53):
            // write to YM2612 PORT 2
            if (vgm->chips.ym2612) {
                vgm->chips.ym2612->write(1, data[1], data[2]);
            }
            data += 3;
            break;
        case (0x5A):
            // write to ADLIB
            if (vgm->chips.ym3812)
                vgm->chips.ym3812->write(0, data[1], data[2]);
            data += 3;
            break;
        case (0x61):
            // wait dd dd samples
            count += _toSamples(read16(data));
            data += 3;
            break;
        case (0x62):
            // wait 735 samples
            count += _toSamples(735);
            data += 1;
            break;
        case (0x63):
            // wait 882 sample
            count += _toSamples(882);
            data += 1;
            break;
        case (0x66):
            // end sound data
            vgm->state.finished = true;
            _mute(vgm);
            break;
        case (0x67): {
            // data block
            assert(data[1] == 0x66);
            const uint8_t tt = data[2];
            const uint32_t ss = *(uint32_t*)(data + 3);
            _handle_data_block(vgm, data + 7, tt, ss);
            // skip over the data block
            data += ss + 7;
            break;
        }
        case (0xB4):
            // write to the NES APU
            if (vgm->chips.nes_apu)
                vgm->chips.nes_apu->write(0, data[1], data[2]);
            data += 3;
            break;
        case (0xB3):
            // write to the GameBoy DMG
            if (vgm->chips.gb_dmg)
                vgm->chips.gb_dmg->write(0, data[1], data[2]);
            data += 3;
            break;
        case (0xBB):
            // write to the atari pokey
            if (vgm->chips.pokey)
                vgm->chips.pokey->write(0, data[1], data[2]);
            data += 3;
            break;
        // reserved opcodes
        default:
            // reserved 1 byte operand range
            if (opcode >= 0x30 && opcode <= 0x3f) {
                data += 2;
                continue;
            }
            // reserved 2 byte operand range
            if (opcode >= 0x40 && opcode <= 0x4E) {
                data += 3;
                continue;
            }
            // reserved 2 byte operand range
            if (opcode >= 0xA1 && opcode <= 0xAF) {
                data += 3;
                continue;
            }
            // reserved 2 byte operand range
            if (opcode >= 0xBC && opcode <= 0xBF) {
                data += 3;
                continue;
            }
            // reserved 3 byte operand range
            if (opcode >= 0xC5 && opcode <= 0xCF) {
                data += 4;
                continue;
            }
            // reserved 3 byte operand range
            if (opcode >= 0xD5 && opcode <= 0xDF) {
                data += 4;
                continue;
            }
            // reserved 4 byte operand range
            if (opcode >= 0xE1 && opcode <= 0xFF) {
                data += 5;
                continue;
            }
            // single byte sample delay
            if ((opcode & 0xF0) == 0x70) {
                count += _toSamples((data[0] & 0x0F) + 1);
                data += 1;
            } else {
                /* unknown opcode */
                vgm->state.finished = true;
                _mute(vgm);
            }
            break;
        }
    }
    vgm->state.stream = data;
    return !vgm->state.finished;
}

struct vgm_context_t* vgm_load(
    const uint8_t* data,
    size_t size,
    struct vgm_chip_bank_t* chips)
{
    assert(data && size && chips);
    // allocate a new vgm context
    struct vgm_context_t* vgm = malloc(sizeof(struct vgm_context_t));
    if (!vgm) {
        return NULL;
    }
    memset(vgm, 0, sizeof(struct vgm_context_t));
    // copy across chip emulators
    memcpy(&(vgm->chips), chips, sizeof(struct vgm_chip_bank_t));
    // copy data state
    vgm->state.raw = data;
    vgm->state.size = size;
    // copy over the vgm header
    memcpy(&vgm->header, data, sizeof(struct vgm_header_t));
    // find start of music data
    if (vgm->header.offset_vgmdata == 0) {
        const uint32_t header_size = 64;
        vgm->state.stream = data + header_size;
    } else {
        const uint32_t start = 0x34 + vgm->header.offset_vgmdata;
        vgm->state.stream = data + start;
    }
    return vgm;
}

void vgm_free(struct vgm_context_t* vgm)
{
    assert(vgm);
    free(vgm);
}

void vgm_render(struct vgm_context_t* cxt, int16_t* dst, uint32_t samples)
{
    uint32_t spill = cxt->state.spill;
    uint32_t watchdog = 10000;
    // while we have more space in the audio frame
    while (samples > 0 && !cxt->state.finished) {
        // how many samples can we render
        uint32_t count = MIN(samples, spill);
        // handle any spill between audio frames
        if (count) {
            // render the requested number of frames
            _vgm_render(cxt, count);
            // advance the sample state
            spill -= count;
            samples -= count;
            dst += count;
        }
        // parse the state to get new samples to render
        else {
            // add current samples to the spill counter
            _vgm_parse(cxt, &spill);
        }
        // test we are not in a runaway loop
        assert(--watchdog);
    }
    cxt->state.spill = spill;
}
