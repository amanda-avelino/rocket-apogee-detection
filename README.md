# Rocket Apogee Detection

**Embedded system for automatic apogee detection and parachute deployment in experimental rockets.**

This project presents the development of an embedded system designed to detect the apogee of an experimental rocket using barometric pressure measurements and automatically trigger a parachute deployment mechanism.

The system was developed using an **ESP32**, a **BMP085 barometric pressure sensor**, a **servo motor**, and a **buzzer**. The software was implemented in **C with ESP-IDF**, using **FreeRTOS**, a finite state machine, and a moving-average filter for pressure data processing.

## Objectives

The main objective of this project was to develop an embedded system capable of automatically detecting the apogee of an experimental rocket and activating a recovery mechanism.

The specific objectives were:

- Acquire atmospheric pressure data using a BMP085 barometric sensor.
- Process the pressure measurements using a moving-average filter.
- Detect the transition from ascent to descent.
- Implement a finite state machine for flight-state management.
- Use FreeRTOS tasks for concurrent system operation.
- Automatically activate a servo motor responsible for the parachute deployment mechanism.
- Provide an audible indication through a buzzer.
- Validate the system through controlled bench tests simulating ascent and descent.

## Hardware

The system was built around an **ESP32** microcontroller and a **BMP085** barometric pressure sensor. The pressure sensor communicates with the ESP32 through the **I²C** protocol.

The recovery mechanism uses an **SG90 servo motor**, while a **buzzer** provides an audible indication after apogee detection.

### Main Components

- ESP32 microcontroller
- BMP085 barometric pressure sensor
- SG90 servo motor
- Buzzer
- Protoboard
- Jumper wires
- USB cable

### Pin Configuration

| Component | ESP32 Pin |
|---|---|
| BMP085 SDA | GPIO 21 |
| BMP085 SCL | GPIO 22 |
| Servo | GPIO 13 |
| Buzzer | GPIO 12 |
| BMP085 VCC | 3.3 V |
| Ground | GND |
