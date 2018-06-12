#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vgm.h"

#ifndef MIN
#define MIN(A, B) (((A) < (B)) ? (A) : (B))
#endif

// convert wait count to audio samples
static uint32_t inline _toSamples(uint32_t count)
{
    const uint64_t rate = 1 << 10;
    return (uint32_t)(((uint64_t)count * rate) >> 10);
}

static uint32_t _vgm_to_millis(struct vgm_context_t* vgm, const uint32_t samples)
{
    return (samples * 10) / 441;
}

void _vgm_data_block(
    struct vgm_context_t* vgm,
    struct vgm_stream_t* stream,
    uint8_t type,
    uint32_t size)
{
    //XXX: for now just skip the data block
    stream->skip(stream, size);
}

static void _vgm_chip_write(
    struct vgm_chip_t* chip,
    uint32_t port,
    uint32_t reg,
    uint32_t data)
{
    if (chip) {
        assert(chip->write);
        chip->write(chip, port, reg, data);
    }
}

static void _vgm_chip_mute(struct vgm_chip_t* chip)
{
    if (chip) {
        assert(chip->mute);
        chip->mute(chip);
    }
}

// silence all output from the output chips
void vgm_mute(struct vgm_context_t* vgm)
{
    assert(vgm);
    _vgm_chip_mute(vgm->chips.ym3812);
    _vgm_chip_mute(vgm->chips.sn76489);
    _vgm_chip_mute(vgm->chips.ym2612);
    _vgm_chip_mute(vgm->chips.nes_apu);
    _vgm_chip_mute(vgm->chips.gb_dmg);
    _vgm_chip_mute(vgm->chips.pokey);
}

// parse a single item from the data stream
static bool _vgm_parse_single(struct vgm_context_t* vgm, uint32_t* delay)
{
    struct vgm_stream_t* stream = vgm->stream;
    assert(stream);
    // parse this vgm opcode
    const uint8_t opcode = vgm->stream->read8(stream);
    switch (opcode) {
    case (0x4f):
    case (0x50): {
        // write to sn76489
        const uint8_t data1 = stream->read8(stream);
        _vgm_chip_write(vgm->chips.sn76489, 0, 0, data1);
        break;
    }
    case (0x52): {
        // write to YM2612 PORT 1
        const uint8_t data1 = stream->read8(stream);
        const uint8_t data2 = stream->read8(stream);
        _vgm_chip_write(vgm->chips.ym2612, 0, data1, data2);
        break;
    }
    case (0x53): {
        // write to YM2612 PORT 2
        const uint8_t data1 = stream->read8(stream);
        const uint8_t data2 = stream->read8(stream);
        _vgm_chip_write(vgm->chips.ym2612, 1, data1, data2);
        break;
    }
    case (0x5A): {
        // write to ADLIB
        const uint8_t data1 = stream->read8(stream);
        const uint8_t data2 = stream->read8(stream);
        _vgm_chip_write(vgm->chips.ym3812, 0, data1, data2);
        break;
    }
    case (0x61): {
        // wait dd dd samples
        const uint16_t samples = stream->read16(stream);
        *delay += _toSamples(samples);
        break;
    }
    case (0x62):
        // wait 735 samples
        *delay += _toSamples(735);
        break;
    case (0x63):
        // wait 882 sample
        *delay += _toSamples(882);
        break;
    case (0x66):
        // end sound data
		printf("finished\n");
        vgm->state.finished = true;
        vgm_mute(vgm);
        break;
    case (0x67): {
        // data block
        uint8_t data1 = stream->read8(stream);
        assert(data1 == 0x66);
        const uint8_t tt = stream->read8(stream);
        const uint32_t ss = stream->read32(stream);
        _vgm_data_block(vgm, stream, tt, ss);
        break;
    }
    case (0xB4): {
        // write to the NES APU
        const uint8_t data1 = stream->read8(stream);
        const uint8_t data2 = stream->read8(stream);
        _vgm_chip_write(vgm->chips.nes_apu, 0, data1, data2);
        break;
    }
    case (0xB3): {
        // write to the GameBoy DMG
        const uint8_t data1 = stream->read8(stream);
        const uint8_t data2 = stream->read8(stream);
        _vgm_chip_write(vgm->chips.gb_dmg, 0, data1, data2);
        break;
    }
    case (0xBB): {
        // write to the atari pokey
        const uint8_t data1 = stream->read8(stream);
        const uint8_t data2 = stream->read8(stream);
        _vgm_chip_write(vgm->chips.pokey, 0, data1, data2);
        break;
    }
    // reserved opcodes
    default:
        // reserved 1 byte operand range
        if (opcode >= 0x30 && opcode <= 0x3f) {
            stream->skip(stream, 1);
			break;
        }
        // reserved 2 byte operand range
        if (opcode >= 0x40 && opcode <= 0x4E) {
            stream->skip(stream, 2);
			break;
        }
        // reserved 2 byte operand range
        if (opcode >= 0xA1 && opcode <= 0xAF) {
            stream->skip(stream, 2);
			break;
        }
        // reserved 2 byte operand range
        if (opcode >= 0xBC && opcode <= 0xBF) {
            stream->skip(stream, 2);
			break;
        }
        // reserved 3 byte operand range
        if (opcode >= 0xC5 && opcode <= 0xCF) {
            stream->skip(stream, 3);
			break;
        }
        // reserved 3 byte operand range
        if (opcode >= 0xD5 && opcode <= 0xDF) {
            stream->skip(stream, 3);
			break;
        }
        // reserved 4 byte operand range
        if (opcode >= 0xE1 && opcode <= 0xFF) {
            stream->skip(stream, 4);
			break;
        }
		// check for packed instructions
		{
			const uint8_t high_nibble = opcode & 0xF0;
			// single byte sample delay
			if (high_nibble == 0x70) {
				*delay += _toSamples((opcode & 0x0F) + 1);
				break;
			}
			if (high_nibble == 0x80) {
				*delay += _toSamples(opcode & 0x0F);
				break;
			}
		}
		// unknown opcode
		{
			printf("unknown opcode: 0x%02x", (int)opcode);
            /* unknown opcode */
            vgm->state.finished = true;
            vgm_mute(vgm);
            return false;
        }
    }
    return true;
}

