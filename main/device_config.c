#include "device_config.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>

#include "sensor.h"

/**
 * @brief Tag for logging messages from the device configuration module.
 */
static const char *TAG = "DEVICE CONFIGURATION";

/**
 * @brief Global device configuration structure, initialized to zero.
 */
Device _device = {0};

/**
 * @brief Frees the memory allocated for the Device structure.
 * @param dev Pointer to the Device structure to free.
 */
void free_device(Device *dev) {
    free(dev->device_name);

    // Free Digital I/O
    free(dev->digital_inputs);
    if (dev->digital_inputs_names) {
        for (size_t i = 0; i < dev->digital_inputs_names_len; i++) {
            free(dev->digital_inputs_names[i]);
        }
        free(dev->digital_inputs_names);
    }
    free(dev->digital_outputs);
    if (dev->digital_outputs_names) {
        for (size_t i = 0; i < dev->digital_outputs_names_len; i++) {
            free(dev->digital_outputs_names[i]);
        }
        free(dev->digital_outputs_names);
    }

    // Free Analog I/O
    free(dev->analog_inputs);
    if (dev->analog_inputs_names) {
        for (size_t i = 0; i < dev->analog_inputs_names_len; i++) {
            free(dev->analog_inputs_names[i]);
        }
        free(dev->analog_inputs_names);
    }
    free(dev->dac_outputs);
    if (dev->dac_outputs_names) {
        for (size_t i = 0; i < dev->dac_outputs_names_len; i++) {
            free(dev->dac_outputs_names[i]);
        }
        free(dev->dac_outputs_names);
    }

    // Free One-Wire
    free(dev->one_wire_inputs);
    if (dev->one_wire_inputs_names) {
        for (size_t i = 0; i < dev->one_wire_inputs_len; i++) {
            if (dev->one_wire_inputs_names[i]) {
                for (size_t j = 0; j < dev->one_wire_inputs_names_len[i]; j++) {
                    free(dev->one_wire_inputs_names[i][j]);
                }
                free(dev->one_wire_inputs_names[i]);
            }
        }
        free(dev->one_wire_inputs_names);
    }
    if (dev->one_wire_inputs_devices_types) {
        for (size_t i = 0; i < dev->one_wire_inputs_len; i++) {
            if (dev->one_wire_inputs_devices_types[i]) {
                for (size_t j = 0; j < dev->one_wire_inputs_devices_types_len[i]; j++) {
                    free(dev->one_wire_inputs_devices_types[i][j]);
                }
                free(dev->one_wire_inputs_devices_types[i]);
            }
        }
        free(dev->one_wire_inputs_devices_types);
    }
    if (dev->one_wire_inputs_devices_addresses) {
        for (size_t i = 0; i < dev->one_wire_inputs_len; i++) {
            if (dev->one_wire_inputs_devices_addresses[i]) {
                for (size_t j = 0; j < dev->one_wire_inputs_devices_addresses_len[i]; j++) {
                    free(dev->one_wire_inputs_devices_addresses[i][j]);
                }
                free(dev->one_wire_inputs_devices_addresses[i]);
            }
        }
        free(dev->one_wire_inputs_devices_addresses);
    }
    free(dev->one_wire_inputs_names_len);
    free(dev->one_wire_inputs_devices_types_len);
    free(dev->one_wire_inputs_devices_addresses_len);

    free(dev->uart);
    free(dev->i2c);
    free(dev->spi);

    if (dev->parent_devices) {
        for (size_t i = 0; i < dev->parent_devices_len; i++) {
            free(dev->parent_devices[i]);
        }
        free(dev->parent_devices);
    }

    // Reset the structure to zeros
    memset(dev, 0, sizeof(Device));
}

