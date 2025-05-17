#include "variables.h"
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include <math.h>
#include "device_config.h"

#include "mqtt.h"
#include "cJSON.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "adc_sensor.h"

/**
 * @brief Tag for logging messages from the variables module.
 */
static const char *TAG = "VARIABLES";

// Global variables
VariablesList variables_list = {0};

/**
 * @brief Handle for the OneWire read task.
 */
static TaskHandle_t one_wire_task_handle = NULL;

/**
 * @brief Task function to read OneWire sensor values.
 * @param pvParameters Task parameters (unused).
 */
static void one_wire_read_task(void *pvParameters);

/**
 * @brief Handle for the ADC sensor read task.
 */
static TaskHandle_t adc_sensor_task_handle = NULL;

/**
 * @brief Task function to read ADC sensor values.
 * @param pvParameters Task parameters (unused).
 */
static void adc_sensor_read_task(void *pvParameters);

/**
 * @brief Initialize the global variable list as empty.
 */
static void variables_list_init(void) {
    variables_list.nodes = NULL;
    variables_list.count = 0;
    variables_list.capacity = 0;
}

/**
 * @brief Add a variable to the global list with the specified type and data.
 * @param type Type of the variable.
 * @param data Pointer to the variable data.
 * @return bool True if addition succeeds, false otherwise.
 */
static bool variables_list_add(VariableType type, void *data) {
    variables_list.nodes[variables_list.count].type = type;
    variables_list.nodes[variables_list.count].data = data;
    variables_list.count++;
    return true;
}

/**
 * @brief Free memory allocated for a single variable.
 * @param type Type of the variable.
 * @param data Pointer to the variable data.
 */
static void free_variable(VariableType type, void *data) {
    if (!data) return;
    Variable *base = NULL;
    switch (type) {
        case VAR_TYPE_DIGITAL_ANALOG_IO: {
            DigitalAnalogInputOutput *daio = (DigitalAnalogInputOutput *)data;
            base = &daio->base;
            if (daio->pin_number) free(daio->pin_number);
            break;
        }
        case VAR_TYPE_ONE_WIRE: {
            OneWireInput *owi = (OneWireInput *)data;
            base = &owi->base;
            if (owi->pin_number) free(owi->pin_number);
            break;
        }
        case VAR_TYPE_ADC_SENSOR: {
            ADCSensor *adcs = (ADCSensor *)data;
            base = &adcs->base;
            if (adcs->sensor_type) free(adcs->sensor_type);
            if (adcs->pd_sck) free(adcs->pd_sck);
            if (adcs->dout) free(adcs->dout);
            if (adcs->sampling_rate) free(adcs->sampling_rate);
            break;
        }
        case VAR_TYPE_BOOLEAN: {
            Boolean *b = (Boolean *)data;
            base = &b->base;
            break;
        }
        case VAR_TYPE_NUMBER: {
            Number *n = (Number *)data;
            base = &n->base;
            break;
        }
        case VAR_TYPE_COUNTER: {
            Counter *c = (Counter *)data;
            base = &c->base;
            break;
        }
        case VAR_TYPE_TIMER: {
            Timer *t = (Timer *)data;
            base = &t->base;
            break;
        }
        case VAR_TYPE_TIME: {
            Time *t = (Time *)data;
            base = &t->base;
            break;
        }
    }
    if (base) {
        if (base->name) free(base->name);
        if (base->type) free(base->type);
    }
    free(data);
}

/**
 * @brief Free the global variable list and all associated variables.
 */
static void variables_list_free(void) {
    if (!variables_list.nodes) return;

    for (size_t i = 0; i < variables_list.count; i++) {
        free_variable(variables_list.nodes[i].type, variables_list.nodes[i].data);
    }

    free(variables_list.nodes);
    variables_list.nodes = NULL;
    variables_list.count = 0;
    variables_list.capacity = 0;
    ESP_LOGI(TAG, "Variables List freed");

    // Delete one_wire_read_task if it exists
    if (one_wire_task_handle) {
        vTaskDelete(one_wire_task_handle);
        one_wire_task_handle = NULL;
        ESP_LOGI(TAG, "Deleted one_wire_read_task");
    }

    // Delete adc_sensor_task_handle if it exists
    if (adc_sensor_task_handle) {
        vTaskDelete(adc_sensor_task_handle);
        adc_sensor_task_handle = NULL;
        ESP_LOGI(TAG, "Deleted adc_sensor_read_task");
    }

    // Free memory for ADC sensor states
    extern ADCSensorState sensor_states[];
    extern int sensor_state_count;
    for (int i = 0; i < sensor_state_count; i++) {
        if (sensor_states[i].name) {
            free(sensor_states[i].name);
        }
    }
    sensor_state_count = 0;
}

