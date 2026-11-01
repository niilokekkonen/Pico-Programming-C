# Pico Programming with C
C programming exercises for the Raspberry Pi Pico (RP2040), written with the
[Pico SDK](https://github.com/raspberrypi/pico-sdk).


GPIOs assigned to LEDs are `20, 21, 22`, and GPIOs `7, 8 ,9` are buttons defined with pull up resistors using the `gpio_pull_up()` function.
GPIO `28` is an OPTO fork optical sensor. Used for calibrating the stepper motor
GPIOs `2, 3, 6, 13` are connected to the stepper motors coils `A, B, C, D`


The repository contains course assignments that use LEDs, buttons, a rotary
encoder and a stepper motor. The most recent assignment is a **unipolar stepper
motor controller** with optical position calibration and a serial command
interface.
## Hardware

The hardware list grows as the assignments expand.

- Raspberry Pi Pico W (RP2040)
- Raspberry Pi Debug Probe
- Development board 
- 28BYJ-48 unipolar stepper motor with a ULN2003 driver board
- OPTO fork (optical sensor), used to calibrate the stepper motor
- Connectors and cables for the peripherals

## Pin Layout

| GPIO              | Function                                   | Notes                                              |
| ----------------- | ------------------------------------------ | -------------------------------------------------- |
| `20`, `21`, `22`  | LEDs                                       | PWM LED dimmer assignment                          |
| `7`, `8`, `9`     | Buttons                                    | Configured with pull-up                            |
| `10`, `11`, `12`  | Rotary encoder                             | Read with GPIO interrupts (dimmer assignment)      |
| `28`              | OPTO fork                                  | Input with pull-up. Used for stepper calibration   |
| `2`               | Stepper coil A (driver input IN1)          | Output                                             |
| `3`               | Stepper coil B (driver input IN2)          | Output                                             |
| `6`               | Stepper coil C (driver input IN3)          | Output                                             |
| `13`              | Stepper coil D (driver input IN4)          | Output                                             |
| `0`, `1`          | UART TX / RX to the debug probe            | Used for `printf` and command input                |

## Requirements

- Pico SDK (set the `PICO_SDK_PATH` environment variable, or let your IDE manage it)
- CMake
- Arm GNU toolchain (`arm-none-eabi-gcc`)
- An IDE with CMake support (CLion or VS Code), or just a terminal
- OpenOCD, if you flash through the Debug Probe

## Build

From the repository root:

```sh
mkdir build
cd build
cmake ..
cmake --build .
```

The build produces an `.elf` and a `.uf2` file for each target. The stepper
motor assignment builds a target named `assignment_3`.

In CLion, open the project folder, let it load the CMake profile, and build the
target from the toolbar.

## Serial console

The stepper assignment prints and reads text through the **UART**, which the
Debug Probe forwards to your computer. This is selected in `CMakeLists.txt`:

```cmake
pico_enable_stdio_usb(assignment_3 0)
pico_enable_stdio_uart(assignment_3 1)
```

**Wiring (probe to Pico)**

| Probe | Pico        |
| ----- | ----------- |
| TX    | GP1 (RX)    |
| RX    | GP0 (TX)    |
| GND   | GND         |

**Terminal settings**

- Port: the Debug Probe's serial port (for example `/dev/ttyACM0` or a `COM` port)
- Baud rate: 115200 (Pico SDK default), 8 data bits, no parity, 1 stop bit
- Line ending sent with Enter: **LF** or **CR+LF**. If the terminal sends only CR,
  the program waits for the end of the line forever

## Stepper motor controller

An interactive command line for a 28BYJ-48 stepper motor driven with the
half-step sequence (about 4096 steps per output revolution). The exact number of
steps per revolution is measured with the OPTO fork, because the gear ratio is
not exactly 1:64.

### Commands

| Command   | Description                                                                                         |
| --------- | --------------------------------------------------------------------------------------------------- |
| `status`  | Prints whether the motor is calibrated and the number of steps per revolution, or "not available"   |
| `calib`   | Calibrates the motor position and measures the steps per revolution                                 |
| `run [N]` | Runs the motor `N` times 1/8 of a revolution. Without `N` it runs one full revolution (same as `8`) |

Rules for `run`:

- `N` is an integer from `0` to `117`. `run 0` does nothing
- `run` is rejected until the motor has been calibrated
- The steps are calculated as `N * steps_per_rev / 8` (multiply first, divide last,
  so integer rounding does not accumulate)
- With the default 1 ms step delay the longest allowed run takes about a minute

Other input rules:

- Text after `status` or `calib` is ignored
- Lines of 19 characters or more are rejected
- Invalid input prints an error message and the prompt returns

## How the stepper control works

**Half-step sequence.** Eight steps in the sequence, alternating one coil and two neighboring coils.
Coil A is the most significant bit of the 4-bit value.

| Step | A | B | C | D | Value    |
| ---- | - | - | - | - | -------- |
| 1    | 1 | 0 | 0 | 0 | `0b1000` |
| 2    | 1 | 1 | 0 | 0 | `0b1100` |
| 3    | 0 | 1 | 0 | 0 | `0b0100` |
| 4    | 0 | 1 | 1 | 0 | `0b0110` |
| 5    | 0 | 0 | 1 | 0 | `0b0010` |
| 6    | 0 | 0 | 1 | 1 | `0b0011` |
| 7    | 0 | 0 | 0 | 1 | `0b0001` |
| 8    | 1 | 0 | 0 | 1 | `0b1001` |

- The rows are stored in a `static const uint8_t` array. The array index is the
  step number (index 0 is step 1), and the index wraps with modulo 8.
- Each 4-bit value is mapped onto the real pins (GP2, GP3, GP6, GP13) by shifting
  each coil bit to its pin number, then written with one masked GPIO write so the
  other GPIOs are not affected and all four coils change at the same time.
- Direction is the same sequence run forwards (index + 1) or backwards
  (index + 7, which equals index - 1 modulo 8).
- After a run the last row stays on, so the coils hold the motor in place (holding
  torque). The sequence index is stored in `motor_state`, so the next command
  continues from the next row without a jump.
- Rotation speed is set by the delay between steps (`MOTOR_SLEEP_TIME_MS`).
  If the motor only vibrates, increase the delay. 


### Calibration

1. The motor runs in one direction until the OPTO fork input has a **falling edge**.
   This first edge only marks the starting point.
2. The program counts the steps between the next falling edges, one revolution at a time.
3. Three revolutions are measured (four falling edges in total) and the average
   is stored as the steps per revolution.

`calib` can be run again at any time. The sequence index is kept between runs, so
the motor continues smoothly from where it stopped. Calibration takes several
seconds. If the sensor cable is disconnected, no edge appears and calibration
never finishes.
