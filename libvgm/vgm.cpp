#define _CRT_SECURE_NO_WARNINGS
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "vgm.h"

#ifndef MIN
#define MIN(A, B) (((A) < (B)) ? (A) : (B))
#endif

void debug_msg(const char *fmt, ...) {}

// convert wait count to audio samples
static uint32_t inline _to_samples(uint32_t count)
{
    const uint64_t rate = 1 << 10;
    return (uint32_t)(((uint64_t)count * rate) >> 10);
}

static uint32_t _vgm_to_millis(const uint32_t samples)
{
    //XXX: this assumes 44100
    return (samples * 10) / 441;
}

void vgm_t::_vgm_data_block(
    uint8_t type,
    uint32_t size)
{
    //XXX: for now just skip the data block
    assert(_stream);
    _stream->skip(size);
}

static void _vgm_chip_write(
    struct vgm_chip_t* chip,
    uint32_t port,
    uint32_t reg,
    uint32_t data)
{
    if (chip) {
        chip->write(port, reg, data);
    }
}

static void _vgm_chip_mute(struct vgm_chip_t* chip)
{
    if (chip) {
        chip->mute();
    }
}

// silence all output from the output chips
void vgm_t::mute()
{
    _vgm_chip_mute(_chips.ym3812);
    _vgm_chip_mute(_chips.sn76489);
    _vgm_chip_mute(_chips.ym2612);
    _vgm_chip_mute(_chips.nes_apu);
    _vgm_chip_mute(_chips.gb_dmg);
    _vgm_chip_mute(_chips.pokey);
}

// parse a single item from the data stream
bool vgm_t::_vgm_parse_single(uint32_t* delay)
{
    assert(_stream);
    // parse this vgm opcode
    const uint8_t opcode = _stream->read8();
    switch (opcode) {
    case (0x4f):
    case (0x50): {
        // write to sn76489
        const uint8_t data1 = _stream->read8();
        _vgm_chip_write(_chips.sn76489, 0, 0, data1);
        break;
    }
    case (0x52): {
        // write to YM2612 PORT 1
        const uint8_t data1 = _stream->read8();
        const uint8_t data2 = _stream->read8();
        _vgm_chip_write(_chips.ym2612, 0, data1, data2);
        break;
    }
    case (0x53): {
        // write to YM2612 PORT 2
        const uint8_t data1 = _stream->read8();
        const uint8_t data2 = _stream->read8();
        _vgm_chip_write(_chips.ym2612, 1, data1, data2);
        break;
    }
    case (0x5A): {
        // write to ADLIB
        const uint8_t data1 = _stream->read8();
        const uint8_t data2 = _stream->read8();
        _vgm_chip_write(_chips.ym3812, 0, data1, data2);
        break;
    }
    case (0x61): {
        // wait dd dd samples
        const uint16_t samples = _stream->read16();
        *delay += _to_samples(samples);
        break;
    }
    case (0x62):
        // wait 735 samples
        *delay += _to_samples(735);
        break;
    case (0x63):
        // wait 882 sample
        *delay += _to_samples(882);
        break;
    case (0x66):
        // end sound data
        _finished = true;
        mute();
        break;
    case (0x67): {
        // data block
        uint8_t data1 = _stream->read8();
        assert(data1 == 0x66);
        const uint8_t tt = _stream->read8();
        const uint32_t ss = _stream->read32();
        _vgm_data_block(tt, ss);
        break;
    }
    case (0xB4): {
        // write to the NES APU
        const uint8_t data1 = _stream->read8();
        const uint8_t data2 = _stream->read8();
        _vgm_chip_write(_chips.nes_apu, 0, data1, data2);
        break;
    }
    case (0xB3): {
        // write to the GameBoy DMG
        const uint8_t data1 = _stream->read8();
        const uint8_t data2 = _stream->read8();
        _vgm_chip_write(_chips.gb_dmg, 0, data1, data2);
        break;
    }
    case (0xBB): {
        // write to the atari pokey
        const uint8_t data1 = _stream->read8();
        const uint8_t data2 = _stream->read8();
        _vgm_chip_write(_chips.pokey, 0, data1, data2);
        break;
    }
    case (0xE0): {
        // PCM data bank, block type 0
        const uint32_t data = _stream->read32();
        // todo
        break;
    }
    // reserved opcodes
    default:
        // reserved 1 byte operand range
        if (opcode >= 0x30 && opcode <= 0x3f) {
            _stream->skip(1);
            break;
        }
        // reserved 2 byte operand range
        if (opcode >= 0x40 && opcode <= 0x4E) {
            _stream->skip(2);
            break;
        }
        // reserved 2 byte operand range
        if (opcode >= 0xA1 && opcode <= 0xAF) {
            _stream->skip(2);
            break;
        }
        // reserved 2 byte operand range
        if (opcode >= 0xBC && opcode <= 0xBF) {
            _stream->skip(2);
            break;
        }
        // reserved 3 byte operand range
        if (opcode >= 0xC5 && opcode <= 0xCF) {
            _stream->skip(3);
            break;
        }
        // reserved 3 byte operand range
        if (opcode >= 0xD5 && opcode <= 0xDF) {
            _stream->skip(3);
            break;
        }
        // reserved 4 byte operand range
        if (opcode >= 0xE1 && opcode <= 0xFF) {
            _stream->skip(4);
            break;
        }
        // check for packed instructions
        {
            const uint8_t high_nibble = opcode & 0xF0;
            // single byte sample delay
            if (high_nibble == 0x70) {
                *delay += _to_samples((opcode & 0x0F) + 1);
                break;
            }
            if (high_nibble == 0x80) {
                *delay += _to_samples(opcode & 0x0F);
                break;
            }
        }
        // unknown opcode
        {
            debug_msg("unknown opcode: 0x%02x", (int)opcode);
            /* unknown opcode */
            _finished = true;
            mute();
            return false;
        }
    }
    return true;
}

