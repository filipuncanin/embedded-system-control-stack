#ifndef MQTT_H
#define MQTT_H

#include "esp_system.h"

#include "config.h"

/**
 * @brief MQTT broker URI, defined in config.h.
 * @note Expected to be defined as MQTT_BROKER_URI in config.h.
 */
// MQTT BROKER
// Defined in config.h
// #define MQTT_BROKER_URI 

/**
 * @brief Topic suffix definitions for MQTT communication.
 */
#define TOPIC_CONNECTION_REQUEST "/connection_request" ///< Suffix for connection request topic.
#define TOPIC_CONNECTION_RESPONSE "/connection_response" ///< Suffix for connection response topic.
#define TOPIC_MONITOR "/monitor" ///< Suffix for monitoring topic.
#define TOPIC_ONE_WIRE "/one_wire" ///< Suffix for one-wire sensor data topic.
#define TOPIC_CONFIG_REQUEST "/config_request" ///< Suffix for configuration request topic.
#define TOPIC_CONFIG_RESPONSE "/config_response" ///< Suffix for configuration response topic.
#define TOPIC_CONFIG_RECEIVE "/config_device" ///< Suffix for receiving configuration topic.

#define TOPIC_CHILDREN_LISTENER "/children_listener" ///< Suffix for children listener topic.

/**
 * @brief Maximum length of an MQTT topic string, including null terminator.
 */
#define MAX_TOPIC_LEN 35

/**
 * @brief Quality of Service level for MQTT messages.
 */
#define MQTT_QOS 1

/**
 * @brief Enumeration of topic indices for accessing the topics array.
 */
enum {
    TOPIC_IDX_CONNECTION_REQUEST, ///< Index for connection request topic.
    TOPIC_IDX_CONNECTION_RESPONSE, ///< Index for connection response topic.
    TOPIC_IDX_MONITOR, ///< Index for monitoring topic.
    TOPIC_IDX_ONE_WIRE, ///< Index for one-wire sensor data topic.
    TOPIC_IDX_CONFIG_REQUEST, ///< Index for configuration request topic.
    TOPIC_IDX_CONFIG_RESPONSE, ///< Index for configuration response topic.
    TOPIC_IDX_CONFIG_RECEIVE, ///< Index for configuration receive topic.
    TOPIC_IDX_CHILDREN_LISTENER, ///< Index for children listener topic.
};

/**
 * @brief External array to store MQTT topic strings.
 * @note Array of 8 topics, each with a maximum length of MAX_TOPIC_LEN.
 */
extern char topics[8][MAX_TOPIC_LEN];

/**
 * @brief Flag indicating whether the application is connected to the MQTT broker.
 */
extern bool app_connected_mqtt;

/**
 * @brief Initializes the MQTT client and sets up communication with the broker.
 */
void mqtt_init(void);

/**
 * @brief Publishes a message to the specified MQTT topic.
 * @param topic The MQTT topic to publish to.
 * @param message The message to publish.
 * @param qos Quality of Service level for the message.
 */
void mqtt_publish(const char *topic, const char *message, int qos);

/**
 * @brief Checks if the MQTT client is connected to the broker.
 * @return bool True if connected, false otherwise.
 */
bool mqtt_is_connected(void);

#endif // MQTT_H