void print_device_info(void){
    ESP_LOGI(TAG, "Device Info:");
    ESP_LOGI(TAG, "  device_name: %s", _device.device_name ? _device.device_name : "(null)");
    ESP_LOGI(TAG, "  logic_voltage: %.1f", _device.logic_voltage);
    ESP_LOGI(TAG, "  digital_inputs: [%zu elements]", _device.digital_inputs_len);
    for (size_t i = 0; i < _device.digital_inputs_len; i++) {
        ESP_LOGI(TAG, "    - %d", _device.digital_inputs[i]);
    }
    ESP_LOGI(TAG, "  digital_inputs_names: [%zu elements]", _device.digital_inputs_names_len);
    for (size_t i = 0; i < _device.digital_inputs_names_len; i++) {
        ESP_LOGI(TAG, "    - %s", _device.digital_inputs_names[i] ? _device.digital_inputs_names[i] : "(null)");
    }
    ESP_LOGI(TAG, "  digital_outputs: [%zu elements]", _device.digital_outputs_len);
    for (size_t i = 0; i < _device.digital_outputs_len; i++) {
        ESP_LOGI(TAG, "    - %d", _device.digital_outputs[i]);
    }
    ESP_LOGI(TAG, "  digital_outputs_names: [%zu elements]", _device.digital_outputs_names_len);
    for (size_t i = 0; i < _device.digital_outputs_names_len; i++) {
        ESP_LOGI(TAG, "    - %s", _device.digital_outputs_names[i] ? _device.digital_outputs_names[i] : "(null)");
    }
    ESP_LOGI(TAG, "  analog_inputs: [%zu elements]", _device.analog_inputs_len);
    for (size_t i = 0; i < _device.analog_inputs_len; i++) {
        ESP_LOGI(TAG, "    - %d", _device.analog_inputs[i]);
    }
    ESP_LOGI(TAG, "  analog_inputs_names: [%zu elements]", _device.analog_inputs_names_len);
    for (size_t i = 0; i < _device.analog_inputs_names_len; i++) {
        ESP_LOGI(TAG, "    - %s", _device.analog_inputs_names[i] ? _device.analog_inputs_names[i] : "(null)");
    }
    ESP_LOGI(TAG, "  dac_outputs: [%zu elements]", _device.dac_outputs_len);
    for (size_t i = 0; i < _device.dac_outputs_len; i++) {
        ESP_LOGI(TAG, "    - %d", _device.dac_outputs[i]);
    }
    ESP_LOGI(TAG, "  dac_outputs_names: [%zu elements]", _device.dac_outputs_names_len);
    for (size_t i = 0; i < _device.dac_outputs_names_len; i++) {
        ESP_LOGI(TAG, "    - %s", _device.dac_outputs_names[i] ? _device.dac_outputs_names[i] : "(null)");
    }
    ESP_LOGI(TAG, "  one_wire_inputs: [%zu elements]", _device.one_wire_inputs_len);
    for (size_t i = 0; i < _device.one_wire_inputs_len; i++) {
        ESP_LOGI(TAG, "    - %d", _device.one_wire_inputs[i]);
    }
    ESP_LOGI(TAG, "  one_wire_inputs_names: [%zu elements]", _device.one_wire_inputs_len);
    for (size_t i = 0; i < _device.one_wire_inputs_len; i++) {
        ESP_LOGI(TAG, "    Pin %zu: [%zu elements]", i, _device.one_wire_inputs_names_len[i]);
        for (size_t j = 0; j < _device.one_wire_inputs_names_len[i]; j++) {
            ESP_LOGI(TAG, "      - %s", _device.one_wire_inputs_names[i][j] ? _device.one_wire_inputs_names[i][j] : "(null)");
        }
    }
    ESP_LOGI(TAG, "  one_wire_inputs_devices_types: [%zu elements]", _device.one_wire_inputs_len);
    for (size_t i = 0; i < _device.one_wire_inputs_len; i++) {
        ESP_LOGI(TAG, "    Pin %zu: [%zu elements]", i, _device.one_wire_inputs_devices_types_len[i]);
        for (size_t j = 0; j < _device.one_wire_inputs_devices_types_len[i]; j++) {
            ESP_LOGI(TAG, "      - %s", _device.one_wire_inputs_devices_types[i][j] ? _device.one_wire_inputs_devices_types[i][j] : "(null)");
        }
    }
    ESP_LOGI(TAG, "  one_wire_inputs_devices_addresses: [%zu elements]", _device.one_wire_inputs_len);
    for (size_t i = 0; i < _device.one_wire_inputs_len; i++) {
        ESP_LOGI(TAG, "    Pin %zu: [%zu elements]", i, _device.one_wire_inputs_devices_addresses_len[i]);
        for (size_t j = 0; j < _device.one_wire_inputs_devices_addresses_len[i]; j++) {
            ESP_LOGI(TAG, "      - %s", _device.one_wire_inputs_devices_addresses[i][j] ? _device.one_wire_inputs_devices_addresses[i][j] : "(null)");
        }
    }
    ESP_LOGI(TAG, "  pwm_channels: %d", _device.pwm_channels);
    ESP_LOGI(TAG, "  max_hardware_timers: %d", _device.max_hardware_timers);
    ESP_LOGI(TAG, "  has_rtos: %s", _device.has_rtos ? "true" : "false");
    ESP_LOGI(TAG, "  uart: [%zu elements]", _device.uart_len);
    for (size_t i = 0; i < _device.uart_len; i++) {
        ESP_LOGI(TAG, "    - %d", _device.uart[i]);
    }
    ESP_LOGI(TAG, "  i2c: [%zu elements]", _device.i2c_len);
    for (size_t i = 0; i < _device.i2c_len; i++) {
        ESP_LOGI(TAG, "    - %d", _device.i2c[i]);
    }
    ESP_LOGI(TAG, "  spi: [%zu elements]", _device.spi_len);
    for (size_t i = 0; i < _device.spi_len; i++) {
        ESP_LOGI(TAG, "    - %d", _device.spi[i]);
    }
    ESP_LOGI(TAG, "  usb: %s", _device.usb ? "true" : "false");
    ESP_LOGI(TAG, "  parent_devices: [%zu elements]", _device.parent_devices_len);
    for (size_t i = 0; i < _device.parent_devices_len; i++) {
        ESP_LOGI(TAG, "    - %s", _device.parent_devices[i]);
    }
}

/**
 * @brief Loads the device configuration from a JSON object.
 * @param device JSON object containing the device configuration.
 */
