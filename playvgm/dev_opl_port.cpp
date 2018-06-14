#include "vgm.h"
#include <intrin.h>

#define SBBASE 0x220

// write to the Adlib chip on ISA bus
static void _port_write(uint32_t port, uint32_t data)
{
    __asm {
        push edx
        mov  edx, port
        mov  eax, data
        out  dx,  al
        pop  edx
    }
    ;
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
    }
    ;
}

static void _opl_write(
    uint32_t port,
    uint32_t reg,
    uint32_t val)
{

    _port_write(SBBASE + 0x8, reg);
    _vgm_delay(6);
    _port_write(SBBASE + 0x9, val);
    _vgm_delay(36);
}

static void _opl_mute()
{
    for (uint32_t i = 0; i < 0xff; ++i) {
        _port_write(SBBASE + 0x8, i);
        _vgm_delay(6);
        _port_write(SBBASE + 0x9, 0);
        _vgm_delay(36);
    }
}

struct dev_opl_t : public vgm_chip_t {

    void write(uint32_t port, uint32_t reg, uint32_t data)
    {
        _opl_write(port, reg, data);
    };

    void mute() override
    {
        _opl_mute();
    }
};

vgm_chip_t* create_dev_opl(uint16_t port) {
  dev_opl_t *dev = new dev_opl_t;
  return dev;
}
