/* Nordic NRF24L01+ chip */

#ifndef NRF24_HPP_
#define NRF24_HPP_

#include <stdint.h>
#include <stddef.h>
#include "spi.hpp"
#include "gpio.hpp"

class Nrf24 {
private:
        Spi &spi;
        const Gpio::Pin csn, ce;
public:
        /* Commands */
        enum {
                CMD_R_REGISTER            = 0x00,
                CMD_W_REGISTER            = 0x20,
                CMD_R_RX_PAYLOAD          = 0x61,
                CMD_W_TX_PAYLOAD          = 0xa0,
                CMD_FLUSH_TX              = 0xe1,
                CMD_FLUSH_RX              = 0xe2,
                CMD_REUSE_TX_PL           = 0xe3,
                CMD_R_RX_PL_WID           = 0x60,
                CMD_W_ACK_PAYLOAD         = 0xa8,
                CMD_W_TX_PAYLOAD_NOACK    = 0xb0,
                CMD_NOP                   = 0xff,
        };

        /* Registers */
        enum {
                REG_CONFIG        = 0x00,
                REG_EN_AA         = 0x01,
                REG_EN_RXADDR     = 0x02,
                REG_SETUP_AW      = 0x03,
                REG_SETUP_RETR    = 0x04,
                REG_RF_CH         = 0x05,
                REG_RF_SETUP      = 0x06,
                REG_STATUS        = 0x07,
                REG_OBSERVE_TX    = 0x08,
                REG_RPD           = 0x09,
                REG_RX_ADDR_P0    = 0x0a,
                REG_RX_ADDR_P1    = 0x0b,
                REG_RX_ADDR_P2    = 0x0c,
                REG_RX_ADDR_P3    = 0x0d,
                REG_RX_ADDR_P4    = 0x0e,
                REG_RX_ADDR_P5    = 0x0f,
                REG_TX_ADDR       = 0x10,
                REG_RX_PW_P0      = 0x11,
                REG_RX_PW_P1      = 0x12,
                REG_RX_PW_P2      = 0x13,
                REG_RX_PW_P3      = 0x14,
                REG_RX_PW_P4      = 0x15,
                REG_RX_PW_P5      = 0x16,
                REG_FIFO_STATUS   = 0x17,
                REG_DYNPD         = 0x1c,
                REG_FEATURE       = 0x1d,
        };

        /* REG_CONFIG */
        enum {
                MASK_RX_DR        = 0x40,
                MASK_TX_DS        = 0x20,
                MASK_MAX_RT       = 0x10,
                EN_CRC            = 0x08,
                CRC0              = 0x04,
                PWR_UP            = 0x02,
                PRIM_RX           = 0x01,
        };

        /* REG_EN_AA */
        enum {
                ENAA_P5           = 0x20,
                ENAA_P4           = 0x10,
                ENAA_P3           = 0x08,
                ENAA_P2           = 0x04,
                ENAA_P1           = 0x02,
                ENAA_P0           = 0x01,
        };

        /* REG_EN_RXADDR */
        enum {
                ERX_P5            = 0x20,
                ERX_P4            = 0x10,
                ERX_P3            = 0x08,
                ERX_P2            = 0x04,
                ERX_P1            = 0x02,
                ERX_P0            = 0x01,
        };

        /* REG_SETUP_AW */
        enum {
                AW_3_BYTES        = 0x01,
                AW_4_BYTES        = 0x02,
                AW_5_BYTES        = 0x03,
        };

        /* REG_SETUP_RETR */
        enum {
                ARD_250us         = 0x00,
                ARD_500us         = 0x10,
                ARD_750us         = 0x20,
                ARD_1000us        = 0x30,
                ARD_1250us        = 0x40,
                ARD_1500us        = 0x50,
                ARD_1750us        = 0x60,
                ARD_2000us        = 0x70,
                ARD_2250us        = 0x80,
                ARD_2500us        = 0x90,
                ARD_2750us        = 0xa0,
                ARD_3000us        = 0xb0,
                ARD_3250us        = 0xc0,
                ARD_3500us        = 0xd0,
                ARD_3750us        = 0xe0,
                ARD_4000us        = 0xf0,
                ARC_DISABLED      = 0x00,
                ARC_1             = 0x01,
                ARC_2             = 0x02,
                ARC_3             = 0x03,
                ARC_4             = 0x04,
                ARC_5             = 0x05,
                ARC_6             = 0x06,
                ARC_7             = 0x07,
                ARC_8             = 0x08,
                ARC_9             = 0x09,
                ARC_10            = 0x0a,
                ARC_11            = 0x0b,
                ARC_12            = 0x0c,
                ARC_13            = 0x0d,
                ARC_14            = 0x0e,
                ARC_15            = 0x0f,
        };

        /* REG_RF_SETUP */
        enum {
                CONT_WAVE         = 0x80,
                PLL_LOCK          = 0x10,
                RF_DR_250kbps     = 0x20,
                RF_DR_1Mbps       = 0x00,
                RF_DR_2Mbps       = 0x08,
                RF_PWR_m18dBm     = 0x00,
                RF_PWR_m12dBm     = 0x02,
                RF_PWR_m6dBm      = 0x04,
                RF_PWR_0dBm       = 0x06,
        };

        /* REG_STATUS */
        enum {
                RX_DR             = 0x40,
                TX_DS             = 0x20,
                MAX_RT            = 0x10,
                RX_P_NO           = 0x0e,
                RX_P_NO_0         = 0x00,
                RX_P_NO_1         = 0x02,
                RX_P_NO_2         = 0x04,
                RX_P_NO_3         = 0x06,
                RX_P_NO_4         = 0x08,
                RX_P_NO_5         = 0x0a,
                RX_P_NOT_USED     = 0x0c,
                RX_P_FIFO_EMPTY   = 0x0e,
                TX_FIFO_FULL      = 0x01,
        };

        /* REG_RPD */
        enum {
                RPD               = 0x01,
        };

        /* REG_FIFO_STATUS */
        enum {
                TX_REUSE          = 0x40,
                TX_FULL           = 0x20,
                TX_EMPTY          = 0x10,
                RX_FULL           = 0x02,
                RX_EMPTY          = 0x01,
        };

        /* REG_DYNPD */
        enum {
                DPL_P5            = 0x20,
                DPL_P4            = 0x10,
                DPL_P3            = 0x08,
                DPL_P2            = 0x04,
                DPL_P1            = 0x02,
                DPL_P0            = 0x01,
        };

        /* REG_FEATURE */
        enum {
                EN_DPL            = 0x04,
                EN_ACK_PAY        = 0x02,
                EN_DYN_ACK        = 0x01,
        };

        /* Power Down -> Standby delay (ms) */
        static constexpr double tpd2stby = 5;

        Nrf24(Spi &spi, Gpio::Pin csn, Gpio::Pin ce);
        void init();

        /* Set CE pin low (0) or high (1) */
        void set_ce(bool val);

        /* CE 10 μs pulse */
        void ce_pulse();

        /* Bulk data read/write, return the status register */
        uint8_t write(uint8_t cmd, const uint8_t *data, size_t len);
        uint8_t read(uint8_t cmd, uint8_t *data, size_t len);

        /* Single-byte data read/write */
        uint8_t write(uint8_t cmd, uint8_t data);  /* Return the status register */
        uint8_t read(uint8_t cmd);                 /* Return the received data */

        /* Write a command without data, return the status register */
        uint8_t write(uint8_t cmd);

        /* Read the status register */
        uint8_t status();
};

#endif