void load_device_configuration(cJSON *device)
{
    // Free existing device memory before loading new configuration
    free_device(&_device);

    // device_name
    cJSON *device_name = cJSON_GetObjectItem(device, "device_name");
    if (device_name && cJSON_IsString(device_name) && device_name->valuestring) {
        _device.device_name = strdup(device_name->valuestring);
        if (!_device.device_name) {
            // Log memory allocation error for device_name
            ESP_LOGE(TAG, "Error allocating memory for device_name");
        }
    }

    // logic_voltage
    cJSON *logic_voltage = cJSON_GetObjectItem(device, "logic_voltage");
    if (logic_voltage && cJSON_IsNumber(logic_voltage)) {
        _device.logic_voltage = logic_voltage->valuedouble;
    }

    // digital_inputs
    cJSON *digital_inputs = cJSON_GetObjectItem(device, "digital_inputs");
    if (digital_inputs && cJSON_IsArray(digital_inputs)) {
        _device.digital_inputs_len = cJSON_GetArraySize(digital_inputs);
        _device.digital_inputs = malloc(_device.digital_inputs_len * sizeof(int));
        if (_device.digital_inputs) {
            for (size_t i = 0; i < _device.digital_inputs_len; i++) {
                cJSON *item = cJSON_GetArrayItem(digital_inputs, i);
                if (item && cJSON_IsNumber(item)) {
                    _device.digital_inputs[i] = item->valueint;
                }
            }
        } else {
            if(_device.digital_inputs_len != 0)
                // Log memory allocation error for digital_inputs
                ESP_LOGE(TAG, "Error allocating memory for digital_inputs");
        }
    }

    // digital_inputs_names
    cJSON *digital_inputs_names = cJSON_GetObjectItem(device, "digital_inputs_names");
    if (digital_inputs_names && cJSON_IsArray(digital_inputs_names)) {
        _device.digital_inputs_names_len = cJSON_GetArraySize(digital_inputs_names);
        _device.digital_inputs_names = malloc(_device.digital_inputs_names_len * sizeof(char *));
        if (_device.digital_inputs_names) {
            for (size_t i = 0; i < _device.digital_inputs_names_len; i++) {
                cJSON *item = cJSON_GetArrayItem(digital_inputs_names, i);
                if (item && cJSON_IsString(item) && item->valuestring) {
                    _device.digital_inputs_names[i] = strdup(item->valuestring);
                    if (!_device.digital_inputs_names[i]) {
                        // Log memory allocation error for digital_inputs_names
                        ESP_LOGE(TAG, "Error allocating memory for digital_inputs_names[%zu]", i);
                    }
                } else {
                    _device.digital_inputs_names[i] = NULL;
                }
            }
        } else {
            if(_device.digital_inputs_names_len != 0)
                // Log memory allocation error for digital_inputs_names array
                ESP_LOGE(TAG, "Error allocating memory for digital_inputs_names array");
        }
    }

    // digital_outputs
    cJSON *digital_outputs = cJSON_GetObjectItem(device, "digital_outputs");
    if (digital_outputs && cJSON_IsArray(digital_outputs)) {
        _device.digital_outputs_len = cJSON_GetArraySize(digital_outputs);
        _device.digital_outputs = malloc(_device.digital_outputs_len * sizeof(int));
        if (_device.digital_outputs) {
            for (size_t i = 0; i < _device.digital_outputs_len; i++) {
                cJSON *item = cJSON_GetArrayItem(digital_outputs, i);
                if (item && cJSON_IsNumber(item)) {
                    _device.digital_outputs[i] = item->valueint;
                }
            }
        } else {
            if(_device.digital_outputs_len != 0)
                // Log memory allocation error for digital_outputs
                ESP_LOGE(TAG, "Error allocating memory for digital_outputs");
        }
    }

    // digital_outputs_names
    cJSON *digital_outputs_names = cJSON_GetObjectItem(device, "digital_outputs_names");
    if (digital_outputs_names && cJSON_IsArray(digital_outputs_names)) {
        _device.digital_outputs_names_len = cJSON_GetArraySize(digital_outputs_names);
        _device.digital_outputs_names = malloc(_device.digital_outputs_names_len * sizeof(char *));
        if (_device.digital_outputs_names) {
            for (size_t i = 0; i < _device.digital_outputs_names_len; i++) {
                cJSON *item = cJSON_GetArrayItem(digital_outputs_names, i);
                if (item && cJSON_IsString(item) && item->valuestring) {
                    _device.digital_outputs_names[i] = strdup(item->valuestring);
                    if (!_device.digital_outputs_names[i]) {
                        // Log memory allocation error for digital_outputs_names
                        ESP_LOGE(TAG, "Error allocating memory for digital_outputs_names[%zu]", i);
                    }
                } else {
                    _device.digital_outputs_names[i] = NULL;
                }
            }
        } else {
            if(_device.digital_outputs_names_len != 0)
                // Log memory allocation error for digital_outputs_names array
                ESP_LOGE(TAG, "Error allocating memory for digital_outputs_names array");
        }
    }

    // analog_inputs
    cJSON *analog_inputs = cJSON_GetObjectItem(device, "analog_inputs");
    if (analog_inputs && cJSON_IsArray(analog_inputs)) {
        _device.analog_inputs_len = cJSON_GetArraySize(analog_inputs);
        _device.analog_inputs = malloc(_device.analog_inputs_len * sizeof(int));
        if (_device.analog_inputs) {
            for (size_t i = 0; i < _device.analog_inputs_len; i++) {
                cJSON *item = cJSON_GetArrayItem(analog_inputs, i);
                if (item && cJSON_IsNumber(item)) {
                    _device.analog_inputs[i] = item->valueint;
                }
            }
        } else {
            if(_device.analog_inputs_len != 0)
                // Log memory allocation error for analog_inputs
                ESP_LOGE(TAG, "Error allocating memory for analog_inputs");
        }
    }

    // analog_inputs_names
    cJSON *analog_inputs_names = cJSON_GetObjectItem(device, "analog_inputs_names");
    if (analog_inputs_names && cJSON_IsArray(analog_inputs_names)) {
        _device.analog_inputs_names_len = cJSON_GetArraySize(analog_inputs_names);
        _device.analog_inputs_names = malloc(_device.analog_inputs_names_len * sizeof(char *));
        if (_device.analog_inputs_names) {
            for (size_t i = 0; i < _device.analog_inputs_names_len; i++) {
                cJSON *item = cJSON_GetArrayItem(analog_inputs_names, i);
                if (item && cJSON_IsString(item) && item->valuestring) {
                    _device.analog_inputs_names[i] = strdup(item->valuestring);
                    if (!_device.analog_inputs_names[i]) {
                        // Log memory allocation error for analog_inputs_names
                        ESP_LOGE(TAG, "Error allocating memory for analog_inputs_names[%zu]", i);
                    }
                } else {
                    _device.analog_inputs_names[i] = NULL;
                }
            }
        } else {
            if(_device.analog_inputs_names_len != 0)
                // Log memory allocation error for analog_inputs_names array
                ESP_LOGE(TAG, "Error allocating memory for analog_inputs_names array");
        }
    }

    // dac_outputs
    cJSON *dac_outputs = cJSON_GetObjectItem(device, "dac_outputs");
    if (dac_outputs && cJSON_IsArray(dac_outputs)) {
        _device.dac_outputs_len = cJSON_GetArraySize(dac_outputs);
        _device.dac_outputs = malloc(_device.dac_outputs_len * sizeof(int));
        if (_device.dac_outputs) {
            for (size_t i = 0; i < _device.dac_outputs_len; i++) {
                cJSON *item = cJSON_GetArrayItem(dac_outputs, i);
                if (item && cJSON_IsNumber(item)) {
                    _device.dac_outputs[i] = item->valueint;
                }
            }
        } else {
            if(_device.dac_outputs_len != 0)
                // Log memory allocation error for dac_outputs
                ESP_LOGE(TAG, "Error allocating memory for dac_outputs");
        }
    }

    // dac_outputs_names
    cJSON *dac_outputs_names = cJSON_GetObjectItem(device, "dac_outputs_names");
    if (dac_outputs_names && cJSON_IsArray(dac_outputs_names)) {
        _device.dac_outputs_names_len = cJSON_GetArraySize(dac_outputs_names);
        _device.dac_outputs_names = malloc(_device.dac_outputs_names_len * sizeof(char *));
        if (_device.dac_outputs_names) {
            for (size_t i = 0; i < _device.dac_outputs_names_len; i++) {
                cJSON *item = cJSON_GetArrayItem(dac_outputs_names, i);
                if (item && cJSON_IsString(item) && item->valuestring) {
                    _device.dac_outputs_names[i] = strdup(item->valuestring);
                    if (!_device.dac_outputs_names[i]) {
                        // Log memory allocation error for dac_outputs_names
                        ESP_LOGE(TAG, "Error allocating memory for dac_outputs_names[%zu]", i);
                    }
                } else {
                    _device.dac_outputs_names[i] = NULL;
                }
            }
        } else {
            if(_device.dac_outputs_names_len != 0)
                // Log memory allocation error for dac_outputs_names array
                ESP_LOGE(TAG, "Error allocating memory for dac_outputs_names array");
        }
    }

    // one_wire_inputs
    cJSON *one_wire_inputs = cJSON_GetObjectItem(device, "one_wire_inputs");
    if (one_wire_inputs && cJSON_IsArray(one_wire_inputs)) {
        _device.one_wire_inputs_len = cJSON_GetArraySize(one_wire_inputs);
        _device.one_wire_inputs = malloc(_device.one_wire_inputs_len * sizeof(int));
        if (_device.one_wire_inputs) {
            for (size_t i = 0; i < _device.one_wire_inputs_len; i++) {
                cJSON *item = cJSON_GetArrayItem(one_wire_inputs, i);
                if (item && cJSON_IsNumber(item)) {
                    _device.one_wire_inputs[i] = item->valueint;
                }
            }
        } else {
            if(_device.one_wire_inputs_len != 0)
                // Log memory allocation error for one_wire_inputs
                ESP_LOGE(TAG, "Error allocating memory for one_wire_inputs");
        }
    }

    // one_wire_inputs_names
    cJSON *one_wire_inputs_names = cJSON_GetObjectItem(device, "one_wire_inputs_names");
    if (one_wire_inputs_names && cJSON_IsArray(one_wire_inputs_names)) {
        _device.one_wire_inputs_names = malloc(_device.one_wire_inputs_len * sizeof(char **));
        _device.one_wire_inputs_names_len = malloc(_device.one_wire_inputs_len * sizeof(size_t));
        if (_device.one_wire_inputs_names && _device.one_wire_inputs_names_len) {
            for (size_t i = 0; i < _device.one_wire_inputs_len; i++) {
                cJSON *sub_array = cJSON_GetArrayItem(one_wire_inputs_names, i);
                if (sub_array && cJSON_IsArray(sub_array)) {
                    _device.one_wire_inputs_names_len[i] = cJSON_GetArraySize(sub_array);
                    _device.one_wire_inputs_names[i] = malloc(_device.one_wire_inputs_names_len[i] * sizeof(char *));
                    if (_device.one_wire_inputs_names[i]) {
                        for (size_t j = 0; j < _device.one_wire_inputs_names_len[i]; j++) {
                            cJSON *item = cJSON_GetArrayItem(sub_array, j);
                            if (item && cJSON_IsString(item) && item->valuestring) {
                                _device.one_wire_inputs_names[i][j] = strdup(item->valuestring);
                                if (!_device.one_wire_inputs_names[i][j]) {
                                    // Log memory allocation error for one_wire_inputs_names
                                    ESP_LOGE(TAG, "Error allocating memory for one_wire_inputs_names[%zu][%zu]", i, j);
                                }
                            } else {
                                _device.one_wire_inputs_names[i][j] = NULL;
                            }
                        }
                    } else {
                        if (_device.one_wire_inputs_names_len[i] != 0)
                            // Log memory allocation error for one_wire_inputs_names subarray
                            ESP_LOGE(TAG, "Error allocating memory for one_wire_inputs_names[%zu]", i);
                    }
                } else {
                    _device.one_wire_inputs_names_len[i] = 0;
                    _device.one_wire_inputs_names[i] = NULL;
                }
            }
        } else {
            // Log memory allocation error for one_wire_inputs_names or its length array
            ESP_LOGE(TAG, "Error allocating memory for one_wire_inputs_names or one_wire_inputs_names_len");
        }
    }

    // one_wire_inputs_devices_types
    cJSON *one_wire_inputs_devices_types = cJSON_GetObjectItem(device, "one_wire_inputs_devices_types");
    if (one_wire_inputs_devices_types && cJSON_IsArray(one_wire_inputs_devices_types)) {
        _device.one_wire_inputs_devices_types = malloc(_device.one_wire_inputs_len * sizeof(char **));
        _device.one_wire_inputs_devices_types_len = malloc(_device.one_wire_inputs_len * sizeof(size_t));
        if (_device.one_wire_inputs_devices_types && _device.one_wire_inputs_devices_types_len) {
            for (size_t i = 0; i < _device.one_wire_inputs_len; i++) {
                cJSON *sub_array = cJSON_GetArrayItem(one_wire_inputs_devices_types, i);
                if (sub_array && cJSON_IsArray(sub_array)) {
                    _device.one_wire_inputs_devices_types_len[i] = cJSON_GetArraySize(sub_array);
                    _device.one_wire_inputs_devices_types[i] = malloc(_device.one_wire_inputs_devices_types_len[i] * sizeof(char *));
                    if (_device.one_wire_inputs_devices_types[i]) {
                        for (size_t j = 0; j < _device.one_wire_inputs_devices_types_len[i]; j++) {
                            cJSON *item = cJSON_GetArrayItem(sub_array, j);
                            if (item && cJSON_IsString(item) && item->valuestring) {
                                _device.one_wire_inputs_devices_types[i][j] = strdup(item->valuestring);
                                if (!_device.one_wire_inputs_devices_types[i][j]) {
                                    // Log memory allocation error for one_wire_inputs_devices_types
                                    ESP_LOGE(TAG, "Error allocating memory for one_wire_inputs_devices_types[%zu][%zu]", i, j);
                                }
                            } else {
                                _device.one_wire_inputs_devices_types[i][j] = NULL;
                            }
                        }
                    } else {
                        if (_device.one_wire_inputs_devices_types_len[i] != 0)
                            // Log memory allocation error for one_wire_inputs_devices_types subarray
                            ESP_LOGE(TAG, "Error allocating memory for one_wire_inputs_devices_types[%zu]", i);
                    }
                } else {
                    _device.one_wire_inputs_devices_types_len[i] = 0;
                    _device.one_wire_inputs_devices_types[i] = NULL;
                }
            }
        } else {
            // Log memory allocation error for one_wire_inputs_devices_types or its length array
            ESP_LOGE(TAG, "Error allocating memory for one_wire_inputs_devices_types or one_wire_inputs_devices_types_len");
        }
    }

    // one_wire_inputs_devices_addresses
    cJSON *one_wire_inputs_devices_addresses = cJSON_GetObjectItem(device, "one_wire_inputs_devices_addresses");
    if (one_wire_inputs_devices_addresses && cJSON_IsArray(one_wire_inputs_devices_addresses)) {
        _device.one_wire_inputs_devices_addresses = malloc(_device.one_wire_inputs_len * sizeof(char **));
        _device.one_wire_inputs_devices_addresses_len = malloc(_device.one_wire_inputs_len * sizeof(size_t));
        if (_device.one_wire_inputs_devices_addresses && _device.one_wire_inputs_devices_addresses_len) {
            for (size_t i = 0; i < _device.one_wire_inputs_len; i++) {
                cJSON *sub_array = cJSON_GetArrayItem(one_wire_inputs_devices_addresses, i);
                if (sub_array && cJSON_IsArray(sub_array)) {
                    _device.one_wire_inputs_devices_addresses_len[i] = cJSON_GetArraySize(sub_array);
                    _device.one_wire_inputs_devices_addresses[i] = malloc(_device.one_wire_inputs_devices_addresses_len[i] * sizeof(char *));
                    if (_device.one_wire_inputs_devices_addresses[i]) {
                        for (size_t j = 0; j < _device.one_wire_inputs_devices_addresses_len[i]; j++) {
                            cJSON *item = cJSON_GetArrayItem(sub_array, j);
                            if (item && cJSON_IsString(item) && item->valuestring) {
                                _device.one_wire_inputs_devices_addresses[i][j] = strdup(item->valuestring);
                                if (!_device.one_wire_inputs_devices_addresses[i][j]) {
                                    // Log memory allocation error for one_wire_inputs_devices_addresses
                                    ESP_LOGE(TAG, "Error allocating memory for one_wire_inputs_devices_addresses[%zu][%zu]", i, j);
                                }
                            } else {
                                _device.one_wire_inputs_devices_addresses[i][j] = NULL;
                            }
                        }
                    } else {
                        if (_device.one_wire_inputs_devices_addresses_len[i] != 0)
                            // Log memory allocation error for one_wire_inputs_devices_addresses subarray
                            ESP_LOGE(TAG, "Error allocating memory for one_wire_inputs_devices_addresses[%zu]", i);
                    }
                } else {
                    _device.one_wire_inputs_devices_addresses_len[i] = 0;
                    _device.one_wire_inputs_devices_addresses[i] = NULL;
                }
            }
        } else {
            // Log memory allocation error for one_wire_inputs_devices_addresses or its length array
            ESP_LOGE(TAG, "Error allocating memory for one_wire_inputs_devices_addresses or one_wire_inputs_devices_addresses_len");
        }
    }

    // pwm_channels
    cJSON *pwm_channels = cJSON_GetObjectItem(device, "pwm_channels");
    if (pwm_channels && cJSON_IsNumber(pwm_channels)) {
        _device.pwm_channels = pwm_channels->valueint;
    }

    // max_hardware_timers
    cJSON *max_hardware_timers = cJSON_GetObjectItem(device, "max_hardware_timers");
    if (max_hardware_timers && cJSON_IsNumber(max_hardware_timers)) {
        _device.max_hardware_timers = max_hardware_timers->valueint;
    }

    // has_rtos
    cJSON *has_rtos = cJSON_GetObjectItem(device, "has_rtos");
    if (has_rtos && cJSON_IsBool(has_rtos)) {
        _device.has_rtos = cJSON_IsTrue(has_rtos);
    }

    // UART
    cJSON *uart = cJSON_GetObjectItem(device, "UART");
    if (uart && cJSON_IsArray(uart)) {
        _device.uart_len = cJSON_GetArraySize(uart);
        _device.uart = malloc(_device.uart_len * sizeof(int));
        if (_device.uart) {
            for (size_t i = 0; i < _device.uart_len; i++) {
                cJSON *item = cJSON_GetArrayItem(uart, i);
                if (item && cJSON_IsNumber(item)) {
                    _device.uart[i] = item->valueint;
                }
            }
        } else {
            // Log memory allocation error for UART
            ESP_LOGE(TAG, "Error allocating memory for UART");
        }
    }

    // I2C
    cJSON *i2c = cJSON_GetObjectItem(device, "I2C");
    if (i2c && cJSON_IsArray(i2c)) {
        _device.i2c_len = cJSON_GetArraySize(i2c);
        _device.i2c = malloc(_device.i2c_len * sizeof(int));
        if (_device.i2c) {
            for (size_t i = 0; i < _device.i2c_len; i++) {
                cJSON *item = cJSON_GetArrayItem(i2c, i);
                if (item && cJSON_IsNumber(item)) {
                    _device.i2c[i] = item->valueint;
                }
            }
        } else {
            // Log memory allocation error for I2C
            ESP_LOGE(TAG, "Error allocating memory for I2C");
        }
    }

    // SPI
    cJSON *spi = cJSON_GetObjectItem(device, "SPI");
    if (spi && cJSON_IsArray(spi)) {
        _device.spi_len = cJSON_GetArraySize(spi);
        _device.spi = malloc(_device.spi_len * sizeof(int));
        if (_device.spi) {
            for (size_t i = 0; i < _device.spi_len; i++) {
                cJSON *item = cJSON_GetArrayItem(spi, i);
                if (item && cJSON_IsNumber(item)) {
                    _device.spi[i] = item->valueint;
                }
            }
        } else {
            // Log memory allocation error for SPI
            ESP_LOGE(TAG, "Error allocating memory for SPI");
        }
    }

    // USB
    cJSON *usb = cJSON_GetObjectItem(device, "USB");
    if (usb && cJSON_IsBool(usb)) {
        _device.usb = cJSON_IsTrue(usb);
    }

    // parent_devices
    cJSON *parent_devices = cJSON_GetObjectItem(device, "parent_devices");
    if (parent_devices && cJSON_IsArray(parent_devices)) {
        _device.parent_devices_len = cJSON_GetArraySize(parent_devices);
        _device.parent_devices = malloc(_device.parent_devices_len * sizeof(char *));
        if (_device.parent_devices) {
            for (size_t i = 0; i < _device.parent_devices_len; i++) {
                cJSON *item = cJSON_GetArrayItem(parent_devices, i);
                if (item && cJSON_IsString(item) && item->valuestring) {
                    _device.parent_devices[i] = strdup(item->valuestring);
                    if (!_device.parent_devices[i]) {
                        // Log memory allocation error for parent_devices
                        ESP_LOGE(TAG, "Error allocating memory for parent_devices[%zu]", i);
                    }
                } else {
                    _device.parent_devices[i] = NULL;
                }
            }
        } else {
            if(_device.parent_devices_len != 0)
                // Log memory allocation error for parent_devices array
                ESP_LOGE(TAG, "Error allocating memory for parent_devices array");
        }
    }
}

