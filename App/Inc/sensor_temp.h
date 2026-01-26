#ifndef SENSOR_TEMP_H
#define SENSOR_TEMP_H

#include <stdint.h>

#define SENSOR_MCP9808_ADDR_MIN      (0x18U)
#define SENSOR_MCP9808_ADDR_MAX      (0x1FU)   /* A2/A1/A0 -> 8 adresses */

#define SENSOR_MCP9808_REG_TA        (0x05U)   /* Ambient Temperature (TA) */

typedef enum
{
    SENSOR_OK = 0,
    SENSOR_ERR_INIT,
    SENSOR_ERR_PARAM,
    SENSOR_ERR_I2C,
    SENSOR_ERR_TIMEOUT
} SensorStatus_t;

SensorStatus_t Sensor_Init(uint8_t addr_7bit);
SensorStatus_t Sensor_ReadTemperature(float * out_celsius);
SensorStatus_t Sensor_SetAddress(uint8_t addr_7bit);
SensorStatus_t Sensor_GetLastTemperature_x10(int16_t * out_temp_x10);

#endif /* SENSOR_TEMP_H */
