#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>

#include "assert.h"
#include "vgm.h"
#include "gzip.h"
#include "config.h"

namespace {

const uint32_t C_VGM_SAMPLE_RATE = 44100;

// convert wait count to audio samples
uint32_t inline _toSamples(uint32_t count)
{
    //XXX: implement rate scaling
    return count;
}

// Gamegear/SegaMegadrive/BBC Micro
void _write_sn76489(sVGMFile* vgm, uint8_t data)
{
    if (!vgm->chip_) {
        // construct on demand
        uint32_t clock = vgm->header->clock_sn76489 & 0x7fffffffu;
        vgm->chip_ = chip_create_sn76489(clock);
    }
    assert(vgm->chip_);
    if (vgm->chip_->id_==e_chip_sn67489) {
        vgm->chip_->write(0, data);
    }
}

// Nintendo Entertainment System
void _write_nes(sVGMFile* vgm, uint8_t reg, uint8_t data)
{
    if (!vgm->chip_) {
        // construct on demand
        vgm->chip_ = chip_create_nes_apu(0);
    }
    assert(vgm->chip_);
    if (vgm->chip_->id_==e_chip_nes_apu) {
        vgm->chip_->write(reg, data);
    }
}

void _write_gb_dmg(sVGMFile* vgm, uint8_t reg, uint8_t data)
{
    //XXX: todo
}

void _write_pokey(sVGMFile* vgm, uint8_t reg, uint8_t data)
{
    //XXX: todo
}

// Soundblaster synth
void _write_ym3812(sVGMFile* vgm, uint8_t reg, uint8_t data)
{
    if (!vgm->chip_) {
        // construct on demand
        vgm->chip_ = chip_create_ym3812(0);
    }
    assert(vgm->chip_);
    if (vgm->chip_->id_==e_chip_ym3812) {
        vgm->chip_->write(reg, data);
    }
}

// Sega Genesis/Mega Drive synth
void _write_ym2612(sVGMFile* vgm, uint32_t port, uint8_t reg, uint8_t data)
{
    if (!vgm->chip_) {
        // construct on demand
        vgm->chip_ = chip_create_ym2612(0);
    }
    assert(vgm->chip_);
    if (vgm->chip_->id_==e_chip_ym2612) {
        vgm->chip_->write(reg, data);
    }
}

void _silence(sVGMFile* vgm)
{
    if (vgm->chip_) {
        vgm->chip_->silence();
    }
}

void _handle_data_block(sVGMFile* vgm, uint8_t *data, uint8_t type, uint32_t size)
{
    //XXX: need to implement data block handling
}

// return samples to render
bool _parse(sVGMFile* vgm, uint32_t & count)
{
    uint8_t * &data = vgm->stream;

    // while we have no samples to render keep parsing
    while (count==0 && !vgm->finished) {

        printf("%02x\n", data[0]);

        // parse this vgm opcode
        switch (uint8_t opcode = data[0]) {
        case (0x4f):
        case (0x50):
            // write to sn76489
            _write_sn76489(vgm, data[1]);
            data += 2;
            break;

        case (0x52) :
            // write to YM2612 PORT 1
            _write_ym2612(vgm, 0, data[1], data[2]);
            data += 3;
            break;

        case (0x53):
            // write to YM2612 PORT 2
            _write_ym2612(vgm, 1, data[1], data[2]);
            data += 3;
            break;

        case (0x5A) :
            _write_ym3812(vgm, data[1], data[2]);
            data += 3;
            break;

        case (0x61):
            // wait dd dd samples
            count += _toSamples(data[1]|(data[2]<<8));
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
            vgm->finished = true;
            _silence(vgm);
            break;

        case (0x67):
            {
            // data block
            assert(data[1]==0x66);
            uint8_t  tt = data[2];
            uint32_t ss = *(uint32_t*)(data+3);
            _handle_data_block(vgm, data+7, tt, ss);
            // skip over the data block
            data += ss+7;
            break;
            }

        case (0xB4):
            // write to the NES APU
            _write_nes(vgm, data[1], data[2]);
            data += 3;
            break;

        case (0xB3):
            // write to the GameBoy DMG
            _write_gb_dmg(vgm, data[1], data[2]);
            data += 3;
            break;

        case (0xBB):
            // write to the atari pokey
            _write_pokey(vgm, data[1], data[2]);
            data += 3;
            break;

        default:

            // reserved 1 byte operand range
            if (opcode>=0x30&&opcode<=0x3f) {
                data += 2;
                continue;
            }

            // reserved 2 byte operand range
            if (opcode>=0x40&&opcode<=0x4E) {
                data += 3;
                continue;
            }

            // reserved 2 byte operand range
            if (opcode>=0xA1&&opcode<=0xAF) {
                data += 3;
                continue;
            }

            // reserved 2 byte operand range
            if (opcode>=0xBC&&opcode<=0xBF) {
                data += 3;
                continue;
            }

            // reserved 3 byte operand range
            if (opcode>=0xC5&&opcode<=0xCF) {
                data += 4;
                continue;
            }

            // reserved 3 byte operand range
            if (opcode>=0xD5&&opcode<=0xDF) {
                data += 4;
                continue;
            }

            // reserved 4 byte operand range
            if (opcode>=0xE1&&opcode<=0xFF) {
                data += 5;
                continue;
            }

            // single byte sample delay
            if ((opcode&0xF0)==0x70) {
                count += _toSamples((data[0]&0x0F)+1);
                data += 1;
            }
            else {
                uint32_t offset = data - vgm->raw;
                printf("Unknown Opcode 0x%02X @ 0x%04X\n", data[0], offset);
                getchar();
                vgm->finished = true;
            }
            break;
        }
    }

    return !vgm->finished;
}

uint32_t _minv(uint32_t a, uint32_t b)
{
    return (a<b) ? a : b;
}

} // namespace {}

