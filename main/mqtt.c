#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "mqtt.h"

#include "nvs_utils.h"
#include "one_wire_detect.h"

#include "esp_mac.h"

#include <stdio.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "conf_task_manager.h"
#include "variables.h"

/**
 * @brief Tag for logging messages from the MQTT module.
 */
static const char *TAG = "MQTT_MODULE";

/**
 * @brief Flag indicating whether the MQTT client is connected to the broker.
 */
static bool mqtt_connected = false;

/**
 * @brief Handle for the MQTT client.
 */
esp_mqtt_client_handle_t mqtt_client;

/**
 * @brief Flag indicating whether the application is connected via MQTT.
 */
bool app_connected_mqtt = false;

/**
 * @brief Timestamp of the last received "Present" message.
 */
static TickType_t last_present_time = 0; // Time of the last "Present" message

/**
 * @brief Handle for the connection timeout task.
 */
static TaskHandle_t connection_timeout_task_handle = NULL; // Handle for the task

/**
 * @brief String to store the device's MAC address as a 12-character hexadecimal string.
 */
static char mac_str[13];

/**
 * @brief Array to store MQTT topic strings, each with a maximum length of MAX_TOPIC_LEN.
 */
char topics[8][MAX_TOPIC_LEN]; // Array for all topics

/**
 * @brief Task to monitor the timeout for "Present" messages.
 * @param pvParameters Unused task parameter.
 */
static void connection_timeout_task(void *pvParameters) {
    while (1) {
        if (app_connected_mqtt && (xTaskGetTickCount() - last_present_time > pdMS_TO_TICKS(10000))) {
            ESP_LOGI(TAG, "No 'Present' message received for 10 seconds, disconnecting app");
            app_connected_mqtt = false;
            mqtt_publish("Disconnected", topics[TOPIC_IDX_CONNECTION_RESPONSE], MQTT_QOS);
            // Delete the task as the application is no longer connected
            connection_timeout_task_handle = NULL;
            vTaskDelete(NULL);
        }
        // Check every second
        vTaskDelay(pdMS_TO_TICKS(1000)); // Check every second
    }
}

/**
 * @brief Handles MQTT events such as connection, disconnection, and data reception.
 * @param arg Unused argument.
 * @param event_base Event base identifier.
 * @param event_id Specific event identifier.
 * @param event_data Pointer to event data.
 */
