#include "ntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "variables.h"

/**
 * @brief Tag for logging messages from the NTP module.
 */
static const char *TAG = "NTP";

/**
 * @brief Global variables to store the current time and date.
 */
int hour, minute, second, day, month, year, day_in_year;

/**
 * @brief Current time in seconds since epoch.
 */
time_t now;

/**
 * @brief Structure to hold broken-down time information.
 */
struct tm timeinfo;

/**
 * @brief Flag indicating whether NTP synchronization is complete.
 */
bool ntp_sync = false;

/**
 * @brief Checks if the system time is synchronized with an NTP server.
 * @return bool True if NTP synchronization is complete, false otherwise.
 */
bool is_ntp_sync(void)
{
    return ntp_sync;
}

/**
 * @brief Callback function triggered on successful NTP time synchronization.
 * @param tv Pointer to the synchronized time value.
 */
void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
    ntp_sync = true;

    // Set timezone to Central European Time with daylight saving rules
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();

    // Log the current local time
    char strftime_buf[64];
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%H:%M:%S %d.%m.%Y.", &timeinfo);
    ESP_LOGI(TAG, "Current Time: %s", strftime_buf);
}

/**
 * @brief Task to continuously update the current time and date.
 * @param arg Unused task parameter.
 */
static void clock_task(void *arg)
{
    while (1)
    {
        // Update current time
        time(&now);
        localtime_r(&now, &timeinfo);

        // Update global time and date variables
        hour = timeinfo.tm_hour;
        minute = timeinfo.tm_min;
        second = timeinfo.tm_sec;
        day = timeinfo.tm_mday;
        month = timeinfo.tm_mon + 1;
        year = timeinfo.tm_year + 1900;
        day_in_year = timeinfo.tm_yday + 1;

        // Update the Current Time variable if it exists
        VariableNode *node = find_current_time_variable();
        if(node){
            Time *t = (Time *)node->data;
            t->value = hour * 10000 + minute * 100 + second;
        }

        // Delay for 1 second
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Initializes NTP client, synchronizes time, and starts the clock task.
 */
void obtain_time(void)
{
    ESP_LOGI(TAG, "Initializing and starting SNTP");

    // Configure SNTP with default settings and specify NTP server
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");

    // Set callback for time synchronization
    config.sync_cb = time_sync_notification_cb;
    esp_netif_sntp_init(&config);

    // Wait for time synchronization
    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 100;
    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count)
    {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    }
    time(&now);
    localtime_r(&now, &timeinfo);
    esp_netif_sntp_deinit();

    // Create clock task to continuously update time
    xTaskCreate(clock_task, "clock", 1024 * 2, NULL, 10, NULL);
}