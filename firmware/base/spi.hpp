/* Abstract SPI interface (master) */

#ifndef SPI_HPP_
#define SPI_HPP_

#include <stdint.h>
#include <stddef.h>

class Spi {
public:
        struct Mode {
                bool cpol: 1;
                bool cpha: 1;
                bool msb_first: 1;
        };

        virtual uint8_t transfer(uint8_t out) = 0;
        virtual void set_mode(Mode m) = 0;

        void write(uint8_t byte) { transfer(byte); }
        uint8_t read() { return transfer(0); }

        void write(const uint8_t *data, size_t len) {
                while (len-- != 0)
                        write(*data++);
        }

        void read(uint8_t *data, size_t len) {
                while (len-- != 0)
                        *data++ = read();
        }
};

#endif
