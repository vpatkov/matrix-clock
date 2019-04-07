/* AVR hardware UART (always 8N1) */

#ifndef UART_HARDWARE_HPP_
#define UART_HARDWARE_HPP_

#include <stdint.h>
#include "uart.hpp"

class UartHardware: public Uart {
public:
        bool init(uint32_t baudrate = 9600);
        uint8_t read() override;
        void write(uint8_t byte) override;
        using Uart::read;
        using Uart::write;
};

#endif