bool load_variables(cJSON *variables) {
    variables_list_free();
    variables_list_init();

    // Count variables and allocate exact capacity
    size_t var_count = cJSON_GetArraySize(variables);
    variables_list.nodes = (VariableNode *)calloc(var_count, sizeof(VariableNode));
    if (!variables_list.nodes) {
        ESP_LOGE(TAG, "Memory allocation failure");
        return false;
    }
    variables_list.capacity = var_count;

    cJSON *var = NULL;
    cJSON_ArrayForEach(var, variables) {
        cJSON *type_item = cJSON_GetObjectItem(var, "Type");
        cJSON *name_item = cJSON_GetObjectItem(var, "Name");

        const char *type_str = type_item->valuestring;
        const char *name = name_item->valuestring;

        VariableType var_type;
        if (strcmp(type_str, "Digital Input") == 0 || strcmp(type_str, "Digital Output") == 0 || 
            strcmp(type_str, "Analog Input") == 0 || strcmp(type_str, "Analog Output") == 0) {
            var_type = VAR_TYPE_DIGITAL_ANALOG_IO;
        } else if (strcmp(type_str, "One Wire Input") == 0) {
            var_type = VAR_TYPE_ONE_WIRE;
        } else if (strcmp(type_str, "ADC Sensor") == 0) {
            var_type = VAR_TYPE_ADC_SENSOR;
        } else if (strcmp(type_str, "Boolean") == 0) {
            var_type = VAR_TYPE_BOOLEAN;
        } else if (strcmp(type_str, "Number") == 0) {
            var_type = VAR_TYPE_NUMBER;
        } else if (strcmp(type_str, "Counter") == 0) {
            var_type = VAR_TYPE_COUNTER;
        } else if (strcmp(type_str, "Timer") == 0) {
            var_type = VAR_TYPE_TIMER;
        } else {
            var_type = VAR_TYPE_TIME;
        }

        void *data = NULL;
        switch (var_type) {
            case VAR_TYPE_DIGITAL_ANALOG_IO: {
                DigitalAnalogInputOutput *daio = (DigitalAnalogInputOutput *)calloc(1, sizeof(DigitalAnalogInputOutput));
                if (!daio) {
                    ESP_LOGE(TAG, "Memory allocation failure");
                    variables_list_free();
                    return false;
                }
                daio->base.name = strdup(name);
                daio->base.type = strdup(type_str);
                daio->pin_number = strdup(cJSON_GetObjectItem(var, "Pin")->valuestring);
                data = daio;
                break;
            }
            case VAR_TYPE_ONE_WIRE: {
                OneWireInput *owi = (OneWireInput *)calloc(1, sizeof(OneWireInput));
                if (!owi) {
                    ESP_LOGE(TAG, "Memory allocation failure");
                    variables_list_free();
                    return false;
                }
                owi->base.name = strdup(name);
                owi->base.type = strdup(type_str);
                owi->pin_number = strdup(cJSON_GetObjectItem(var, "Pin")->valuestring);
                data = owi;
                break;
            }
            case VAR_TYPE_ADC_SENSOR: {
                ADCSensor *adcs = (ADCSensor *)calloc(1, sizeof(ADCSensor));
                if (!adcs) {
                    ESP_LOGE(TAG, "Memory allocation failure");
                    variables_list_free();
                    return false;
                }
                adcs->base.name = strdup(name);
                adcs->base.type = strdup(type_str);
                adcs->sensor_type = strdup(cJSON_GetObjectItem(var, "Sensor Type")->valuestring);
                adcs->pd_sck = strdup(cJSON_GetObjectItem(var, "PD_SCK")->valuestring);
                adcs->dout = strdup(cJSON_GetObjectItem(var, "DOUT")->valuestring);
                adcs->map_low = cJSON_GetObjectItem(var, "Map Low")->valuedouble;
                adcs->map_high = cJSON_GetObjectItem(var, "Map High")->valuedouble;
                adcs->gain = cJSON_GetObjectItem(var, "Gain")->valuedouble;
                adcs->sampling_rate = strdup(cJSON_GetObjectItem(var, "Sampling Rate")->valuestring);
                data = adcs;

                // Initialize ADC sensor
                esp_err_t ret = adc_sensor_init(adcs->sensor_type, adcs->pd_sck, adcs->dout);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to initialize ADC Sensor '%s': %d", name, ret);
                    free_variable(VAR_TYPE_ADC_SENSOR, adcs);
                    continue; // Skip adding this sensor
                }
                break;
            }
            case VAR_TYPE_BOOLEAN: {
                Boolean *b = (Boolean *)malloc(sizeof(Boolean));
                if (!b) {
                    ESP_LOGE(TAG, "Memory allocation failure");
                    variables_list_free();
                    return false;
                }
                b->base.name = strdup(name);
                b->base.type = strdup(type_str);
                b->value = cJSON_IsTrue(cJSON_GetObjectItem(var, "Value"));
                data = b;
                break;
            }
            case VAR_TYPE_NUMBER: {
                Number *n = (Number *)malloc(sizeof(Number));
                if (!n) {
                    ESP_LOGE(TAG, "Memory allocation failure");
                    variables_list_free();
                    return false;
                }
                n->base.name = strdup(name);
                n->base.type = strdup(type_str);
                n->value = cJSON_GetObjectItem(var, "Value")->valuedouble;
                data = n;
                break;
            }
            case VAR_TYPE_COUNTER: {
                Counter *c = (Counter *)malloc(sizeof(Counter));
                if (!c) {
                    ESP_LOGE(TAG, "Memory allocation failure");
                    variables_list_free();
                    return false;
                }
                c->base.name = strdup(name);
                c->base.type = strdup(type_str);
                c->pv = cJSON_GetObjectItem(var, "PV")->valuedouble;
                c->cv = cJSON_GetObjectItem(var, "CV")->valuedouble;
                c->cu = cJSON_IsTrue(cJSON_GetObjectItem(var, "CU"));
                c->cd = cJSON_IsTrue(cJSON_GetObjectItem(var, "CD"));
                c->qu = cJSON_IsTrue(cJSON_GetObjectItem(var, "QU"));
                c->qd = cJSON_IsTrue(cJSON_GetObjectItem(var, "QD"));
                data = c;
                break;
            }
            case VAR_TYPE_TIMER: {
                Timer *t = (Timer *)malloc(sizeof(Timer));
                if (!t) {
                    ESP_LOGE(TAG, "Memory allocation failure");
                    variables_list_free();
                    return false;
                }
                t->base.name = strdup(name);
                t->base.type = strdup(type_str);
                t->pt = cJSON_GetObjectItem(var, "PT")->valuedouble;
                t->et = cJSON_GetObjectItem(var, "ET")->valuedouble;
                t->in = cJSON_IsTrue(cJSON_GetObjectItem(var, "IN"));
                t->q = cJSON_IsTrue(cJSON_GetObjectItem(var, "Q"));
                data = t;
                break;
            }
            case VAR_TYPE_TIME: {
                Time *t = (Time *)malloc(sizeof(Time));
                if (!t) {
                    ESP_LOGE(TAG, "Memory allocation failure");
                    variables_list_free();
                    return false;
                }
                t->base.name = strdup(name);
                t->base.type = strdup(type_str);
                t->value = cJSON_GetObjectItem(var, "Value")->valuedouble;
                data = t;
                break;
            }
        }

        if (!variables_list_add(var_type, data)) {
            free_variable(var_type, data);
            variables_list_free();
            return false;
        }
    }

    // Create one_wire_read_task only if needed
    bool has_one_wire = false;
    for (size_t i = 0; i < variables_list.count; i++) {
        if (variables_list.nodes[i].type == VAR_TYPE_ONE_WIRE) {
            has_one_wire = true;
            break;
        }
    }
    if (has_one_wire) {
        if (xTaskCreate(one_wire_read_task, "one_wire_read_task", 4096, NULL, 5, &one_wire_task_handle) != pdPASS) {
            ESP_LOGE(TAG, "Failed to create one_wire_read_task");
            variables_list_free();
            return false;
        }
        ESP_LOGI(TAG, "Created one_wire_read_task");
    }

    // Create adc_sensor_read_task only if needed
    bool has_adc_sensor = false;
    for (size_t i = 0; i < variables_list.count; i++) {
        if (variables_list.nodes[i].type == VAR_TYPE_ADC_SENSOR) {
            has_adc_sensor = true;
            break;
        }
    }
    if (has_adc_sensor) {
        if (xTaskCreate(adc_sensor_read_task, "adc_sensor_read_task", 4096, NULL, 5, &adc_sensor_task_handle) != pdPASS) {
            ESP_LOGE(TAG, "Failed to create adc_sensor_read_task");
            variables_list_free();
            return false;
        }
        ESP_LOGI(TAG, "Created adc_sensor_read_task");
    }

    return true;
}

