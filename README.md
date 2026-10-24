# Computer-Temperature-Sensor

Reads the internal air temperature of a PC case and displays the value on an OLED. LEDs give visual status feedback.

**Status:** Firmware complete and tested on a Nucleo-L432KC dev board. Schematic complete (KiCad, ERC clean). PCB layout is next.

---

## Overview

This project is a custom PCB built around an STM32 microcontroller. The system measures the air temperature inside a PC case and displays it in real time (in both °C and °F) on an OLED screen. It also provides visual status using LEDs, including fault indication if the sensor or display fails.

The project follows a strict sequential methodology: documentation → prototyping on a dev board → schematic → PCB layout → firmware flashing on the final board.

---

## System Behavior

1. The temperature sensor measures the **air inside the PC case**.
2. The board is powered via **USB (5V)** from the PC.
3. The board regulates 5V down to **3.3V**.
4. The STM32 reads temperature over **I2C**.
5. The OLED displays the temperature continuously in °C and °F.
6. LEDs provide visual feedback (normal vs hot, plus fault indication).

---

## Architecture

```
USB 5V → LDO → 3.3V rail

STM32 ↔ I2C3 ↔ MCP9808 (Temp Sensor)
STM32 ↔ I2C1 ↔ SSD1306 (OLED)

STM32 → LEDs (status)
STM32 ↔ SWD (program/debug)
```

The sensor and the OLED are on **two separate I2C buses**, each with its own pull-up resistors. This matches the firmware that was tested on the dev board.

---

## Parts

### Microcontroller (MCU)

**STM32L432KCU6** (UFQFPN-32 package)
- Cortex-M4 @ up to 80 MHz (this firmware runs it at 32 MHz from the internal MSI oscillator)
- 256 KB Flash / 64 KB RAM
- Operates at 1.7–3.6 V
- UFQFPN-32 package (5×5 mm, 32-pin)

Design choices:
- Using the **bare IC (not a development board)**
- Prototyping used the "STM32 Nucleo-32 Development Board with STM32L432KC MCU" (Nucleo-L432KC)
- **No external crystals.** Both HSE and LSE are disabled; the internal MSI oscillator drives the PLL

Link(s):
- https://www.st.com/en/microcontrollers-microprocessors/stm32l432kc.html#overview
- https://www.tme.com/il/en/details/nucleo-l432kc/stm-development-kits/stmicroelectronics/

---

### Temperature Sensor

**MCP9808 (Bare IC - 8-MSOP)**

- Digital I2C temperature sensor
- High accuracy (±0.25°C)
- High resolution (0.0625°C)
- Operates at 3.3V
- I2C address 0x18 (7-bit)

Design choices:
- Using **bare IC (not breakout)**
- Prototyping used the MCP9808 Breakout Board from Adafruit
- Placed **at PCB edge** for accurate air measurement
- Includes **0.1 µF decoupling capacitor**
- Address pins (A0, A1, A2) tied to GND
- ALERT pin left unconnected (no-connect flag in the schematic)

Link(s):
- Breakout Board: https://www.adafruit.com/product/1782
- https://ww1.microchip.com/downloads/en/DeviceDoc/25095A.pdf
- https://www.digikey.com/en/products/detail/microchip-technology/MCP9808-E-MS/2802083

---

### OLED Display