static void mqtt_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            // Handle successful connection to the MQTT broker
            ESP_LOGI(TAG, "MQTT Connected to broker");
            mqtt_connected = true;
            // Subscribe to relevant topics
            esp_mqtt_client_subscribe(mqtt_client, topics[TOPIC_IDX_CONNECTION_REQUEST], MQTT_QOS); // Application requests connection with the device
            esp_mqtt_client_subscribe(mqtt_client, topics[TOPIC_IDX_CONFIG_REQUEST], MQTT_QOS);     // Application requests configuration from the device
            esp_mqtt_client_subscribe(mqtt_client, topics[TOPIC_IDX_CONFIG_RECEIVE], MQTT_QOS);     // Application sends configuration to the device
            esp_mqtt_client_subscribe(mqtt_client, topics[TOPIC_IDX_CHILDREN_LISTENER], MQTT_QOS);  // Application sends configuration to the device
            break;
        case MQTT_EVENT_DISCONNECTED:
            // Handle disconnection from the MQTT broker
            ESP_LOGI(TAG, "MQTT Disconnected");
            mqtt_connected = false;
            app_connected_mqtt = false;
            // Delete the task if it exists
            if (connection_timeout_task_handle != NULL) {
                vTaskDelete(connection_timeout_task_handle);
                connection_timeout_task_handle = NULL;
            }
            break;
        case MQTT_EVENT_SUBSCRIBED:
            // Log successful subscription to a topic
            ESP_LOGI(TAG, "Subscribed to topic");
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            // Log successful unsubscription from a topic
            ESP_LOGI(TAG, "Unsubscribed from topic");
            break;
        case MQTT_EVENT_DATA:
            // Handle incoming MQTT data
            // Validate topic before processing
            if (event->topic == NULL || event->topic_len == 0) {
                ESP_LOGE(TAG, "Received MQTT message with invalid topic (NULL or empty)");
                break;
            }
            // Application requests connection with the device
            else if (strncmp(event->topic, topics[TOPIC_IDX_CONNECTION_REQUEST], event->topic_len) == 0) 
            {
                if (strncmp(event->data, "Present", event->data_len) == 0) {
                    // Update last presence timestamp for "Present" message
                    last_present_time = xTaskGetTickCount(); // Update presence time
                }
                else if(app_connected_mqtt && strncmp(event->data, "Disconnect", event->data_len) == 0){
                    // Handle app disconnection
                    ESP_LOGI(TAG, "App disconnected");
                    app_connected_mqtt = false;
                    // Delete the task if it exists
                    if (connection_timeout_task_handle != NULL) {
                        vTaskDelete(connection_timeout_task_handle);
                        connection_timeout_task_handle = NULL;
                    }
                }
                else if (!app_connected_mqtt && strncmp(event->data, "Connect", event->data_len) == 0){
                    // Handle app connection
                    ESP_LOGI(TAG, "App connected");
                    app_connected_mqtt = true;
                    last_present_time = xTaskGetTickCount(); // Update presence time
                    mqtt_publish("Connected", topics[TOPIC_IDX_CONNECTION_RESPONSE], MQTT_QOS);
                    // Create task to monitor connection timeout
                    if (xTaskCreate(connection_timeout_task, "connection_timeout_task", 2048, NULL, 5, &connection_timeout_task_handle) != pdPASS) {
                        ESP_LOGE(TAG, "Failed to create connection timeout task");
                    }
                }
            }
            // Application requests configuration from the device
            else if (strncmp(event->topic, topics[TOPIC_IDX_CONFIG_REQUEST], event->topic_len) == 0 && app_connected_mqtt)
            {
                ESP_LOGI(TAG, "Configuration requested");
                char *nvs_data = NULL;
                size_t nvs_data_len = 0;
                // Load configuration from NVS
                esp_err_t ret = load_config_from_nvs(&nvs_data, &nvs_data_len);
                if (ret == ESP_OK && nvs_data != NULL) {
                    // Publish configuration to response topic
                    mqtt_publish(nvs_data, topics[TOPIC_IDX_CONFIG_RESPONSE], MQTT_QOS);
                    free(nvs_data);
                    ESP_LOGI(TAG, "Configuration sent successfully");
                } else {
                    ESP_LOGE(TAG, "Configuration sent unsuccessfully");
                }
            }
            // Application sends configuration to the device
            else if (strncmp(event->topic, topics[TOPIC_IDX_CONFIG_RECEIVE], event->topic_len) == 0) 
            {
                // Process received configuration
                configure(event->data, event->data_len, false);
            }
            // Update variables based on information received from remote devices
            else if (strncmp(event->topic, topics[TOPIC_IDX_CHILDREN_LISTENER], event->topic_len) == 0)
            {
                // Update variables based on data from remote devices
                char *json_buffer = strndup(event->data, event->data_len);
                if (json_buffer == NULL) {
                    ESP_LOGE(TAG, "Failed to allocate memory for JSON buffer");
                    return;
                }
                update_variables_from_children(json_buffer);
                free(json_buffer);
            }
            break;
        case MQTT_EVENT_ERROR:
            // Log MQTT error
            ESP_LOGE(TAG, "MQTT Error");
            break;
        default:
            break;
    }
}

void mqtt_init() {
    // Configure MQTT client with broker URI
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    // Register event handler for MQTT events
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    // Start the MQTT client
    esp_mqtt_client_start(mqtt_client);

    // Initialize app connection state
    app_connected_mqtt = false;

    // Get MAC address
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(mac_str, sizeof(mac_str), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Initialize topics with MAC prefix
    const char *suffixes[] = {
        TOPIC_CONNECTION_REQUEST,
        TOPIC_CONNECTION_RESPONSE,
        TOPIC_MONITOR,
        TOPIC_ONE_WIRE,
        TOPIC_CONFIG_REQUEST,
        TOPIC_CONFIG_RESPONSE,
        TOPIC_CONFIG_RECEIVE,
        TOPIC_CHILDREN_LISTENER,
    };
    for (int i = 0; i < 8; i++) {
        snprintf(topics[i], MAX_TOPIC_LEN, "%s%s", mac_str, suffixes[i]);
    }

    // Log the device's MAC address
    ESP_LOGI(TAG, "MAC Address: %s", mac_str);
}

void mqtt_publish(const char *message, const char* topic, int qos) {
    // Publish message if connected to the broker
    if (mqtt_connected)
        esp_mqtt_client_publish(mqtt_client, topic, message, 0, qos, 0);
}

bool mqtt_is_connected(void) {
    return mqtt_connected;
}