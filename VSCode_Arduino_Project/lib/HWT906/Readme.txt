# HWT906 Parser Library

A lightweight C library for parsing 33-byte serial frames from the WitMotion HWT906 IMU. It extracts and stores acceleration, gyroscope, and Euler angle data, providing simple APIs to retrieve the latest values.

## 📦 Features

- Parses raw serial frames from HWT906 (0x55 header format)
- Validates checksum and frame type
- Stores latest values for:
  - Acceleration (g)
  - Gyroscope (°/s)
  - Euler angles (°)
- Minimal dependencies (pure C, no external libraries)

## 🔧 Usage

### 1. Include the Header

```c
#include "hwt906_parser.h"


Introduction
The WitMotion HWT906 is a widely used 9-axis inertial measurement unit (IMU) sensor that communicates over serial interfaces such as UART, I²C, or SPI. Its popularity in the hobbyist, industrial, and research communities is driven by its balance of performance, cost, and cross-platform compatibility. However, real-time use in embedded and desktop systems frequently necessitates replacing interpreted language parsing (e.g., Python) with minimal, dependency-free C code optimized for size, speed, and portability.

This report offers a comprehensive, stepwise design and implementation plan for a C library dedicated to parsing the 33-byte binary serial frames output by the HWT906, internally storing the latest measured values for acceleration, gyroscope, and Euler (angle) data, and exposing a minimal, clear API suitable for both embedded and desktop applications. To ensure maximal relevance, we map each design decision to best practices in the field of C library development, embedded-friendly data structures, and both Python and C idioms observed in existing parsers.

1. WitMotion HWT906 Protocol Overview
The HWT906 uses what WitMotion terms the "Wit Standard Protocol," which serializes sensor measurements in rigidly structured 33-byte frames. Each data frame corresponds to a particular measurement set (e.g., acceleration, angular rate, angle, magnetometer, etc.), with each set mapped to a specific ID byte. These frames are repeated at the sensor's output frequency, and each contains raw sensor values in little-endian signed/integer format, along with a trailing one-byte checksum.

Key Points of the Protocol
Frame Start: Every frame begins with two specific header bytes—typically 0x55 followed by a frame type identifier (e.g., 0x51 for acceleration, 0x52 for angular velocity, 0x53 for angles, etc.).

Data Payload: After the header come sets of 2-byte (16-bit) signed integer values, each representing sensor measurements.

Checksum: The last byte in the frame is a checksum—typically a 1-byte sum modulo 256 over the preceding 10 bytes of payload (excluding the 0x55 header).

Fixed Frame Size: Frames always have a fixed length, enabling simple, efficient parsing routines.

Message Repetition: Each physical output "frame" contains up to 33 logical frames, or up to 9 different message types for each data update interval.