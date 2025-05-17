#include "conf_task_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "string.h"
#include <cJSON.h>

#include "nvs_utils.h"
#include "device_config.h"
#include "variables.h"
#include "ladder_elements.h"

/**
 * @brief Tag for logging messages from the configuration task manager module.
 */
static const char *TAG = "conf_task_manager";

/**
 * @brief Timeout duration for configuration data reception (10 seconds).
 */
static const int CONFIG_TIMEOUT_MS = 10000; // 10 seconds timeout

/**
 * @brief Timer handle for configuration timeout.
 */
static TimerHandle_t config_timeout_timer = NULL;

/**
 * @brief Dynamic buffer for storing configuration data parts.
 */
static char *large_buffer = NULL;

/**
 * @brief Total number of bytes received in the buffer.
 */
static size_t total_received = 0;

/**
 * @brief Structure to store task information.
 */
typedef struct {
    TaskHandle_t handle; ///< Handle of the FreeRTOS task.
    cJSON *wire_copy;   ///< Copy of the JSON wire configuration.
} TaskInfo;

/**
 * @brief Array of task information structures.
 */
static TaskInfo *tasks = NULL;

/**
 * @brief Number of tasks currently managed.
 */
static int num_tasks = 0;

/**
 * @brief Callback function for configuration timeout.
 * @param xTimer Handle of the timer that triggered the callback.
 */
static void config_timeout_callback(TimerHandle_t xTimer) {
    // Log warning and clear buffer on timeout
    ESP_LOGW(TAG, "Configuration timeout - clearing buffer");
    if (large_buffer) {
        free(large_buffer);
        large_buffer = NULL;
        total_received = 0;
    }
}

// Forward declarations
static bool process_node(cJSON *node, bool *condition);
static bool process_nodes(cJSON *nodes, bool *condition, cJSON **last_coil);
static void process_coil(cJSON *node, bool condition);

/**
 * @brief Processes a single ladder node (excluding Coil nodes).
 * @param node JSON object representing the node.
 * @param condition Pointer to the current condition state.
 * @return bool Updated condition state or false on error.
 */
