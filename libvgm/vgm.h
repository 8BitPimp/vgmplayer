#pragma once
#include <cstdint>
#include <memory>

#include "vgm_header.h"

struct vgm_chip_t {

    virtual ~vgm_chip_t() {};

    virtual void set_clock(uint32_t clock){};
    virtual void write(uint32_t port, uint32_t reg, uint32_t data){};
    virtual void render(int16_t* dst, uint32_t samples){};
    virtual void render(int32_t* dst, uint32_t samples){};
    virtual void mute(){};
};

struct vgm_chip_bank_t {

    vgm_chip_bank_t()
        : ym3812(nullptr)
        , sn76489(nullptr)
        , ym2612(nullptr)
        , nes_apu(nullptr)
        , gb_dmg(nullptr)
        , pokey(nullptr)
    {
    }

    struct vgm_chip_t* ym3812;
    struct vgm_chip_t* sn76489;
    struct vgm_chip_t* ym2612;
    struct vgm_chip_t* nes_apu;
    struct vgm_chip_t* gb_dmg;
    struct vgm_chip_t* pokey;
};

struct vgm_stream_t {

    virtual ~vgm_stream_t() {};

    virtual uint8_t read8() = 0;
    virtual uint16_t read16() = 0;
    virtual uint32_t read32() = 0;
    virtual void read(void* dst, uint32_t size) = 0;
    virtual void skip(uint32_t size) = 0;
};

struct vgm_t {

    vgm_t()
        : _stream(nullptr)
        , _delay(0)
        , _finished(true)
    {
    }

    bool load(
        struct vgm_stream_t* stream,
        struct vgm_chip_bank_t* chips);

    bool advance();

    // return number of samples till next vgm event
    uint32_t get_delay_samples();

    // return milli before next vgm event
    uint32_t get_delay_ms();

    // mute all chip
    void mute();

    // has the vgm streeam finished
    bool finished();

    // return chip bank
    const vgm_chip_bank_t &chips() const {
      return _chips;
    }

protected:
    bool _vgm_parse_single(uint32_t*);
    void _vgm_data_block(uint8_t type, uint32_t size);

    // vgm data stream
    struct vgm_stream_t* _stream;
    // samples before next vgm event
    int32_t _delay;
    // vgm stream has ended
    bool _finished;
    // bank of chip devices
    struct vgm_chip_bank_t _chips;
    // vgm stream header
    struct vgm_header_t _header;
};
