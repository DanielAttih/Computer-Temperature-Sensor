# Computer-Temperature-Sensor
Reads the internal temperature of my PC and displays the value on an OLED. Includes 2 LEDs as visual feedback.
https://chatgpt.com/c/69e398d1-85f8-83ea-8de9-20dd3073235b 

## Steps
1. The sensor measures the air inside the PC.
2. The board gets power through USB from the PC. USB provides 5V, and the board converts it to 3.3V for the sensor.

## Parts
### CHIP
The chip I will be using is the STM32L432KCT6
- Cortex-M4 @ 80MHz
- 256 KB Flash / 64 KB RAM
- Low power. Runs at 1.7–3.6 V.
- LQFP-48
  
### Digital Temp
I decided to go with a digital sensor rather than an analog one because of noise.
MCP9808 temperature sensor. https://www.adafruit.com/product/1782
<img width="1148" height="854" alt="image" src="https://github.com/user-attachments/assets/fcaa3141-4b07-43a9-88f2-0f14c3897ff3" />
- High accuracy (±0.25°C)
- High resolution (0.0625°C)
- Simple I2C interface
I will be using the bare-metal chip, which will be soldered on the PCB away from heat sources, and also adding the coupling capacitors.

### OLED Display
Decided to go with an OLED rather than an LCD due to the wires. The OLED requires significantly fewer wires compared to the LCD. Also, the OLED and temp sensor can share the same I2C bus, which reduces the wires to the CPU.
SSD1306 0.96" Rectangular (128×64)
<img width="596" height="581" alt="image" src="https://github.com/user-attachments/assets/6f6ccc4a-5112-4401-b515-0d0702caa1e3" />
- Easy to mount
- Good resolution for text
- Most common (libraries use this)
We will use the whole module, and it will be connected with a 4-pin header, which makes it easier to mount and replace.

### LED
The LEDs are soldered directly onto the PCB. Green and Red status.

### Program and Debug
I will be using the standard STM32 programming/debug method, which is a 5-pin SWD Programming header.
- Simpler than USB programming
- Reliable
- very useful if the board does not work on the first power-up

### Voltage Regulator
 A simple 3.3V LDO regulator.

 ### Power input
 Micro-USB
 - Simpler
 - Common
 - Older








