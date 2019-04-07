/* Abstract I2C interface (master) */

#ifndef I2C_HPP_
#define I2C_HPP_

#include <stdint.h>
#include <stddef.h>

class I2c {
public:
        /* Write transaction: START SLA+W (DATA ack)... [STOP]
         * Read transaction: START SLA+R [data ACK]... (data NACK) [STOP]
         *
         * Note that addr is a 7-bit integer, i.e. 0xxx xxxx, not xxxx xxx0.
         *
         * The functions return number of successfully written/read bytes.
         * After an error STOP will be unconditionally issued.
         */
        virtual size_t write(uint8_t addr, const uint8_t *data, size_t len, bool stop) = 0;
        virtual size_t read(uint8_t addr, uint8_t *data, size_t len, bool stop) = 0;
};

#endif
