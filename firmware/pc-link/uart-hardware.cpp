#include <avr/io.h>
#include "uart-hardware.hpp"
#include "shared.hpp"

bool UartHardware::init(uint32_t baudrate)
{
        const uint16_t ubrr = F_CPU/8/baudrate - 1;

        if (ubrr > 4095)
                return false;

        UBRRH = ubrr >> 8;
        UBRRL = ubrr & 0xff;

        UCSRA = 1<<TXC | 1<<U2X;
        UCSRB = 1<<RXEN | 1<<TXEN;
        UCSRC = 1<<URSEL | 1<<UCSZ1 | 1<<UCSZ0;

        return true;
}

void UartHardware::write(uint8_t data)
{
        while ((UCSRA & 1<<UDRE) == 0)
                memory_barrier();
        UDR = data;
}

uint8_t UartHardware::read()
{
        while ((UCSRA & 1<<RXC) == 0)
                memory_barrier();
        return UDR;
}
