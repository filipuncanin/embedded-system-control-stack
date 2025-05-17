#ifndef LADDER_ELEMENTS_H
#define LADDER_ELEMENTS_H

#include <stdbool.h>

/**
 * @brief Maximum number of variables that can detect a rising edge (for mathematical operations, coils, one-shot coils, counters, timers, reset).
 */
#define MAX_ONE_SHOT_STATES 64

/**
 * @brief Maximum number of timer states.
 */
#define MAX_TIMER_STATES 32

/**
 * @brief Normally Open (NO) Contact: Returns true (active) when the associated signal is true, otherwise false (inactive).
 * @param var_name Name of the variable to check.
 * @return bool True if the signal is active, false otherwise.
 */
bool no_contact(const char *var_name);

/**
 * @brief Normally Closed (NC) Contact: Returns true (active) when the associated signal is false (inactive), otherwise false (inactive).
 * @param var_name Name of the variable to check.
 * @return bool True if the signal is inactive, false otherwise.
 */
bool nc_contact(const char *var_name);

/**
 * @brief Coil: Writes the current value (true/false) to the target variable.
 * @param var_name Name of the target variable.
 * @param condition The condition value to write.
 */
void coil(const char *var_name, bool condition);

/**
 * @brief One Shot Positive Coil: Writes true only on the rising edge (first time the signal becomes true), otherwise false.
 * @param var_name Name of the target variable.
 * @param condition The condition to evaluate for the rising edge.
 */
void one_shot_positive_coil(const char *var_name, bool condition);

/**
 * @brief Set Coil: Writes true when the signal is true and retains the value until reset.
 * @param var_name Name of the target variable.
 * @param condition The condition to set the coil.
 */
void set_coil(const char *var_name, bool condition);

/**
 * @brief Reset Coil: Writes false when the signal is true and retains the value until set.
 * @param var_name Name of the target variable.
 * @param condition The condition to reset the coil.
 */
void reset_coil(const char *var_name, bool condition);

/**
 * @brief Add: Performs addition (A + B = C).
 * @param var_name_a Name of the first input variable.
 * @param var_name_b Name of the second input variable.
 * @param var_name_c Name of the output variable.
 * @param condition Condition to enable the operation.
 */
void add(const char *var_name_a, const char *var_name_b, const char *var_name_c, bool condition);

/**
 * @brief Subtract: Performs subtraction (A - B = C).
 * @param var_name_a Name of the first input variable.
 * @param var_name_b Name of the second input variable.
 * @param var_name_c Name of the output variable.
 * @param condition Condition to enable the operation.
 */
void subtract(const char *var_name_a, const char *var_name_b, const char *var_name_c, bool condition);

/**
 * @brief Multiply: Performs multiplication (A * B = C).
 * @param var_name_a Name of the first input variable.
 * @param var_name_b Name of the second input variable.
 * @param var_name_c Name of the output variable.
 * @param condition Condition to enable the operation.
 */
void multiply(const char *var_name_a, const char *var_name_b, const char *var_name_c, bool condition);

/**
 * @brief Divide: Performs division (A / B = C).
 * @param var_name_a Name of the first input variable.
 * @param var_name_b Name of the second input variable.
 * @param var_name_c Name of the output variable.
 * @param condition Condition to enable the operation.
 */
void divide(const char *var_name_a, const char *var_name_b, const char *var_name_c, bool condition);

/**
 * @brief Move: Copies the value of A to B.
 * @param var_name_a Name of the source variable.
 * @param var_name_b Name of the destination variable.
 * @param condition Condition to enable the operation.
 */
void move(const char *var_name_a, const char *var_name_b, bool condition);

/**
 * @brief Greater: Checks if A is greater than B (A > B).
 * @param var_name_a Name of the first variable.
 * @param var_name_b Name of the second variable.
 * @return bool True if A > B, false otherwise.
 */
bool greater(const char *var_name_a, const char *var_name_b);

/**
 * @brief Less: Checks if A is less than B (A < B).
 * @param var_name_a Name of the first variable.
 * @param var_name_b Name of the second variable.
 * @return bool True if A < B, false otherwise.
 */
bool less(const char *var_name_a, const char *var_name_b);

/**
 * @brief Greater or Equal: Checks if A is greater than or equal to B (A >= B).
 * @param var_name_a Name of the first variable.
 * @param var_name_b Name of the second variable.
 * @return bool True if A >= B, false otherwise.
 */
bool greater_or_equal(const char *var_name_a, const char *var_name_b);

/**
 * @brief Less or Equal: Checks if A is less than or equal to B (A <= B).
 * @param var_name_a Name of the first variable.
 * @param var_name_b Name of the second variable.
 * @return bool True if A <= B, false otherwise.
 */
bool less_or_equal(const char *var_name_a, const char *var_name_b);

/**
 * @brief Equal: Checks if A is equal to B (A == B).
 * @param var_name_a Name of the first variable.
 * @param var_name_b Name of the second variable.
 * @return bool True if A == B, false otherwise.
 */
bool equal(const char *var_name_a, const char *var_name_b);

/**
 * @brief Not Equal: Checks if A is not equal to B (A != B).
 * @param var_name_a Name of the first variable.
 * @param var_name_b Name of the second variable.
 * @return bool True if A != B, false otherwise.
 */
bool not_equal(const char *var_name_a, const char *var_name_b);

/**
 * @brief Count Up: Increments the counter variable.
 * @param var_name Name of the counter variable.
 * @param condition Condition to enable counting.
 */
void count_up(const char *var_name, bool condition);

/**
 * @brief Count Down: Decrements the counter variable.
 * @param var_name Name of the counter variable.
 * @param condition Condition to enable counting.
 */
void count_down(const char *var_name, bool condition);

/**
 * @brief Timer On-Delay: Activates the timer with an on-delay mechanism.
 * @param var_name Name of the timer variable.
 * @param condition Condition to start the timer.
 * @return bool True when the timer reaches its setpoint, false otherwise.
 */
bool timer_on(const char *var_name, bool condition);

/**
 * @brief Timer Off-Delay: Activates the timer with an off-delay mechanism.
 * @param var_name Name of the timer variable.
 * @param condition Condition to start the timer.
 * @return bool True when the timer is active, false after the delay expires.
 */
bool timer_off(const char *var_name, bool condition);

/**
 * @brief Reset: Resets the specified variable (e.g., counter or timer).
 * @param var_name Name of the variable to reset.
 * @param condition Condition to trigger the reset.
 */
void reset(const char *var_name, bool condition);

#endif // LADDER_ELEMENTS_H