struct vgm_context_t* vgm_load(
    struct vgm_stream_t* stream,
    struct vgm_chip_bank_t* chips)
{
    assert(stream && chips);
    // allocate a new vgm context
    struct vgm_context_t* vgm =
		(struct vgm_context_t*)malloc(sizeof(struct vgm_context_t));
    if (!vgm) {
        return NULL;
    }
    memset(vgm, 0, sizeof(struct vgm_context_t));
    // copy across chip emulators
    memcpy(&(vgm->chips), chips, sizeof(struct vgm_chip_bank_t));
    // copy data state
    vgm->stream = stream;
    // copy over the vgm header
    const size_t vgm_hdr_size = sizeof(struct vgm_header_t);
    vgm->stream->read(stream, &(vgm->header), vgm_hdr_size);
    uint32_t to_skip = 0;
    // check VGM header
    if (memcmp(&(vgm->header.vgm_ident), "Vgm ", 4) != 0) {
        return NULL;
    }
    // find start of music data
    if (vgm->header.offset_vgmdata == 0) {
        const uint32_t start = 0x40;
        to_skip = start - vgm_hdr_size;
    } else {
        const struct vgm_header_t* header = &vgm->header;
        const uint32_t start = 0x34 + vgm->header.offset_vgmdata;
        to_skip = start - vgm_hdr_size;
    }
    // skip to start of vgm stream
    if (to_skip) {
        vgm->stream->skip(stream, to_skip);
    }
    return vgm;
}

void vgm_free(struct vgm_context_t* vgm)
{
    assert(vgm);
    free(vgm);
}

bool vgm_advance(struct vgm_context_t* vgm)
{
    assert(vgm);
    // keep only error from last conversion
    {
        // convert to milliseconds
        const uint32_t ms = (vgm->state.delay * 10) / 441;
        // convert back to samples and remove
        vgm->state.delay -= ((ms * 441) / 10);
        assert(vgm->state.delay>=0);
    }
    uint32_t samples = 0;
    uint32_t watchdog = 1000;
    // while we have no new samples keep parsing
    while (samples == 0 && !vgm->state.finished) {
        if (!_vgm_parse_single(vgm, &samples)) {
            vgm->state.finished = true;
            return false;
        }
        assert(--watchdog);
    }
    // accumulate
    vgm->state.delay += samples;
    return true;
}

uint32_t vgm_delay(struct vgm_context_t* vgm)
{
    assert(vgm);
    return _vgm_to_millis(vgm, vgm->state.delay);
}

bool vgm_finished(struct vgm_context_t* vgm)
{
    return vgm->state.finished;
}
