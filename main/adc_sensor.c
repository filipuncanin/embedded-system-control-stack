#include "adc_sensor.h"
#include <string.h>
#include "device_config.h"
#include "TM7711.h"

/**
 * @brief Tag for logging messages from the ADC sensor module.
 */
static const char *TAG = "ADC_SENSOR";

/**
 * @brief Array to store the state of all ADC sensors.
 * This array holds the state for up to MAX_ADC_SENSORS sensors.
 */
ADCSensorState sensor_states[MAX_ADC_SENSORS];

/**
 * @brief Counter for the number of configured ADC sensors.
 * Tracks the total number of sensors currently in use.
 */
int sensor_state_count = 0;

/**
 * @brief Maps a value from one range to another.
 * @param value The input value to map.
 * @param fromLow The lower bound of the input range.
 * @param fromHigh The upper bound of the input range.
 * @param toLow The lower bound of the output range.
 * @param toHigh The upper bound of the output range.
 * @return double The mapped value in the target range.
 */
double map_value(double value, double fromLow, double fromHigh, double toLow, double toHigh) {
    if (fromHigh == fromLow) {
        return toLow; // Prevents division by zero
    }
    return (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
}

/**
 * @brief Finds or adds a sensor state based on the sensor name.
 * @param sensor_name The name of the sensor to find or add.
 * @return ADCSensorState* Pointer to the sensor state, or NULL if capacity is exceeded.
 */
static ADCSensorState *find_or_add_sensor_state(const char *sensor_name) {
    // Check if sensor already exists
    for (int i = 0; i < sensor_state_count; i++) {
        if (sensor_states[i].name && strcmp(sensor_states[i].name, sensor_name) == 0) {
            return &sensor_states[i];
        }
    }
    // Add new sensor if capacity allows
    if (sensor_state_count < MAX_ADC_SENSORS) {
        sensor_states[sensor_state_count].name = strdup(sensor_name); // Allocate memory for sensor name
        sensor_states[sensor_state_count].last_value = 0.0;           // Initialize last value
        sensor_states[sensor_state_count].has_value = false;          // Initialize value flag
        sensor_states[sensor_state_count].buffer_index = 0;           // Initialize buffer index
        sensor_states[sensor_state_count].buffer_count = 0;           // Initialize buffer count
        for (int j = 0; j < VALUE_BUFFER_SIZE; j++) {
            sensor_states[sensor_state_count].value_buffer[j] = 0.0;  // Clear value buffer
        }
        return &sensor_states[sensor_state_count++];                 // Return new sensor state and increment count
    }
    ESP_LOGE(TAG, "Sensor capacity exceeded");
    return NULL;
}

esp_err_t adc_sensor_init(char *sensor_type, char *pd_sck, char *dout){
    gpio_num_t dout_pin, pd_sck_pin;
    esp_err_t ret;

    // Find GPIO pins by name
    if (!find_pin_by_name(pd_sck, &pd_sck_pin)) {
        ESP_LOGE(TAG, "PD_SCK pin %s not found", pd_sck);
        return ESP_ERR_INVALID_ARG;
    }
    if (!find_pin_by_name(dout, &dout_pin)) {
        ESP_LOGE(TAG, "DOUT pin %s not found", dout);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Initialize TM7711 sensor if specified
    if (strcmp(sensor_type, "TM7711") == 0) {
        // Initialize TM7711 with specified pins
        ret = tm7711_init(dout_pin, pd_sck_pin);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "TM7711 initialization failed: %d", ret);
            return ret;
        }
        return ESP_OK;
    }

    // Return error for unsupported sensor types
    return ESP_ERR_NOT_SUPPORTED;
}

double adc_sensor_read(char *sensor_type, char *pd_sck, char *dout, double map_low, double map_high, double gain, char *sampling_rate, const char *sensor_name) {
    gpio_num_t dout_pin, pd_sck_pin;
    unsigned long data = 0;
    esp_err_t ret;

    // Find GPIO pins by name
    if (!find_pin_by_name(pd_sck, &pd_sck_pin)) {
        ESP_LOGE(TAG, "PD_SCK pin %s not found", pd_sck);
        return 0.0;
    }
    if (!find_pin_by_name(dout, &dout_pin)) {
        ESP_LOGE(TAG, "DOUT pin %s not found", dout);
        return 0.0;
    }

    // Validate mapping parameters and gain
    if (map_low == map_high || gain < 0) {
        ESP_LOGE(TAG, "Invalid mapping parameters or gain");
        return 0.0;
    }
    
    // Handle TM7711 sensor reading
    if (strcmp(sensor_type, "TM7711") == 0) {
        unsigned char next_select;

        // Map sampling rate string to corresponding constant
        if (strcmp(sampling_rate, "10Hz") == 0) {
            next_select = CH1_10HZ;
        } else if (strcmp(sampling_rate, "40Hz") == 0) {
            next_select = CH1_40HZ;
        } else if (strcmp(sampling_rate, "Temperature") == 0) {
            next_select = CH2_TEMP;
        } else {
            ESP_LOGE(TAG, "Unsupported sampling_rate value: %s", sampling_rate);
            return 0.0;
        }

        // Read data from TM7711 sensor
        ret = tm7711_read(next_select, dout_pin, pd_sck_pin, &data);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "TM7711 read failed: %d", ret);
            return 0.0;
        }

        // Find or add sensor state
        ADCSensorState *state = find_or_add_sensor_state(sensor_name);
        if (!state) {
            return 0.0;
        }

        // Check for extreme values (min=0, max=16777215 for 24-bit ADC)
        if (data == 0 || data == 16777215) {
            ESP_LOGW(TAG, "Extreme value detected for %s: %lu, returning last value", sensor_name, data);
            return state->has_value ? state->last_value : 0.0;
        }

        // Map the raw data to the specified range
        double mapped_value = map_value((double)data, 0, 16777215, map_low, map_high);

        // Update the sensor value buffer
        state->value_buffer[state->buffer_index] = mapped_value;
        state->buffer_index = (state->buffer_index + 1) % VALUE_BUFFER_SIZE;
        if (state->buffer_count < VALUE_BUFFER_SIZE) {
            state->buffer_count++;
        }

        // Calculate the average of buffered values
        double sum = 0.0;
        for (int i = 0; i < state->buffer_count; i++) {
            sum += state->value_buffer[i];
        }
        double avg_value = sum / state->buffer_count;

        // Update the last value and validity flag
        state->last_value = avg_value;
        state->has_value = true;

        return avg_value;
    }

    // Log error for unsupported sensor types
    ESP_LOGE(TAG, "Unsupported sensor type: %s", sensor_type);
    return 0.0;
}