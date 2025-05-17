#ifndef SENSORS_H
#define SENSORS_H

/**
 * @brief Read data from a one-wire sensor.
 * @param sensor_type Type of the one-wire sensor (e.g., DS18B20).
 * @param sensor_address Address or identifier of the sensor.
 * @param pin GPIO pin number connected to the one-wire bus.
 * @return float Sensor reading value, or a default value (e.g., 0.0) on error.
 */
float read_one_wire_sensor(const char *sensor_type, const char *sensor_address, int pin);

#endif // SENSORS_H