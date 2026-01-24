/**
 * @file sensor_temp.c
 * @brief Provide temperature in °C from an LM75 sensor over I2C.
 * @copyright
 * © 2025 SYLORIA — MIT License — BAQUEY Lucas (contact@syloria.fr)
 */

#include "sensor_temp.h"
#include "stm32f4xx_hal.h"

/* CubeMX generated handle */
extern I2C_HandleTypeDef hi2c1;

/* --- Define --- */
#define SENSOR_I2C_TRIALS           (3U)
#define SENSOR_I2C_TIMEOUT_MS       (10U)

#define SENSOR_LM75_TEMP_BYTES      (2U)
#define SENSOR_LM75_RAW_MASK        (0xFF80U)   /* Keep bits [15:7] */
#define SENSOR_LM75_DIV_128         (128U)      /* /128 == >>7 (avoid signed shift) */

/* --- Prototypes --- */
static uint8_t	sensor_addr	= SENSOR_LM75_DEFAULT_ADDR;
static int16_t	last_temp_x10 = 0;              /* Last temperature *10 (e.g., 235 => 23.5°C) */
static SensorStatus_t last_status	= SENSOR_ERR_INIT;/* Last sensor operation status */

/* --- Statics functions --- */
static uint16_t Sensor_ToHalAddress(uint8_t addr_7bit)
{
    /* HAL expects 7-bit address left-aligned (addr << 1) */
    return (uint16_t)((uint16_t)addr_7bit << 1U);
}

/* --- Public API --- */
SensorStatus_t Sensor_Init(uint8_t addr)
{
    SensorStatus_t    sensor_status = SENSOR_OK;
    HAL_StatusTypeDef hal_status    = HAL_OK;

    /* Reset last known values on init */
    last_status   = SENSOR_ERR_INIT;
    last_temp_x10 = 0;

    /* Validate LM75 7-bit address range */
    if ((addr < SENSOR_LM75_MIN_ADDR) || (addr > SENSOR_LM75_MAX_ADDR))
    {
        sensor_status = SENSOR_ERR_PARAM;
    }
    else
    {
        uint16_t addr_hal = Sensor_ToHalAddress(addr);

        /* I2C presence check (ACK) */
        hal_status = HAL_I2C_IsDeviceReady(&hi2c1,
                                           addr_hal,
                                           SENSOR_I2C_TRIALS,
                                           (uint32_t)SENSOR_I2C_TIMEOUT_MS);

        if (hal_status == HAL_OK)
        {
            sensor_addr = addr;
            sensor_status = SENSOR_OK;
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

    /* Store init result as last status */
    last_status = sensor_status;

    return sensor_status;
}

SensorStatus_t Sensor_ReadTemperature(float * out_celsius)
{
    SensorStatus_t    sensor_status = SENSOR_OK;
    HAL_StatusTypeDef hal_status    = HAL_OK;

    if (out_celsius == NULL)
    {
        sensor_status = SENSOR_ERR_PARAM;
    }
    else
    {
        uint8_t  rx_buf[SENSOR_LM75_TEMP_BYTES];
        uint16_t addr_hal;
        uint16_t raw_u16;
        int16_t  raw_s16;
        int16_t  code;

        addr_hal = Sensor_ToHalAddress(sensor_addr);

        /* Read LM75 temperature register (2 bytes) */
        hal_status = HAL_I2C_Mem_Read(&hi2c1,
                                      addr_hal,
                                      (uint16_t)SENSOR_LM75_REG_TEMP,
                                      I2C_MEMADD_SIZE_8BIT,
                                      &rx_buf[0],
                                      (uint16_t)SENSOR_LM75_TEMP_BYTES,
                                      (uint32_t)SENSOR_I2C_TIMEOUT_MS);

        if (hal_status == HAL_OK)
        {
            /* Assemble MSB/LSB */
            raw_u16 = (uint16_t)(((uint16_t)rx_buf[0] << 8U) | (uint16_t)rx_buf[1]);

            /* Keep only meaningful bits [15:7] */
            raw_u16 = (uint16_t)(raw_u16 & (uint16_t)SENSOR_LM75_RAW_MASK);

            /* Interpret as signed value (negative temperatures supported) */
            raw_s16 = (int16_t)raw_u16;

            /* /128 == >>7; division avoids signed shift concerns */
            code = (int16_t)(raw_s16 / (int16_t)SENSOR_LM75_DIV_128);

            /* LM75: LSB = 0.5°C */
            *out_celsius = ((float)code) * 0.5F;

            /* Cache last value as x10 without float: (code * 0.5°C) * 10 = code * 5 */
            last_temp_x10 = (int16_t)(code * 5);
            sensor_status = SENSOR_OK;
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

    /* Store last operation status */
    last_status = sensor_status;

    return sensor_status;
}

SensorStatus_t Sensor_SetAddress(uint8_t new_addr)
{
    /* Reuse init to validate and update address */
    return Sensor_Init(new_addr);
}

/* Get the last temperature value*10 */
SensorStatus_t Sensor_GetLastTemperature_x10(int16_t * out_temp_x10)
{
    SensorStatus_t status = last_status;

    if (out_temp_x10 == NULL)
    {
        status = SENSOR_ERR_PARAM;
    }
    else
    {
        *out_temp_x10 = last_temp_x10;
    }

    return status;
}
