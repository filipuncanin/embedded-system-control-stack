#include "freertos/FreeRTOS.h"
#include "freertos/task.h" 
#include "esp_system.h"
#include "driver/gpio.h"
#include <esp_log.h>                
#include <esp_err.h>                             

#include "wifi.h"
#include "mqtt.h"
#include "nvs_utils.h"

#include "variables.h"

#include "one_wire_detect.h"
#include "conf_task_manager.h"

#include "ble.h"

/**
 * @brief GPIO pin used for output on this specific device.
 * This pin (GPIO18) is configured for output and is specific to this device; it is not required for all devices.
 */
#define GPIO18_OUTPUT_PIN 18

/**
 * @brief Tag for logging messages from the main application module.
 */
static const char *TAG = "filip_device";

/**
 * @brief Main application entry point.
 * Initializes hardware, network, and communication modules, then enters an infinite loop
 * to handle periodic tasks such as sending variables and publishing sensor data.
 */
void app_main(void)
{
    // Initialize GPIO18 for output (specific to this device, not required for all devices)
    gpio_reset_pin(GPIO18_OUTPUT_PIN);
    gpio_set_direction(GPIO18_OUTPUT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO18_OUTPUT_PIN, 1);

    // Initialize Non-Volatile Storage (NVS) for Wi-Fi and configuration
    esp_err_t ret = nvs_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS, halting...");
        return;
    }

    // Load configuration from NVS
    char *nvs_data = NULL;
    size_t nvs_data_len = 0;
    ret = load_config_from_nvs(&nvs_data, &nvs_data_len);
    if (ret == ESP_OK && nvs_data != NULL) {
        configure(nvs_data, nvs_data_len, true); // Apply loaded configuration
        free(nvs_data);                          // Free allocated memory
    }

    // Initialize Wi-Fi (includes NTP and MQTT initialization within wifi.c)
    wifi_init();

    // Initialize Bluetooth Low Energy (BLE)
    ble_init();

    // Counter for periodic tasks
    while(1){
        // Log free heap size
        // printf("Heap %lu bytes\n", esp_get_free_heap_size());

        // Send variables to parent devices if MQTT is connected
        if (mqtt_is_connected()){
            send_variables_to_parents(); 
        }   

        // Publish sensor data if the application is connected via MQTT
        if(app_connected_mqtt) {
            char *monitor_json = read_variables_json(); // Read variables as JSON
            if (monitor_json) {
                mqtt_publish(monitor_json, topics[TOPIC_IDX_MONITOR], MQTT_QOS); // Publish variables
                free(monitor_json); // Free allocated memory
            }
            char *one_wire_json = search_for_one_wire_sensors(); // Read one-wire sensor data
            if (one_wire_json) {
                mqtt_publish(one_wire_json, topics[TOPIC_IDX_ONE_WIRE], MQTT_QOS); // Publish sensor data
                free(one_wire_json); // Free allocated memory
            }
        } else if (app_connected_ble) {
            // Placeholder for BLE-specific functionality (currently empty)
        }

        // Delay for 100ms before the next iteration
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}