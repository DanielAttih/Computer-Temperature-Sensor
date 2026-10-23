# PC Case Temp Monitor: firmware notes (v2, 2026-10-22)

Board: Nucleo-L432KC (STM32L432KC), CubeMX/HAL firmware.
Status: firmware complete on the dev board, all fault paths tested. Next phase: schematic.

## 1. What the firmware does
1. Boots, prints `~ START ~` over UART (PuTTY, 115200).
2. Initializes the MCP9808 sensor. If it fails, retries forever (red LED flashes after 3 failures).
3. Initializes the SSD1306 OLED, 3 attempts. If it fails, continues without a display (blue LED flashes).
4. Every 500 ms: reads the temperature, shows C and F on the OLED, prints to UART, and sets the red/green LEDs.
5. If the sensor stops answering mid-run: after 3 failed reads in a row, shows SENSOR ERR and flashes red. Recovers on its own.

## 2. Pin map (verify against the .ioc)

| Function | Pin | Notes |
|---|---|---|
| OLED SDA / SCL | PB7 / PB6 | I2C1. The OLED driver is hardwired to `hi2c1`. |
| MCP9808 SDA / SCL | PB4 / PA7 | I2C3 |
| UART TX (debug) | PA2 | USART2, goes to the ST-LINK virtual COM port, shows in PuTTY |
| Green LED | PA3 | output |
| Red LED | PA4 | output |
| Blue LED | PA1 | output. NOT PA5, see section 7 |
| Board LED LD3 | PB3 | used for the very first blink test only |

Addresses: MCP9808 = 0x18 (0x30 shifted for HAL), OLED = 0x3C (0x78 shifted).

## 3. Files

| File | Role |
|---|---|
| `MCP9808.h` / `MCP9808.c` | Sensor driver (my code) |
| `ssd1306.h` / `ssd1306.c` / `fonts.h` | OLED driver (from my embedded systems course) |
| `main.c` | Application logic: init, main loop, LEDs, fault handling |

## 4. MCP9808 driver

### Constants
| Name | Value | Meaning |
|---|---|---|
| `MCP9808_I2C_ADDR` | `0x18 << 1` | Device address (which chip) |
| `MCP9808_REG_TEMP` | `0x05` | Register address (which register inside the chip): temperature |
| `MCP9808_REG_MANUF` | `0x06` | Register address: manufacturer ID |
| `MCP9808_MANUF_ID` | `0x0054` | Expected ID value |

Device address = which chip on the bus. Register address = which register inside it.

### Functions
- `static bool read_reg16(hi2c, reg, *out)`: private helper. Reads 2 bytes from a register and combines them (high byte first). Returns true if `HAL_I2C_Mem_Read` returned `HAL_OK`.
- `bool MCP9808_Init(hi2c)`: reads the manufacturer ID and checks it equals 0x0054. True = sensor present and genuine.
- `bool MCP9808_ReadTempC(hi2c, *tempC, *tempF)`: reads register 0x05, outputs Celsius and Fahrenheit through the two pointers. True = success. The name is historical, it returns both units.

### Temperature conversion (inside ReadTempC)
1. `raw = reg & 0x1FFF`: drop the 3 alert-flag bits, keep 13 bits.
2. `if (raw & 0x1000) raw -= 0x2000;`: bit 12 is the sign, so sign-extend the 13-bit value.
3. `*tempC = raw * 0.0625f;`: each count is 1/16 degree C.
4. `*tempF = (*tempC * 9.0f / 5.0f) + 32.0f;`

### Why pointers for the outputs
A function can only `return` one value, and the return value is used for success/failure. The extra results come back through pointers: pass `&tempC` and the function writes into your variable with `*tempC = ...`.

### What changed since v1
- Added the `float *tempF` argument and the Fahrenheit line (v2).
- Everything else is the same as the first driver version.

## 5. main.c structure

### Defines and variables
```c
#define TEMP_HOT_F      90.0f   /* red ON at or above this (use 90 for real, 77 was only for testing) */
#define TEMP_OK_F       85.0f   /* red OFF only below this: the gap is the hysteresis */
#define MAX_READ_FAILS  3       /* consecutive failed reads before declaring a sensor fault */

bool    oledOK      = false;    /* did the OLED initialize?                        */
bool    tooHot      = false;    /* current hysteresis state (remembered each loop) */
uint8_t failCount   = 0;        /* consecutive failed reads so far                 */
bool    sensorFault = false;    /* true while the sensor-fault screen is showing   */
char    buf_C[16], buf_F[16];   /* text for the OLED                               */
```

### Startup (USER CODE BEGIN 2)
- **Sensor:** `while (!MCP9808_Init(&hi2c3))` loops until the sensor answers. From the 3rd failure the red LED toggles each pass. On success the red LED is forced off.
- **OLED:** a `for` loop tries `SSD1306_Init()` up to 3 times. On success it draws the title and the "Temp: " label. On failure it sets `oledOK = false` and the program continues.