VariableNode *find_variable(const char *search_name) {
    for (size_t i = 0; i < variables_list.count; i++) {
        VariableNode *node = &variables_list.nodes[i];
        Variable *base = NULL;

        switch (node->type) {
            case VAR_TYPE_DIGITAL_ANALOG_IO: {
                DigitalAnalogInputOutput *dio = (DigitalAnalogInputOutput *)node->data;
                base = &dio->base;
                break;
            }
            case VAR_TYPE_ONE_WIRE: {
                OneWireInput *owi = (OneWireInput *)node->data;
                base = &owi->base;
                break;
            }
            case VAR_TYPE_ADC_SENSOR: {
                ADCSensor *adcs = (ADCSensor *)node->data;
                base = &adcs->base;
                break;
            }
            case VAR_TYPE_BOOLEAN: {
                Boolean *b = (Boolean *)node->data;
                base = &b->base;
                break;
            }
            case VAR_TYPE_NUMBER: {
                Number *n = (Number *)node->data;
                base = &n->base;
                break;
            }
            case VAR_TYPE_TIME: {
                Time *t = (Time *)node->data;
                base = &t->base;
                break;
            }
            case VAR_TYPE_COUNTER: {
                Counter *c = (Counter *)node->data;
                base = &c->base;
                break;
            }
            case VAR_TYPE_TIMER: {
                Timer *t = (Timer *)node->data;
                base = &t->base;
                break;
            }
        }
        if (base && strcmp(base->name, search_name) == 0) 
            return node;
    }
    return NULL;
}

