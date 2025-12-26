/**
 * @file sensor.c
 * @brief fournir une température en °C à partir du module LM75 en I2C
 * @copyright
 * © 2025 Syloria — MIT
 */

#include <sensor_temp.h>
#include "stm32f4xx_hal.h"

extern I2C_HandleTypeDef hi2c1; /*hi2c1 est généré par CubeMX*/

#define SENSOR_I2C_TRIALS           (3U)
#define SENSOR_I2C_TIMEOUT_MS       (10U)
#define SENSOR_LM75_TEMP_BYTES      (2U)
#define SENSOR_LM75_RAW_MASK        (0xFF80U)  /* bits [15:7] utiles (9-bit signe) */
#define SENSOR_LM75_DIV_128         (128)      /* evite shift sur signe : /128 == >>7 */

static uint8_t sensor_addr = SENSOR_LM75_DEFAULT_ADDR;

static uint16_t Sensor_ToHalAddress(uint8_t addr_7bit)
{
    /* HAL attend généralement une adresse alignée à gauche (7-bit << 1) */
    return (uint16_t)((uint16_t)addr_7bit << 1U);
}

SensorStatus_t Sensor_Init(uint8_t addr)
{
	SensorStatus_t sensor_status = SENSOR_OK;
	HAL_StatusTypeDef hal_status;	/* Recuperation status I2C */



	/* Verification adresse est 7 bits) */
	if ((addr < SENSOR_LM75_MIN_ADDR) || (addr > SENSOR_LM75_MAX_ADDR))
	{

		sensor_status = SENSOR_ERR_PARAM;
	}

	else
	{
		uint16_t addr_hal;

		/* Test de connexion I2C */
		addr_hal = Sensor_ToHalAddress(addr);
		hal_status = HAL_I2C_IsDeviceReady(&hi2c1, addr_hal, SENSOR_I2C_TRIALS, SENSOR_I2C_TIMEOUT_MS);

		if (hal_status == HAL_OK)
		{
			sensor_addr = addr;
		}
		else if (hal_status == HAL_TIMEOUT)
		{
			sensor_status = SENSOR_ERR_TIMEOUT;
		}
		else
		{
			sensor_status = SENSOR_ERR_I2C;
		}

	}

	return sensor_status;
}

SensorStatus_t Sensor_ReadTemperature(float * out_celsius)
{
    SensorStatus_t sensor_status = SENSOR_OK;
    HAL_StatusTypeDef hal_status;


	if (out_celsius == NULL)
	{
		sensor_status = SENSOR_ERR_PARAM;
	}
	else
	{
	    uint8_t rx_buf[SENSOR_LM75_TEMP_BYTES];
	    uint16_t addr_hal;
	    uint16_t raw_u16;
	    int16_t raw_s16;
	    int16_t code;

		/* envoyer une trame demande temperature */
		addr_hal = Sensor_ToHalAddress(sensor_addr);
		hal_status = HAL_I2C_Mem_Read(&hi2c1, addr_hal, SENSOR_LM75_REG_TEMP, I2C_MEMADD_SIZE_8BIT, &rx_buf[0], (uint16_t)SENSOR_LM75_TEMP_BYTES, SENSOR_I2C_TIMEOUT_MS);

		if (hal_status == HAL_OK)
		{

			 /* Assemble MSB/LSB */
			raw_u16 = (uint16_t)(((uint16_t)rx_buf[0] << 8U) | (uint16_t)rx_buf[1]);

			/* Garder uniquement bits [15:7] */
			raw_u16 = (uint16_t)(raw_u16 & (uint16_t)SENSOR_LM75_RAW_MASK);

			/* Interprete comme signe (temperature negative OK) */
			raw_s16 = (int16_t)raw_u16;

			/* /128 == >>7, evite un shift sur type signe (MISRA-friendly) */
			code = (int16_t)(raw_s16 / (int16_t)SENSOR_LM75_DIV_128);

			/* LSB = 0.5 degC (LM75 datasheet) */
			*out_celsius = ((float)code) * 0.5F;

		}
		else if (hal_status == HAL_TIMEOUT)
		{
			sensor_status = SENSOR_ERR_TIMEOUT;
		}
		else
		{
			sensor_status = SENSOR_ERR_I2C;
		}


	}

	return sensor_status;
}

SensorStatus_t  Sensor_SetAddress(uint8_t new_addr)
{
	return Sensor_Init(new_addr);
}



