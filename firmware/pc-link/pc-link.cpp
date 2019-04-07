/* Main program */

#include <stdint.h>
#include <avr/io.h>
#include "common.hpp"
#include "delay.hpp"
#include "shared.hpp"
#include "gpio.hpp"
#include "uart-hardware.hpp"
#include "spi-hardware.hpp"
#include "nrf24.hpp"

/* Peripherals are configured for 1 MHz system clock */
static_assert(F_CPU == 1e6, "");

constexpr Gpio::Pin nrf_csn = Gpio::B2, nrf_ce = Gpio::B1, nrf_irq = Gpio::B0;
constexpr Gpio::Pin spi_mosi = Gpio::B3, spi_miso = Gpio::B4, spi_sck = Gpio::B5;
constexpr Gpio::Pin led = Gpio::D6;

constexpr uint32_t uart_baudrate = 9600;

constexpr uint8_t my_addr[] = {0xe7, 0x4f, 0xec, 0xe8, 0x37};

static UartHardware uart;
static SpiHardware spi {spi_mosi, spi_miso, spi_sck};
static Nrf24 nrf24 {spi, nrf_csn, nrf_ce};

static void nrf24_setup()
{
        /* 3-byte address, 2402 MHz, 1 Mbit/s */
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_CONFIG,
                Nrf24::MASK_RX_DR | Nrf24::MASK_TX_DS | Nrf24::MASK_MAX_RT |
                Nrf24::EN_CRC | Nrf24::CRC0 | Nrf24::PWR_UP);
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_EN_RXADDR, Nrf24::ERX_P0);
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_RF_SETUP,
                Nrf24::RF_DR_1Mbps | Nrf24::RF_PWR_0dBm);
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_FEATURE, Nrf24::EN_DYN_ACK);
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_SETUP_AW, Nrf24::AW_5_BYTES);

        /* Set my address */
        static_assert(size(my_addr) == 5, "");
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_TX_ADDR, my_addr, size(my_addr));
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_RX_ADDR_P0, my_addr, size(my_addr));

        /* Wait for Standby mode */
        delay_ms(Nrf24::tpd2stby);
}

static void nrf24_transmit(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
        /* Flush TX FIFO */
        nrf24.write(Nrf24::CMD_FLUSH_TX);

        /* Transmit the data */
        const uint8_t d[] = {hours, minutes, seconds};
        nrf24.write(Nrf24::CMD_W_TX_PAYLOAD_NOACK, d, size(d));
        nrf24.ce_pulse();

        /* Wait until done */
        while ((nrf24.status() & (Nrf24::TX_DS | Nrf24::MAX_RT)) == 0)
                memory_barrier();

        /* Clear interrupt flags */
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_STATUS,
                Nrf24::RX_DR | Nrf24::TX_DS | Nrf24::MAX_RT); 
}

static void blink()
{
        Gpio::write(led, 1);
        delay_ms(300);
        Gpio::write(led, 0);
}

int main()
{
        /* GPIO init */
        PORTB = 0b10000100;
        DDRB  = 0b00101110;
        PORTC = 0b11111011;
        DDRC  = 0b00000000;
        PORTD = 0b10111100;
        DDRD  = 0b01000000;

        /* Disable Analog Comparator */
        ACSR = 1<<ACD;

        /* Other setups */
        uart.init(uart_baudrate);
        spi.init(SpiHardware::div_2);
        nrf24.init();
        nrf24_setup();

        while (true)
        {
                /* Synchronization */
                while (uart.read() != 0xaa)
                        memory_barrier();

                /* Read the packet: hours, minutes, seconds, checksum */
                uint8_t d[4];
                uart.read(d, size(d));

                /* Ack with my checksum */
                uint8_t checksum = ~(d[0]^d[1]^d[2]);
                uart.write(checksum);

                /* If ok, transmit the time */
                if (d[3] == checksum) {
                        nrf24_transmit(d[0], d[1], d[2]);
                        blink();
                }

        }

        return 0;  /* Never be here */
}
