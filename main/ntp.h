#ifndef NTP_H
#define NTP_H

#include <sys/time.h>
#include "esp_netif_sntp.h"
#include "esp_sntp.h"

/**
 * @brief Global variables to store the current time and date.
 */
extern int hour;        ///< Current hour (0-23).
extern int minute;      ///< Current minute (0-59).
extern int second;      ///< Current second (0-59).
extern int day;         ///< Current day of the month (1-31).
extern int month;       ///< Current month (1-12).
extern int year;        ///< Current year (e.g., 2025).
extern int day_in_year; ///< Current day of the year (1-366).

/**
 * @brief Initializes NTP client and synchronizes system time with an NTP server.
 */
void obtain_time(void);

/**
 * @brief Checks if the system time is synchronized with an NTP server.
 * @return bool True if NTP synchronization is complete, false otherwise.
 */
bool is_ntp_sync(void);

#endif