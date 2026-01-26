#include "sensor_temp.h"
#include "stm32f4xx_hal.h"

extern I2C_HandleTypeDef hi2c1;

#define SENSOR_I2C_TRIALS            (3U)
#define SENSOR_I2C_TIMEOUT_MS        (10U)

#define SENSOR_TEMP_BYTES            (2U)

/* MCP9808 TA format */
#define MCP9808_FLAG_MASK            (0xE0U)   /* bits 15..13 */
#define MCP9808_TEMP_MSB_MASK        (0x1FU)   /* bits 12..8 without flags */
#define MCP9808_SIGN_BIT             (0x10U)   /* bit 12 in msb after masking flags */
#define MCP9808_NEG_MSB_MASK         (0x0FU)

static uint8_t  sensor_addr = SENSOR_MCP9808_ADDR_MIN;
static int16_t  last_temp_x10 = 0;
static SensorStatus_t last_status = SENSOR_ERR_INIT;

static uint16_t Sensor_ToHalAddress(uint8_t addr_7bit)
{
    return (uint16_t)((uint16_t)addr_7bit << 1U);
}

SensorStatus_t Sensor_Init(uint8_t addr_7bit)
{
    SensorStatus_t status = SENSOR_OK;
    HAL_StatusTypeDef hal_status;

    last_status = SENSOR_ERR_INIT;
    last_temp_x10 = 0;

    if ((addr_7bit < SENSOR_MCP9808_ADDR_MIN) || (addr_7bit > SENSOR_MCP9808_ADDR_MAX))
    {
        status = SENSOR_ERR_PARAM;
    }
    else
    {
        const uint16_t addr_hal = Sensor_ToHalAddress(addr_7bit);

        hal_status = HAL_I2C_IsDeviceReady(&hi2c1,
                                           addr_hal,
                                           SENSOR_I2C_TRIALS,
                                           (uint32_t)SENSOR_I2C_TIMEOUT_MS);

        if (hal_status == HAL_OK)
        {
            sensor_addr = addr_7bit;
            status = SENSOR_OK;
        }
        else if (hal_status == HAL_TIMEOUT)
        {
            status = SENSOR_ERR_TIMEOUT;
        }
        else
        {
            status = SENSOR_ERR_I2C;
        }
    }

    last_status = status;
    return status;
}

SensorStatus_t Sensor_ReadTemperature(float * out_celsius)
{
    SensorStatus_t status = SENSOR_OK;
    HAL_StatusTypeDef hal_status;

    if (out_celsius == NULL)
    {
        status = SENSOR_ERR_PARAM;
    }
    else
    {
        uint8_t rx[SENSOR_TEMP_BYTES];
        const uint16_t addr_hal = Sensor_ToHalAddress(sensor_addr);

        hal_status = HAL_I2C_Mem_Read(&hi2c1,
                                      addr_hal,
                                      (uint16_t)SENSOR_MCP9808_REG_TA,
                                      I2C_MEMADD_SIZE_8BIT,
                                      &rx[0],
                                      (uint16_t)SENSOR_TEMP_BYTES,
                                      (uint32_t)SENSOR_I2C_TIMEOUT_MS);

        if (hal_status == HAL_OK)
        {
            /* MCP9808 TA register is 16-bit: [15:13]=flags, [12]=sign, [11:0]=temp*16 */
            const uint16_t raw16 = (uint16_t)(((uint16_t)rx[0] << 8U) | (uint16_t)rx[1]);
            const uint16_t ta16  = (uint16_t)(raw16 & 0x1FFFU);      /* clear flags */
            const uint16_t mag16 = (uint16_t)(ta16 & 0x0FFFU);       /* magnitude (temp*16) */
            float temp_c;

            if ((ta16 & 0x1000U) != 0U)
            {
                /* Negative temperature: Temp = (mag/16) - 256 */
                temp_c = ((float)mag16 / 16.0F) - 256.0F;
            }
            else
            {
                /* Positive temperature */
                temp_c = (float)mag16 / 16.0F;
            }

            *out_celsius = temp_c;

            /* Cache x10 (arrondi simple) */
            if (temp_c >= 0.0F)
            {
                last_temp_x10 = (int16_t)((temp_c * 10.0F) + 0.5F);
            }
            else
            {
                last_temp_x10 = (int16_t)((temp_c * 10.0F) - 0.5F);
            }

            status = SENSOR_OK;
        }
        else if (hal_status == HAL_TIMEOUT)
        {
            status = SENSOR_ERR_TIMEOUT;
        }
        else
        {
            status = SENSOR_ERR_I2C;
        }
    }

    last_status = status;
    return status;
}


SensorStatus_t Sensor_SetAddress(uint8_t addr_7bit)
{
    return Sensor_Init(addr_7bit);
}

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
