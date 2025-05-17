#include "ladder_elements.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "esp_timer.h"

#include "device_config.h"
#include "variables.h"

/**
 * @brief Tag for logging messages from the ladder logic module.
 */
static const char *TAG = "LADDER";

/**
 * @brief Structure to track the previous state for one-shot positive coils.
 */
typedef struct {
    char var_name[MAX_VAR_NAME_LENGTH]; ///< Name of the variable.
    bool prev_state;                    ///< Previous state of the variable.
} OneShotState;

/**
 * @brief Structure to track the state of timers.
 */
typedef struct {
    char var_name[MAX_VAR_NAME_LENGTH]; ///< Name of the timer variable.
    int64_t start_time;                 ///< Start time in microseconds.
    bool running;                       ///< Indicates if the timer is active.
} TimerState;

/**
 * @brief Array to store one-shot states.
 */
static OneShotState one_shot_states[MAX_ONE_SHOT_STATES] = {0};

/**
 * @brief Current number of one-shot states.
 */
static size_t one_shot_count = 0;

/**
 * @brief Array to store timer states.
 */
static TimerState timer_states[MAX_TIMER_STATES] = {0};

/**
 * @brief Current number of timer states.
 */
static size_t timer_state_count = 0;

/**
 * @brief Helper function to find or add a one-shot state for a variable.
 * @param var_name Name of the variable.
 * @return bool* Pointer to the previous state, or NULL if the limit is exceeded.
 */
static bool *get_one_shot_state(const char *var_name) {
    // Check if the state already exists
    for (size_t i = 0; i < one_shot_count; i++) {
        if (strcmp(one_shot_states[i].var_name, var_name) == 0) {
            return &one_shot_states[i].prev_state;
        }
    }
    // Add a new state if within limits
    if (one_shot_count < MAX_ONE_SHOT_STATES) {
        strncpy(one_shot_states[one_shot_count].var_name, var_name, MAX_VAR_NAME_LENGTH - 1);
        one_shot_states[one_shot_count].var_name[MAX_VAR_NAME_LENGTH - 1] = '\0';
        one_shot_states[one_shot_count].prev_state = false;
        one_shot_count++;
        return &one_shot_states[one_shot_count - 1].prev_state;
    }
    ESP_LOGE(TAG, "Too many one-shot states for %s", var_name);
    return NULL;
}

/**
 * @brief Helper function to find or add a timer state for a variable.
 * @param var_name Name of the timer variable.
 * @return TimerState* Pointer to the timer state, or NULL if the limit is exceeded.
 */
static TimerState *get_timer_state(const char *var_name) {
    // Check if the state already exists
    for (size_t i = 0; i < timer_state_count; i++) {
        if (strcmp(timer_states[i].var_name, var_name) == 0) {
            return &timer_states[i];
        }
    }
    // Add a new state if within limits
    if (timer_state_count < MAX_TIMER_STATES) {
        strncpy(timer_states[timer_state_count].var_name, var_name, MAX_VAR_NAME_LENGTH - 1);
        timer_states[timer_state_count].var_name[MAX_VAR_NAME_LENGTH - 1] = '\0';
        timer_states[timer_state_count].start_time = 0;
        timer_states[timer_state_count].running = false;
        timer_state_count++;
        return &timer_states[timer_state_count - 1];
    }
    ESP_LOGE(TAG, "Too many timer states for %s", var_name);
    return NULL;
}

/**
 * @brief Detects a rising edge (transition from false to true) for a variable.
 * @param var_name Name of the variable.
 * @param condition Current condition to evaluate.
 * @return bool True if a rising edge is detected, false otherwise.
 */
bool r_trig(const char *var_name, bool condition) {
    bool *prev_state = get_one_shot_state(var_name);
    if (!prev_state) {
        return false;
    }

    bool result = condition && !(*prev_state);
    *prev_state = condition;

    // Log the rising edge detection (commented out)
    // ESP_LOGI(TAG, "R_TRIG: %s condition=%d, result=%d", var_name, condition, result);
    return result;
}

/**
 * @brief Detects a falling edge (transition from true to false) for a variable.
 * @param var_name Name of the variable.
 * @param condition Current condition to evaluate.
 * @return bool True if a falling edge is detected, false otherwise.
 */
