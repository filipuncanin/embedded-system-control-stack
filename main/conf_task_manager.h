#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <stdbool.h>

/**
 * @brief Configures tasks based on provided data.
 * @param data Pointer to the configuration data.
 * @param data_len Length of the configuration data.
 * @param loaded_from_nvs Indicates if the data was loaded from non-volatile storage.
 */
void configure(const char *data, int data_len, bool loaded_from_nvs);

/**
 * @brief Deletes all configured tasks.
 */
void delete_all_tasks(void);

#endif // TASK_MANAGER_H