bool find_pin_by_name(const char *pin_name, gpio_num_t *pin) {
    if (!pin_name || !pin) {
        return false;
    }

    // Check digital inputs
    for (size_t i = 0; i < _device.digital_inputs_names_len && i < _device.digital_inputs_len; i++) {
        if (_device.digital_inputs_names[i] && strcmp(pin_name, _device.digital_inputs_names[i]) == 0) {
            *pin = (gpio_num_t)_device.digital_inputs[i];
            return true;
        }
    }

    // Check digital outputs
    for (size_t i = 0; i < _device.digital_outputs_names_len && i < _device.digital_outputs_len; i++) {
        if (_device.digital_outputs_names[i] && strcmp(pin_name, _device.digital_outputs_names[i]) == 0) {
            *pin = (gpio_num_t)_device.digital_outputs[i];
            return true;
        }
    }

    // Check analog inputs
    for (size_t i = 0; i < _device.analog_inputs_names_len && i < _device.analog_inputs_len; i++) {
        if (_device.analog_inputs_names[i] && strcmp(pin_name, _device.analog_inputs_names[i]) == 0) {
            *pin = (gpio_num_t)_device.analog_inputs[i];
            return true;
        }
    }

    // Check DAC outputs
    for (size_t i = 0; i < _device.dac_outputs_names_len && i < _device.dac_outputs_len; i++) {
        if (_device.dac_outputs_names[i] && strcmp(pin_name, _device.dac_outputs_names[i]) == 0) {
            *pin = (gpio_num_t)_device.dac_outputs[i];
            return true;
        }
    }

    // Check OneWire inputs
    for (size_t i = 0; i < _device.one_wire_inputs_len; i++) {
        for (size_t j = 0; j < _device.one_wire_inputs_names_len[i]; j++) {
            if (_device.one_wire_inputs_names[i][j] && strcmp(pin_name, _device.one_wire_inputs_names[i][j]) == 0) {
                *pin = (gpio_num_t)_device.one_wire_inputs[i];
                return true;
            }
        }
    }

    return false;
}

