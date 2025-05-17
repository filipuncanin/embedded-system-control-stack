#include "ble.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "driver/gpio.h"

#include "esp_mac.h"

#include "conf_task_manager.h"
#include "nvs_utils.h"

#include "variables.h"
#include "one_wire_detect.h"

/**
 * @brief Tag for logging messages from the BLE server module.
 */
char *TAG = "BLE-Server";

/**
 * @brief Default MTU size for BLE communication.
 * Initially set to the minimum value of 23 bytes.
 */
static uint16_t ble_mtu = 23; // Default minimum MTU

/**
 * @brief BLE address type (public or random).
 */
uint8_t ble_addr_type;

/**
 * @brief Function prototype for starting BLE advertising.
 */
void ble_app_advertise(void);

/**
 * @brief Connection handle for the active BLE connection.
 * Initialized to BLE_HS_CONN_HANDLE_NONE when no connection exists.
 */
uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE; 

/**
 * @brief Flag indicating whether the application is connected via BLE.
 */
bool app_connected_ble = false;

/**
 * @brief Handles read requests for the configuration characteristic.
 * Loads configuration from NVS and sends it in chunks based on the MTU size.
 * @param conn_handle Connection handle for the BLE connection.
 * @param attr_handle Attribute handle of the characteristic.
 * @param ctxt Context for the GATT access operation.
 * @param arg Unused argument.
 * @return int 0 on success, or a BLE_ATT error code on failure.
 */
static int configuration_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    static char *nvs_data = NULL;
    static size_t nvs_data_len = 0;
    static size_t offset = 0;

    // Load configuration from NVS if not already loaded
    if (nvs_data == NULL) {
        ESP_LOGI(TAG, "Client requested configuration");
        esp_err_t ret = load_config_from_nvs(&nvs_data, &nvs_data_len);
        if (ret != ESP_OK || nvs_data == NULL) {
            ESP_LOGE(TAG, "Failed to load config from NVS");
            return 0;
        }
    }

    // Check if all data has been sent
    if (offset >= nvs_data_len) {
        // All data sent, return empty response to signal end
        ESP_LOGI(TAG, "Configuration sent successfully. (End of data reached, sending empty response)");
        if (nvs_data != NULL) {
            free(nvs_data); // Free allocated memory
            nvs_data = NULL;
            nvs_data_len = 0;
            offset = 0;
        }
        return 0; // Success, no more data to send
    }

    // Calculate chunk size based on remaining data and MTU
    size_t remaining = nvs_data_len - offset;
    size_t current_chunk_size = (remaining > (ble_mtu - 3)) ? (ble_mtu - 3) : remaining;

    // Append chunk to response buffer
    int rc = os_mbuf_append(ctxt->om, &nvs_data[offset], current_chunk_size);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to append data to mbuf: %d", rc);
        return BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    offset += current_chunk_size; // Update offset for next read
    return 0; 
}

/**
 * @brief Handles write requests for the configuration characteristic.
 * Applies the received configuration data.
 * @param conn_handle Connection handle for the BLE connection.
 * @param attr_handle Attribute handle of the characteristic.
 * @param ctxt Context for the GATT access operation.
 * @param arg Unused argument.
 * @return int 0 on success.
 */
static int configuration_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    const char *data = (const char *)ctxt->om->om_data;
    configure(data, ctxt->om->om_len, false); // Apply configuration
    return 0;
}

/**
 * @brief Handles read requests for the monitor characteristic.
 * Reads variable data as JSON and sends it in chunks based on the MTU size.
 * @param conn_handle Connection handle for the BLE connection.
 * @param attr_handle Attribute handle of the characteristic.
 * @param ctxt Context for the GATT access operation.
 * @param arg Unused argument.
 * @return int 0 on success, or a BLE_ATT error code on failure.
 */
