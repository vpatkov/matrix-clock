#include <avr/io.h>
#include "spi-usi.hpp"

SpiUsi::SpiUsi(Gpio::Pin usi_do_, Gpio::Pin usi_di_, Gpio::Pin usi_usck_):
        usi_do(usi_do_), usi_di(usi_di_), usi_usck(usi_usck_) {}

static uint8_t mirror(uint8_t d)
{
        uint8_t m = 0;
        for (uint8_t i = 0; i < 8; i++) {
                m <<= 1;
                m |= d & 1;
                d >>= 1;
        }
        return m;
}

void SpiUsi::init(Mode m)
{
        Gpio::write(usi_do, 0);
        Gpio::set(usi_di, Gpio::tri);
        set_mode(m);
}

uint8_t SpiUsi::transfer(uint8_t out)
{
        USIDR = mode.msb_first ? out : mirror(out);
        USISR = 1<<USIOIF;

        const uint8_t r = (mode.cpha != mode.cpol) ?
                1<<USIWM0 | 1<<USICS1 | 1<<USICS0 | 1<<USICLK | 1<<USITC :  /* Read at negedge, write at posedge */
                1<<USIWM0 | 1<<USICS1 | 1<<USICLK | 1<<USITC;               /* Read at posedge, write at negedge */

        do {
                USICR = r;
        } while ((USISR & 1<<USIOIF) == 0);

        return mode.msb_first ? USIBR : mirror(USIBR);
}

void SpiUsi::set_mode(Mode m)
{
        mode = m;
        Gpio::write(usi_usck, m.cpol);
}
