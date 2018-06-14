#include "../libserial/serial.h"
#include "vgm.h"

static void _dev_serial_write(
    serial_t* serial,
    uint32_t port,
    uint32_t reg,
    uint32_t val)
{
    if (serial) {
        serial_send(serial, (const uint8_t*)&reg, 1);
        serial_send(serial, (const uint8_t*)&val, 1);
    }
}

static void _dev_serial_mute(serial_t* serial)
{
    if (serial) {
        uint8_t zero = 0;
        for (uint32_t i = 0; i < 0xff; ++i) {
            serial_send(serial, (const uint8_t*)&i, 1);
            serial_send(serial, (const uint8_t*)&zero, 1);
        }
    }
}

struct dev_serial_t : public vgm_chip_t {

    serial_t* _serial;

    dev_serial_t(serial_t* serial)
        : _serial(serial)
    {
    }

    void write(uint32_t port, uint32_t reg, uint32_t data)
    {
        _dev_serial_write(_serial, port, reg, data);
    };

    void mute() override
    {
        _dev_serial_mute(_serial);
    }
};

vgm_chip_t* create_dev_serial(uint16_t port)
{
    serial_t* serial = serial_open(SERIAL_COM(port), 9600);
    if (!serial) {
//        printf("Unable to open serial port");
        return nullptr;
    }
}
