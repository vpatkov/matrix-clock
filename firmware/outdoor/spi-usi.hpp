/* SPI implementation for AVR using USI */

#ifndef SPI_USI_HPP_
#define SPI_USI_HPP_

#include <stdint.h>
#include "spi.hpp"
#include "gpio.hpp"

class SpiUsi: public Spi {
private:
        const Gpio::Pin usi_do, usi_di, usi_usck;
        Mode mode;
public:
        SpiUsi(Gpio::Pin usi_do, Gpio::Pin usi_di, Gpio::Pin usi_usck);
        void init(Mode m = {0, 0, 1});
        uint8_t transfer(uint8_t out) override;
        void set_mode(Mode m) override;
};

#endif