VariableNode *find_current_time_variable(void) {
    for (size_t i = 0; i < variables_list.count; i++) {
        VariableNode *node = &variables_list.nodes[i];
        if (node->type == VAR_TYPE_TIME) {
            Time *t = (Time *)node->data;
            if (strcmp(t->base.type, "Current Time") == 0) {
                return node;
            }
        }
    }
    return NULL;
}

/**
 * @brief Parse a variable name into base name and suffix.
 * @param var_name Full variable name.
 * @param base_name Buffer to store the base name.
 * @param base_name_size Size of the base_name buffer.
 * @param suffix Pointer to store the suffix (e.g., ".CU").
 */
void parse_variable_name(const char *var_name, char *base_name, size_t base_name_size, const char **suffix) {
    *suffix = NULL;
    base_name[0] = '\0';

    if (strchr(var_name, '.')) {
        const char *dot = strrchr(var_name, '.');
        if (strcmp(dot, ".CU") == 0 || strcmp(dot, ".CD") == 0 || 
                strcmp(dot, ".QU") == 0 || strcmp(dot, ".QD") == 0 || 
                strcmp(dot, ".IN") == 0 || strcmp(dot, ".Q") == 0 || 
                strcmp(dot, ".PV") == 0 || strcmp(dot, ".CV") == 0 || 
                strcmp(dot, ".PT") == 0 || strcmp(dot, ".ET") == 0) {
            *suffix = dot;
            size_t base_len = dot - var_name;
            strncpy(base_name, var_name, base_len);
            base_name[base_len] = '\0';
        }
    }

    if (!*suffix) {
        strncpy(base_name, var_name, base_name_size - 1);
        base_name[base_name_size - 1] = '\0';
    }
}

