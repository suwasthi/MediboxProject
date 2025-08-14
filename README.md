# Wokwi Medibox Project

This project simulates a smart medicine box (Medibox) running on an ESP32 platform. It helps users manage their medication schedules effectively by combining sensor monitoring, real-time alerts, and remote control capabilities.

## Medibox Functionality

- The Medibox continuously measures temperature and humidity using a DHT22 sensor to ensure medicine storage conditions are optimal.
- It monitors ambient light intensity with an LDR sensor and controls a servo motor that can adjust a physical component based on light levels.
- The device displays current time, temperature, humidity, and alarm settings on an OLED screen for easy user interaction.
- Multiple alarms can be set for medication times with snooze and cancel options, accompanied by buzzer sounds and LED indicators.
- The system allows users to configure time zone, alarm times, and other settings through push buttons and an intuitive menu interface.

## Key Features

- **Sensor Monitoring:** Accurate temperature, humidity, and light sensing to maintain optimal medicine storage conditions.
- **Alarm System:** Configurable alarms with audible buzzer and visual LED alerts, including snooze functionality.
- **OLED Display Interface:** Real-time display of time, sensor readings, and alarm statuses.
- **User Interaction:** Easy navigation and configuration using push buttons.
- **WiFi Connectivity and MQTT Communication:**  
  The Medibox connects to WiFi and publishes sensor data to MQTT topics, enabling remote monitoring and control.
- **Node-RED Integration:**  
  Seamless integration with Node-RED allows users to visualize sensor data, receive alerts, and adjust device parameters remotely by subscribing and publishing to MQTT topics. This feature extends the Medibox functionality beyond the physical device and provides a user-friendly dashboard for management.

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

1. Assemble hardware components as per the code pin assignments.  
2. Upload the code to your ESP32 device.  
3. Connect the device to your WiFi network.  
4. Use push buttons to set time zone, alarms, and other parameters.  
5. Sensor data is published periodically over MQTT to topics such as `ADMIN-TEMP` and `ADMIN-LIGHT-AVG`.  
6. Use Node-RED to subscribe to these MQTT topics to monitor sensor readings and control device parameters remotely.  
7. Alarms notify the user via buzzer and LED indicators at scheduled times.