// =================== INITIALIZATION DIGITAL I/O ===================
/**
 * @brief Initializes digital input pins.
 */
void init_digital_inputs(void) {
    if (_device.digital_inputs == NULL || _device.digital_inputs_len == 0) {
        // Log warning if no digital inputs are available
        ESP_LOGW(TAG, "No digital inputs to initialize");
        return;
    }
    
    for (size_t i = 0; i < _device.digital_inputs_len; i++) {
        gpio_num_t pin = (gpio_num_t)_device.digital_inputs[i];
        const char *name = _device.digital_inputs_names && i < _device.digital_inputs_names_len && _device.digital_inputs_names[i] ? _device.digital_inputs_names[i] : "unnamed";

        // Log initialization of digital input
        ESP_LOGI(TAG, "Initializing digital input %s on GPIO %d", name, pin);

        gpio_config_t io_conf = {
            .intr_type = GPIO_INTR_DISABLE,
            .mode = GPIO_MODE_INPUT,
            .pin_bit_mask = (1ULL << pin),
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .pull_up_en = GPIO_PULLUP_DISABLE
        };

        esp_err_t err = gpio_config(&io_conf);
        if (err != ESP_OK) {
            // Log error during GPIO configuration
            ESP_LOGE(TAG, "Error configuring GPIO %d: %s", pin, esp_err_to_name(err));
            continue;
        }

        // Log successful initialization
        ESP_LOGI(TAG, "Digital input %s (GPIO %d) successfully initialized", name, pin);
    }
}