static bool process_node(cJSON *node, bool *condition) {
    if (!node || !condition || !cJSON_IsObject(node)) {
        // Log error for invalid node or condition
        ESP_LOGE(TAG, "Invalid node or condition");
        return false;
    }

    cJSON *type = cJSON_GetObjectItem(node, "Type");
    if (!cJSON_IsString(type)) {
        // Log error if node type is missing or invalid
        ESP_LOGE(TAG, "Node missing Type or Type is not a string");
        return false;
    }

    if (strcmp(type->valuestring, "LadderElement") == 0) {
        cJSON *element_type = cJSON_GetObjectItem(node, "ElementType");
        cJSON *combo_values = cJSON_GetObjectItem(node, "ComboBoxValues");

        if (!cJSON_IsString(element_type) || !cJSON_IsArray(combo_values)) {
            // Log error if LadderElement is missing required fields
            ESP_LOGE(TAG, "LadderElement missing ElementType or ComboBoxValues");
            return false;
        }

        // Get arguments from ComboBoxValues
        cJSON *arg1 = cJSON_GetArrayItem(combo_values, 0);
        cJSON *arg2 = cJSON_GetArrayItem(combo_values, 1);
        cJSON *arg3 = cJSON_GetArrayItem(combo_values, 2);

        const char *var1 = (arg1 && cJSON_IsString(arg1)) ? arg1->valuestring : NULL;
        const char *var2 = (arg2 && cJSON_IsString(arg2)) ? arg2->valuestring : NULL;
        const char *var3 = (arg3 && cJSON_IsString(arg3)) ? arg3->valuestring : NULL;

        // Handle Contacts and Comparisons
        if (strcmp(element_type->valuestring, "NOContact") == 0 && var1) {
            bool result = no_contact(var1);
            ESP_LOGD(TAG, "NOContact(%s) = %d", var1, result);
            *condition &= result;
            return *condition;
        } else if (strcmp(element_type->valuestring, "NCContact") == 0 && var1) {
            bool result = nc_contact(var1);
            ESP_LOGD(TAG, "NCContact(%s) = %d", var1, result);
            *condition &= result;
            return *condition;
        } else if (strcmp(element_type->valuestring, "GreaterCompare") == 0 && var1 && var2) {
            bool result = greater(var1, var2);
            ESP_LOGD(TAG, "GreaterCompare(%s, %s) = %d", var1, var2, result);
            *condition &= result;
            return *condition;
        } else if (strcmp(element_type->valuestring, "LessCompare") == 0 && var1 && var2) {
            bool result = less(var1, var2);
            ESP_LOGD(TAG, "LessCompare(%s, %s) = %d", var1, var2, result);
            *condition &= result;
            return *condition;
        } else if (strcmp(element_type->valuestring, "GreaterOrEqualCompare") == 0 && var1 && var2) {
            bool result = greater_or_equal(var1, var2);
            ESP_LOGD(TAG, "GreaterOrEqualCompare(%s, %s) = %d", var1, var2, result);
            *condition &= result;
            return *condition;
        } else if (strcmp(element_type->valuestring, "LessOrEqualCompare") == 0 && var1 && var2) {
            bool result = less_or_equal(var1, var2);
            ESP_LOGD(TAG, "LessOrEqualCompare(%s, %s) = %d", var1, var2, result);
            *condition &= result;
            return *condition;
        } else if (strcmp(element_type->valuestring, "EqualCompare") == 0 && var1 && var2) {
            bool result = equal(var1, var2);
            ESP_LOGD(TAG, "EqualCompare(%s, %s) = %d", var1, var2, result);
            *condition &= result;
            return *condition;
        } else if (strcmp(element_type->valuestring, "NotEqualCompare") == 0 && var1 && var2) {
            bool result = not_equal(var1, var2);
            ESP_LOGD(TAG, "NotEqualCompare(%s, %s) = %d", var1, var2, result);
            *condition &= result;
            return *condition;
        }
        // Actions executed immediately if condition is true
        else if (strcmp(element_type->valuestring, "AddMath") == 0 && var1 && var2 && var3) {
            add(var1, var2, var3, *condition);
            return *condition;
        } else if (strcmp(element_type->valuestring, "SubtractMath") == 0 && var1 && var2 && var3) {
            subtract(var1, var2, var3, *condition);
            return *condition;
        } else if (strcmp(element_type->valuestring, "MultiplyMath") == 0 && var1 && var2 && var3) {
            multiply(var1, var2, var3, *condition);
            return *condition;
        } else if (strcmp(element_type->valuestring, "DivideMath") == 0 && var1 && var2 && var3) {
            divide(var1, var2, var3, *condition);
            return *condition;
        } else if (strcmp(element_type->valuestring, "MoveMath") == 0 && var1 && var2) {
            move(var1, var2, *condition);
            return *condition;
        } else if (strcmp(element_type->valuestring, "CountUp") == 0 && var1) {
            count_up(var1, *condition);
            return *condition;
        } else if (strcmp(element_type->valuestring, "CountDown") == 0 && var1) {
            count_down(var1, *condition);
            return *condition;
        } else if (strcmp(element_type->valuestring, "OnDelayTimer") == 0 && var1) {
            bool result = timer_on(var1, *condition);
            *condition &= result;
            return *condition;
        } else if (strcmp(element_type->valuestring, "OffDelayTimer") == 0 && var1) {
            bool result = timer_off(var1, *condition);
            // Note: Uses = instead of &= because this timer sets true regardless of prior elements
            *condition = result;
            return *condition;
        } else if (strcmp(element_type->valuestring, "Reset") == 0 && var1) {
            reset(var1, *condition);
            return *condition;
        }
        return *condition; // Unknown element doesn't change condition
    } else if (strcmp(type->valuestring, "Branch") == 0) {
        cJSON *nodes1 = cJSON_GetObjectItem(node, "Nodes1");
        cJSON *nodes2 = cJSON_GetObjectItem(node, "Nodes2");

        if (!cJSON_IsArray(nodes1) || !cJSON_IsArray(nodes2)) {
            // Log error if Branch is missing required node arrays
            ESP_LOGE(TAG, "Branch missing Nodes1 or Nodes2 arrays");
            return false;
        }

        bool nodes1_condition = true; // Independent condition for Nodes1
        bool nodes2_condition = true; // Independent condition for Nodes2
        cJSON *nodes1_last_coil = NULL;
        cJSON *nodes2_last_coil = NULL;

        // Process both branches
        bool nodes1_active = process_nodes(nodes1, &nodes1_condition, &nodes1_last_coil);
        bool nodes2_active = process_nodes(nodes2, &nodes2_condition, &nodes2_last_coil);

        // Log branch conditions
        ESP_LOGD(TAG, "Branch: Nodes1_active=%d (cond=%d), Nodes2_active=%d (cond=%d)", 
                 nodes1_active, nodes1_condition, nodes2_active, nodes2_condition);

        // OR logic: at least one branch must be active
        bool branch_condition = nodes1_active || nodes2_active;
        *condition &= branch_condition;

        // Process coils only if present (shouldn't be in this JSON, but handle for robustness)
        if (nodes1_last_coil && nodes1_condition) {
            // Log warning for unexpected coil in Nodes1
            ESP_LOGW(TAG, "Unexpected coil in Nodes1");
            process_coil(nodes1_last_coil, nodes1_condition);
        }
        if (nodes2_last_coil && nodes2_condition) {
            // Log warning for unexpected coil in Nodes2
            ESP_LOGW(TAG, "Unexpected coil in Nodes2");
            process_coil(nodes2_last_coil, nodes2_condition);
        }

        return *condition;
    }

    // Log warning for unknown node type
    ESP_LOGW(TAG, "Unknown node type: %s", type->valuestring);
    return false;
}

