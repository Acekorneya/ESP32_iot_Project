# ESP32 Environmental Monitoring and Control System

This project involves an ESP32-based system for monitoring environmental parameters and controlling devices like fans and LEDs. The system uses sensors for temperature, humidity, pressure, air quality index (IAQ), and dew point, and displays the data on an LCD and a web interface. Additionally, it allows manual and automatic control of a fan based on temperature thresholds.

## Table of Contents
- [Features](#features)
- [Components](#components)
- [Circuit Diagram](#circuit-diagram)
- [Installation](#installation)
- [Usage](#usage)
- [Web Interface](#web-interface)
- [LCD Display](#lcd-display)
- [Data Logging](#data-logging)
- [Contributing](#contributing)
- [License](#license)

## Features
- Monitor temperature, humidity, pressure, IAQ, dew point, and altitude.
- Display sensor readings on a 16x2 I2C LCD.
- Control a fan based on temperature thresholds with manual and automatic modes.
- Web interface for real-time monitoring and control.
- Data logging to SD card.
- ESP-NOW communication for remote control.

## Components
- ESP32 development board
- BME680 sensor for environmental data
- 16x2 I2C LCD display
- LED
- Fan
- SD card module
- Additional components: resistors, jumper wires, breadboard, etc.

## Circuit Diagram
To be added.

## Installation

### Hardware Setup
1. Connect the BME680 sensor to the ESP32:
   - SDA to GPIO 21
   - SCL to GPIO 22
2. Connect the 16x2 I2C LCD to the ESP32:
   - SDA to GPIO 21
   - SCL to GPIO 22
3. Connect the LED to GPIO 27.
4. Connect the fan control circuit to an appropriate GPIO pin.
5. Connect the SD card module to the ESP32:
   - MISO to GPIO 19
   - MOSI to GPIO 23
   - SCK to GPIO 18
   - CS to GPIO 5

### Software Setup
1. Clone this repository:
   ```bash
   git clone https://github.com/Acekorneya/Symbol_Neural_Predictions.git
   cd Symbol_Neural_Predictions
   ```

2. Install the necessary libraries in the Arduino IDE or PlatformIO:
   - `LiquidCrystal_I2C`
   - `WiFi`
   - `WebServer`
   - `Wire`
   - `BSEC`
   - `esp_now`
   - `SD_MMC`
   - `FS`

3. Open the project in your preferred development environment and upload the code to the ESP32.