sVGMFile* vgm_load(const char* path)
{
    int32_t size = 0;
    uint8_t* stream = gzOpen(path, &size);
    if (stream == nullptr)
        return nullptr;

    if (memcmp(stream, "Vgm ", 4)) {
        gzClose(stream);
        return nullptr;
    }

    sVGMFile* vgm = new sVGMFile;
    memset(vgm, 0, sizeof(sVGMFile));

    vgm->header = (sVGMHeader*)stream;
    vgm->raw = stream;

    // find start of music data
    if (vgm->header->offset_vgmdata==0) {
        const uint32_t header_size = 64;
        vgm->stream = stream+header_size;
    }
    else {
        const uint32_t start = 0x34+vgm->header->offset_vgmdata;
        vgm->stream = stream + start;
    }
    return vgm;
}

void vgm_free(sVGMFile* vgm)
{
    if (vgm->chip_) {
        delete vgm->chip_;
    }
    gzClose(vgm->raw);
    delete vgm;
}

void vgm_render(sVGMFile* vgm, int16_t* dst, uint32_t samples)
{
    // clear the sound buffer
    memset(dst, 0, samples * sizeof(int16_t));

    uint32_t spill = vgm->spill;

    uint32_t watchdog = 10000;

    // while we have more space in the audio frame
    while (samples > 0 && !vgm->finished) {
        // how many samples can we render
        uint32_t count = _minv(samples, spill);
        // handle any spill between audio frames
        if (count) {
            // render the requested number of frames
            if (vgm->chip_) {
                vgm->chip_->render(dst, count);
            }
            // advance the sample stream
            spill   -= count;
            samples -= count;
            dst     += count;
        }
        // parse the stream to get new samples to render
        else {
            // add current samples to the spill counter
            _parse(vgm, spill);
        }

        // test we are not in a runaway loop
        assert(--watchdog);
    }
    vgm->spill = spill;
}
