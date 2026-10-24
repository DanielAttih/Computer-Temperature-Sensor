/*
 * MCP9808.c -  Driver implementation for the MCP9808 I2C temperature sensor
 *
 *  Created on: May 10, 2026
 *      Author: Daniel Attih
 */

#include "MCP9808.h"

/* ---------------------------------------------------------------------------
 * read_reg16 (private helper)
 *
 * Reads one 16-bit register from the sensor.
 *
 * "static" means only this file can call it; it is not part of the public
 * interface in the header.
 *
 * Parameters:
 *   hi2c : the I2C bus handle (e.g. &hi2c3)
 *   reg  : register address inside the chip (e.g. 0x05)
 *   out  : pointer to where the 16-bit result is stored
 *
 * Returns true on success, false if the I2C transaction failed.
 *
 * This replaces the HAL_I2C_Mem_Read + "(buf[0] << 8) | buf[1]" lines that
 * used to be repeated in main.c.
 * ------------------------------------------------------------------------ */
static bool read_reg16(I2C_HandleTypeDef *hi2c, uint8_t reg, uint16_t *out)
{
	uint8_t buf[2];   /* Raw bytes from the sensor: buf[0] = high byte,
                         buf[1] = low byte (MSB first)                     */

    /* HAL_I2C_Mem_Read arguments, in order:
     *   bus, device address, register address, register-address size,
     *   buffer to fill, number of bytes, timeout in ms.
     * It returns HAL_OK (which is 0) on success.                           */
	if (HAL_I2C_Mem_Read(hi2c, MCP9808_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buf, 2, 100) != HAL_OK)
	{
		return false;	/* Sensor didn't answer or bus error */
	}

    /* Combine the two bytes into one 16-bit number.
     * Example: buf[0]=0x00, buf[1]=0x54  ->  0x0054                        */
	*out = (buf[0] << 8) | buf[1];
	return true;
}

/* ---------------------------------------------------------------------------
 * MCP9808_Init
 *
 * Confirms the sensor is connected and is really an MCP9808 by reading its
 * manufacturer ID register and comparing it to the expected value.
 *
 * ------------------------------------------------------------------------ */
bool MCP9808_Init(I2C_HandleTypeDef *hi2c)
{
	uint16_t id;	/* Will hold the manufacturer ID read from the chip */

	/* If the read itself fails (wiring, wrong bus, no power), give up. */
	if (!read_reg16(hi2c, MCP9808_REG_MANUF, &id))
	{
		return false;
	}

	/* The comparison yields true only if the ID is 0x0054. */
	return (id == MCP9808_MANUF_ID);
}

/* ---------------------------------------------------------------------------
 * MCP9808_ReadTempC
 *
 * Reads the ambient temperature register and converts it to degrees C.
 *
 * Parameters:
 *   hi2c  : I2C bus handle
 *   tempC : pointer to a float that receives the result. We pass a pointer
 *           because the function's return value is already used for
 *           true/false success. The pointer lets us hand back a second
 *           piece of information (the temperature).
 * ------------------------------------------------------------------------ */
bool MCP9808_ReadTempC(I2C_HandleTypeDef *hi2c, float *tempC, float *tempF)
{
    uint16_t reg;   /* The raw 16-bit value of the temperature register */

    if (!read_reg16(hi2c, MCP9808_REG_ADDR, &reg)) {
        return false;	/* Read failed; leave *tempC unchanged */
    }

    /* The register layout is:
     *   bits 15..13 : alert flags (not temperature, so we throw them away)
     *   bit  12     : sign (1 = negative)
     *   bits 11..0  : temperature magnitude
     * Masking with 0x1FFF keeps only the lower 13 bits.				*/
    int16_t raw = reg & 0x1FFF;

    /* The 13-bit value is two's complement. If bit 12 is set the number is
     * negative, so subtract 2^13 (0x2000 = 8192) to turn it into a proper
     * negative int16_t ("sign extension").  							*/
    if (raw & 0x1000) raw -= 0x2000;

    /* Each count = 1/16 degree C = 0.0625 C (datasheet resolution).
     * The 'f' makes it a float constant, avoiding slow double math.        */
    *tempC = raw * 0.0625f;	/* "*tempC =" writes through the pointer */

    /* Celsius to Fahrenheit: F = C * 9/5 + 32.
     * Uses the freshly written Celsius value (*tempC reads it back).       */
	*tempF = (*tempC * 9.0f / 5.0f) + 32.0f;

    return true;
}
