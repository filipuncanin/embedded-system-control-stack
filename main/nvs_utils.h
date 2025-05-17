#ifndef NVS_UTILS_H
#define NVS_UTILS_H

#include <esp_err.h>

/**
 * @brief Initializes the Non-Volatile Storage (NVS) system.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t nvs_init(void);

/**
 * @brief Saves configuration data to NVS.
 * @param data Pointer to the configuration data to be saved.
 * @param data_len Length of the configuration data in bytes.
 */
void save_config_to_nvs(const char *data, int data_len);

/**
 * @brief Loads configuration data from NVS.
 * @param data Pointer to a buffer where the loaded data will be stored.
 * @param data_len Pointer to a variable where the length of the loaded data will be stored.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t load_config_from_nvs(char **data, size_t *data_len);

/**
 * @brief Deletes configuration data from NVS.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t delete_config_from_nvs(void);

#endif // NVS_UTILS_H