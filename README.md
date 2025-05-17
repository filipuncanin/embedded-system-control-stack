# Embedded Device Control and Monitoring Firmware with Remote Configuration Support

## Overview

This project delivers a sophisticated control system powered by the ESP32 microcontroller, designed for seamless remote configuration through a desktop application. By leveraging JSON configuration files, such as `configuration_example.json`, users can dynamically update the device’s behavior without reprogramming, making it ideal for automation, IoT, and industrial control applications. The firmware, built on the [ESP-IDF framework](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/index.html), supports a wide range of sensors, communication protocols, and programmable logic control (PLC) via ladder logic, ensuring flexibility and scalability.



The system’s core strength lies in its ability to receive new configurations remotely, eliminating the need for physical access or firmware updates. This is achieved through a desktop application that sends JSON files defining hardware settings, variables, and control logic to the device via BLE or MQTT. The `configuration_example.json` file serves as an example, demonstrating how digital inputs can control outputs in a simple relay-like setup.

## Key Features

| **Feature** | **Description** |
|-------------|-----------------|
| **Remote Configuration** | Update device behavior using JSON configurations sent via BLE or MQTT from a C# WPF application, eliminating the need for reprogramming. |
| **Sensor Support** | Interfaces with ADC Sensors for analog measurements, OneWire sensors and configurable digital/analog I/O pins. |
| **Communication Protocols** | Supports Wi-Fi for MQTT and NTP, BLE for local configuration, and MQTT for remote data exchange and control. |
| **Ladder Logic** | Implements PLC-style logic through JSON-defined ladder diagrams, supporting contacts, coils, timers, counters, and math operations. |
| **Variable Management** | Manages diverse variable types (Boolean, Number, Counter, Timer, Time) for flexible logic and data processing. |
| **Non-Volatile Storage (NVS)** | Persists configurations across reboots, ensuring consistent operation. |

## System Architecture

The system comprises two primary components:

### ESP32-S3 Firmware
- **Framework**: Built using ESP-IDF v5.4.1 or later, leveraging FreeRTOS for task management.
- **Functionality**: Handles hardware interfaces, sensor readings, ladder logic execution, and communication.
- **Modules**:
  - `device_config.c`: Initializes device and pin configurations.
  - `conf_task_manager.c`: Manages ladder logic tasks dynamically.
  - `adc_sensor.c`: Interfaces with ADC sensors.
  - `one_wire_detect.c`: Detects and reads OneWire sensors.
  - `ble.c`: Implements a BLE GATT server for configuration and monitoring.
  - `mqtt.c`: Manages MQTT communication for data publishing and configuration updates.
  - `wifi.c`: Handles Wi-Fi connectivity and NTP synchronization.
  - `variables.c`: Manages variable storage and updates.
  - `ladder_elements.c`: Defines ladder logic elements (e.g., NO/NC contacts, coils).

### Desktop Application (not included in this repository)
- **Purpose**: Provides a user-friendly interface to design hardware settings, variables, and ladder logic.
- **Key Features**:
  - **Ladder Diagram Editor**: Allows users to create rungs with elements like NO/NC contacts, coils, timers, counters, and comparisons using a drag-and-drop entry system.
  - **Variable Configuration**: Supports defining digital/analog inputs/outputs, booleans, numbers, timers, counters, and sensor variables with associated pins and values.
  - **Monitoring Panel**: Displays real-time data, including OneWire sensor statuses (e.g., DS18x20 temperature sensors) and variable states, with options to add new sensors.
  - **Communication**: Sends JSON configurations to the device via BLE or MQTT and monitors device status.

### Communication Flow
- **Configuration**: The desktop application sends JSON files to the device, which stores them in NVS and applies them instantly.
- **Data Exchange**: The device publishes sensor data and variable states via MQTT topics, and the application subscribes for real-time updates.
- **Control**: The application can send commands or update variables via BLE characteristics or MQTT messages.

## Inter-Device Communication

The application supports configuring inter-device communication, enabling multiple ESP32 devices to share data. Users can assign one or more parent devices to an ESP32 in the hardware configuration section of the interface. Once set, the device automatically sends its data (e.g., variable states, sensor readings) to its designated parent devices via MQTT. This feature facilitates coordinated control in distributed IoT or automation systems.

- **Configuration**: In the desktop app Parent(s) configuration panel, specify parent devices by adding their MAC addresses.

## Hardware Requirements

