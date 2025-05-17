#include "one_wire_detect.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "cJSON.h"
#include "onewire.h"

#include "device_config.h"

/**
 * @brief Tag for logging messages from the one-wire detection module.
 */
static const char *TAG = "ONE_WIRE_DETECT";

// Constants for debouncing
#define DETECTION_THRESHOLD 3  ///< Number of consecutive detections to confirm a sensor.
#define MISS_THRESHOLD 3       ///< Number of consecutive misses to remove a sensor.

// Structure to track sensor state
typedef struct {
    int pin;                  ///< GPIO pin connected to the one-wire bus.
    char address[17];         ///< Hex string representation of the sensor address.
    int detection_count;      ///< Positive for detections, negative for misses.
} SensorState;

// Static variables
static SensorState *sensor_states = NULL;  ///< Array of sensor states.
static size_t sensor_count = 0;           ///< Current number of detected sensors.
static size_t sensor_capacity = 0;        ///< Capacity of the sensor_states array.

/**
 * @brief Search for one-wire sensors on configured pins and return their addresses as JSON.
 * @return char* JSON string containing detected sensor pins and addresses, or NULL on error.
 */
char *search_for_one_wire_sensors(void) {
    onewire_search_t search;
    onewire_addr_t addr;

    // Validate device configuration
    if (!_device.one_wire_inputs || _device.one_wire_inputs_len == 0) {
        // Create empty JSON
        cJSON *root = cJSON_CreateObject();
        cJSON *pins = cJSON_CreateArray();
        cJSON_AddItemToObject(root, "pins", pins);
        char *json_str = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);
        return json_str;
    }

    // Temporary array to mark which sensors were seen in this scan
    bool *seen = calloc(sensor_count, sizeof(bool));
    if (!seen && sensor_count > 0) {
        ESP_LOGE(TAG, "Failed to allocate seen array");
        return NULL;
    }

    // Create JSON object for stable sensors
    cJSON *root = cJSON_CreateObject();
    cJSON *pins = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "pins", pins);

    // Scan each pin
    for (size_t pin_index = 0; pin_index < _device.one_wire_inputs_len; pin_index++) {
        int one_wire_gpio = _device.one_wire_inputs[pin_index];

        // Create JSON for the current pin
        cJSON *pin_obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(pin_obj, "pin", one_wire_gpio);
        cJSON *addresses = cJSON_CreateArray();
        cJSON_AddItemToObject(pin_obj, "addresses", addresses);

        // Find all devices on the OneWire bus for the current pin
        onewire_search_start(&search);
        while ((addr = onewire_search_next(&search, one_wire_gpio)) != ONEWIRE_NONE) {
            // Format address as hexadecimal string
            char addr_str[17];
            snprintf(addr_str, sizeof(addr_str), "%016llX", addr);

            // Check if sensor exists in state list
            int sensor_index = -1;
            for (size_t i = 0; i < sensor_count; i++) {
                if (sensor_states && sensor_states[i].pin == one_wire_gpio && strcmp(sensor_states[i].address, addr_str) == 0) {
                    sensor_index = i;
                    if (seen) seen[i] = true;
                    break;
                }
            }

            // Update or add sensor state
            if (sensor_index >= 0) {
                // Existing sensor, increment detection count
                if (sensor_states[sensor_index].detection_count < DETECTION_THRESHOLD) {
                    sensor_states[sensor_index].detection_count++;
                }
            } else {
                // New sensor, add to state list
                if (sensor_count >= sensor_capacity) {
                    size_t new_capacity = sensor_capacity ? sensor_capacity * 2 : 8;
                    SensorState *new_states = realloc(sensor_states, new_capacity * sizeof(SensorState));
                    if (!new_states) {
                        ESP_LOGE(TAG, "Failed to allocate sensor states");
                        free(seen);
                        cJSON_Delete(root);
                        return NULL;
                    }
                    sensor_states = new_states;
                    sensor_capacity = new_capacity;
                }
                sensor_states[sensor_count] = (SensorState){
                    .pin = one_wire_gpio,
                    .detection_count = 1
                };
                strncpy(sensor_states[sensor_count].address, addr_str, sizeof(sensor_states[sensor_count].address));
                if (seen) seen[sensor_count] = true;
                sensor_count++;
            }
        }

        // Add stable sensors to JSON (detection_count >= DETECTION_THRESHOLD)
        for (size_t i = 0; i < sensor_count; i++) {
            if (sensor_states && sensor_states[i].pin == one_wire_gpio && sensor_states[i].detection_count >= DETECTION_THRESHOLD) {
                cJSON_AddItemToArray(addresses, cJSON_CreateString(sensor_states[i].address));
            }
        }

        // Add pin object to the pins array
        cJSON_AddItemToArray(pins, pin_obj);
    }

    // Update miss counts for sensors not seen in this scan
    for (size_t i = 0; i < sensor_count; i++) {
        if (seen && !seen[i]) {
            if (sensor_states && sensor_states[i].detection_count > -MISS_THRESHOLD) {
                sensor_states[i].detection_count--;
            }
        }
    }

    // Remove sensors that have been missed too many times
    for (size_t i = 0; i < sensor_count;) {
        if (sensor_states && sensor_states[i].detection_count <= -MISS_THRESHOLD) {
            for (size_t j = i; j < sensor_count - 1; j++) {
                sensor_states[j] = sensor_states[j + 1];
            }
            sensor_count--;
        } else {
            i++;
        }
    }

    if (seen) {
        free(seen);
    }

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_str;
}