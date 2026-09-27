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