bool f_trig(const char *var_name, bool condition) {
    bool *prev_state = get_one_shot_state(var_name);
    if (!prev_state) {
        return false;
    }

    bool result = !condition && (*prev_state);
    *prev_state = condition;

    // Log the falling edge detection (commented out)
    // ESP_LOGI(TAG, "F_TRIG: %s condition=%d, result=%d", var_name, condition, result);
    return result;
}

// ============== CONTACTS ===============
bool no_contact(const char *var_name) {
    bool result = read_variable(var_name);
    // Log the NO contact state (commented out)
    // ESP_LOGI(TAG, "NO Contact: %s value=%d", var_name, result);
    return !result;
}

bool nc_contact(const char *var_name) {
    bool result = read_variable(var_name);
    // Log the NC contact state (commented out)
    // ESP_LOGI(TAG, "NC Contact: %s value=%d", var_name, result);
    return result;
}

// =============== COILS =================
void coil(const char *var_name, bool condition) {
    // Log the coil operation (commented out)
    // ESP_LOGI(TAG, "Coil: %s value=%d", var_name, condition);
    write_variable(var_name, condition);
}

void one_shot_positive_coil(const char *var_name, bool condition) {
    bool *prev_state = get_one_shot_state(var_name);
    if (!prev_state) {
        return;
    }

    bool output = condition && !(*prev_state);
    *prev_state = condition;

    // Log the one-shot positive coil operation (commented out)
    // ESP_LOGI(TAG, "One Shot Positive Coil: %s value=%d", var_name, output);
    write_variable(var_name, output);
}

void set_coil(const char *var_name, bool condition) {
    if (condition) {
        // Log the set coil operation (commented out)
        // ESP_LOGI(TAG, "Set Coil: %s value=%d", var_name, condition);
        write_variable(var_name, true);
    }
}

void reset_coil(const char *var_name, bool condition) {
    if (condition) {
        // Log the reset coil operation (commented out)
        // ESP_LOGI(TAG, "Reset Coil: %s value=%d", var_name, condition);
        write_variable(var_name, false);
    }
}

// ============== MATH ===============
void add(const char *var_name_a, const char *var_name_b, const char *var_name_c, bool condition) {
    if (r_trig(var_name_c, condition)) {
        double a = read_numeric_variable(var_name_a);
        double b = read_numeric_variable(var_name_b);
        // Log the add operation (commented out)
        // ESP_LOGI(TAG, "Add: %s (%f) + %s (%f) => %s (%f)", var_name_a, a, var_name_b, b, var_name_c, a + b);
        write_numeric_variable(var_name_c, a + b);
    }
}

void subtract(const char *var_name_a, const char *var_name_b, const char *var_name_c, bool condition) {
    if (r_trig(var_name_c, condition))
    {
        double a = read_numeric_variable(var_name_a);
        double b = read_numeric_variable(var_name_b);
        // Log the subtract operation (commented out)
        // ESP_LOGI(TAG, "Subtract: %s (%f) - %s (%f) => %s (%f)", var_name_a, a, var_name_b, b, var_name_c, a - b);
        write_numeric_variable(var_name_c, a - b);
    }
}

void multiply(const char *var_name_a, const char *var_name_b, const char *var_name_c, bool condition) {
    if (r_trig(var_name_c, condition)) {
        double a = read_numeric_variable(var_name_a);
        double b = read_numeric_variable(var_name_b);
        // Log the multiply operation (commented out)
        // ESP_LOGI(TAG, "Multiply: %s (%f) * %s (%f) => %s (%f)", var_name_a, a, var_name_b, b, var_name_c, a * b);
        write_numeric_variable(var_name_c, a * b);
    }
}

void divide(const char *var_name_a, const char *var_name_b, const char *var_name_c, bool condition) {
    if (r_trig(var_name_c, condition)) {
        double a = read_numeric_variable(var_name_a);
        double b = read_numeric_variable(var_name_b);

        // Check for division by zero
        if (fabs(b) < 1e-6) {
            // Log division by zero error
            ESP_LOGE(TAG, "Division by zero for %s", var_name_b);
            return;
        }

        // Log the divide operation (commented out)
        // ESP_LOGI(TAG, "Divide: %s (%f) / %s (%f) => %s (%f)", var_name_a, a, var_name_b, b, var_name_c, a / b);
        write_numeric_variable(var_name_c, a / b);
    }
}

