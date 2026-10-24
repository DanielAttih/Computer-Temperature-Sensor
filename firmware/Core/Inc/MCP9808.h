/*
 * MCP9808.h - Driver header for the MCP9808 I2C temperature sensor
 *
 *  Created on: May 10, 2026
 *      Author: Daniel Attih
 */

#ifndef INC_MCP9808_H_
#define INC_MCP9808_H_

#include "main.h"
#include <stdbool.h>

/* ---------------------------------------------------------------------------
 * Constants (from the MCP9808 datasheet)
 * ------------------------------------------------------------------------ */

/* Device address. The sensor's 7-bit address is 0x18 when the A0/A1/A2 pins
 * are tied to GND. HAL wants the address shifted left by 1 bit (the lowest
 * bit is reserved for read/write), hence (0x18 << 1) = 0x30.               */
#define MCP9808_I2C_ADDR 	(0x18 << 1)

/* Register addresses = "apartment numbers" inside the chip.                */
#define MCP9808_REG_ADDR 	0x05   /* Ambient temperature register          */
#define MCP9808_REG_MANUF 	0x06   /* Manufacturer ID register              */

/* The value the manufacturer ID register must contain. Reading 0x0054
 * proves we are really talking to an MCP9808 (and that wiring is good).    */
#define MCP9808_MANUF_ID 	0x0054

/* ---------------------------------------------------------------------------
 * Public functions (implemented in MCP9808.c)
 * ------------------------------------------------------------------------ */

/* Checks the sensor is present and is really an MCP9808.
 * hi2c : which I2C bus the sensor is on (e.g. &hi2c3)
 * Returns true if OK, false if no response or wrong ID.                    */
bool MCP9808_Init(I2C_HandleTypeDef *hi2c);

/* Reads the temperature in degrees Celsius.
 * hi2c  : which I2C bus the sensor is on
 * tempC : pointer to a float the result is written into (Celsius)
 * tempF : pointer to the float the result is written into (Fahrenheit)
 * Returns true if the read worked, false if it failed (tempC and tempF are then
 * left untouched).
 * NOTE: the name still says "C" for historical reasons; it returns both. */
bool MCP9808_ReadTempC(I2C_HandleTypeDef *hi2c, float *tempC, float *tempF);

#endif /* INC_MCP9808_H_ */

