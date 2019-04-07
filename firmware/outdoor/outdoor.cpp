/* Main program */

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "common.hpp"
#include "delay.hpp"
#include "shared.hpp"
#include "gpio.hpp"
#include "spi-usi.hpp"
#include "nrf24.hpp"
#include "max31723.hpp"

/* Peripherals are configured for 1 MHz system clock */
static_assert(F_CPU == 1e6, "");

constexpr Gpio::Pin usi_di = Gpio::A6, usi_do = Gpio::A5, usi_usck = Gpio::A4;
constexpr Gpio::Pin nrf_csn = Gpio::B2, nrf_ce = Gpio::B0, nrf_irq = Gpio::A7;
constexpr Gpio::Pin max_ce = Gpio::A1;
constexpr Gpio::Pin led = Gpio::A0;

constexpr uint8_t battery_adc_channel = 2;

constexpr uint8_t my_addr[] = {0xc8, 0xb4, 0xe1, 0x65, 0x3b};

static SpiUsi spi {usi_do, usi_di, usi_usck};
static Nrf24 nrf24 {spi, nrf_csn, nrf_ce};
static Max31723 max31723 {spi, max_ce};

EMPTY_INTERRUPT(WATCHDOG_vect);  /* Wake up only */

static void nrf24_setup()
{
        /* 3-byte address, 2402 MHz, 1 Mbit/s */
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_EN_RXADDR, Nrf24::ERX_P0);
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_RF_SETUP,
                Nrf24::RF_DR_1Mbps | Nrf24::RF_PWR_0dBm);
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_FEATURE, Nrf24::EN_DYN_ACK);
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_SETUP_AW, Nrf24::AW_5_BYTES);

        /* Set my address */
        static_assert(size(my_addr) == 5, "");
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_TX_ADDR, my_addr, size(my_addr));
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_RX_ADDR_P0, my_addr, size(my_addr));
}

static void nrf24_power(bool on)
{
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_CONFIG,
                on ?
                Nrf24::MASK_RX_DR | Nrf24::MASK_TX_DS | Nrf24::MASK_MAX_RT |
                Nrf24::EN_CRC | Nrf24::PWR_UP | Nrf24::CRC0
                :
                Nrf24::MASK_RX_DR | Nrf24::MASK_TX_DS | Nrf24::MASK_MAX_RT |
                Nrf24::EN_CRC | Nrf24::CRC0);
}

static void nrf24_transmit(int8_t temperature, uint8_t battery_level)
{
        /* Power up the RF chip */
        nrf24_power(1);
        delay_ms(Nrf24::tpd2stby);

        /* Flush TX FIFO */
        nrf24.write(Nrf24::CMD_FLUSH_TX);

        /* Transmit the data */
        const uint8_t d[] = {static_cast<uint8_t>(temperature), battery_level};
        nrf24.write(Nrf24::CMD_W_TX_PAYLOAD_NOACK, d, size(d));
        nrf24.ce_pulse();

        /* Wait until done */
        while ((nrf24.status() & (Nrf24::TX_DS | Nrf24::MAX_RT)) == 0)
                memory_barrier();

        /* Clear interrupt flags */
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_STATUS,
                Nrf24::RX_DR | Nrf24::TX_DS | Nrf24::MAX_RT); 

        /* Shut down the RF chip */
        nrf24_power(0);
}

static int8_t get_temperature()
{
        /* Start a single conversion */
        max31723.write(Max31723::REG_CONF_STATUS,
                Max31723::CONF_STATUS_1SHOT | Max31723::CONF_STATUS_SD);

        /* Wait until done */
        delay_ms(25);
        while (max31723.read(Max31723::REG_CONF_STATUS) & Max31723::CONF_STATUS_1SHOT)
                memory_barrier();

        /* Return the MSB (°C, signed) */
        return max31723.read(Max31723::REG_TEMPERATURE_MSB);
}

static uint8_t get_battery_level()
{
        /* Power up the ADC */
        PRR &= ~(1<<PRADC);

        /* Start a single conversion (VCC reference, 125 kHz clock) */
        ADCSRA = 1<<ADEN | 1<<ADSC | 1<<ADPS1 | 1<<ADPS0;

        /* Wait until done */
        while (ADCSRA & (1<<ADSC))
                memory_barrier();

        /* Read the MSB */
        uint8_t msb = ADCH;

        /* Shut down the ADC */
        ADCSRA = 0;
        PRR |= 1<<PRADC;

        return msb;
}

int main()
{
        /* GPIO init */
        PORTA = 0b00001000;
        DDRA  = 0b00110011;
        PORTB = 0b11110110;
        DDRB  = 0b00000101;

        /* Disable unused peripherals */
        ACSR = 1<<ACD;
        PRR = 1<<PRTIM1 | 1<<PRTIM0;

        /* Watchdog interrupt every 8 sec */
        WDTCSR = 1<<WDIF | 1<<WDIE | 1<<WDP3 | 1<<WDP0;

        /* ADC setup */
        static_assert(battery_adc_channel <= 7, "");
        ADMUX = battery_adc_channel;
        ADCSRB = 1<<ADLAR;

        /* Other setups */
        spi.init();
        max31723.init();
        nrf24.init();
        nrf24_setup();
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sei();

        delay_ms(500);  /* Skip transients */

        for (uint8_t cnt = 0; ; cnt++) {
                if ((cnt % 32) == 0) {  /* Every 256 sec */
                        int8_t temp = get_temperature();
                        uint8_t bat = get_battery_level();
                        nrf24_transmit(temp, bat);
                }
                sleep_mode();
        }

        return 0;  /* Never be here */
}
