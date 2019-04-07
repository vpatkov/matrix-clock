/* Maxim MAX7221 chip */

#ifndef MAX7221_HPP_
#define MAX7221_HPP_

#include <stdint.h>
#include <stddef.h>
#include "gpio.hpp"
#include "spi.hpp"

class Max7221 {
private:
        Spi &spi;
        const Gpio::Pin cs;
public:
        enum {
                REG_NOP                 = 0x0000,
                REG_DIGIT0              = 0x0100,
                REG_DIGIT1              = 0x0200,
                REG_DIGIT2              = 0x0300,
                REG_DIGIT3              = 0x0400,
                REG_DIGIT4              = 0x0500,
                REG_DIGIT5              = 0x0600,
                REG_DIGIT6              = 0x0700,
                REG_DIGIT7              = 0x0800,
                REG_DECODE_MODE         = 0x0900,
                REG_INTENSITY           = 0x0a00,
                REG_SCAN_LIMIT          = 0x0b00,
                REG_SHUTDOWN            = 0x0c00,
                REG_DISPLAY_TEST        = 0x0f00,
        };

        Max7221(Spi &spi, Gpio::Pin cs);
        void init();

        /* 16-bit write */
        void write(uint16_t data);

        /* Bulk write. This is useful for daisy-chained chips: each chip need
         * a 16-bit word, the first written word is shifted to the last chip
         * in the chain. Write REG_NOP to chips that should be skipped.
         */
        void write(const uint16_t *data, size_t len);
};

#endif
