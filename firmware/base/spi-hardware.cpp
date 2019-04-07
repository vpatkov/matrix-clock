#include <avr/io.h>
#include "spi-hardware.hpp"
#include "common.hpp"
#include "shared.hpp"

SpiHardware::SpiHardware(Gpio::Pin mosi_, Gpio::Pin miso_, Gpio::Pin sck_):
        mosi(mosi_), miso(miso_), sck(sck_) {}

void SpiHardware::init(Divider div, Mode m)
{
        Gpio::write(mosi, 0);
        Gpio::set(miso, Gpio::tri);
        SPCR = 1<<SPE | 1<<MSTR | (div & 3);
        set_bits(SPSR, 1<<SPI2X, div == div_2);
        set_mode(m);
}

uint8_t SpiHardware::transfer(uint8_t out)
{
        SPDR = out;
        while ((SPSR & 1<<SPIF) == 0)
                memory_barrier();
        return SPDR;
}

void SpiHardware::set_mode(Mode m)
{
        set_bits(SPCR, 1<<CPOL, m.cpol);
        set_bits(SPCR, 1<<CPHA, m.cpha);
        set_bits(SPCR, 1<<DORD, !m.msb_first);
        Gpio::write(sck, m.cpol);
}