void move(const char *var_name_a, const char *var_name_b, bool condition) {
    double a = read_numeric_variable(var_name_a);
    // Log the move operation (commented out)
    // ESP_LOGI(TAG, "Move: %s (%f) => %s (%f)", var_name_a, a, var_name_b, a);
    write_numeric_variable(var_name_b, a);
}

// ============== COMPARE ===============
bool greater(const char *var_name_a, const char *var_name_b) {
    double a = read_numeric_variable(var_name_a);
    double b = read_numeric_variable(var_name_b);
    // Log the greater comparison (commented out)
    // ESP_LOGI(TAG, "Greater: %s (%f) > %s (%f): %d", var_name_a, a, var_name_b, b, a > b);
    return a > b;
}

bool less(const char *var_name_a, const char *var_name_b) {
    double a = read_numeric_variable(var_name_a);
    double b = read_numeric_variable(var_name_b);
    // Log the less comparison (commented out)
    // ESP_LOGI(TAG, "Less: %s (%f) < %s (%f): %d", var_name_a, a, var_name_b, b, a < b);
    return a < b;
}

bool greater_or_equal(const char *var_name_a, const char *var_name_b) {
    double a = read_numeric_variable(var_name_a);
    double b = read_numeric_variable(var_name_b);
    // Log the greater or equal comparison (commented out)
    // ESP_LOGI(TAG, "Greater Or Equal: %s (%f) >= %s (%f): %d", var_name_a, a, var_name_b, b, a >= b);
    return a >= b;
}

bool less_or_equal(const char *var_name_a, const char *var_name_b) {
    double a = read_numeric_variable(var_name_a);
    double b = read_numeric_variable(var_name_b);
    // Log the less or equal comparison (commented out)
    // ESP_LOGI(TAG, "Less Or Equal: %s (%f) <= %s (%f): %d", var_name_a, a, var_name_b, b, a <= b);
    return a <= b;
}

bool equal(const char *var_name_a, const char *var_name_b) {
    double a = read_numeric_variable(var_name_a);
    double b = read_numeric_variable(var_name_b);
    // Log the equal comparison (commented out)
    // ESP_LOGI(TAG, "Equal: %s (%f) == %s (%f): %d", var_name_a, a, var_name_b, b, a == b);
    return a == b;
}

bool not_equal(const char *var_name_a, const char *var_name_b) {
    double a = read_numeric_variable(var_name_a);
    double b = read_numeric_variable(var_name_b);
    // Log the not equal comparison (commented out)
    // ESP_LOGI(TAG, "Not Equal: %s (%f) != %s (%f): %d", var_name_a, a, var_name_b, b, a != b);
    return a != b;
}

// ======= COUNTERS / TIMERS ============
void count_up(const char *var_name, bool condition) {
    if (r_trig(var_name, condition)) {
        VariableNode *node = find_variable(var_name);
        Counter *c = (Counter *)node->data;
        c->cv += 1.0; // Increment CV by 1.0
        c->qu = (c->cv >= c->pv); // Update QU
        c->qd = (c->cv <= 0.0);  // Update QD
        // Log the counter increment (commented out)
        // ESP_LOGI(TAG, "Counter: %s (cv: %f) incremented", var_name, c->cv);
    }
}

void count_down(const char *var_name, bool condition) {
    if (r_trig(var_name, condition)) {
        VariableNode *node = find_variable(var_name);
        Counter *c = (Counter *)node->data;
        c->cv -= 1.0; // Decrement CV by 1.0
        c->qu = (c->cv >= c->pv); // Update QU
        c->qd = (c->cv <= 0.0);  // Update QD
        // Log the counter decrement (commented out)
        // ESP_LOGI(TAG, "Counter: %s (cv: %f) decremented", var_name, c->cv);
    }
}

