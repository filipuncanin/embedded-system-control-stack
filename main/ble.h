#ifndef BLE_H
#define BLE_H

#include <stdint.h>
#include "nimble/nimble_port.h"
#include "host/ble_hs.h"

/**
 * @brief UUID for the BLE service.
 */
#define SERVICE_UUID                    0x1234

/**
 * @brief UUID for the read configuration characteristic.
 */
#define READ_CONFIGURATION_CHAR_UUID    0xFFF1

/**
 * @brief UUID for the write configuration characteristic.
 */
#define WRITE_CONFIGURATION_CHAR_UUID   0xFFF2

/**
 * @brief UUID for the read monitor characteristic.
 */
#define READ_MONITOR_CHAR_UUID        0xFFF3

/**
 * @brief UUID for the read one-wire characteristic.
 */
#define READ_ONE_WIRE_CHAR_UUID       0xFFF4

/**
 * @brief Pointer to the monitor data buffer.
 */
extern char *monitor_data;

/**
 * @brief Length of the monitor data buffer.
 */
extern size_t monitor_data_len;

/**
 * @brief Current offset in the monitor data buffer.
 */
extern size_t monitor_offset;

/**
 * @brief Flag indicating if monitor data is being read.
 */
extern bool monitor_reading;

/**
 * @brief Flag indicating if the BLE application is connected.
 */
extern bool app_connected_ble;

/**
 * @brief Initializes the BLE subsystem.
 */
void ble_init(void);

#endif // BLE_H