static int monitor_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    static char *monitor_data = NULL;
    static size_t monitor_data_len = 0;
    static size_t monitor_offset = 0;

    // Load monitor data if not already loaded
    if (monitor_data == NULL) {
        monitor_data = read_variables_json(); // Read variables as JSON
        if (monitor_data == NULL) {
            ESP_LOGI(TAG, "No monitor data available");
            return 0;
        }
        monitor_data_len = strlen(monitor_data);
        monitor_offset = 0;
    }
    
    // Check if all data has been sent
    if (monitor_offset >= monitor_data_len) {
        free(monitor_data); // Free allocated memory
        monitor_data = NULL; 
        monitor_data_len = 0; 
        monitor_offset = 0; 
        return 0; // Empty response signals end
    }

    // Calculate chunk size based on remaining data and MTU
    size_t remaining = monitor_data_len - monitor_offset;
    size_t chunk_size = (remaining > (ble_mtu - 3)) ? (ble_mtu - 3) : remaining;

    // Append chunk to response buffer
    int rc = os_mbuf_append(ctxt->om, &monitor_data[monitor_offset], chunk_size);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to append monitor data to mbuf: %d", rc);
        free(monitor_data); // Free memory on error
        monitor_data = NULL; 
        monitor_data_len = 0; 
        monitor_offset = 0; 
        return BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    monitor_offset += chunk_size; // Update offset for next read
    return 0;
}

/**
 * @brief Handles read requests for the one-wire sensor characteristic.
 * Reads one-wire sensor data and sends it in chunks based on the MTU size.
 * @param conn_handle Connection handle for the BLE connection.
 * @param attr_handle Attribute handle of the characteristic.
 * @param ctxt Context for the GATT access operation.
 * @param arg Unused argument.
 * @return int 0 on success, or a BLE_ATT error code on failure.
 */
static int one_wire_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    static char *one_wire_data = NULL;
    static size_t one_wire_data_len = 0;
    static size_t one_wire_offset = 0;

    // Load one-wire sensor data if not already loaded
    if (one_wire_data == NULL) {
        one_wire_data = search_for_one_wire_sensors(); // Read one-wire sensor data
        if (one_wire_data == NULL) {
            ESP_LOGI(TAG, "No one-wire data available");
            return 0;
        }
        one_wire_data_len = strlen(one_wire_data);
        one_wire_offset = 0;
    }

    // Check if all data has been sent
    if (one_wire_offset >= one_wire_data_len) {
        free(one_wire_data); // Free allocated memory
        one_wire_data = NULL; 
        one_wire_data_len = 0; 
        one_wire_offset = 0; 
        return 0; // Empty response signals end
    }

    // Calculate chunk size based on remaining data and MTU
    size_t remaining = one_wire_data_len - one_wire_offset;
    size_t chunk_size = (remaining > (ble_mtu - 3)) ? (ble_mtu - 3) : remaining;

    // Append chunk to response buffer
    int rc = os_mbuf_append(ctxt->om, &one_wire_data[one_wire_offset], chunk_size);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to append one-wire data to mbuf: %d", rc);
        free(one_wire_data); // Free memory on error
        one_wire_data = NULL;
        one_wire_data_len = 0;
        one_wire_offset = 0;
        return BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    one_wire_offset += chunk_size; // Update offset for next read
    return 0;
}

/**
 * @brief GATT service definitions for the BLE server.
 * Defines a primary service with characteristics for configuration read/write,
 * monitor data read, and one-wire sensor data read.
 */
static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(SERVICE_UUID), 
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                .uuid = BLE_UUID16_DECLARE(READ_CONFIGURATION_CHAR_UUID),
                .flags = BLE_GATT_CHR_F_READ,
                .access_cb = configuration_read // Read configuration
            },
            {
                .uuid = BLE_UUID16_DECLARE(WRITE_CONFIGURATION_CHAR_UUID),
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                .access_cb = configuration_write // Write configuration
            },
            {
                .uuid = BLE_UUID16_DECLARE(READ_MONITOR_CHAR_UUID),
                .flags = BLE_GATT_CHR_F_READ,
                .access_cb = monitor_read // Read monitor data
            },
            {
                .uuid = BLE_UUID16_DECLARE(READ_ONE_WIRE_CHAR_UUID),
                .flags = BLE_GATT_CHR_F_READ,
                .access_cb = one_wire_read // Read one-wire sensor data
            },
            {0} // Terminator for characteristics array
        }
    },
    {0} // Terminator for services array
};

/**
 * @brief Handles BLE GAP events such as connection, disconnection, and MTU updates.
 * @param event The GAP event structure.
 * @param arg Unused argument.
 * @return int 0 on success.
 */
