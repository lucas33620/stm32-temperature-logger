/**
 * @file sensor_temp.h
 * @brief fournir une température en °C à partir du module LM75 en I2C
 * @copyright
 * © 2025 SYLORIA — MIT License — BAQUEY Lucas (contact@syloria.fr)
 */

#ifndef SENSOR_TEMP_H
#define SENSOR_TEMP_H

#include <stdint.h>

/* Datasheet LM75 */
#define SENSOR_LM75_MIN_ADDR         (0x48U)
#define SENSOR_LM75_DEFAULT_ADDR     (0x48U) /* LM75 I2C address is 7-bit */
#define SENSOR_LM75_MAX_ADDR         (0x4FU)
#define SENSOR_LM75_REG_TEMP         (0x00U)

typedef enum
{
    SENSOR_OK = 0,
    SENSOR_ERR_INIT,
    SENSOR_ERR_PARAM,
    SENSOR_ERR_I2C,
    SENSOR_ERR_TIMEOUT
} SensorStatus_t;

SensorStatus_t Sensor_Init(uint8_t addr);
SensorStatus_t Sensor_ReadTemperature(float * out_celsius);
SensorStatus_t Sensor_SetAddress(uint8_t addr);
SensorStatus_t Sensor_GetLastTemperature_x10(int16_t * out_temp_x10); /* Getter: last temperature *10 (e.g. 235 => 23.5°C) */

#endif /* SENSOR_TEMP_H */
