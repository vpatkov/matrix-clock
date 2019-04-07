#include "max7221.hpp"

constexpr Spi::Mode spi_mode = {
        .cpol = 0,
        .cpha = 0,
        .msb_first = 1,
};

Max7221::Max7221(Spi &spi_, Gpio::Pin cs_):
        spi(spi_), cs(cs_) {}

void Max7221::init()
{
        Gpio::write(cs, 1);
}

void Max7221::write(uint16_t data)
{
        write(&data, 1);
}

void Max7221::write(const uint16_t *data, size_t len)
{
        spi.set_mode(spi_mode);
        Gpio::write(cs, 0);
        for (; len-- != 0; data++) {
                spi.write(*data >> 8);
                spi.write(*data & 0xff);
        }
        Gpio::write(cs, 1);
}