/**
 * @brief Initializes digital output pins.
 */
void init_digital_outputs(void) {
    if (_device.digital_outputs == NULL || _device.digital_outputs_len == 0) {
        // Log warning if no digital outputs are available
        ESP_LOGW(TAG, "No digital outputs to initialize");
        return;
    }

    for (size_t i = 0; i < _device.digital_outputs_len; i++) {
        gpio_num_t pin = (gpio_num_t)_device.digital_outputs[i];
        const char *name = _device.digital_outputs_names && i < _device.digital_outputs_names_len && _device.digital_outputs_names[i] ? _device.digital_outputs_names[i] : "unnamed";

        // Log initialization of digital output
        ESP_LOGI(TAG, "Initializing digital output %s on GPIO %d", name, pin);

        gpio_config_t io_conf = {
            .intr_type = GPIO_INTR_DISABLE,                // Disable interrupts
            .mode = GPIO_MODE_INPUT_OUTPUT,                // Set as output (with input for reading)
            .pin_bit_mask = (1ULL << pin),                 // Pin mask (64-bit)
            .pull_down_en = GPIO_PULLDOWN_DISABLE,         // Disable pull-down
            .pull_up_en = GPIO_PULLUP_DISABLE              // Disable pull-up
        };

        esp_err_t err = gpio_config(&io_conf);
        if (err != ESP_OK) {
            // Log error during GPIO configuration
            ESP_LOGE(TAG, "Error configuring GPIO %d: %s", pin, esp_err_to_name(err));
            continue;
        }

        // Log successful initialization
        ESP_LOGI(TAG, "Digital output %s (GPIO %d) successfully initialized", name, pin);
    }
}

