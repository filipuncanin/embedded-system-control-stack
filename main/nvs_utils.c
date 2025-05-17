#include "nvs_utils.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

/**
 * @brief Namespace used for storing data in NVS.
 */
#define NVS_NAMESPACE "storage"

/**
 * @brief Key used to store JSON configuration data in NVS.
 */
#define NVS_KEY "json_config"

/**
 * @brief Tag for logging messages from the NVS utility module.
 */
static const char *TAG = "nvs_module";

/**
 * @brief Initializes the Non-Volatile Storage (NVS) system.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t nvs_init(void) {
    esp_err_t err;

    // Attempt to initialize NVS
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // If NVS is full or version is incompatible, erase and retry
        ESP_LOGW(TAG, "NVS partition full or version mismatch, erasing...");
        err = nvs_flash_erase();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to erase NVS: %s", esp_err_to_name(err));
            return err;
        }
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error initializing NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "NVS initialized successfully");
    }
    return err;
}

/**
 * @brief Saves configuration data to NVS.
 * @param data Pointer to the configuration data to be saved.
 * @param data_len Length of the configuration data in bytes.
 */
void save_config_to_nvs(const char *data, int data_len) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Open NVS namespace in read-write mode
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS: %s", esp_err_to_name(err));
        return;
    }

    // Save data as a binary blob
    err = nvs_set_blob(nvs_handle, NVS_KEY, data, data_len);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving data: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "JSON configuration successfully saved in NVS");
    }

    // Commit changes to ensure they are written to flash
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS: %s", esp_err_to_name(err));
    }

    // Close NVS handle
    nvs_close(nvs_handle);
}

/**
 * @brief Loads configuration data from NVS.
 * @param data Pointer to a buffer where the loaded data will be stored.
 * @param data_len Pointer to a variable where the length of the loaded data will be stored.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t load_config_from_nvs(char **data, size_t *data_len) {
    nvs_handle_t nvs_handle;
    esp_err_t err;
    size_t required_size = 0;

    // Initialize output parameters
    *data = NULL;
    *data_len = 0;

    // Open NVS namespace in read-only mode
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS %s namespace: %s", NVS_NAMESPACE, esp_err_to_name(err));
        return err;
    }

    // Get the size of the stored blob
    err = nvs_get_blob(nvs_handle, NVS_KEY, NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "JSON configuration data not found in NVS");
        nvs_close(nvs_handle);
        return err;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading size: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Check if data exists
    if (required_size == 0) {
        ESP_LOGW(TAG, "No data found for key %s", NVS_KEY);
        nvs_close(nvs_handle);
        return ESP_ERR_NVS_NOT_FOUND;
    }

    // Allocate memory for the data
    *data = (char *)malloc(required_size + 1); // +1 for null terminator
    if (*data == NULL) {
        ESP_LOGE(TAG, "Memory allocation error");
        nvs_close(nvs_handle);
        return ESP_ERR_NO_MEM;
    }

    // Read the data
    err = nvs_get_blob(nvs_handle, NVS_KEY, *data, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading data: %s", esp_err_to_name(err));
        free(*data);
        *data = NULL;
        nvs_close(nvs_handle);
        return err;
    }

    // Add null terminator
    (*data)[required_size] = '\0';
    ESP_LOGI(TAG, "JSON configuration successfully read from NVS");
    *data_len = required_size;

    // Close NVS handle
    nvs_close(nvs_handle);
    return ESP_OK;
}

/**
 * @brief Deletes configuration data from NVS.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t delete_config_from_nvs(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Open NVS namespace in read-write mode
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS %s namespace: %s", NVS_NAMESPACE, esp_err_to_name(err));
        return err;
    }

    // Erase the specified key
    err = nvs_erase_key(nvs_handle, NVS_KEY);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "JSON configuration data not found in NVS, nothing to delete");
        nvs_close(nvs_handle);
        return err;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error deleting data: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Commit changes to ensure deletion is written to flash
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS after deletion: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    ESP_LOGI(TAG, "JSON configuration successfully deleted from NVS");
    // Close NVS handle
    nvs_close(nvs_handle);
    return ESP_OK;
}