**SSD1306 OLED (0.96", 128×64, I2C)**

- Rectangular display
- Good resolution for text
- Very common and well-supported
- Uses I2C, address 0x3C (7-bit)

Design choices:
- Using **module (not bare display)**
- Connected via **1×4 header (2.54 mm)**
- On its **own I2C bus** (I2C1), separate from the sensor
- 4.7 µF capacitor (C11) from 3.3V to GND at the header's power pin, for the display's pulsed current
- The module has its own pull-ups (measured roughly 11–12 kΩ on SDA and 19–20 kΩ on SCL, in circuit). The board still has its own 4.7 kΩ pull-ups on this bus; the two sets sit in parallel. Mark them DNP if the module's pull-ups are later confirmed sufficient.

Pinout (matches the printed order on the module):
1. GND
2. 3.3V
3. SCL
4. SDA

Link:
https://www.amazon.com/UCTRONICS-SSD1306-Self-Luminous-Display-Raspberry/dp/B072Q2X2LL

---

### LEDs

Normal operation:
- **Green LED solid** → normal temperature
- **Red LED solid** → high temperature
- **Blue LED solid** → OLED initialized and working

Fault indication:
- **Red LED flashing** → MCP9808 initialization failed (startup) or sensor stopped responding (during operation)
- **Blue LED flashing** → OLED initialization failed

Design:
- SMD 0805 LEDs
- Each LED has a **220 Ω** series resistor (same value as used on the breadboard prototype)
- Estimated current at 3.3V: roughly 5–6 mA for red and green, roughly 1–2 mA for blue (blue LEDs have a higher forward voltage, so it will be dimmer; actual values depend on the LED part chosen)
- Status LEDs are driven directly by STM32 GPIO pins

Link(s):
https://www.digikey.com/en/products/filter/led-indication-discrete/0805/105?s=N4IgjCBcpgTAnBaIDGUBmBDANgZwKYA0IAbgHZQAuATgK5EgD2UA2iAMzzuwAcIAusQAOlKCADKNAJZkA5iAC%2BSoA

---

### Programming & Debugging

**5-pin SWD Header (1×5, 2.54 mm)**

Pinout:
1. 3.3V
2. SWDIO
3. GND
4. SWCLK
5. NRST

- Standard STM32 debug interface
- Reliable and simple
- Flashing and debugging use an external ST-LINK probe connected to this header
- Pin 1 (3.3V) is a voltage reference tied to the board's 3.3V rail. On many ST-LINK V2 clones this pin is an output, so leave that wire disconnected when the board is already powered over USB.

---

### Debug UART Header

**2-pin header (1×2, 2.54 mm)**

Pinout:
1. TX (PA2, USART2_TX)
2. GND

- Connect a 3.3V USB-serial adapter to view debug messages in a terminal (115200 baud, 8N1), as was done with PuTTY on the Nucleo
- Transmit only (RX is not used)
- Used for bring-up and fault messages (for example sensor and OLED init status)

---

### Power System

#### Input

**Micro-USB (Type-B): Amphenol 10118194-0001LF**
- Used for **power only**
- Provides 5V

Connections:
- VBUS → 5V rail
- GND and shield → GND
- D+ / D- / ID → NC (no-connect flags in the schematic)

Link(s):
- https://www.amphenol-cs.com/product/101181940001lf.html
- https://www.digikey.com/en/products/detail/amphenol-cs-fci/10118194-0001LF/2785389

---

#### Voltage Regulator

**AP2112K-3.3TRG1 (LDO)**

- Output: 3.3V
- Max current: 600 mA
- EN pin tied to VIN (regulator always enabled)

Capacitors:
- CIN = 1 µF (C8, input, on 5V rail side)
- COUT = 1 µF (C9, output, on 3.3V rail side)
- CBULK = 10 µF, 16 V (C7, on 5V rail, before LDO, for USB inrush)

Link(s):
- https://www.digikey.com/en/products/detail/diodes-incorporated/AP2112K-3-3TRG1/4470746
- https://www.mouser.com/ProductDetail/Diodes-Incorporated/AP2112K-3.3TRG1?qs=x6A8l6qLYDDPYHosCdzh%2FA%3D%3D&srsltid=AfmBOop9CtVNQ4PzkGa5PHwTKG7HFoA91sVGvN81iwmOFJOXiIA_Ls7D

---

### Power Distribution

5V → LDO → 3.3V rail

3.3V powers:
- STM32
- MCP9808
- OLED
- LEDs
- SWD reference

---

### Power Budget

Approximate worst-case figures:

| Component | Current |
|---|---|
| STM32L432 active | ~10 mA |
| SSD1306 OLED | ~20 mA |
| MCP9808 | ~0.4 mA |
| 3 × LEDs (220 Ω: ~5.5 + ~5.5 + ~1.4 mA) | ~12 mA |
| **Total** | **~43 mA** |

Well within the AP2112K's 600 mA rating. (Red and green are never on together in normal operation, so real draw is usually lower.)

---

## Supporting Components

### STM32 Decoupling
- One 0.1 µF cap per VDD pin (C1 on pin 1, C2 on pin 17), placed as close to each pin as possible
- One 0.1 µF cap (C3) on VDDA (pin 5), plus 1 µF (C4) on VDDA
- 4.7 µF bulk capacitor (C5) on the 3.3V rail near the MCU
- The exposed pad (pin 33) and VSS pins 16 and 32 all go to GND

Note: VDDA must be powered and decoupled even if ADC/DAC are not used.

Part: Generic 0402 or 0603 MLCC, 0.1 µF / 10V / X5R or better

---

### I2C Buses

Two independent buses, one device each:

| Bus | Device | Address (7-bit) | SDA | SCL |
|---|---|---|---|---|
| I2C1 | SSD1306 OLED | 0x3C | PB7 | PB6 |
| I2C3 | MCP9808 | 0x18 | PB4 | PA7 |

Pull-ups (one pair per bus, 4.7 kΩ to 3.3V):
- I2C3 (MCP9808): R2 on SDA, R3 on SCL
- I2C1 (OLED): R4 on SDA, R5 on SCL

Note: the OLED module has its own pull-ups (see OLED Display above), so the I2C1 pair is in parallel with them. The MCP9808 is a bare IC, so its pull-ups are the only ones on I2C3.

Net names in the schematic: `I2C1_SDA`, `I2C1_SCL`, `I2C3_SDA`, `I2C3_SCL`.

---

### Reset Circuit

Pushbutton: Panasonic EVQ-PUJ02K (or equivalent SMD 3 × 4 mm)

- NRST → pushbutton → GND
- 100 nF capacitor (C6) from NRST to GND, for noise immunity and button debounce
- No external pull-up needed (NRST has an internal pull-up)
- NRST also goes to SWD header pin 5 so the probe can reset the MCU

Link(s):
https://www.digikey.com/en/products/detail/panasonic-electronic-components/EVQ-PUJ02K/286348

---

### Boot Configuration

**BOOT0 pull-down resistor: 10 kΩ (0402 or 0603)**

- BOOT0 (pin 31, labeled PH3 on the symbol) → 10 kΩ (R1) → GND
- Keeps STM32 in normal flash boot mode
- Programming is over SWD only (USB is power-only), so no bootloader jumper

Part: Generic 0402 or 0603 thick film resistor 10 kΩ

---

### Test Points

Added for debugging (2 mm pads, TP1 to TP6):
- 3.3V
- GND
- I2C1 SDA and SCL (OLED bus)
- I2C3 SDA and SCL (sensor bus)

---

## Schematic Reference Designators

| Ref | Part | Value / note |
|---|---|---|
| U1 | STM32L432KCU6 | MCU |
| U2 | AP2112K-3.3 | 3.3V LDO |
| U3 | MCP9808 (MSOP-8) | Temperature sensor |
| J_USB | Micro-USB (Amphenol 10118194-0001LF) | Power only |
| J_SWD | 1×5 header | SWD |
| J_OLED | 1×4 header | OLED module |
| J_UART | 1×2 header | Debug TX + GND |
| SW1 | Panasonic EVQ-PUJ series pushbutton | Reset |
| D1, D2, D3 | 0805 LEDs | Green (PA3), red (PA4), blue (PA1) |
| R1 | 10 kΩ | BOOT0 pull-down |
| R2, R3 | 4.7 kΩ | I2C3 SDA, SCL pull-ups |
| R4, R5 | 4.7 kΩ | I2C1 SDA, SCL pull-ups |
| R6, R7, R8 | 220 Ω | Green, red, blue LED series resistors |
| C1, C2, C3 | 100 nF | MCU VDD (×2) and VDDA decoupling |
| C4 | 1 µF | VDDA |
| C5 | 4.7 µF | 3.3V bulk near MCU |
| C6 | 100 nF | NRST |
| C7 | 10 µF, 16 V | 5V bulk |
| C8 | 1 µF | LDO input |
| C9 | 1 µF | LDO output |
| C10 | 100 nF | MCP9808 decoupling |
| C11 | 4.7 µF | OLED header supply |
| TP1 to TP6 | 2 mm test pads | 3.3V, GND, I2C1 SDA/SCL, I2C3 SDA/SCL |

---

## PCB Design Choices

- 2-layer PCB
- Top layer: components + signals
- Bottom layer: ground plane
- Board size: ~50 mm × 40 mm
- Shape: rectangular
- 4 × M3 mounting holes
- Board version number in silkscreen (v1.0)
- Component labels and header pin labels in silkscreen

---

## Pin Assignments

Defined in STM32CubeMX and tested on the Nucleo-L432KC. Matches the final schematic (ERC clean, 2026-10-23).

| Function | MCU pin | Package pin | Peripheral / Mode | Schematic net |
|---|---|---|---|---|
| OLED SDA | PB7 | 30 | I2C1_SDA | `I2C1_SDA` |
| OLED SCL | PB6 | 29 | I2C1_SCL | `I2C1_SCL` |
| MCP9808 SDA | PB4 | 27 | I2C3_SDA | `I2C3_SDA` |
| MCP9808 SCL | PA7 | 13 | I2C3_SCL | `I2C3_SCL` |
| Green LED | PA3 | 9 | GPIO output | `LED_GREEN` |
| Red LED | PA4 | 10 | GPIO output | `LED_RED` |
| Blue LED | PA1 | 7 | GPIO output | `LED_BLUE` |
| Debug UART TX | PA2 | 8 | USART2_TX (to debug header) | `UART_TX` |
| SWDIO | PA13 | 23 | SWD | direct to header pin 2 |
| SWCLK | PA14 | 24 | SWD | direct to header pin 4 |
| NRST | NRST | 4 | Reset | `NRST` |
| BOOT0 | PH3-BOOT0 | 31 | Boot | Pulled to GND (R1) |

All other MCU pins are unused and marked no-connect.

On the Nucleo-L432KC, USART2 TX (PA2) reaches the PC through the board's ST-LINK virtual COM port. On the custom PCB it goes to the debug UART header and needs a USB-serial adapter.

---

## Firmware

Written in C with STM32CubeMX / HAL, developed in STM32CubeIDE.

### Behavior

- **Startup:** retries the MCP9808 until it answers (red LED flashes after the first few failed attempts). Tries the OLED up to 3 times, then continues without a display if it fails (blue LED flashes).
- **Main loop (every 500 ms):** reads the temperature, shows °C and °F on the OLED, and updates the red/green LEDs.
- **Temperature thresholds (with hysteresis):** red LED turns **on at 90 °F or above** and turns **off only below 85 °F**. Between 85 and 90 °F the LEDs keep their previous state, so the LED does not flicker near the threshold.
- **Sensor fault during operation:** after 3 failed reads in a row, the OLED shows `SENSOR ERR`, the green LED turns off, and the red LED flashes. The I2C peripheral is reset on failed reads, and the system recovers automatically when the sensor responds again.

| Situation | Result |
|---|---|
| Sensor missing at boot | Retries forever, red LED flashes, recovers if reconnected |
| OLED missing at boot | Continues without display, blue LED flashes, LEDs and temperature logic still work |
| Sensor lost while running | `SENSOR ERR` on OLED, red LED flashing, recovers on reconnect |

### Source files

| File | Purpose |
|---|---|
| `MCP9808.c` / `MCP9808.h` | MCP9808 sensor driver (`MCP9808_Init`, `MCP9808_ReadTempC` returning °C and °F) |
| `ssd1306.c` / `ssd1306.h` / `fonts.h` | OLED driver, third-party (see below) |
| `main.c` | Initialization, main loop, LED logic, fault handling |

### Third-party code

The SSD1306 OLED driver (`ssd1306.c`, `ssd1306.h`, and the font files) was provided in an embedded systems course. It is based on work by Tilen Majerle and Alexander Lutsai and is licensed under the **GNU GPL v3 or later**. Its original license headers are kept in the source files.

### Dev-board note (Nucleo-L432KC)

On the Nucleo-32 board, solder bridges SB16 and SB18 connect PA5 to PB7 and PA6 to PB6. Using PA5 or PA6 as outputs interferes with the I2C1 pins (it broke OLED initialization during prototyping). This only affects the Nucleo board; the custom PCB does not have these bridges.

---

## Notes

- STM32 internal clock is used (no external crystals; HSE and LSE are both disabled)
- The OLED and the sensor are on separate I2C buses
- Sensor placement is critical for accurate readings (placed at PCB edge)
- Designed for simplicity and first-pass success
- Pin assignments were defined in STM32CubeMX during firmware development and were verified against the final schematic

---

## Functionality Flow Chart

_(to be added)_
