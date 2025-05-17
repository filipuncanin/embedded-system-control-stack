#ifndef WIFI_H
#define WIFI_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "config.h"

/**
 * @brief Maximum number of reconnection attempts (0 means infinite retries).
 */
#define MAX_RETRY_COUNT 0  

/**
 * @brief WiFi connection timeout in milliseconds (10 seconds).
 */
#define WIFI_TIMEOUT_MS 10000  

/**
 * @brief WiFi credentials (defined in config.h).
 * @note These are not defined here but expected to be set in config.h.
 */
// #define WIFI_SSID  
// #define WIFI_PASS    

/**
 * @brief Bit definition for WiFi connected status in event group.
 */
#define WIFI_CONNECTED_BIT BIT0  

/**
 * @brief Bit definition for WiFi failure status in event group.
 */
#define WIFI_FAIL_BIT      BIT1  

/**
 * @brief Get the handle to the WiFi event group.
 * @return EventGroupHandle_t Handle to the WiFi event group.
 */
EventGroupHandle_t wifi_get_event_group(void);

/**
 * @brief Initialize the WiFi module and start the connection process.
 */
void wifi_init(void);

/**
 * @brief Stop the WiFi module and disconnect.
 */
void wifi_stop(void);

/**
 * @brief Check if the WiFi is currently connected.
 * @return bool True if connected, false otherwise.
 */
bool wifi_is_connected(void);

#endif // WIFI_H