bool read_variable(const char *var_name) {
    char base_name[MAX_VAR_NAME_LENGTH];
    const char *variable_parameter;
    parse_variable_name(var_name, base_name, sizeof(base_name), &variable_parameter);
    VariableNode *node = find_variable(base_name);

    switch (node->type) {
        case VAR_TYPE_DIGITAL_ANALOG_IO: {
            DigitalAnalogInputOutput *dio = (DigitalAnalogInputOutput *)node->data;
            Variable *base = &dio->base;
            if (strcmp(base->type, "Digital Input") == 0) 
                return get_digital_input_value(dio->pin_number);
            else if (strcmp(base->type, "Digital Output") == 0)
                return get_digital_output_value(dio->pin_number);
            break;
        }
        case VAR_TYPE_BOOLEAN: {
            Boolean *b = (Boolean *)node->data;
            return b->value;
            break;
        }
        case VAR_TYPE_COUNTER: {
            Counter *c = (Counter *)node->data;
            if (strcmp(variable_parameter, ".CU") == 0) return c->cu;
            else if (strcmp(variable_parameter, ".CD") == 0) return c->cd;
            else if (strcmp(variable_parameter, ".QU") == 0) return c->qu;
            else if (strcmp(variable_parameter, ".QD") == 0) return c->qd;
            break;
        }
        case VAR_TYPE_TIMER: {
            Timer *t = (Timer *)node->data;
            if (strcmp(variable_parameter, ".IN") == 0) return t->in;
            else if (strcmp(variable_parameter, ".Q") == 0) return t->q;
            break;
        }
        default:
            break;
    }
    return false;
}

void write_variable(const char *var_name, bool value) {
    char base_name[MAX_VAR_NAME_LENGTH];
    const char *variable_parameter;
    parse_variable_name(var_name, base_name, sizeof(base_name), &variable_parameter);
    VariableNode *node = find_variable(base_name);

    switch (node->type) {
        case VAR_TYPE_DIGITAL_ANALOG_IO: {
            DigitalAnalogInputOutput *dio = (DigitalAnalogInputOutput *)node->data;
            set_digital_output_value(dio->pin_number, value);
            return;
        }
        case VAR_TYPE_BOOLEAN: {
            Boolean *b = (Boolean *)node->data;
            b->value = value;
            return;
        }
        case VAR_TYPE_COUNTER: {
            Counter *c = (Counter *)node->data;
            if (strcmp(variable_parameter, ".CU") == 0) {
                c->cu = value;
                return;
            } else if (strcmp(variable_parameter, ".CD") == 0) {
                c->cd = value;
                return;
            } else if (strcmp(variable_parameter, ".QU") == 0) {
                c->qu = value;
                return;
            } else if (strcmp(variable_parameter, ".QD") == 0) {
                c->qd = value;
                return;
            }
            break;
        }
        case VAR_TYPE_TIMER: {
            Timer *t = (Timer *)node->data;
            if (strcmp(variable_parameter, ".IN") == 0) {
                t->in = value;
                return;
            } else if (strcmp(variable_parameter, ".Q") == 0) {
                t->q = value;
                return;
            }
            break;
        }
        default:
            break;
    }
}

double read_numeric_variable(const char *var_name) {
    char base_name[MAX_VAR_NAME_LENGTH];
    const char *variable_parameter;
    parse_variable_name(var_name, base_name, sizeof(base_name), &variable_parameter);
    VariableNode *node = find_variable(base_name);

    switch (node->type) {
        case VAR_TYPE_DIGITAL_ANALOG_IO: {
            DigitalAnalogInputOutput *dio = (DigitalAnalogInputOutput *)node->data;
            Variable *base = &dio->base;
            if (strcmp(base->type, "Analog Input") == 0) 
                return get_digital_input_value(dio->pin_number);
            else if (strcmp(base->type, "Analog Output") == 0) 
                return get_analog_output_value(dio->pin_number);
            break;
        }
        case VAR_TYPE_ONE_WIRE: {
            OneWireInput *owi = (OneWireInput *)node->data;
            return owi->value;
        }
        case VAR_TYPE_ADC_SENSOR: {
            ADCSensor *adcs = (ADCSensor *)node->data;
            return adcs->value;
        }
        case VAR_TYPE_NUMBER: {
            Number *n = (Number *)node->data;
            return n->value;
        }
        case VAR_TYPE_TIME: {
            Time *t = (Time *)node->data;
            return t->value;
        }
        case VAR_TYPE_COUNTER: {
            Counter *c = (Counter *)node->data;
            if (strcmp(variable_parameter, ".PV") == 0) 
                return c->pv;
            else if (strcmp(variable_parameter, ".CV") == 0) 
                return c->cv;
            break;
        }
        case VAR_TYPE_TIMER: {
            Timer *t = (Timer *)node->data;
            if (strcmp(variable_parameter, ".PT") == 0) {
                return t->pt;
            } else if (strcmp(variable_parameter, ".ET") == 0) {
                return t->et;
            }
        }
        default: 
            break;
    }
    return 0;
}

