#include "nrf24.hpp"
#include "delay.hpp"

constexpr Spi::Mode spi_mode = {
        .cpol = 0,
        .cpha = 0,
        .msb_first = 1,
};

Nrf24::Nrf24(Spi &spi_, Gpio::Pin csn_, Gpio::Pin ce_):
        spi(spi_), csn(csn_), ce(ce_) {}

void Nrf24::init()
{
        Gpio::write(csn, 1);
        Gpio::write(ce, 0);
}

void Nrf24::set_ce(bool val)
{
        Gpio::write(ce, val);
}

void Nrf24::ce_pulse()
{
        set_ce(1);
        delay_us(10);
        set_ce(0);
}

uint8_t Nrf24::write(uint8_t cmd, const uint8_t *data, size_t len)
{
        spi.set_mode(spi_mode);
        Gpio::write(csn, 0);
        uint8_t status = spi.transfer(cmd);
        spi.write(data, len);
        Gpio::write(csn, 1);
        return status;
}

uint8_t Nrf24::read(uint8_t cmd, uint8_t *data, size_t len)
{
        spi.set_mode(spi_mode);
        Gpio::write(csn, 0);
        uint8_t status = spi.transfer(cmd);
        spi.read(data, len);
        Gpio::write(csn, 1);
        return status;
}

uint8_t Nrf24::write(uint8_t cmd, uint8_t data)
{
        return write(cmd, &data, 1);
}

uint8_t Nrf24::read(uint8_t cmd)
{
        uint8_t d;
        read(cmd, &d, 1);
        return d;
}

uint8_t Nrf24::write(uint8_t cmd)
{
        return write(cmd, nullptr, 0);
}

uint8_t Nrf24::status()
{
        return write(CMD_NOP);
}