/**
 * @brief Processes an array of ladder nodes, identifying the last coil if present.
 * @param nodes JSON array of nodes.
 * @param condition Pointer to the current condition state.
 * @param last_coil Pointer to store the last coil node, if any.
 * @return bool Updated condition state or false if the node list is empty or invalid.
 */
static bool process_nodes(cJSON *nodes, bool *condition, cJSON **last_coil) {
    if (!nodes || !cJSON_IsArray(nodes) || !condition || !last_coil) {
        // Log error for invalid nodes array or parameters
        ESP_LOGE(TAG, "Invalid nodes array or parameters");
        return false;
    }

    *last_coil = NULL;
    bool all_conditions_met = *condition;
    int node_count = cJSON_GetArraySize(nodes);

    if (node_count == 0) {
        // Empty node list
        return false;
    }

    // Check if the last node is a coil
    cJSON *last_node = cJSON_GetArrayItem(nodes, node_count - 1);
    cJSON *last_type = cJSON_GetObjectItem(last_node, "Type");
    cJSON *last_element_type = cJSON_GetObjectItem(last_node, "ElementType");

    if (last_type && cJSON_IsString(last_type) && 
        strcmp(last_type->valuestring, "LadderElement") == 0 &&
        last_element_type && cJSON_IsString(last_element_type) &&
        (strcmp(last_element_type->valuestring, "Coil") == 0 ||
         strcmp(last_element_type->valuestring, "OneShotPositiveCoil") == 0 ||
         strcmp(last_element_type->valuestring, "SetCoil") == 0 ||
         strcmp(last_element_type->valuestring, "ResetCoil") == 0)) {
        *last_coil = last_node;
        node_count--; // Exclude the coil from condition processing
    }

    // Process all nodes except the last coil (if it was a coil)
    for (int i = 0; i < node_count; i++) {
        cJSON *node = cJSON_GetArrayItem(nodes, i);
        all_conditions_met = process_node(node, &all_conditions_met);
    }

    *condition = all_conditions_met;
    // Log processed nodes condition
    ESP_LOGD(TAG, "Nodes processed, condition=%d", *condition);
    return all_conditions_met;
}

/**
 * @brief Processes a Coil node.
 * @param node JSON object representing the coil node.
 * @param condition Current condition state.
 */
static void process_coil(cJSON *node, bool condition) {
    if (!node || !cJSON_IsObject(node)) {
        // Log error for invalid coil node
        ESP_LOGE(TAG, "Invalid coil node");
        return;
    }

    cJSON *type = cJSON_GetObjectItem(node, "Type");
    if (!cJSON_IsString(type) || strcmp(type->valuestring, "LadderElement") != 0) {
        // Log error if coil is not a LadderElement
        ESP_LOGE(TAG, "Coil node is not a LadderElement");
        return;
    }

    cJSON *element_type = cJSON_GetObjectItem(node, "ElementType");
    cJSON *combo_values = cJSON_GetObjectItem(node, "ComboBoxValues");

    if (!cJSON_IsString(element_type) || !cJSON_IsArray(combo_values)) {
        // Log error if coil is missing required fields
        ESP_LOGE(TAG, "Coil missing ElementType or ComboBoxValues");
        return;
    }

    cJSON *arg1 = cJSON_GetArrayItem(combo_values, 0);
    const char *var1 = (arg1 && cJSON_IsString(arg1)) ? arg1->valuestring : NULL;

    if (var1) {
        if (strcmp(element_type->valuestring, "Coil") == 0) {
            // Log and process standard coil
            ESP_LOGD(TAG, "Coil(%s, %d)", var1, condition);
            coil(var1, condition);
        } else if (strcmp(element_type->valuestring, "OneShotPositiveCoil") == 0) {
            // Log and process one-shot positive coil
            ESP_LOGD(TAG, "OneShotPositiveCoil(%s, %d)", var1, condition);
            one_shot_positive_coil(var1, condition);
        } else if (strcmp(element_type->valuestring, "SetCoil") == 0) {
            // Log and process set coil
            ESP_LOGD(TAG, "SetCoil(%s, %d)", var1, condition);
            set_coil(var1, condition);
        } else if (strcmp(element_type->valuestring, "ResetCoil") == 0) {
            // Log and process reset coil
            ESP_LOGD(TAG, "ResetCoil(%s, %d)", var1, condition);
            reset_coil(var1, condition);
        } else {
            // Log warning for unknown coil type
            ESP_LOGW(TAG, "Unknown coil type: %s", element_type->valuestring);
        }
    } else {
        // Log error if coil is missing variable name
        ESP_LOGE(TAG, "Coil missing variable name");
    }
}

