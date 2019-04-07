/* DHT22 sensor */

#ifndef DHT22_HPP_
#define DHT22_HPP_

#include <stdint.h>
#include "gpio.hpp"

class Dht22 {
private:
        const Gpio::Pin data_pin;
        int16_t temperature;
        uint16_t humidity;
public:
        explicit Dht22(Gpio::Pin data_pin);
        void init();

        /* Read the sensor. Returns true if received valid data. */
        bool read();

        /* Temperature in °C/10 */
        int16_t get_temperature();

        /* Relative humidity in %/10 */
        uint16_t get_humidity();
};

#endif