void write_numeric_variable(const char *var_name, double value) {
    char base_name[MAX_VAR_NAME_LENGTH];
    const char *variable_parameter;
    parse_variable_name(var_name, base_name, sizeof(base_name), &variable_parameter);
    VariableNode *node = find_variable(base_name);

    switch (node->type) {
        case VAR_TYPE_DIGITAL_ANALOG_IO: {
            DigitalAnalogInputOutput *dio = (DigitalAnalogInputOutput *)node->data;
            int int_value = (int)round(value);
            uint8_t scaled_value = int_value < 0 ? 0 : (int_value > 255 ? 255 : (uint8_t)int_value);
            set_analog_output_value(dio->pin_number, scaled_value);
            break;
        }
        case VAR_TYPE_NUMBER: {
            Number *n = (Number *)node->data;
            n->value = value;
            break;
        }
        case VAR_TYPE_TIME: {
            Time *t = (Time *)node->data;
            t->value = value;
            break;
        }
        case VAR_TYPE_COUNTER: {
            Counter *c = (Counter *)node->data;
            if (strcmp(variable_parameter, ".PV") == 0) c->pv = value;
            else if (strcmp(variable_parameter, ".CV") == 0) c->cv = value;
            break;
        }
        case VAR_TYPE_TIMER: {
            Timer *t = (Timer *)node->data;
            if (strcmp(variable_parameter, ".PT") == 0) t->pt = value;
            else if (strcmp(variable_parameter, ".ET") == 0) t->et = value;
            break;
        }
        default:
            break;
    }
}

/**
 * @brief Task function to periodically read OneWire sensor values.
 * @param pvParameters Task parameters (unused).
 */
