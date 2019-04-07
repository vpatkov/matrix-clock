/* Abstract UART interface */

#ifndef UART_HPP_
#define UART_HPP_

#include <stdint.h>
#include <stddef.h>

class Uart {
public:
        virtual uint8_t read() = 0;
        virtual void write(uint8_t byte) = 0;

        void read(uint8_t *data, size_t len) {
                while (len-- != 0)
                        *data++ = read();
        }

        void write(const uint8_t *data, size_t len) {
                while (len-- != 0)
                        write(*data++);
        }
};

#endif
