/* AVR hardware SPI (master) */

#ifndef SPI_HARDWARE_HPP_
#define SPI_HARDWARE_HPP_

#include <stdint.h>
#include "spi.hpp"
#include "gpio.hpp"

class SpiHardware: public Spi {
private:
        const Gpio::Pin mosi, miso, sck;
public:
        /* F_CPU dividers for SCK */
        enum Divider: uint8_t {
                div_2   = 4,
                div_4   = 0,
                div_16  = 1,
                div_64  = 2,
                div_128 = 3,
        };
        SpiHardware(Gpio::Pin mosi, Gpio::Pin miso, Gpio::Pin sck);
        void init(Divider div = div_128, Mode m = {0, 0, 1});
        uint8_t transfer(uint8_t out) override;
        void set_mode(Mode m) override;
};

#endif