bool timer_on(const char *var_name, bool condition) {
    VariableNode *node = find_variable(var_name);

    Timer *t = (Timer *)node->data;
    TimerState *state = get_timer_state(var_name);
    if (!state) {
        // Log failure to get timer state
        ESP_LOGE(TAG, "Failed to get state for timer %s", var_name);
        return false;
    }

    // Update input
    t->in = condition;

    // If PT <= 0, timer does not run
    if (t->pt <= 0) {
        t->et = 0;
        t->q = false;
        state->running = false;
        // Log timer stopped due to invalid PT (commented out)
        // ESP_LOGI(TAG, "TON: %s PT<=0, Q=false, ET=0", var_name);
        return false;
    }

    if (condition) {
        if (!state->running && !t->q) { // Start timer only if not active and Q is false
            state->start_time = esp_timer_get_time();
            state->running = true;
            // Log timer start (commented out)
            // ESP_LOGI(TAG, "TON: %s started", var_name);
        }

        if (state->running) {
            // Calculate elapsed time
            int64_t current_time = esp_timer_get_time();
            t->et = (double)(current_time - state->start_time) / 1000.0; // Microseconds to milliseconds
            if (t->et > t->pt) {
                t->et = t->pt; // Limit ET to PT
                state->running = false; // Stop further counting
            }

            // Check if PT is reached
            t->q = (t->et >= t->pt);
        } else {
            // If timer is stopped (ET >= PT), maintain Q=true and ET=PT
            t->et = t->pt;
            t->q = true;
        }

        // Log timer state (commented out)
        // ESP_LOGI(TAG, "TON: %s IN=%d, ET=%f, Q=%d", var_name, t->in, t->et, t->q);
    } else {
        // Reset timer
        t->et = 0;
        t->q = false;
        state->running = false;
        // Log timer stop (commented out)
        // ESP_LOGI(TAG, "TON: %s stopped, Q=false, ET=0", var_name);
    }

    return t->q;
}

bool timer_off(const char *var_name, bool condition) {
    VariableNode *node = find_variable(var_name);

    Timer *t = (Timer *)node->data;
    TimerState *state = get_timer_state(var_name);
    if (!state) {
        // Log failure to get timer state
        ESP_LOGE(TAG, "Failed to get state for timer %s", var_name);
        return false;
    }

    // Update input
    t->in = condition;

    // If PT <= 0, timer does not run
    if (t->pt <= 0) {
        t->et = 0;
        t->q = condition;
        state->running = false;
        // Log timer stopped due to invalid PT (commented out)
        // ESP_LOGI(TAG, "TOF: %s PT<=0, Q=%d, ET=0", var_name, t->q);
        return t->q;
    }

    if (condition) {
        // When IN=true, Q=true and timer does not run
        t->q = true;
        t->et = 0;
        state->running = false;
        // Log timer state when input is true (commented out)
        // ESP_LOGI(TAG, "TOF: %s IN=true, Q=true, ET=0", var_name);
    } else {
        if (!state->running && t->q) { // Start timer only if Q is true
            state->start_time = esp_timer_get_time();
            state->running = true;
            // Log timer start (commented out)
            // ESP_LOGI(TAG, "TOF: %s started", var_name);
        }

        if (state->running) {
            // Calculate elapsed time
            int64_t current_time = esp_timer_get_time();
            t->et = (double)(current_time - state->start_time) / 1000.0; // Microseconds to milliseconds
            if (t->et > t->pt) {
                t->et = t->pt; // Limit ET to PT
                state->running = false; // Stop further counting
            }

            // Q remains true while ET < PT
            t->q = (t->et < t->pt);
        } else if (!t->q) {
            // If timer is not running and Q=false, maintain ET=0
            t->et = 0;
        }

        // Log timer state (commented out)
        // ESP_LOGI(TAG, "TOF: %s IN=%d, ET=%f, Q=%d", var_name, t->in, t->et, t->q);
    }

    return t->q;
}

void reset(const char *var_name, bool condition) {
    if (r_trig(var_name, condition)) {
        VariableNode *node = find_variable(var_name);

        if (node->type == VAR_TYPE_COUNTER) {
            Counter *c = (Counter *)node->data;
            bool action_taken = false;

            if (c->cu) {
                c->cv = 0.0;
                action_taken = true;
            }
            if (c->cd) {
                c->cv = c->pv;
                action_taken = true;
            }

            if (action_taken) {
                c->qu = (c->cv >= c->pv);
                c->qd = (c->cv <= 0.0);
            } 
            // Log counter reset
            ESP_LOGI(TAG, "Counter: %s reset (cv: %f)", var_name, c->cv);
        } else if (node->type == VAR_TYPE_TIMER) {
            Timer *t = (Timer *)node->data;
            TimerState *state = get_timer_state(var_name);
            if (state) {
                t->et = 0;
                t->q = false;
                t->in = false;
                state->running = false;
                // Log timer reset
                ESP_LOGI(TAG, "Timer: %s reset (ET=0, Q=false, IN=false)", var_name);
            }
        } 
    }
}