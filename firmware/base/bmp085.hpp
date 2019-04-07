/* Bosch BMP085 sensor */

#ifndef BMP085_HPP_
#define BMP085_HPP_

#include <stdint.h>
#include "i2c.hpp"

class Bmp085 {
private:
        struct Calibration {
                int16_t  ac1, ac2, ac3;
                uint16_t ac4, ac5, ac6;
                int16_t  b1, b2, mb, mc, md;
        };

        I2c &i2c;
        Calibration cal;
        int32_t ut, up;

        bool read_eeprom(uint8_t addr, uint16_t &data);
        bool read_calibration();
        bool read_uncompensated(bool p, int32_t &val);
public:
        explicit Bmp085(I2c &i2c);
        bool init();

        /* Make a measurement and read the measured data */
        bool read();

        /* Air pressure in Pa */
        uint32_t get_pressure();

        /* Temperature in °C/10 */
        int16_t get_temperature();
};

#endif
