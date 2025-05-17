#ifndef ONE_WIRE_DETECT_H
#define ONE_WIRE_DETECT_H

/**
 * @brief Searches for one-wire sensors on configured GPIO pins and returns their addresses as a JSON string.
 * @return char* JSON string containing detected sensor pins and addresses, or NULL on error.
 */
char *search_for_one_wire_sensors(void);

#endif // ONE_WIRE_DETECT_H