#include "max31723.hpp"

constexpr Spi::Mode spi_mode = {
        .cpol = 0,
        .cpha = 1,
        .msb_first = 1,
};

Max31723::Max31723(Spi &spi_, Gpio::Pin ce_):
        spi(spi_), ce(ce_) {}

void Max31723::init()
{
        Gpio::write(ce, 0);
}

void Max31723::write(uint8_t reg, uint8_t data)
{
        spi.set_mode(spi_mode);
        Gpio::write(ce, 1);
        spi.write(reg | 0x80);
        spi.write(data);
        Gpio::write(ce, 0);
}

uint8_t Max31723::read(uint8_t reg)
{
        spi.set_mode(spi_mode);
        Gpio::write(ce, 1);
        spi.write(reg);
        uint8_t data = spi.read();
        Gpio::write(ce, 0);
        return data;
}