bool vgm_t::load(
    struct vgm_stream_t* stream,
    struct vgm_chip_bank_t* chips)
{
    assert(stream && chips);
    _stream = stream;
    _chips = *chips;
    _finished = false;
    _delay = 0;
    memset(&_header, 0, sizeof(_header));
    // copy over the vgm header
    const size_t vgm_hdr_size = sizeof(struct vgm_header_t);
    _stream->read(&(_header), vgm_hdr_size);
    uint32_t to_skip = 0;
    // check VGM header
    if (memcmp(&(_header.vgm_ident), "Vgm ", 4) != 0) {
        return false;
    }
    // find start of music data
    if (_header.offset_vgmdata == 0) {
        const uint32_t start = 0x40;
        to_skip = start - vgm_hdr_size;
    } else {
        const struct vgm_header_t* header = &_header;
        const uint32_t start = 0x34 + _header.offset_vgmdata;
        to_skip = start - vgm_hdr_size;
    }
    // skip to start of vgm stream
    if (to_skip) {
        _stream->skip(to_skip);
    }
    return true;
}

bool vgm_t::advance()
{
#if 0
    // keep only error from last conversion
    {
        // convert to milliseconds
        // XXX: this assumes a 44100 sample rate
        const uint32_t ms = (_delay * 10) / 441;
        // convert back to samples and remove
        _delay -= ((ms * 441) / 10);
        assert(_delay >= 0);
    }
#else
    _delay = 0;
#endif
    uint32_t samples = 0;
    uint32_t watchdog = 1000;
    // while we have no new samples keep parsing
    while (samples == 0 && !_finished) {
        if (!_vgm_parse_single(&samples)) {
            _finished = true;
            return false;
        }
        assert(--watchdog);
    }
    // accumulate
    _delay += samples;
    return true;
}

uint32_t vgm_t::get_delay_ms()
{
    return _vgm_to_millis(_delay);
}

uint32_t vgm_t::get_delay_samples()
{
    return _delay;
}

bool vgm_t::finished()
{
    return _finished;
}