/**
 * @brief Main task function to process a wire (ladder logic block).
 * @param pvParameters Pointer to the JSON wire configuration.
 */
static void process_block_task(void *pvParameters) {
    cJSON *wire = (cJSON *)pvParameters;

    // Get Nodes
    cJSON *nodes = cJSON_GetObjectItem(wire, "Nodes");
    if (!nodes || !cJSON_IsArray(nodes)) {
        // Log error and clean up if Nodes array is invalid
        ESP_LOGE(TAG, "Invalid or missing Nodes array in wire");
        cJSON_Delete(wire);
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        bool condition = true;
        cJSON *last_coil = NULL;

        // Process nodes and identify the last coil
        process_nodes(nodes, &condition, &last_coil);

        // Process the coil if present
        if (last_coil) {
            process_coil(last_coil, condition);
        } 

        // Delay to prevent excessive CPU usage
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Clean up (unreachable due to infinite loop)
    cJSON_Delete(wire);
    vTaskDelete(NULL);
}

/**
 * @brief Deletes all tasks and cleans up associated resources.
 */
void delete_all_tasks(void) {
    // Stop timeout timer if it exists
    if (config_timeout_timer) {
        xTimerStop(config_timeout_timer, portMAX_DELAY);
        xTimerDelete(config_timeout_timer, portMAX_DELAY);
        config_timeout_timer = NULL;
    }

    // Free large_buffer if it exists
    if (large_buffer) {
        free(large_buffer);
        large_buffer = NULL;
        total_received = 0;
    }

    // Delete all tasks
    if (tasks) {
        for (int i = 0; i < num_tasks; i++) {
            if (tasks[i].handle) {
                vTaskDelete(tasks[i].handle);
                // Log task deletion
                ESP_LOGI(TAG, "Deleted task %d", i);
                tasks[i].handle = NULL;
            }
            if (tasks[i].wire_copy) {
                cJSON_Delete(tasks[i].wire_copy);
                // Log wire copy cleanup
                ESP_LOGI(TAG, "Freed wire_copy for task %d", i);
                tasks[i].wire_copy = NULL;
            }
        }
        free(tasks);
        tasks = NULL;
        num_tasks = 0;
    }
}

void configure(const char *data, int data_len, bool loaded_from_nvs) {
    // Create/restart timeout timer
    if (config_timeout_timer == NULL) {
        config_timeout_timer = xTimerCreate(
            "ConfigTimeout",
            pdMS_TO_TICKS(CONFIG_TIMEOUT_MS),
            pdFALSE, // One-shot timer
            (void*)0,
            config_timeout_callback
        );
        if (config_timeout_timer == NULL) {
            // Log error if timer creation fails
            ESP_LOGE(TAG, "Failed to create config timeout timer");
            return;
        }
    }
    xTimerReset(config_timeout_timer, portMAX_DELAY);

    // Allocate or expand buffer for new part
    char *new_buffer = realloc(large_buffer, total_received + data_len + 1); // +1 for null-termination
    if (!new_buffer) {
        // Log error and clean up if memory allocation fails
        ESP_LOGE(TAG, "Memory allocation failed for buffer");
        free(large_buffer);
        large_buffer = NULL;
        total_received = 0;
        xTimerStop(config_timeout_timer, portMAX_DELAY);
        return;
    }
    large_buffer = new_buffer;

    // Copy new part into buffer
    memcpy(large_buffer + total_received, data, data_len);
    total_received += data_len;
    large_buffer[total_received] = '\0'; // Null-termination for safe parsing

    // Log received data
    ESP_LOGI(TAG, "Received %d bytes, total: %zu", data_len, total_received);

    // Attempt to parse JSON
    cJSON *json = cJSON_Parse(large_buffer);
    if (json) {
        // JSON is valid, complete message received
        ESP_LOGI(TAG, "Complete JSON received, length: %zu bytes", total_received);

        // Stop timeout timer
        xTimerStop(config_timeout_timer, portMAX_DELAY);

        // Save to NVS
        if(!loaded_from_nvs) {
            delete_config_from_nvs();
            save_config_to_nvs(large_buffer, total_received);
        }

        // Delete all previous tasks
        delete_all_tasks();

        // Get Device data
        cJSON *device = cJSON_GetObjectItem(json, "Device");
        device_init(device);
        print_device_info();

        // Get variables
        cJSON *variables = cJSON_GetObjectItem(json, "Variables");
        load_variables(variables);

        cJSON *wires = cJSON_GetObjectItem(json, "Wires");
        if (!cJSON_IsArray(wires)) {
            // Log error and clean up if Wires is not an array
            ESP_LOGE(TAG, "Wires is not an array");
            cJSON_Delete(json);
            free(large_buffer);
            large_buffer = NULL;
            total_received = 0;
            return;
        }

        int array_size = cJSON_GetArraySize(wires);
        // Log number of wires found
        ESP_LOGI(TAG, "Found wires: %d", array_size);

        // Allocate memory for task array
        if (esp_get_free_heap_size() < array_size * sizeof(TaskInfo) + (array_size * 4096) + 1024) {
            // Log error and clean up if insufficient heap memory
            ESP_LOGE(TAG, "Insufficient heap memory for %d tasks", array_size);
            cJSON_Delete(json);
            free(large_buffer);
            large_buffer = NULL;
            total_received = 0;
            return;
        }

        tasks = calloc(array_size, sizeof(TaskInfo));
        if (!tasks) {
            // Log error and clean up if task array allocation fails
            ESP_LOGE(TAG, "Memory allocation failed for %d tasks", array_size);
            cJSON_Delete(json);
            free(large_buffer);
            large_buffer = NULL;
            total_received = 0;
            return;
        }
        num_tasks = array_size;

        //printf("Heap before %lu\n", esp_get_free_heap_size());

        // Iterate through wires and create tasks
        for (int i = 0; i < array_size; i++) {
            cJSON *wire = cJSON_GetArrayItem(wires, i);
            if (!cJSON_IsObject(wire)) {
                // Log warning and skip if wire is not an object
                ESP_LOGW(TAG, "Wire %d is not an object, skipping", i);
                continue;
            }

            // Duplicate wire JSON object
            cJSON *wire_copy = cJSON_Duplicate(wire, true);
            if (!wire_copy) {
                // Log error if wire duplication fails
                ESP_LOGE(TAG, "Failed to duplicate wire %d", i);
                continue;
            }

            // Check available stack space
            if (uxTaskGetStackHighWaterMark(NULL) < 1024) {
                // Log warning and skip if stack space is low
                ESP_LOGW(TAG, "Low stack space, skipping task %d", i);
                cJSON_Delete(wire_copy);
                continue;
            }

            // Create task
            char task_name[16];
            snprintf(task_name, sizeof(task_name), "Wire%d", i);
            tasks[i].wire_copy = wire_copy; // Store wire_copy for cleanup
            if (xTaskCreate(process_block_task, task_name, 4096, wire_copy, 5, &tasks[i].handle) != pdPASS) {
                // Log error if task creation fails
                ESP_LOGE(TAG, "Failed to create task %d", i);
                cJSON_Delete(wire_copy);
                tasks[i].wire_copy = NULL;
            } else {
                // Log successful task creation
                ESP_LOGI(TAG, "Created task %d for wire %d", i, i);
            }

            // Delay between task creations to avoid overloading
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        //printf("Heap after %lu\n", esp_get_free_heap_size());

        // Free memory and reset state
        cJSON_Delete(json);
        free(large_buffer);
        large_buffer = NULL;
        total_received = 0;
    } else {
        // Log info if JSON is incomplete
        ESP_LOGI(TAG, "JSON incomplete, waiting for next part...");
        cJSON_Delete(json);
    }
}