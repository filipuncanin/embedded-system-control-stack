#ifndef _TM7711_H_
#define _TM7711_H_

#include <driver/gpio.h>

/**
 * @brief Mode for Channel 1 with 10 Hz sampling rate.
 */
#define CH1_10HZ    0x01

/**
 * @brief Mode for Channel 1 with 40 Hz sampling rate.
 */
#define CH1_40HZ    0x02

/**
 * @brief Mode for Channel 2, temperature measurement.
 */
#define CH2_TEMP    0x03

/**
 * @brief Number of clock pulses for Channel 1 at 10 Hz.
 */
#define CH1_10HZ_CLK  25

/**
 * @brief Number of clock pulses for Channel 1 at 40 Hz.
 */
#define CH1_40HZ_CLK  27

/**
 * @brief Number of clock pulses for Channel 2 temperature measurement.
 */
#define CH2_TEMP_CLK  26

/**
 * @brief Initialize the TM7711 ADC with specified pins.
 * @param dout_pin GPIO pin for data output.
 * @param sck_pin GPIO pin for serial clock.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t tm7711_init(int dout_pin, int sck_pin);

/**
 * @brief Read data from the TM7711 ADC.
 * @param next_select Mode selection for the next reading (e.g., CH1_10HZ, CH1_40HZ, CH2_TEMP).
 * @param dout_pin GPIO pin for data output.
 * @param sck_pin GPIO pin for serial clock.
 * @param data Pointer to store the read data.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t tm7711_read(unsigned char next_select, int dout_pin, int sck_pin, unsigned long *data);

#endif