// =========== INITIALIZATION ANALOG I/O (not implemented) ===================
/**
 * @brief Initializes analog input pins (not implemented).
 */
void init_analog_inputs(void) {

}

/**
 * @brief Initializes analog output pins (not implemented).
 */
void init_analog_outputs(void) {

}

// ================= INITIALIZATION ONEWIRE INPUTS ===================
/**
 * @brief Initializes one-wire input pins.
 */
void init_one_wire_inputs(void) {
    if (_device.one_wire_inputs == NULL || _device.one_wire_inputs_len == 0) {
        // Log warning if no one-wire inputs are available
        ESP_LOGW(TAG, "No OneWire inputs to initialize");
        return;
    }

    for (size_t i = 0; i < _device.one_wire_inputs_len; i++) {
        gpio_num_t pin = (gpio_num_t)_device.one_wire_inputs[i];

        // Log initialization of one-wire input
        ESP_LOGI(TAG, "Initializing OneWire input on GPIO %d", pin);

        // Reset pin to default state (commented out)
        // esp_err_t err; // = gpio_reset_pin(pin);
        // if (err != ESP_OK) {
        //     ESP_LOGE(TAG, "Error resetting GPIO %d: %s", pin, esp_err_to_name(err));
        //     continue;
        // }

        // Set direction to input
        esp_err_t err = gpio_set_direction(pin, GPIO_MODE_INPUT);
        if (err != ESP_OK) {
            // Log error setting GPIO direction
            ESP_LOGE(TAG, "Error setting direction for GPIO %d: %s", pin, esp_err_to_name(err));
            continue;
        }

        // Set pull-up mode (required for OneWire)
        err = gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
        if (err != ESP_OK) {
            // Log error setting pull mode
            ESP_LOGE(TAG, "Error setting pull mode for GPIO %d: %s", pin, esp_err_to_name(err));
            continue;
        }

        // Log successful initialization
        ESP_LOGI(TAG, "OneWire input (GPIO %d) successfully initialized", pin);
    }
}