static void one_wire_read_task(void *pvParameters)
{
    while (1)
    {
        for (size_t i = 0; i < variables_list.count; i++)
        {
            VariableNode *node = &variables_list.nodes[i];
            if (node->type == VAR_TYPE_ONE_WIRE)
            {
                OneWireInput *owi = (OneWireInput *)node->data;
                owi->value = get_one_wire_value(owi->pin_number);
                vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second after each read
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second at end
    }
}

/**
 * @brief Task function to periodically read ADC sensor values.
 * @param pvParameters Task parameters (unused).
 */
static void adc_sensor_read_task(void *pvParameters)
{
    while (1)
    {
        for (size_t i = 0; i < variables_list.count; i++)
        {
            VariableNode *node = &variables_list.nodes[i];
            if (node->type == VAR_TYPE_ADC_SENSOR)
            {
                ADCSensor *adcs = (ADCSensor *)node->data;
                double value = adc_sensor_read(adcs->sensor_type, adcs->pd_sck, adcs->dout, 
                                              adcs->map_low, adcs->map_high, adcs->gain, 
                                              adcs->sampling_rate, adcs->base.name);
                if (value != 0.0 || adcs->value == 0.0) { // Update only if value is valid or previous was 0
                    adcs->value = value;
                    // ESP_LOGI(TAG, "ADC Sensor '%s' value: %f", adcs->base.name, adcs->value);
                } else {
                    ESP_LOGW(TAG, "Invalid value for ADC Sensor '%s', keeping old: %f", adcs->base.name, adcs->value);
                }
                // Adjust delay based on sampling_rate
                int delay_ms = (strcmp(adcs->sampling_rate, "10Hz") == 0) ? 150 : 100;
                vTaskDelay(pdMS_TO_TICKS(delay_ms));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second between cycles
    }
}

/**
 * @brief Read all variables as a JSON string.
 * @return char* JSON string containing all variables, or NULL on error.
 */
char *read_variables_json(void) {
    // Create JSON array for all variables
    cJSON *variables_array = cJSON_CreateArray();
    if (!variables_array) {
        ESP_LOGE(TAG, "Failed to create JSON array");
        return NULL;
    }

    // Iterate through all variables in the list
    for (size_t i = 0; i < variables_list.count; i++) {
        VariableNode *node = &variables_list.nodes[i];
        cJSON *var_json = cJSON_CreateObject();
        if (!var_json) {
            ESP_LOGE(TAG, "Failed to create JSON object for variable");
            continue;
        }

        Variable *base = NULL;
        switch (node->type) {
            case VAR_TYPE_DIGITAL_ANALOG_IO: {
                DigitalAnalogInputOutput *dio = (DigitalAnalogInputOutput *)node->data;
                base = &dio->base;
                cJSON_AddStringToObject(var_json, "Type", base->type);
                cJSON_AddStringToObject(var_json, "Name", base->name);
                cJSON_AddStringToObject(var_json, "Pin", dio->pin_number);
                if (strcmp(base->type, "Digital Input") == 0 || strcmp(base->type, "Digital Output") == 0)
                    cJSON_AddNumberToObject(var_json, "Value", read_variable(base->name));
                else if (strcmp(base->type, "Analog Input") == 0 || strcmp(base->type, "Analog Output") == 0)
                    cJSON_AddNumberToObject(var_json, "Value", read_numeric_variable(base->name));
                break;
            }
            case VAR_TYPE_ONE_WIRE: {
                OneWireInput *owi = (OneWireInput *)node->data;
                base = &owi->base;
                cJSON_AddStringToObject(var_json, "Type", base->type);
                cJSON_AddStringToObject(var_json, "Name", base->name);
                cJSON_AddStringToObject(var_json, "Pin", owi->pin_number);
                cJSON_AddNumberToObject(var_json, "Value", owi->value);
                break;
            }
            case VAR_TYPE_ADC_SENSOR: {
                ADCSensor *adcs = (ADCSensor *)node->data;
                base = &adcs->base;
                cJSON_AddStringToObject(var_json, "Type", base->type);
                cJSON_AddStringToObject(var_json, "Name", base->name);
                cJSON_AddStringToObject(var_json, "SensorType", adcs->sensor_type);
                cJSON_AddStringToObject(var_json, "PD_SCK", adcs->pd_sck);
                cJSON_AddStringToObject(var_json, "DOUT", adcs->dout);
                cJSON_AddNumberToObject(var_json, "MapLow", adcs->map_low);
                cJSON_AddNumberToObject(var_json, "MapHigh", adcs->map_high);
                cJSON_AddNumberToObject(var_json, "Gain", adcs->gain);
                cJSON_AddStringToObject(var_json, "SamplingRate", adcs->sampling_rate);
                cJSON_AddNumberToObject(var_json, "Value", adcs->value);
                break;
            }
            case VAR_TYPE_BOOLEAN: {
                Boolean *b = (Boolean *)node->data;
                base = &b->base;
                cJSON_AddStringToObject(var_json, "Type", base->type);
                cJSON_AddStringToObject(var_json, "Name", base->name);
                cJSON_AddBoolToObject(var_json, "Value", b->value);
                break;
            }
            case VAR_TYPE_NUMBER: {
                Number *n = (Number *)node->data;
                base = &n->base;
                cJSON_AddStringToObject(var_json, "Type", base->type);
                cJSON_AddStringToObject(var_json, "Name", base->name);
                cJSON_AddNumberToObject(var_json, "Value", n->value);
                break;
            }
            case VAR_TYPE_COUNTER: {
                Counter *c = (Counter *)node->data;
                base = &c->base;
                cJSON_AddStringToObject(var_json, "Type", base->type);
                cJSON_AddStringToObject(var_json, "Name", base->name);
                cJSON_AddNumberToObject(var_json, "PV", c->pv);
                cJSON_AddNumberToObject(var_json, "CV", c->cv);
                cJSON_AddBoolToObject(var_json, "CU", c->cu);
                cJSON_AddBoolToObject(var_json, "CD", c->cd);
                cJSON_AddBoolToObject(var_json, "QU", c->qu);
                cJSON_AddBoolToObject(var_json, "QD", c->qd);
                break;
            }
            case VAR_TYPE_TIMER: {
                Timer *t = (Timer *)node->data;
                base = &t->base;
                cJSON_AddStringToObject(var_json, "Type", base->type);
                cJSON_AddStringToObject(var_json, "Name", base->name);
                cJSON_AddNumberToObject(var_json, "PT", t->pt);
                cJSON_AddNumberToObject(var_json, "ET", t->et);
                cJSON_AddBoolToObject(var_json, "IN", t->in);
                cJSON_AddBoolToObject(var_json, "Q", t->q);
                break;
            }
            case VAR_TYPE_TIME: {
                Time *t = (Time *)node->data;
                base = &t->base;
                cJSON_AddStringToObject(var_json, "Type", base->type);
                cJSON_AddStringToObject(var_json, "Name", base->name);
                cJSON_AddNumberToObject(var_json, "Value", t->value);
                break;
            }
        }

        cJSON_AddItemToArray(variables_array, var_json);
    }

    // Convert JSON to string
    char *json_str = cJSON_PrintUnformatted(variables_array);
    if (!json_str) {
        ESP_LOGE(TAG, "Failed to print JSON");
        cJSON_Delete(variables_array);
        return NULL;
    }

    // Free JSON array
    cJSON_Delete(variables_array);

    // Return JSON string
    return json_str;
}

void update_variables_from_children(const char *json_str) {

    // ESP_LOGI(TAG, "Configuration received: %s", json_str);

    // Parse JSON string
    cJSON *json = cJSON_Parse(json_str);
    if (!json) {
        ESP_LOGE(TAG, "Failed to parse JSON: %s", cJSON_GetErrorPtr());
        return;
    }

    // Iterate through variables in the global list
    for (size_t i = 0; i < variables_list.count; i++) {
        VariableNode *node = &variables_list.nodes[i];
        Variable *base = NULL;

        // Process only Boolean and Number variables
        if (node->type == VAR_TYPE_BOOLEAN) {
            Boolean *b = (Boolean *)node->data;
            base = &b->base;

            // Find matching field in JSON
            cJSON *json_item = cJSON_GetObjectItem(json, base->name);
            if (json_item && cJSON_IsBool(json_item)) {
                b->value = cJSON_IsTrue(json_item);
                //ESP_LOGI(TAG, "Updated Boolean variable '%s' to %s", base->name, b->value ? "true" : "false");
            }
        }
        else if (node->type == VAR_TYPE_NUMBER) {
            Number *n = (Number *)node->data;
            base = &n->base;

            // Find matching field in JSON
            cJSON *json_item = cJSON_GetObjectItem(json, base->name);
            if (json_item && cJSON_IsNumber(json_item)) {
                n->value = json_item->valuedouble;
                //ESP_LOGI(TAG, "Updated Number variable '%s' to %f", base->name, n->value);
            }
        }
    }

    // Free JSON memory
    cJSON_Delete(json);
}

void send_variables_to_parents() {
    // Create JSON object for Boolean and Number variables
    cJSON *variables_json = cJSON_CreateObject();
    if (!variables_json) {
        ESP_LOGE(TAG, "Failed to create JSON object");
        return;
    }

    // Iterate through variables in the global list
    for (size_t i = 0; i < variables_list.count; i++) {
        VariableNode *node = &variables_list.nodes[i];
        Variable *base = NULL;

        // Process only Boolean and Number variables
        if (node->type == VAR_TYPE_BOOLEAN) {
            Boolean *b = (Boolean *)node->data;
            base = &b->base;
            cJSON_AddBoolToObject(variables_json, base->name, b->value);
        }
        else if (node->type == VAR_TYPE_NUMBER) {
            Number *n = (Number *)node->data;
            base = &n->base;
            cJSON_AddNumberToObject(variables_json, base->name, n->value);
        }
    }

    // Convert JSON to string
    char *json_str = cJSON_PrintUnformatted(variables_json);
    if (!json_str) {
        ESP_LOGE(TAG, "Failed to print JSON");
        cJSON_Delete(variables_json);
        return;
    }

    // Iterate through parent devices and send JSON to each topic
    for (size_t i = 0; i < _device.parent_devices_len; i++) {
        if (_device.parent_devices[i]) {
            // Form topic: parent_devices[i] + TOPIC_CHILDREN_LISTENER
            size_t topic_len = strlen(_device.parent_devices[i]) + strlen(TOPIC_CHILDREN_LISTENER) + 1;
            char *topic = (char *)malloc(topic_len);
            if (!topic) {
                ESP_LOGE(TAG, "Failed to allocate memory for topic");
                continue;
            }
            snprintf(topic, topic_len, "%s%s", _device.parent_devices[i], TOPIC_CHILDREN_LISTENER);

            // Publish JSON string to MQTT topic
            mqtt_publish(json_str, topic, 0);
            //ESP_LOGI(TAG, "Sent variables to parent '%s' on topic '%s'", _device.parent_devices[i], topic);

            // Free topic memory
            free(topic);
        }
    }

    // Free memory
    cJSON_Delete(variables_json);
    free(json_str);
}