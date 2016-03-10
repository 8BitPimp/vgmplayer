#pragma once
#include <stdint.h>

enum chip_type_e
{
    e_chip_sn67489,
    e_chip_nes_apu,
    e_chip_ym3812,
    e_chip_ym2612,
};

struct vgm_chip_t
{
    const chip_type_e id_;

    vgm_chip_t(chip_type_e id)
        : id_(id)
    {
    }

    virtual ~vgm_chip_t() {}

    virtual void init() = 0;
    virtual void write(uint32_t reg, uint32_t data) = 0;
    virtual void render(int16_t * dst, uint32_t len) = 0;
    virtual void silence() = 0;
};

vgm_chip_t * chip_create_sn76489(uint32_t clock);
vgm_chip_t * chip_create_nes_apu(uint32_t clock);
vgm_chip_t * chip_create_ym3812 (uint32_t clock);
vgm_chip_t * chip_create_ym2612 (uint32_t clock);