static int ble_gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        // Handle connection event
        ESP_LOGI(TAG, "EVENT CONNECT %s, conn_handle=%d", event->connect.status == 0 ? "OK!" : "FAILED!", event->connect.conn_handle);
        if (event->connect.status == 0) {
            conn_handle = event->connect.conn_handle;  // Store connection handle
            ESP_LOGI(TAG, "Client connected successfully");
            app_connected_ble = true; // Set BLE connection flag
        } else {
            ble_app_advertise(); // Restart advertising on connection failure
        }
        break;
    case BLE_GAP_EVENT_DISCONNECT: 
        // Handle disconnection event
        if (event->disconnect.conn.conn_handle == conn_handle) {
            ESP_LOGI(TAG, "EVENT DISCONNECT, reason=%d, conn_handle=%d", event->disconnect.reason, event->disconnect.conn.conn_handle);
            conn_handle = BLE_HS_CONN_HANDLE_NONE;  // Reset connection handle
            app_connected_ble = false; // Clear BLE connection flag
        } else {
            ESP_LOGW(TAG, "Other disconnect, conn_handle: %d", conn_handle);
        }
        ble_app_advertise(); // Restart advertising
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        // Handle advertising completion event
        ESP_LOGI(TAG, "EVENT ADV_COMPLETE");
        ble_app_advertise(); // Restart advertising
        break;
    case BLE_GAP_EVENT_MTU:
        // Handle MTU update event
        ESP_LOGI(TAG, "MTU updated: %d", event->mtu.value);
        ble_mtu = event->mtu.value; // Update MTU size
        break;
    default:
        // Log unhandled GAP events
        ESP_LOGI(TAG, "Unhandled GAP event: %d", event->type);
        break;
    }
    return 0;
}

/**
 * @brief Starts BLE advertising with the device name and service UUID.
 */
void ble_app_advertise(void)
{
    struct ble_hs_adv_fields fields;

    // Initialize advertising fields
    memset(&fields, 0, sizeof(fields));
    const char *device_name = ble_svc_gap_device_name(); 
    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;

    // Set service UUID for advertising
    ble_uuid16_t uuid16 = BLE_UUID16_INIT(SERVICE_UUID); 
    fields.uuids16 = &uuid16;
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    // Set advertising fields
    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to set advertising fields: %d", rc);
        return;
    }

    // Configure advertising parameters
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; // Undirected connectable mode
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; // General discoverable mode
    adv_params.itvl_min = 0x20;  // Minimum advertising interval
    adv_params.itvl_max = 0x40;  // Maximum advertising interval

    // Start advertising
    rc = ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL); 
    if (rc != 0) {
        ESP_LOGE(TAG, "Advertising start failed: %d", rc);
    } else {
        ESP_LOGI(TAG, "Advertising started successfully");
    }
}

/**
 * @brief Callback function called when the BLE stack is synchronized.
 * Initiates advertising after setting the BLE address type.
 */
void ble_app_on_sync(void)
{
    ESP_LOGI(TAG, "BLE sync completed");
    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay to ensure stability
    if (ble_hs_id_infer_auto(0, &ble_addr_type) != 0) {
        ESP_LOGE(TAG, "Failed to infer BLE address type!");
        return;
    }
    ble_app_advertise(); // Start advertising
}

/**
 * @brief Task function for running the NimBLE host.
 * Runs indefinitely until nimble_port_stop() is called.
 * @param param Unused parameter.
 */
void host_task(void *param)
{
    nimble_port_run(); // Run the NimBLE host stack
}

/**
 * @brief Sets the BLE device name based on the Bluetooth MAC address.
 * Creates a device name in the format "ESP_XXYYZZ" using the first three bytes of the MAC address.
 */
static void set_ble_name_from_mac() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_BT); // Read Bluetooth MAC address
    char dev_name[11];
    snprintf(dev_name, sizeof(dev_name), "ESP_%02X%02X%02X", mac[0], mac[1], mac[2]); // Format device name
    int rc = ble_svc_gap_device_name_set(dev_name); // Set device name
    if (rc != 0) ESP_LOGE(TAG, "Failed to set device name: %d", rc);
}

/**
 * @brief Initializes the BLE stack and services.
 * Sets up the NimBLE host, GAP, GATT, and custom services, and starts the host task.
 */
void ble_init(void) {
    // Initialize NimBLE stack
    ESP_ERROR_CHECK(nimble_port_init());
    set_ble_name_from_mac(); // Set device name based on MAC
    ble_svc_gap_init(); // Initialize GAP service
    ble_svc_gatt_init(); // Initialize GATT service
    ble_gatts_count_cfg(gatt_svcs); // Configure GATT services
    ESP_ERROR_CHECK(ble_gatts_add_svcs(gatt_svcs)); // Add GATT services
    ble_hs_cfg.sync_cb = ble_app_on_sync; // Set synchronization callback

    // Start NimBLE host task
    nimble_port_freertos_init(host_task);
}