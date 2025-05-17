#include "TM7711.h"
#include "esp_system.h"
#include "esp_err.h"
#include "rom/ets_sys.h"

esp_err_t tm7711_init(int dout_pin, int sck_pin) {
    esp_err_t ret = ESP_OK;

    // Reset GPIO pins
    ret |= gpio_reset_pin(sck_pin);
    ret |= gpio_set_direction(sck_pin, GPIO_MODE_OUTPUT);
    ret |= gpio_reset_pin(dout_pin);
    ret |= gpio_set_direction(dout_pin, GPIO_MODE_INPUT);

    // Send reset pulse (SCK high for >200us)
    gpio_set_level(sck_pin, 1);
    ets_delay_us(200);
    gpio_set_level(sck_pin, 0);

    return ret;
}

esp_err_t tm7711_read(unsigned char next_select, int dout_pin, int sck_pin, unsigned long *data) {
    unsigned char i;
    unsigned long data_temp = 0;
    unsigned char pulses;
    int timeout;
    int retries = 3; // Maximum 3 attempts

    // Determine additional clock pulses and timeout based on mode
    switch (next_select) {
        case CH1_10HZ:
            timeout = 120000; // 120ms for 10Hz
            pulses = CH1_10HZ_CLK - 24;  // 1 pulse
            break;
        case CH1_40HZ:
            timeout = 30000;  // 30ms for 40Hz
            pulses = CH1_40HZ_CLK - 24;  // 3 pulses
            break;
        case CH2_TEMP:
            timeout = 60000;  // 60ms for temperature
            pulses = CH2_TEMP_CLK - 24;  // 2 pulses
            break;
        default:
            return ESP_ERR_INVALID_ARG;
    }

    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    while (retries--) {
        // Wait for DOUT to go low (data ready)
        int temp_timeout = timeout;
        while (gpio_get_level(dout_pin) && temp_timeout--) {
            ets_delay_us(1);
        }
        if (temp_timeout <= 0) {
            if (retries == 0) {
                return ESP_ERR_TIMEOUT;
            }
            continue; // Retry
        }

        // Read 24 bits
        for (i = 0; i < 24; i++) {
            gpio_set_level(sck_pin, 1);  // SCK high
            ets_delay_us(5);             // Wait 5us
            data_temp <<= 1;             // Shift data left
            if (gpio_get_level(dout_pin)) {
                data_temp |= 1;          // Read bit from DOUT
            }
            gpio_set_level(sck_pin, 0);  // SCK low
            ets_delay_us(5);             // Wait 5us
        }

        // Send additional clock pulses for next mode
        for (i = 0; i < pulses; i++) {
            gpio_set_level(sck_pin, 1);
            ets_delay_us(1);
            gpio_set_level(sck_pin, 0);
            ets_delay_us(1);
        }

        *data = data_temp;
        return ESP_OK;
    }
    return ESP_ERR_TIMEOUT;
}