#include "dht22.hpp"
#include "delay.hpp"
#include "common.hpp"
#include "shared.hpp"

static_assert(F_CPU >= 8e6, "");

static void wait_posedge(Gpio::Pin pin)
{
        while (Gpio::read(pin))
                memory_barrier();
        while (!Gpio::read(pin))
                memory_barrier();
}

static void wait_negedge(Gpio::Pin pin)
{
        while (!Gpio::read(pin))
                memory_barrier();
        while (Gpio::read(pin))
                memory_barrier();
}

static bool read_bit(Gpio::Pin pin)
{
        wait_posedge(pin);
        delay_us(30);
        return Gpio::read(pin);
}

static uint8_t read_byte(Gpio::Pin pin)
{
        uint8_t byte = 0;
        for (uint8_t i = 0; i < 8; i++) {
                byte <<= 1;
                if (read_bit(pin))
                        byte |= 1;
        }
        return byte;
}

Dht22::Dht22(Gpio::Pin p): data_pin(p) {}

void Dht22::init()
{
        Gpio::set(data_pin, Gpio::tri);
}

bool Dht22::read()
{
        uint8_t data[5];

        /* Write START */
        Gpio::write(data_pin, 0);
        delay_ms(20);

        atomic_block
        {
                /* Read RESPONSE */
                Gpio::set(data_pin, Gpio::tri);
                delay_us(50);
                if (Gpio::read(data_pin))
                        return false;

                /* Read the data */
                wait_negedge(data_pin);
                for (uint8_t i = 0; i < 5; ++i)
                        data[i] = read_byte(data_pin);
        }

        humidity = concat16(data[0], data[1]);
        temperature = concat16(data[2], data[3]);
        if (temperature & 0x8000)
                temperature = -(temperature & 0x7fff);

        return ((data[0]+data[1]+data[2]+data[3]) & 0xff) == data[4];
}

int16_t Dht22::get_temperature()
{
        return temperature;
}

uint16_t Dht22::get_humidity()
{
        return humidity;
}
