#ifndef _ADC_SENSOR_H_
#define _ADC_SENSOR_H_

#include "driver/gpio.h"

/**
 * @brief Maximum number of ADC sensors supported.
 * This macro defines the maximum number of ADC sensors that can be managed.
 */
#define MAX_ADC_SENSORS 10 // Adjust based on the number of sensors

/**
 * @brief Size of the buffer to store the last sensor values.
 * This macro specifies that the last 3 values are stored for each sensor.
 */
#define VALUE_BUFFER_SIZE 3 // Store the last 3 values

/**
 * @brief Structure to hold the state of an ADC sensor.
 * This structure stores information about the sensor's name, last read value,
 * whether it has a valid value, and a buffer for recent values.
 */
typedef struct {
    char *name;                    // Name of the sensor
    double last_value;             // Last recorded sensor value
    bool has_value;                // Flag indicating if the sensor has a valid value
    double value_buffer[VALUE_BUFFER_SIZE]; // Buffer to store the last few sensor values
    int buffer_index;              // Current index in the value buffer
    int buffer_count;              // Number of values currently stored in the buffer
} ADCSensorState;

/**
 * @brief Array to store the state of all ADC sensors.
 * This external array holds the state for each configured ADC sensor.
 */
extern ADCSensorState sensor_states[];

/**
 * @brief Counter for the number of configured ADC sensors.
 * This external variable tracks the total number of sensors in use.
 */
extern int sensor_state_count;

/**
 * @brief Initializes an ADC sensor with the specified configuration.
 * @param sensor_type Type of the sensor (e.g., model or identifier).
 * @param pd_sck Pin used for the clock signal.
 * @param dout Pin used for data output.
 * @return esp_err_t Error code indicating success or failure of initialization.
 */
esp_err_t adc_sensor_init(char *sensor_type, char *pd_sck, char *dout);

/**
 * @brief Reads the value from an ADC sensor and maps it to a specified range.
 * @param sensor_type Type of the sensor (e.g., model or identifier).
 * @param pd_sck Pin used for the clock signal.
 * @param dout Pin used for data output.
 * @param map_low Lower bound of the mapped output range.
 * @param map_high Upper bound of the mapped output range.
 * @param gain Gain factor to apply to the sensor reading.
 * @param sampling_rate Sampling rate for reading the sensor.
 * @param sensor_name Name of the sensor for identification.
 * @return double The mapped sensor value.
 */
double adc_sensor_read(char *sensor_type, char *pd_sck, char *dout, double map_low, double map_high, double gain, char *sampling_rate, const char *sensor_name);

#endif