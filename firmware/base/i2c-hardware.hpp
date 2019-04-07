/* AVR hardware I2C (TWI) */

#ifndef I2C_HARDWARE_HPP_
#define I2C_HARDWARE_HPP_

#include <stdint.h>
#include <stddef.h>
#include "i2c.hpp"

class I2cHardware: public I2c {
public:
        bool init(uint32_t freq);
        void deinit();
        size_t write(uint8_t addr, const uint8_t *data, size_t len, bool stop) override;
        size_t read(uint8_t addr, uint8_t *data, size_t len, bool stop) override;
};

#endif
