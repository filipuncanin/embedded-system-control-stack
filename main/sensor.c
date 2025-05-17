#include "sensor.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <string.h>

// Temperature sensors
#include "ds18x20.h"

/**
 * @brief Tag for logging messages from the sensors module.
 */
static const char *TAG = "SENSORS";

/**
 * @brief Convert a hex string to a onewire_addr_t (64-bit address).
 * @param sensor_address Hex string representing the sensor address.
 * @return onewire_addr_t Parsed address, or DS18X20_ANY on failure.
 */
static onewire_addr_t parse_sensor_address(const char *sensor_address) {
    if (!sensor_address || strlen(sensor_address) != 16) {
        ESP_LOGE(TAG, "Invalid sensor address format");
        return DS18X20_ANY;
    }

    uint64_t addr = 0;
    if (sscanf(sensor_address, "%016llx", &addr) != 1) {
        ESP_LOGE(TAG, "Failed to parse sensor address");
        return DS18X20_ANY;
    }
    return (onewire_addr_t)addr;
}

float read_one_wire_sensor(const char *sensor_type, const char *sensor_address, int pin) {
    float value = 0.0f;
    esp_err_t err;
    onewire_addr_t addr = parse_sensor_address(sensor_address);

    if (!sensor_type) {
        ESP_LOGE(TAG, "Invalid sensor type");
        return 0;
    }

    // Read temperature from a DS18x20 sensor
    if (strcmp(sensor_type, "DS18S20/DS1820 (Temperature Sensor)") == 0 || strcmp(sensor_type, "DS1822 (Temperature Sensor)") == 0) {
        err = ds18s20_measure_and_read(pin, addr, &value);
    } else if (strcmp(sensor_type, "DS18B20 (Temperature Sensor)") == 0) {
        err = ds18b20_measure_and_read(pin, addr, &value);
    } else if (strcmp(sensor_type, "MAX31850 (Temperature Sensor)") == 0) {
        err = max31850_measure_and_read(pin, addr, &value);
    } 
    // Add other sensor types here
    else {
        ESP_LOGE(TAG, "Unknown sensor type: %s", sensor_type);
        return 0;
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read sensor value: %s", esp_err_to_name(err));
        return 0;
    }

    return value;
}