# Rocket Apogee Detection

**Embedded system for automatic apogee detection and parachute deployment in experimental rockets.**

This project presents the development of an embedded system designed to detect the apogee of an experimental rocket using barometric pressure measurements and automatically trigger a parachute deployment mechanism.

The system was developed using an **ESP32**, a **BMP085 barometric pressure sensor**, a **servo motor**, and a **buzzer**. The software was implemented in **C with ESP-IDF**, using **FreeRTOS**, a finite state machine, and a moving-average filter for pressure data processing.
