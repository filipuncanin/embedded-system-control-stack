#ifndef VARIABLES_H
#define VARIABLES_H

#include <stdbool.h>
#include <stddef.h>
#include <cJSON.h>

/**
 * @brief Maximum length for variable names.
 */
#define MAX_VAR_NAME_LENGTH 64

/**
 * @brief Enum for different variable types.
 */
typedef enum {
    VAR_TYPE_DIGITAL_ANALOG_IO, ///< Digital or analog input/output.
    VAR_TYPE_ONE_WIRE,          ///< One-wire input.
    VAR_TYPE_ADC_SENSOR,        ///< ADC sensor.
    VAR_TYPE_BOOLEAN,           ///< Boolean variable.
    VAR_TYPE_NUMBER,            ///< Numeric variable.
    VAR_TYPE_COUNTER,           ///< Counter variable.
    VAR_TYPE_TIMER,             ///< Timer variable.
    VAR_TYPE_TIME               ///< Time variable.
} VariableType;

/**
 * @brief Base structure for a variable.
 */
typedef struct {
    char *name;  ///< Variable name.
    char *type;  ///< Variable type as a string.
} Variable;

/**
 * @brief Structure for digital/analog input/output variables.
 */
typedef struct {
    Variable base;      ///< Base variable structure.
    char *pin_number;   ///< Pin number for the I/O.
} DigitalAnalogInputOutput;

/**
 * @brief Structure for one-wire input variables.
 */
typedef struct {
    Variable base;      ///< Base variable structure.
    char *pin_number;   ///< Pin number for the one-wire device.
    double value;       ///< Current value of the one-wire sensor.
} OneWireInput;

/**
 * @brief Structure for ADC sensor variables.
 */
typedef struct {
    Variable base;        ///< Base variable structure.
    char *sensor_type;    ///< Type of the sensor.
    char *pd_sck;         ///< Pin for power-down and serial clock.
    char *dout;           ///< Data output pin.
    double map_low;       ///< Lower mapping range for the sensor value.
    double map_high;      ///< Upper mapping range for the sensor value.
    double gain;          ///< Gain factor for the sensor.
    char *sampling_rate;  ///< Sampling rate for the sensor.
    double value;         ///< Current value of the ADC sensor.
} ADCSensor;

/**
 * @brief Structure for boolean variables.
 */
typedef struct {
    Variable base;  ///< Base variable structure.
    bool value;     ///< Boolean value.
} Boolean;

/**
 * @brief Structure for numeric variables.
 */
typedef struct {
    Variable base;  ///< Base variable structure.
    double value;   ///< Numeric value.
} Number;

/**
 * @brief Structure for counter variables.
 */
typedef struct {
    Variable base;  ///< Base variable structure.
    double pv;      ///< Preset value.
    double cv;      ///< Current value.
    bool cu;        ///< Count up flag.
    bool cd;        ///< Count down flag.
    bool qu;        ///< Output for count up.
    bool qd;        ///< Output for count down.
} Counter;

/**
 * @brief Structure for timer variables.
 */
typedef struct {
    Variable base;  ///< Base variable structure.
    double pt;      ///< Preset time.
    double et;      ///< Elapsed time.
    bool in;        ///< Input state.
    bool q;         ///< Output state.
} Timer;

/**
 * @brief Structure for time variables.
 */
typedef struct {
    Variable base;  ///< Base variable structure.
    double value;   ///< Time value.
} Time;

/**
 * @brief Structure for a variable node in the variables list.
 */
typedef struct {
    VariableType type;  ///< Type of the variable.
    void *data;         ///< Pointer to the variable data.
} VariableNode;

/**
 * @brief Structure for managing a list of variables.
 */
typedef struct {
    VariableNode *nodes;  ///< Array of variable nodes.
    size_t count;         ///< Current number of variables.
    size_t capacity;      ///< Capacity of the array.
} VariablesList;

/**
 * @brief Global list of variables.
 */
extern VariablesList variables_list;

/**
 * @brief Load variables from a cJSON object.
 * @param variables cJSON object containing variable definitions.
 * @return bool True if loading succeeds, false otherwise.
 */
bool load_variables(cJSON *variables);

/**
 * @brief Find a variable by name.
 * @param search_name Name of the variable to find.
 * @return VariableNode* Pointer to the variable node, or NULL if not found.
 */
VariableNode *find_variable(const char *search_name);

/**
 * @brief Find the current time variable.
 * @return VariableNode* Pointer to the time variable node, or NULL if not found.
 */
VariableNode *find_current_time_variable(void);

/**
 * @brief Read a boolean variable by name.
 * @param var_name Name of the variable to read.
 * @return bool The value of the variable, or false if not found.
 */
bool read_variable(const char *var_name);

/**
 * @brief Write a boolean value to a variable.
 * @param var_name Name of the variable to write to.
 * @param value Boolean value to write.
 */
void write_variable(const char *var_name, bool value);

/**
 * @brief Read a numeric variable by name.
 * @param var_name Name of the variable to read.
 * @return double The value of the variable, or 0.0 if not found.
 */
double read_numeric_variable(const char *var_name);

/**
 * @brief Write a numeric value to a variable.
 * @param var_name Name of the variable to write to.
 * @param value Numeric value to write.
 */
void write_numeric_variable(const char *var_name, double value);

/**
 * @brief Read all variables as a JSON string.
 * @return char* JSON string containing all variables, or NULL on error.
 */
char *read_variables_json(void);

/**
 * @brief Update variables from a JSON string received from child nodes.
 * @param data_len Length of the data.
 */
void update_variables_from_children(const char *json_str);

/**
 * @brief Send variable updates to parent nodes.
 */
void send_variables_to_parents(void);

#endif // VARIABLES_H