# AVR FreeRTOS Temperature-Controlled Motor System

This project implements a multi-tasking embedded system on an AVR microcontroller (ATmega series) using **FreeRTOS**. The system monitors ambient temperature to control a motor automatically, while providing a high-priority hardware manual override via a push button.

## Features

* **Real-Time OS:** Built on FreeRTOS for efficient task scheduling and resource management.
* **Automated Control:** Motor triggers based on temperature thresholds (LM35 sensor).
* **Safety Override:** Physical button interrupt to force-stop the motor regardless of sensor data.
* **Inter-Task Communication:** Uses **Queues** for state passing and **Mutexes** for thread-safe UART logging.
* **UART Debugging:** Real-time feedback of system status, sensor readings, and motor actions.

## System Architecture

The application is divided into three primary tasks with distinct priorities:

| Task Name | Priority | Function |
| --- | --- | --- |
| **Button Monitor** | 3 (Highest) | Scans for manual override. Sets the `overrideActive` flag. |
| **Temp Monitor** | 2 (Medium) | Reads LM35 sensor and sends desired motor state to the queue. |
| **Motor Control** | 1 (Lowest) | Listens to the queue and drives the motor hardware. |

### Logic Flow

1. **Input:** The system polls the LM35 sensor every 500ms.
2. **Logic:** If Temp > Threshold, it requests `MOTOR_ON`.
3. **Override:** If the button is pressed, the Button Task clears the queue and sets an override flag, blocking sensor logic until released.
4. **Logging:** All tasks share the UART peripheral, protected by an `xUART_Mutex` to prevent message corruption.

##  Requirements

### Hardware

* AVR Microcontroller (e.g., ATmega32/328P/128)
* LM35 Temperature Sensor
* DC Motor with Driver (e.g., L293D or ULN2003)
* Push Button
* USB-to-UART Converter (for debugging)

### Software

* **AVR-GCC** compiler
* **FreeRTOS** Kernel source files
* Hardware drivers included in the project: `uart.h`, `lm35.h`, `button.h`, `motor.h`

## Getting Started

1. **Clone the Repository:**
```bash
git clone https://github.com/yourusername/your-repo-name.git

```


2. **Configure Clock Speed:** Ensure `F_CPU` in the source code matches your hardware (default set to **8MHz**).
3. **Compile:**

```bash
    avr-gcc -mmcu=atmega32 -Wall -Os -o main.elf main.c ... (include RTOS files)
    ```
4.  **Flash:**
    ```bash
    avrdude -c usbasp -p m32 -U flash:w:main.hex
    ```

## UART Output Example
```text
[Sensor] Temp: 28 C
[Motor] Action: STOPPED
[Sensor] Temp: 32 C
[Motor] Action: RUNNING
[OVERRIDE] Button Open: Forcing Motor OFF
[Motor] Action: STOPPED



```