| **Component** | **Details** |
|---------------|-------------|
| **Microcontroller** | ESP32 module (e.g., ESP32-S3) |
| **Sensors** | TM7711 ADC, DS18x20 OneWire temperature sensors ... |
| **Peripherals** | GPIO pins (e.g., 48, 47, 33, 34 for inputs; 37, 38, 39, 40 for outputs), UART, I2C, SPI, OneWire interfaces |
| **Network** | Wi-Fi access point for MQTT and NTP |
| **Power** | Power supply compatible with ESP32 |

## Software Requirements

| **Component** | **Details** |
|---------------|-------------|
| **ESP-IDF** | Latest stable version (v5.4.1 or later recommended) [ESP-IDF Releases](https://github.com/espressif/esp-idf/releases) |
| **Desktop Application** | Required for configuration (not included in this repository) |
| **Dependencies** | - ESP-IDF libraries: `driver`, `esp_wifi`, `nvs_flash`, `mqtt`, `json`, `bt`.<br>- External components: `onewire` and `ds18x20` from [esp-idf-lib](https://github.com/UncleRus/esp-idf-lib), included in the project via `EXTRA_COMPONENT_DIRS` in `CMakeLists.txt`. |
| **Tools** | CMake 3.16+, Python 3, MQTT broker (e.g., Mosquitto) |

## Installation

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/your-repo/esp32-s3-control-system.git
   cd esp32-s3-control-system
   ```

2. **Set Up ESP-IDF**: Follow the [ESP-IDF Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/index.html) to install and configure ESP-IDF.

3. **Configure Wi-Fi and MQTT**: Copy `config/config.h.example` to `config/config.h` and update:
   ```c
   #define WIFI_SSID "your_wifi_ssid"
   #define WIFI_PASS "your_wifi_password"
   #define MQTT_BROKER_URI "mqtt://username:password@your_mqtt_broker"
   ```

4. **Build the Firmware**:
   ```bash
   idf.py build
   ```

5. **Flash the Firmware**:
   ```bash
   idf.py -p /dev/ttyUSB0 flash
   ```

6. **Monitor Output**:
   ```bash
   idf.py monitor
   ```

7. **Set Desktop Application**: Configure the application to connect via BLE or MQTT, ensuring it can communicate with the device.

## Usage

### Configuring the Device
1. Launch the C# WPF application.
2. Connect to the device via BLE (name: `ESP_XXYYZZ`) or MQTT.
3. Define:
   - **Variables**: Set up digital inputs, outputs, timers, counters, etc., using the variable configuration panel (e.g., add a timer with preset time or a counter with initial value).
   - **Ladder Logic**: Create logic using the ladder diagram editor by adding rungs with elements like NOContact, Coil, TimerOnDelay, or comparisons, accessible via the toolbar.
4. Save the configuration and send it to the device. The device applies and stores it in NVS.
5. Use the monitoring panel to add OneWire sensors (e.g., DS18x20) and view real-time statuses.



### Example Configuration
The `configuration_example.json` configures four digital input to control digital output:
```json
{
  "Device": {
    "device_name": "ESP32-S3",
    "logic_voltage": 3.3,
    "digital_inputs": [ 48, 47, 33, 34 ],
    "digital_inputs_names": [ "I1", "I2", "I3", "I4" ],
    "digital_outputs": [ 37, 38, 39, 40 ],
    "digital_outputs_names": [ "T REL", "REL1", "REL2", "REL3" ],
    "analog_inputs": [],
    "analog_inputs_names": [],
    "dac_outputs": [],
    "dac_outputs_names": [],
    "one_wire_inputs": [ 36 ],
    "one_wire_inputs_names": [ [] ],
    "one_wire_inputs_devices_types": [ [] ],
    "one_wire_inputs_devices_addresses": [ [] ],
    "pwm_channels": 8,
    "max_hardware_timers": 4,
    "has_rtos": true,
    "UART": [ 1, 2 ],
    "I2C": [ 0, 1 ],
    "SPI": [ 1, 2 ],
    "USB": true,
    "parent_devices": []
  },
  "Variables": [
    {"Type": "Digital Input", "Name": "dig_in_1", "Pin": "I1"},
    {"Type": "Digital Output", "Name": "dig_out_1", "Pin": "T REL"},
  ],
  "Wires": [
    {
      "Nodes": [
        {"Type": "LadderElement", "ElementType": "NOContact", "ComboBoxValues": ["dig_in_1"]},
        {"Type": "LadderElement", "ElementType": "Coil", "ComboBoxValues": ["dig_out_1"]}
      ]
    }
  ]
}
```
This configuration maps input `I1` to output `T REL`, activating the output when the input is high.

### Monitoring
- **BLE**: Access variable states via `READ_MONITOR_CHAR_UUID`.
- **MQTT**: Subscribe to topics like `/monitor` for updates.
- **Logs**: Use `idf.py monitor` for debugging.

## Configuration Format

The JSON configuration is structured into three main sections:

1. **Device**:
   - Defines hardware capabilities, including digital inputs/outputs, OneWire inputs, PWM channels, timers, and communication interfaces (UART, I2C, SPI, USB).
   - Example: Inputs on GPIO 48, 47, 33, 34; outputs on 37, 38, 39, 40.

2. **Variables**:
   - Includes digital inputs/outputs, booleans, numbers, timers, counters, and time variables.
   - Example: `dig_in_1` (input), `dig_out_1` (output), `timer_1` (5000ms).

3. **Wires**:
   - Defines ladder logic as rungs with nodes (contacts, coils, etc.).
   - Example: `dig_in_1` (NOContact) to `dig_out_1` (Coil).

## Ladder Logic

Ladder logic mimics PLC programming, with each “wire” representing a rung:
- **Elements**: Normally Open (NO) and Normally Closed (NC) contacts, coils, timers (on/off-delay), counters (up/down), comparisons (>, <, =), and math operations (add, subtract, multiply, divide).
- **Example**:
  ```json
  {
    "Nodes": [
      {"Type": "LadderElement", "ElementType": "NOContact", "ComboBoxValues": ["dig_in_1"]},
      {"Type":"LadderElement","ElementType":"OnDelayTimer","ComboBoxValues":["timer_1"]},
      {"Type": "LadderElement", "ElementType": "Coil", "ComboBoxValues": ["dig_out_1"]}
    ]
  }
  ```
  This activates `dig_out_1` after `dig_in_1` is high for `timer_1` preset time seconds.

## Adding New Functionality

To add new functionality:
1. **Define Variables**: Add new variables (e.g., timers, counters) in the desktop application.
2. **Design Logic**: Create ladder diagrams incorporating new variables.
3. **Send Configuration**: Save and transmit the JSON file to the device.

**Example**: To add temperature-based control:
```json
{
  "Variables": [
    {"Type": "One Wire Input", "Name": "temp_sensor", "Pin": 36},
    {"Type": "Number", "Name": "threshold", "Value": 25.0}
  ],
  "Wires": [
    {
      "Nodes": [
        {"Type": "GreaterCompare", "Variables": ["temp_sensor", "threshold"]},
        {"Type": "Coil", "Variable": "dig_out_1"}
      ]
    }
  ]
}
```
This turns on `dig_out_1` if the temperature exceeds 25°C.

## Technical Details

- **Data Structures**: `Device`, `Variable`, `Wire`, `ADCSensor`, `OneWireInput` for configuration and state management.
- **Task Management**: FreeRTOS tasks handle sensor reading, logic execution, and communication.
- **Communication**:
  - **BLE**: GATT server with characteristics (`READ_CONFIGURATION_CHAR_UUID`, `WRITE_CONFIGURATION_CHAR_UUID` ...).
  - **MQTT**: Topics include `/config_request`, `/config_response`, `/monitor`...
- **NVS**: Stores configurations under the “storage” namespace.
- **Error Handling**: CRC checks for OneWire data, logging via module-specific TAGs.

## File Structure

```
esp32-s3-control-system/
├── config/
│   ├── config.h.example        # Example configuration for Wi-Fi and MQTT
├── main/
│   ├── adc_sensor.c            # ADC sensor (TM7711) interface
│   ├── ble.c                   # BLE GATT server implementation
│   ├── conf_task_manager.c     # Ladder logic task management
│   ├── device_config.c         # Device and pin configuration
│   ├── mqtt.c                  # MQTT communication
│   ├── one_wire_detect.c       # OneWire sensor detection
│   ├── variables.c             # Variable management
│   ├── wifi.c                  # Wi-Fi connectivity
│   ├── CMakeLists.txt          # Component build configuration
├── CMakeLists.txt              # Project build configuration
├── sdkconfig.defaults          # Default ESP-IDF settings
```

## Contributing

1. Fork the repository.
2. Create a feature branch (`git checkout -b feature/your-feature`).
3. Commit changes (`git commit -m "Add your feature"`).
4. Push to the branch (`git push origin feature/your-feature`).
5. Open a pull request.


## Contact

For support, contact filipuncanin@gmail.com or open a GitHub issue.