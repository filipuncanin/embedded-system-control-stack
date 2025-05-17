#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "string.h"
#include "wifi.h"

#include "ntp.h"
#include "mqtt.h"

/**
 * @brief Handle for the WiFi event group.
 */
static EventGroupHandle_t wifi_event_group;

/**
 * @brief Tag for logging messages from the WiFi module.
 */
static const char *TAG = "wifi_module";

/**
 * @brief Counter for reconnection attempts.
 */
static int retry_count = 0;

/**
 * @brief Instance for handling any WiFi event.
 */
static esp_event_handler_instance_t instance_any_id = NULL;

/**
 * @brief Instance for handling IP event when STA gets an IP.
 */
static esp_event_handler_instance_t instance_got_ip = NULL;

/**
 * @brief WiFi event handler to manage connection states.
 * @param arg User data (unused).
 * @param event_base Event base (WIFI_EVENT or IP_EVENT).
 * @param event_id Specific event ID.
 * @param event_data Event data (unused).
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    // Check if the event group is valid
    if (wifi_event_group == NULL) {
        ESP_LOGW(TAG, "Event group is null, ignoring event");
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);

        if (MAX_RETRY_COUNT == 0) {
            // Infinite retry logic
            vTaskDelay(pdMS_TO_TICKS(WIFI_TIMEOUT_MS));
            esp_wifi_connect();
            ESP_LOGI(TAG, "Retrying WiFi connection");
        } else if (retry_count < MAX_RETRY_COUNT) {
            // Limited retry logic
            vTaskDelay(pdMS_TO_TICKS(WIFI_TIMEOUT_MS));
            esp_wifi_connect();
            retry_count++;
            ESP_LOGI(TAG, "Retry to connect to the AP (%d/%d)", retry_count, MAX_RETRY_COUNT);
        } else {
            // Max retry limit reached
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "Failed to connect after %d retries", MAX_RETRY_COUNT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        retry_count = 0;  // Reset counter on successful connection
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        // Initialize NTP
        obtain_time();
        // Initialize MQTT
        mqtt_init();
    }
}

/**
 * @brief Initialize the WiFi module and start the connection process.
 */
void wifi_init(void)
{
    wifi_event_group = xEventGroupCreate();

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = {0}, 
            .password = {0},
        },
    };
    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password) - 1);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();

    // Wait for connection or failure
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to Wi-Fi: %s", WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "Failed to connect to Wi-Fi: %s", WIFI_SSID);
    }
}

/**
 * @brief Get the handle to the WiFi event group.
 * @return EventGroupHandle_t Handle to the WiFi event group.
 */
EventGroupHandle_t wifi_get_event_group(void)
{
    return wifi_event_group;
}

/**
 * @brief Stop the WiFi module and disconnect.
 */
void wifi_stop(void) {
    // Deregister event handlers
    if (instance_any_id != NULL) {
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id);
        instance_any_id = NULL;
    }
    if (instance_got_ip != NULL) {
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip);
        instance_got_ip = NULL;
    }

    // Stop WiFi
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();

    // Delete event loop
    esp_event_loop_delete_default();

    // Delete event group
    if (wifi_event_group != NULL) {
        vEventGroupDelete(wifi_event_group);
        wifi_event_group = NULL;
    }

    ESP_LOGI(TAG, "Disconnected from Wi-Fi: %s", WIFI_SSID);
}

/**
 * @brief Check if the WiFi is currently connected.
 * @return bool True if connected, false otherwise.
 */
bool wifi_is_connected(void) {
    return wifi_event_group != NULL && (xEventGroupGetBits(wifi_event_group) & WIFI_CONNECTED_BIT);
}