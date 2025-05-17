#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include "driver/gpio.h"
#include "cJSON.h"
#include "esp_log.h"

/**
 * @brief Structure defining the device configuration.
 */
typedef struct {
    char *device_name;            ///< Name of the device.
    double logic_voltage;         ///< Logic voltage level of the device.

    // Digital I/O
    int *digital_inputs;          ///< Array of digital input GPIO pins.
    size_t digital_inputs_len;    ///< Length of the digital inputs array.
    char **digital_inputs_names;  ///< Array of names for digital inputs.
    size_t digital_inputs_names_len; ///< Length of the digital inputs names array.
    int *digital_outputs;         ///< Array of digital output GPIO pins.
    size_t digital_outputs_len;   ///< Length of the digital outputs array.
    char **digital_outputs_names; ///< Array of names for digital outputs.
    size_t digital_outputs_names_len; ///< Length of the digital outputs names array.

    // Analog I/O
    int *analog_inputs;           ///< Array of analog input GPIO pins.
    size_t analog_inputs_len;     ///< Length of the analog inputs array.
    char **analog_inputs_names;   ///< Array of names for analog inputs.
    size_t analog_inputs_names_len; ///< Length of the analog inputs names array.
    int *dac_outputs;             ///< Array of DAC output GPIO pins.
    size_t dac_outputs_len;       ///< Length of the DAC outputs array.
    char **dac_outputs_names;     ///< Array of names for DAC outputs.
    size_t dac_outputs_names_len; ///< Length of the DAC outputs names array.

    // One-Wire
    int *one_wire_inputs;         ///< Array of one-wire input GPIO pins.
    size_t one_wire_inputs_len;   ///< Length of the one-wire inputs array.
    char ***one_wire_inputs_names; ///< Array of arrays of names for one-wire inputs.
    size_t *one_wire_inputs_names_len; ///< Array of lengths for one-wire inputs names.
    char ***one_wire_inputs_devices_types; ///< Array of arrays of device types for one-wire inputs.
    size_t *one_wire_inputs_devices_types_len; ///< Array of lengths for one-wire device types.
    char ***one_wire_inputs_devices_addresses; ///< Array of arrays of device addresses for one-wire inputs.
    size_t *one_wire_inputs_devices_addresses_len; ///< Array of lengths for one-wire device addresses.

    // Other
    int pwm_channels;             ///< Number of PWM channels available.
    int max_hardware_timers;      ///< Maximum number of hardware timers.
    bool has_rtos;                ///< Indicates if the device has an RTOS.
    int *uart;                    ///< Array of UART interface pins.
    size_t uart_len;              ///< Length of the UART array.
    int *i2c;                     ///< Array of I2C interface pins.
    size_t i2c_len;               ///< Length of the I2C array.
    int *spi;                     ///< Array of SPI interface pins.
    size_t spi_len;               ///< Length of the SPI array.
    bool usb;                     ///< Indicates if the device has USB support.

    char **parent_devices;        ///< Array of parent device identifiers.
    size_t parent_devices_len;    ///< Length of the parent devices array.
} Device;

/**
 * @brief Global variable holding the device configuration.
 */
extern Device _device;

/**
 * @brief Prints information about the device configuration.
 */
void print_device_info(void);

/**
 * @brief Finds a GPIO pin number based on its name.
 * @param pin_name Name of the pin to find.
 * @param pin Pointer to store the found GPIO pin number.
 * @return bool True if the pin is found, false otherwise.
 */
bool find_pin_by_name(const char *pin_name, gpio_num_t *pin);

/**
 * @brief Initializes the device configuration from a JSON object.
 * @param device JSON object containing the device configuration.
 */
void device_init(cJSON *device);

/**
 * @brief Gets the value of a digital input pin by its name.
 * @param pin_name Name of the digital input pin.
 * @return bool The value of the digital input (true for high, false for low).
 */
bool get_digital_input_value(const char *pin_name);

/**
 * @brief Sets the value of a digital output pin by its name.
 * @param pin_name Name of the digital output pin.
 * @param value Value to set (0 for low, non-zero for high).
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t set_digital_output_value(const char *pin_name, int value);

/**
 * @brief Gets the value of a digital output pin by its name.
 * @param pin_name Name of the digital output pin.
 * @return bool The value of the digital output (true for high, false for low).
 */
bool get_digital_output_value(const char *pin_name);

/**
 * @brief Gets the value of an analog input pin by its name.
 * @param pin_name Name of the analog input pin.
 * @return int The analog input value.
 */
int get_analog_input_value(const char *pin_name);

/**
 * @brief Sets the value of an analog output (DAC) pin by its name.
 * @param pin_name Name of the DAC output pin.
 * @param value Value to set (typically 0-255 for 8-bit DAC).
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t set_analog_output_value(const char *pin_name, uint8_t value);

/**
 * @brief Gets the value of an analog output (DAC) pin by its name.
 * @param pin_name Name of the DAC output pin.
 * @return int The current DAC output value.
 */
int get_analog_output_value(const char *pin_name);

/**
 * @brief Gets the value from a one-wire device by its pin name.
 * @param pin_name Name of the one-wire input pin.
 * @return float The value read from the one-wire device.
 */
float get_one_wire_value(const char *pin_name);

#endif // DEVICE_CONFIG_H