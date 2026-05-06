# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Flash Commands

This project uses **PlatformIO** targeting an **Arduino Due (dueUSB)** via an **Atmel-ICE** debugger/programmer.

```bash
# Build
pio run

# Build and upload via Atmel-ICE
pio run --target upload

# Open serial monitor (COM3, 115200 baud)
pio device monitor

# Clean build artifacts
pio run --target clean
```

Serial protocol uses `CmdMessenger` over UART at 115200 baud. The monitor port is hardcoded to `COM3` in `platformio.ini`.

## Architecture Overview

### Hardware Target
- **Arduino Due (SAM3X8E ARM Cortex-M3)** — uses SAM-specific registers directly (e.g., `PIOB->PIO_PER`)
- **3x stepper motors** controlled via SPI (TMC driver ICs), chip-selects on pins 2, 10, 11; enable pins on 3, 5, 6
- **HWT906 IMU** on UART1 (pins 18/19)
- **BLE UART module** on UART3 (pins 16/17), using Seeed nRF52 BLE UART library
- **Analog joystick** on A0/A1

### Application Layer (`src/Applications/`)
Each subsystem is encapsulated as a class with `Init()` and `Service*()` methods:

| Class | File | Responsibility |
|---|---|---|
| `HWT906_App` | `HWT906_App.cpp` | IMU init, interrupt-driven data collection, averaging |
| `BLE_Bridge_App` | `BLE_Bridge_App.cpp` | BLE UART init and framed serial bridge |
| `System_Control_App` | `System_Control_App.cpp` | Motor/joystick init, all CmdMessenger command handlers |
| `LED_App` | `LED_App.cpp` | Status LED blink patterns encoding init state |

### Motor & Joystick Control (`src/JoystickMotorControl/`)
- `CombinedControl` wraps `MotorControl motor[3]` + `Joystick` into one interface
- `MotorControl` speaks SPI to the TMC stepper driver per motor
- Motor 3 (index 2) is disabled at startup and enabled on-demand; `ServiceMotor3PowerDisable()` monitors velocity to auto-cut power

### Scheduling (`lib/AsyncTask-master/`)
`AsyncTask` (cooperative, non-preemptive) schedules four periodic tasks from `setup()`:
- `Mtr3PowerDisableCheck` — 1000 ms
- `ServiceIMUapp` — 233 ms (`IMU_DATA_ACQUSITION_PERIOD`)
- `ServiceLEDapp` — 250 ms (`LED_FREQ_RATE_MS`)
- `ServiceJSswitch` — 500 ms (`JS_SWITCH_CHK`)

### BLE Framing (`lib/BLE_Bridge/`)
`BLEBRIDGE_LIB` provides CRC32-framed packet build/parse over the BLE UART channel. Both `Serial` (USB) and BLE echo the same response strings from command callbacks.

### Serial Command Protocol
Commands are received via `CmdMessenger` on `Serial` (USB-CDC). Every response string ends with `;`. Key response prefixes: `imu,`, `m,`, `v,`, `x,`, `S,1;` (success), `S,0;` (fail), `p,PONG;`.

### Init State Flags (`SystemInitState_e` in `System_definitions.h`)
Bit-field in `SystemInitState` byte: bits 0–4 flag BLE, HWT906, and Motor 1–3 init failures. `LED_App` encodes this byte as a blink pattern.

### Debug Macros
Enable by uncommenting in `System_definitions.h`: `DEBUG_MOTOR`, `DEBUG_HOME`, `DEBUG_DIR`, `DEBUG_COM`. The global `DEBUG=0` flag is set via `build_flags` in `platformio.ini`.

## Key Conventions
- All response strings use `outputStr` (a module-level `String` with 128-byte reserved buffer); always call `outputStr.remove(0)` before building a new response.
- Motor index 0 = Motor 1, index 1 = Motor 2, index 2 = Motor 3 (the vertical/lift axis).
- Position units are raw TMC register counts; 200 microsteps = one full motor step (1.8° motor).
- IMU data is transmitted as hex-encoded `floatUnion` values (6 axes) in the `imu,` frame.
