# Computer-Temperature-Sensor

Reads the internal air temperature of a PC and displays the value on an OLED. Includes LEDs for visual feedback.

---

## Overview

This project is a custom PCB built around an STM32 microcontroller. The system measures the air temperature inside a PC case and displays it in real time on an OLED screen. It also provides visual status using LEDs.

---

## System Behavior

1. The temperature sensor measures the **air inside the PC case**.
2. The board is powered via **USB (5V)** from the PC.
3. The board regulates 5V down to **3.3V**.
4. The STM32 reads temperature over **I2C**.
5. The OLED displays the temperature continuously.
6. LEDs provide visual feedback (normal vs hot).

---

## Architecture

USB 5V → LDO → 3.3V rail

STM32 ↔ I2C ↔ MCP9808 (Temp Sensor)
STM32 ↔ I2C ↔ SSD1306 (OLED)

STM32 → LEDs (status)
STM32 ↔ SWD (program/debug)


---

## Parts

### Microcontroller (MCU)

**STM32L432KCT6**
- Cortex-M4 @ 80 MHz
- 256 KB Flash / 64 KB RAM
- Operates at 1.7–3.6 V
- LQFP-48 package

📷 Image:  
https://www.st.com/resource/en/package_outline/lqfp48.png  

---

### Temperature Sensor

**MCP9808 (Bare IC - 8-MSOP)**

- Digital I2C temperature sensor
- High accuracy (±0.25°C)
- High resolution (0.0625°C)
- Operates at 3.3V

Design choices:
- Using **bare IC (not breakout)**
- Placed **at PCB edge** for accurate air measurement
- Includes **0.1 µF decoupling capacitor**
- Address pins (A0, A1, A2) tied to GND
- ALERT pin not used

📷 Image:  
https://ww1.microchip.com/downloads/en/DeviceDoc/25095A.pdf  

---

### OLED Display

**SSD1306 OLED (0.96", 128×64, I2C)**

- Rectangular display
- Good resolution for text
- Very common and well-supported
- Uses I2C (shares bus with sensor)

Design choices:
- Using **module (not bare display)**
- Connected via **1×4 header (2.54 mm)**

Pinout:
1 - GND
2 - 3.3V
3 - SCL
4 - SDA


📷 Image:  
https://cdn-shop.adafruit.com/970x728/326-00.jpg  

---

### LEDs

- **Green LED** → normal temperature  
- **Red LED** → high temperature  
- **Power LED** → board powered  

Design:
- SMD 0805 LEDs
- Each LED has a **330 Ω resistor**
- Status LEDs controlled by STM32 GPIO

📷 Image:  
https://cdn.sparkfun.com//assets/parts/1/0/8/9/9/15193-LED_Red_0805.jpg  

---

### Programming & Debugging

**5-pin SWD Header (1×5, 2.54 mm)**

Pinout:
1 - 3.3V
2 - SWDIO
3 - GND
4 - SWCLK
5 - NRST


- Standard STM32 debug interface
- Reliable and simple

📷 Image:  
https://upload.wikimedia.org/wikipedia/commons/thumb/1/1f/Pin_header_2.54mm.jpg/640px-Pin_header_2.54mm.jpg  

---

### Power System

#### Input

**Micro-USB (Type-B)**  
- Used for **power only**
- Provides 5V

Connections:
VBUS → 5V rail
GND → GND
D+ / D- → NC


📷 Image:  
https://cdn.sparkfun.com//assets/parts/1/1/4/7/4/14417-Micro_USB_B_Connector.jpg  

---

#### Voltage Regulator

**AP2112K-3.3TRG1 (LDO)**

- Output: 3.3V
- Max current: 600 mA

Capacitors:
CIN = 1 µF (input)
COUT = 1 µF (output)


📷 Image:  
https://www.diodes.com/assets/Package-Images/SOT25.png  

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

## Supporting Components

### STM32 Decoupling
Each VDD → 0.1 µF → GND
VDDA → 0.1 µF → GND
3.3V rail → 4.7 µF bulk capacitor


---

### I2C Bus

Shared between:
- MCP9808
- SSD1306

Pull-ups:
SDA → 4.7 kΩ → 3.3V
SCL → 4.7 kΩ → 3.3V


---

### Reset Circuit
NRST → pushbutton → GND


---

### Boot Configuration
BOOT0 → 10 kΩ → GND


---

### Test Points

Added for debugging:
3.3V
GND
SDA
SCL


---

## PCB Design Choices

- 2-layer PCB
- Top layer: components + signals
- Bottom layer: ground plane
- Board size: ~50 mm × 40 mm
- Shape: rectangular
- 4 × M3 mounting holes

---

## Notes

- STM32 internal clock is used (no external crystal)
- OLED and sensor share I2C bus
- Sensor placement is critical for accurate readings
- Designed for simplicity and first-pass success
