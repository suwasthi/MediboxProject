# MediboxProject

# Wokwi Medibox Simulation Project

This project is a simulation of a smart medicine box (Medibox) implemented on an ESP32 using the Wokwi platform. It combines sensor data acquisition, MQTT communication, an OLED display, servo control, and alarm functionality to remind users about their medication schedules.

## Features

- Reads temperature and humidity using DHT22 sensor.
- Measures ambient light intensity with LDR sensor and controls a servo motor accordingly.
- Displays current time, temperature, humidity, and alarm settings on an OLED screen.
- Connects to WiFi and communicates via MQTT protocol.
- Allows integration with **Node-RED** for real-time monitoring and control via MQTT topics.
- Allows setting multiple alarms with snooze and cancel options.
- Interactive menu controlled via push buttons.
- Configurable time zone and alarm times.
- Audible buzzer alarm with musical notes.
- Visual LED indicators for alerts.

## Hardware Components

- ESP32 Development Board
- DHT22 Temperature and Humidity Sensor
- LDR (Light Dependent Resistor)
- Servo Motor
- OLED Display (SSD1306)
- Buzzer
- Push Buttons (Cancel, OK, Up, Down, Snooze)
- LEDs

## Libraries Used

- Wire
- WiFi
- time
- Adafruit_GFX
- Adafruit_SSD1306
- DHTesp
- PubSubClient
- ESP32Servo

## How to Use

1. Connect the hardware components according to the defined pins in the code.
2. Upload the code to your ESP32 device.
3. Ensure the ESP32 is connected to a WiFi network named `"Wokwi-GUEST"` (or modify SSID in code).
4. Use the push buttons to navigate the menu, set the time zone, and configure alarms.
5. The device reads sensor data periodically and publishes temperature and light intensity data via MQTT.
6. Use **Node-RED** to subscribe to MQTT topics (e.g., `ADMIN-TEMP`, `ADMIN-LIGHT-AVG`, `MEDIBOX/motor_angle`) for real-time visualization and control.
7. When an alarm time is reached, the buzzer and LED notify the user.

## MQTT Topics Used

| Topic                   | Description                            |
|-------------------------|------------------------------------|
| `ADMIN-TEMP`            | Publishes current temperature data  |
| `ADMIN-LIGHT-AVG`       | Publishes average light sensor value |
| `MEDIBOX/motor_angle`   | Publishes servo motor angle          |
| `ADMIN-ts`              | Subscribe to update light sample time interval (`ts`) |
| `ADMIN-tu`              | Subscribe to update light averaging time interval (`tu`) |
| `MEDIBOX/theta_offset`  | Subscribe to update servo angle offset |
| `MEDIBOX/gamma`         | Subscribe to update gamma parameter  |
| `MEDIBOX/Tmed`          | Subscribe to update median temperature parameter |

## Notes

- Modify WiFi credentials in the code if needed.
- MQTT broker is set to `broker.hivemq.com`.
- Time synchronization is done via NTP (`pool.ntp.org`).
- Adjust sensor pins and settings in the code as per your hardware setup.
- To use Node-RED, create MQTT input nodes to listen on the topics above for dashboard visualization or custom actions.