### Main loop (every 500 ms)
1. Blue LED: solid if `oledOK`, toggling (flashing) if not.
2. Read temperature.
3. **On success:** reset `failCount`; if recovering from a fault, clear the flag and redraw the OLED labels; draw C and F on the OLED (only if `oledOK`); print to UART; run the hysteresis and set the red/green LEDs.
4. **On failure:** print the failure; reset I2C3 (`HAL_I2C_DeInit` then `MX_I2C3_Init`); increment `failCount` (capped at 3); at 3, show SENSOR ERR, turn green off, flash red.
5. If `oledOK`, `SSD1306_UpdateScreen()`.

### Hysteresis (red/green)
```c
if (!tooHot && tempF >= TEMP_HOT_F)      tooHot = true;
else if (tooHot && tempF < TEMP_OK_F)    tooHot = false;
```
Between 85 and 90 F, the LEDs keep their previous state, so the red LED doesn't flicker when the temperature hovers near 90. Then:
```c
HAL_GPIO_WritePin(RED_LED_GPIO_Port,   RED_LED_Pin,   tooHot ? GPIO_PIN_SET   : GPIO_PIN_RESET);
HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, tooHot ? GPIO_PIN_RESET : GPIO_PIN_SET);
```
`condition ? a : b` is a one-line if/else.

### Helper
`static void oled_draw_static(void)` clears the OLED buffer and redraws "PC Case Temp" and "Temp: ". Used at startup and after recovering from a sensor fault.

## 6. Fault handling summary

| Situation | What happens |
|---|---|
| Sensor missing at boot | Retries forever. Red LED flashes from the 3rd failure. Recovers if reconnected. |
| OLED missing at boot | 3 attempts, then continues without display. Blue LED flashes. UART and LEDs still work. |
| Sensor lost mid-run | Reads 1-2 failing: UART message only. 3 in a row: green off, red flashing, OLED shows SENSOR ERR. |
| Sensor reconnected | First good read clears the fault and normal display and LEDs resume. No reset needed. |

Why the I2C3 re-init matters: pulling the wire mid-transaction left the I2C peripheral stuck in an error state, so it never recovered after replugging. Resetting the peripheral on each failed read fixed it.

## 7. Lessons learned (read these before the PCB)

- **PA5 and PA6 on the Nucleo-32:** solder bridges SB16 and SB18 tie PA5 to PB7 (I2C1 SDA) and PA6 to PB6 (I2C1 SCL). A blue LED on PA5 held the OLED's SDA line and made OLED init fail. Fix: use other pins on the dev board. The custom PCB has no such bridges, so this is a Nucleo-only issue.
- **UART for PuTTY:** the ST-LINK virtual COM port is USART2 on PA2, not USART1 on PA9/PA10.
- **Clocks (changed 2026-10-22):** LSE and HSE are both disabled, so no external crystals are needed. The MCU runs from the internal MSI (4 MHz) through the PLL (x16, /1, /2) at 32 MHz. MSI is no longer auto-calibrated against the 32 kHz crystal, so it is accurate to about 1% instead of about 0.25%, which does not matter for UART, I2C or the 500 ms loop. PA0, PC14 and PC15 are free pins. `Error_Handler()` can no longer hang on an oscillator that fails to start.
- **`HAL_OK` is 0, not true.** Compare with `== HAL_OK`, never `if (HAL_...())`.
- **Function calls need to be inside a function.** A call at file scope gives "initializer is not constant".
- **`%f` in printf** needs "Use float with printf from newlib-nano" enabled in the project settings.
- **Interrupt mode (`GPIO_MODE_IT_*`) is for inputs.** LEDs are plain `GPIO_Output`. The temperature is a computed value, not a pin signal.
- **Debug prints and LED logic stay outside `if (oledOK)`.** The display must never decide whether monitoring works.
- **OLED text:** draw once, then only overwrite the numbers using fixed-width formatting (`%6.2f`), and never `SSD1306_Fill` inside the loop unless you redraw the fixed text.

## 8. Test checklist
1. Normal: C and F on the OLED, UART matches, blue solid.
2. Heat the sensor past the hot threshold: red on, green off. Cool down: green returns.
3. OLED unplugged at reset: blue flashes, UART still prints, LEDs still work.
4. Sensor SDA unplugged at reset: red flashes. Replug: recovers.
5. Sensor SDA pulled mid-run: after about 2 s, red flashing and SENSOR ERR. Replug: recovers, labels redraw.

## 9. For the schematic phase
- I2C buses are separate: sensor on I2C3, OLED on I2C1. Each needs pull-ups (check the breakout boards had them and whether the PCB needs its own).
- LEDs are driven directly from MCU pins through series resistors (values in the project documentation).
- Debug UART: add a 2-pin header (PA2 TX + GND, transmit only) so a 3.3V USB-serial adapter can show the debug text in a terminal. The USB connector is power-only, so the text cannot go over that cable.
- Flashing and debugging: use an external ST-LINK probe on the 5-pin SWD header (3.3V, SWDIO, GND, SWCLK, NRST). Using the Nucleo-32's own ST-LINK for an external board was not confirmed.
- Avoid carrying the Nucleo's PA5/PA6 problem over: it does not exist on the custom board.