// =================== DEVICE INITIALIZATION ===================
void device_init(cJSON *device)
{
    load_device_configuration(device);

    init_digital_inputs();
    init_digital_outputs();
    init_analog_inputs();
    init_analog_outputs();
    init_one_wire_inputs();
}

// ========================= DIGITAL I/O ===========================
bool get_digital_input_value(const char *pin_name) {
    gpio_num_t pin;
    if (!find_pin_by_name(pin_name, &pin)) {
        // Log error if digital input is not found
        ESP_LOGE(TAG, "Digital input %s not found", pin_name);
        return -1;
    }

    bool value = gpio_get_level(pin);
    // Log digital input value (commented out)
    // ESP_LOGI(TAG, "Digital input %s (GPIO %d): %d", pin_name, pin, value);
    return value;
}

esp_err_t set_digital_output_value(const char *pin_name, int value) {
    gpio_num_t pin;
    if (!find_pin_by_name(pin_name, &pin)) {
        // Log error if digital output is not found
        ESP_LOGE(TAG, "Digital output %s not found", pin_name);
        return ESP_ERR_NOT_FOUND;
    }

    esp_err_t err = gpio_set_level(pin, value);
    if (err != ESP_OK) {
        // Log error setting digital output
        ESP_LOGE(TAG, "Error setting digital output %s (GPIO %d): %s", pin_name, pin, esp_err_to_name(err));
        return err;
    }

    // Log digital output value set (commented out)
    // ESP_LOGI(TAG, "Digital output %s (GPIO %d) set to %d", pin_name, pin, value);
    return ESP_OK;
}

bool get_digital_output_value(const char *pin_name) {
    gpio_num_t pin;
    if (!find_pin_by_name(pin_name, &pin)) {
        // Log error if digital output is not found
        ESP_LOGE(TAG, "Digital output %s not found", pin_name);
        return -1;
    }

    bool value = gpio_get_level(pin);
    // Log digital output value (commented out)
    // ESP_LOGI(TAG, "Digital output %s (GPIO %d): %d", pin_name, pin, value);
    return value;
}

// ================== ANALOG I/O (not implemented) ===========================
int get_analog_input_value(const char *pin_name) {
    return -1;
}

esp_err_t set_analog_output_value(const char *pin_name, uint8_t value) {
    return ESP_OK;
}

int get_analog_output_value(const char *pin_name) {
    return -1;
}

// ========================= ONE WIRE ===========================
float get_one_wire_value(const char *pin_name) {
    if (!pin_name) {
        // Log error if pin name is not provided
        ESP_LOGE(TAG, "No OneWire input name provided");
        return -1.0f;
    }

    // Find the corresponding OneWire pin and device index
    for (size_t i = 0; i < _device.one_wire_inputs_len; i++) {
        if (_device.one_wire_inputs_names[i]) {
            for (size_t j = 0; j < _device.one_wire_inputs_names_len[i]; j++) {
                if (_device.one_wire_inputs_names[i][j] && strcmp(pin_name, _device.one_wire_inputs_names[i][j]) == 0) {
                    // Found the name, check type and address
                    if (j < _device.one_wire_inputs_devices_types_len[i] && j < _device.one_wire_inputs_devices_addresses_len[i]  &&  i < _device.one_wire_inputs_len) {
                        const char *sensor_type = _device.one_wire_inputs_devices_types[i][j];
                        const char *sensor_address = _device.one_wire_inputs_devices_addresses[i][j];
                        gpio_num_t pin = (gpio_num_t)_device.one_wire_inputs[i];
                        if (sensor_type && sensor_address) {
                            // Call external function to read the sensor
                            float value = read_one_wire_sensor(sensor_type, sensor_address, pin);
                            return value;
                            return 0; // Note: This line is unreachable due to the previous return
                        } else {
                            // Log error if sensor type or address is missing
                            ESP_LOGE(TAG, "Missing type or address for OneWire sensor %s", pin_name);
                            return -1.0f;
                        }
                    } else {
                        // Log error if array lengths do not match
                        ESP_LOGE(TAG, "Array length mismatch for OneWire sensor %s", pin_name);
                        return -1.0f;
                    }
                }
            }
        }
    }

    // Log error if OneWire input is not found
    ESP_LOGE(TAG, "OneWire input %s not found", pin_name);
    return -1.0f;
}