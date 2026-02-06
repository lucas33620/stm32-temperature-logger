/**
 * @file    sensor_temp.c
 * @brief   Sensor management module for MCP9808.
 * @company Syloria
 * @author  Lucas Baquey
 * * @copyright MIT License
 * * Summary: Permission is granted, free of charge, to any person obtaining a copy
 * of this software to deal in the Software without restriction, including the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies.
 * The software is provided "as is", without warranty of any kind.
 */

/** @section Include */
#include "app_mcp9808.h"
#include "stm32f4xx_hal.h"

extern I2C_HandleTypeDef hi2c1;

/** @section Define*/
#define SENSOR_TEMP_BYTES            (2U)   
#define SENSOR_INVALID_TEMP_X10     (INT16_MIN)

/** @section Typedef */

/**
 * @brief Internal states of the sensor module (finite state machine).
 *
 * * @note Safety Objective: Precisely control actions at each tick.
 */
typedef enum
{
    SENSOR_STATE_UNINIT = 0, /* Uninitialized: Limited API, no I2C access allowed */
    SENSOR_STATE_IDLE,       /* Idle: Waiting for the next measurement cycle */
    SENSOR_STATE_READ_REQ,   /* Read Request: Triggers I2C read of the temperature register */
    SENSOR_STATE_VALIDATE,   /* Validation: Verifies and converts raw data to Celsius x10 */
    SENSOR_STATE_FAULT       /* Fault: Error state with recovery strategy */
} SensorState_t;

/**
 * @brief Internal sensor context (static storage, no dynamic allocation).
 *
 * * @note Deterministic Objective: Data is centralized and updated by Sensor_Tick().
 */
typedef struct
{
    /* Configuration */
    uint8_t  addr_7bit;     /* I2C 7-bit device address currently used by the driver. */
    uint16_t sample_div;    /* Sampling divider: perform one sensor acquisition every N scheduler ticks. */
    uint16_t sample_cnt;    /* Tick counter for sampling: increments each tick until it reaches sample_div. */

    /* FSM */
    SensorState_t  state;   /* Finite State Machine (FSM) current state: controls acquisition/validation/fault flow. */

    /* Data */
    int16_t  last_temp_x10; /* Last valid temperature in fixed-point (°C * 10), used as deterministic cached output. */
    uint16_t age_ticks;     /* Age of last valid sample in ticks: increments every tick, reset to 0 on successful sample. */
    uint8_t  rx_buf[SENSOR_TEMP_BYTES]; /* Raw bytes read from sensor register (temporary buffer for conversion/validation). */

    /* Diagnostics */
    SensorStatus_t last_status;          /* Status of the last acquisition attempt (OK or last error code). */
    SensorStatus_t latched_status;       /* Status of the lastched error (error code). */
    uint16_t fault_count;               /* Total number of acquisition failures since init (monotonic diagnostic counter). */
    uint8_t  consecutive_failures;      /* Number of failures in a row (resets to 0 on success), used for latching fault. */

} SensorCtx_t;



/** @section static variables */

static SensorCtx_t g_sensor;

/** @section static functions */

static SensorStatus_t Sensor_IsAddrValid(uint8_t addr_7bit);
static SensorStatus_t Sensor_IsAddrReady(uint8_t addr_7bit);
static uint16_t Sensor_ToHalAddress(uint8_t addr_7bit);
static SensorStatus_t Sensor_ConvertTa_to_x10(const uint8_t *rx, uint16_t len, int16_t *out_x10);
static void Sensor_OnFailure(SensorStatus_t err);
static void Sensor_OnSuccess(int16_t t_x10);

/**
 * @brief  Handles sensor communication or logic failures.
 * * @param err The error status that triggered the failure handler.
 * @param  
 * * @return None.
 * * @note  Increments fault counters and transitions the state machine to FAULT if the
 * consecutive failure threshold (SENSOR_MAX_CONSEC_FAIL) is reached.
 */
static void Sensor_OnFailure(SensorStatus_t err)
{
    g_sensor.last_status = err;
    
    if (g_sensor.fault_count < UINT16_MAX)
    {
        g_sensor.fault_count++;
    }


    if (g_sensor.consecutive_failures < UINT8_MAX)
    {
        g_sensor.consecutive_failures++;
    }


    if (g_sensor.consecutive_failures >= SENSOR_MAX_CONSEC_FAIL)
    {
        g_sensor.state = SENSOR_STATE_FAULT;
        g_sensor.sample_cnt = 0U; /* propre */
        g_sensor.latched_status = err; 
        g_sensor.last_temp_x10 = SENSOR_INVALID_TEMP_X10;

    }
    else
    {
        g_sensor.state = SENSOR_STATE_IDLE;
    }
}

/**
 * @brief  Updates the internal context upon a successful measurement.
 * * @param t_x10 The validated temperature value multiplied by 10.
 * @param  
 * * @return None.
 * * @note  Resets the age counter and consecutive failure tracker to zero.
 */
static void Sensor_OnSuccess(int16_t t_x10)
{
    g_sensor.last_temp_x10 = t_x10;
    g_sensor.age_ticks = 0U;
    g_sensor.last_status = SENSOR_OK;
    g_sensor.state = SENSOR_STATE_IDLE;
    g_sensor.consecutive_failures = 0U;
}

/**
 * @brief  Validates if the provided 7-bit I2C address is within the allowed range.
 * * @param addr_7bit The 7-bit address to check.
 * @param  
 * * @return SENSOR_OK if valid, SENSOR_ERR_PARAM otherwise.
 * * @note  Checks against SENSOR_MCP9808_ADDR_MIN and SENSOR_MCP9808_ADDR_MAX.
 */
static SensorStatus_t Sensor_IsAddrValid(uint8_t addr_7bit)
{
    SensorStatus_t sensor_status = SENSOR_OK;
    
    if (addr_7bit < SENSOR_MCP9808_ADDR_MIN || addr_7bit > SENSOR_MCP9808_ADDR_MAX)
    {
        sensor_status = SENSOR_ERR_PARAM;
    }
    return sensor_status;
}

/**
 * @brief  Checks if the device is responsive on the I2C bus.
 * * @param addr_7bit The 7-bit I2C address of the sensor.
 * @param  
 * * @return SENSOR_OK if ready, SENSOR_ERR_TIMEOUT or SENSOR_ERR_I2C on failure.
 * * @note  Uses HAL_I2C_IsDeviceReady with defined trial and timeout constants.
 */
static SensorStatus_t Sensor_IsAddrReady(uint8_t addr_7bit)
{
    SensorStatus_t sensor_status = SENSOR_OK;
    HAL_StatusTypeDef   hal_status;

    /* Ensure that sensor is ready on I2C bus */
    const uint16_t addr_hal = Sensor_ToHalAddress(addr_7bit);
    hal_status = HAL_I2C_IsDeviceReady(&hi2c1, addr_hal, SENSOR_I2C_TRIALS, SENSOR_I2C_TIMEOUT_MS);

    if (hal_status == HAL_OK)
    {
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

    return sensor_status;
}


/**
 * @brief  Converts a 7-bit I2C address to the 8-bit format required by the STM32 HAL.
 * * @param addr_7bit The standard 7-bit I2C address.
 * @param  
 * * @return The 8-bit shifted address (uint16_t).
 * * @note  The HAL library expects the address to be left-shifted by 1 bit to make room for the R/W bit.
 */
static uint16_t Sensor_ToHalAddress(uint8_t addr_7bit)
{
    /* HAL expects left-shifted 7-bit address (8-bit format) */
    return (uint16_t)((uint16_t)addr_7bit << 1);
}

/**
 * @brief  Converts raw I2C data from the Ambient Temperature (Ta) register to Celsius x10.
 * * @param rx Array containing the 2 bytes received from the sensor (MSB first).
 * @param len Length of the rx buffer (expected to be 2).
 * * @return SENSOR_OK if conversion succeeded, SENSOR_ERR_PARAM if pointer is NULL.
 */
static SensorStatus_t Sensor_ConvertTa_to_x10(const uint8_t *rx, uint16_t len, int16_t *out_x10)
{
    SensorStatus_t sensor_status = SENSOR_OK;

    if((rx == NULL) || (out_x10 == NULL) || (len != 2U))
    {
        sensor_status = SENSOR_ERR_PARAM;
    }
    else
    {
        /*
        *   TA: AMBIENT TEMPERATURE REGISTER p.24 datasheet
        *   [15:13] : Flag = TCRIT; TUPPER; TLOWER
        *   [12:0] :  SIGN + TA as signed 13-bit two's complement, Q4 (LSB = 2^-4 °C)
        */
        const uint16_t raw  = (uint16_t)(((uint16_t)rx[0] << 8) | (uint16_t)rx[1]);
        const uint16_t ta13 = (uint16_t)(raw & 0x1FFFU); /* Ignore alert flags bits 15..13 (keep sign + temperature bits) */
        
        /* Converted TA p.25 datasheet 
        * Sign-extend 13-bit two's complement into int32 (Q4 units)
        */
        int32_t q4;
        if((ta13 & 0x1000U) != 0U)  /* sign bit TA[12]*/
        {   
            q4 = (int32_t)ta13 - 0x2000; /* subtract 8192 -> sign-extend 13-bit */
        }
        else
        {
            q4 = (int32_t)ta13;
        }

        /* Convert Q4 to x10°C:
        temp(°C) = q4 / 16
        temp_x10 = (q4 * 10) / 16
        Use symmetric rounding (important for negatives)
        */
        int32_t x10;
        if (q4 >= 0)
        {
            x10 = (q4 * 10 + 8) / 16;
        }
        else
        {
            x10 = (q4 * 10 - 8) / 16;
        }

        /* Defensive clamp to int16_t */
        if (x10 > (int32_t)INT16_MAX) 
        { 
            x10 = (int32_t)INT16_MAX; 
        }
        if (x10 < (int32_t)INT16_MIN) 
        { 
            x10 = (int32_t)INT16_MIN; 
        }

        *out_x10 = (int16_t)x10;
        sensor_status = SENSOR_OK;
    }

    return sensor_status;
}

/** @section globals functions */

/**
 * @brief  Initialize the MCP9808 module and internal context.
 *
 * @note   Prerequisite: the I2C peripheral must be initialized before calling.
 * @note   This function may perform a bounded communication check (device ready).
 */
SensorStatus_t Sensor_Init(uint8_t addr_7bit)
{
    SensorStatus_t sensor_status = SENSOR_OK;

    /* Default safe context */
    g_sensor.state = SENSOR_STATE_UNINIT;
    g_sensor.addr_7bit  = addr_7bit;
    g_sensor.last_temp_x10 = SENSOR_INVALID_TEMP_X10;

    g_sensor.age_ticks = 0U;
    g_sensor.fault_count = 0U;
    g_sensor.consecutive_failures = 0U;
    g_sensor.latched_status = SENSOR_OK;

    g_sensor.last_status = SENSOR_ERR_INIT;
    g_sensor.sample_div = SENSOR_SAMPLE_DIV;
    g_sensor.sample_cnt = 0U;

    /* Validate address */
    sensor_status = Sensor_SetAddress(addr_7bit);
    
    if (sensor_status == SENSOR_OK)
    {
        g_sensor.state = SENSOR_STATE_IDLE;
        g_sensor.last_status = SENSOR_OK;
    }
    else
    {
        /* Keep state UNINIT and store the real failure reason */
        g_sensor.last_status = sensor_status;
        g_sensor.fault_count++; 
    }

    return sensor_status;
}

/**
 * @brief  Periodic function called from the cooperative scheduler.
 *
 * @note   Call this at a fixed period (e.g., every SENSOR_TICK_MS).
 * @note   The module performs sampling every SENSOR_SAMPLE_DIV ticks
 *         (e.g., 10 ticks @ 10ms => 100ms).
 * @note   1 ticks = 1 state => 3 ticks = 1 cycle (IDEL => REQ => VALIDATE)
 * @note   When a FAULT is latched, this function performs no acquisition until
 *         Sensor_RequestRecovery() is called.
 */
SensorStatus_t Sensor_Tick(void)
{
    SensorStatus_t sensor_status = SENSOR_OK;
    HAL_StatusTypeDef hal_status;

    if(g_sensor.state == SENSOR_STATE_UNINIT)
    {
        g_sensor.last_status = SENSOR_ERR_INIT;
        sensor_status = SENSOR_ERR_INIT;
    }
    else 
    {

        if (g_sensor.age_ticks < UINT16_MAX)
        {
            g_sensor.age_ticks++;
        }

        /* FSM tick-driven:
        * One scheduler tick advances at most one FSM step.
        * Note: the READ step performs a bounded blocking I2C transaction (timeout <= SENSOR_I2C_TIMEOUT_MS).
        */
        switch (g_sensor.state)
        {
            case SENSOR_STATE_IDLE:
            {           
                g_sensor.sample_cnt++;
                if (g_sensor.sample_cnt >= g_sensor.sample_div)
                {
                    g_sensor.sample_cnt = 0U;
                    g_sensor.state = SENSOR_STATE_READ_REQ;
                }
                break;
            }

            case SENSOR_STATE_READ_REQ:
            {
                const uint16_t addr_hal = Sensor_ToHalAddress(g_sensor.addr_7bit);
                /* I2C blocking (can last several milliseconds in a single tick) */
                hal_status = HAL_I2C_Mem_Read(&hi2c1,
                                            addr_hal,
                                            (uint16_t)SENSOR_MCP9808_REG_TA,
                                            I2C_MEMADD_SIZE_8BIT,
                                            &g_sensor.rx_buf[0],
                                            (uint16_t)SENSOR_TEMP_BYTES,
                                            (uint32_t)SENSOR_I2C_TIMEOUT_MS);

                if (hal_status == HAL_OK)
                {
                    g_sensor.state = SENSOR_STATE_VALIDATE;
                }
                else
                {
                    if (hal_status == HAL_TIMEOUT)
                    {
                        sensor_status = SENSOR_ERR_TIMEOUT;
                    }
                    else
                    {
                        sensor_status = SENSOR_ERR_I2C;
                    }
                    Sensor_OnFailure(sensor_status);
                }
                break;
            }

            case SENSOR_STATE_VALIDATE:
            {
                int16_t t_x10 = SENSOR_INVALID_TEMP_X10;

                sensor_status = Sensor_ConvertTa_to_x10(g_sensor.rx_buf,
                                                    SENSOR_TEMP_BYTES,
                                                    &t_x10);

                if (sensor_status == SENSOR_OK)
                {
                    Sensor_OnSuccess(t_x10);
                }
                else
                {
                    Sensor_OnFailure(sensor_status);
                }
                break;
            }

            case SENSOR_STATE_FAULT:
            {
                /* Passive state: corrective action is awaited */
                break;
            }

            default:
            {
                g_sensor.last_status = SENSOR_ERR_INIT;
                g_sensor.last_temp_x10 = SENSOR_INVALID_TEMP_X10;
                g_sensor.state = SENSOR_STATE_FAULT;
                break;
            }
        }
    }
    return g_sensor.last_status;
}

/**
 * @brief  Configures a new 7-bit I2C address for the sensor.
 * * @note  Changing the address might require a device reset.
 */
SensorStatus_t Sensor_SetAddress(uint8_t addr_7bit)
{
    SensorStatus_t sensor_status;
    sensor_status = Sensor_IsAddrValid(addr_7bit);

    /* Check ready only if valid */
    if (sensor_status == SENSOR_OK)
    {
        sensor_status = Sensor_IsAddrReady(addr_7bit);
    }
    
    if (sensor_status == SENSOR_OK)
    {
        g_sensor.addr_7bit = addr_7bit;
    }

    g_sensor.last_status = sensor_status;
    return sensor_status;
}

/**
 * @brief  Get the last recorded module status (cached).
 * 
 * @note   This function does not perform any I2C transaction.
 * @note   This status typically reflects the last acquisition attempt and/or
 *         the current latched FAULT condition.
 */
SensorStatus_t Sensor_GetLastStatus(SensorStatus_t *out_status)
{
    SensorStatus_t sensor_status = SENSOR_OK;
    if (out_status == NULL)
    {
        sensor_status = SENSOR_ERR_PARAM;
    }
    else
    {
        *out_status = g_sensor.last_status;
    }

    return sensor_status;
}

/**
 * @brief  Returns the time elapsed since the last valid measurement.
 * * @note  Expressed in scheduler ticks.
 */
SensorStatus_t Sensor_GetAgeTicks(uint16_t *out_age)
{
    SensorStatus_t sensor_status = SENSOR_OK;

    if (out_age == NULL)
    {   
        sensor_status = SENSOR_ERR_PARAM;
    }
    else 
    {
        *out_age = g_sensor.age_ticks;
    }
    return sensor_status;
}

/**
 * @brief  Get the last valid temperature (cached) in fixed-point format (°C * 10).
 *
 * @note   Getter only: this function never performs any I2C transaction.
 * @note   This function reports data usability (valid + freshness). It does not
 *         guarantee that the last acquisition attempt was successful: check
 *         Sensor_GetLastStatus() for health/diagnostics.
 * @note   Example: 255 means 25.5°C. This avoids float usage in safety-critical paths.
 */
SensorStatus_t Sensor_GetLastTemperature_x10(int16_t *out_temp_x10)
{
    SensorStatus_t sensor_status = SENSOR_OK;

    if(out_temp_x10 == NULL)
    {
        sensor_status = SENSOR_ERR_PARAM;
    }

    else if (g_sensor.last_temp_x10 == SENSOR_INVALID_TEMP_X10)
    {
        sensor_status = SENSOR_ERR_INIT;
    }
    else if (g_sensor.age_ticks >= SENSOR_STALE_TICKS)
    {
        sensor_status = SENSOR_ERR_TIMEOUT;
    }

    else
    {
        *out_temp_x10 = g_sensor.last_temp_x10;
    }

    return sensor_status;
}

/**
 * @brief  Get the total number of acquisition faults detected since initialization.
 *
 * @note   The counter is saturating (stops incrementing at UINT16_MAX).
 * @note   Useful for diagnostics / health monitoring.
 */
SensorStatus_t Sensor_GetFaultCount(uint16_t *out_cnt)
{
    SensorStatus_t sensor_satus = SENSOR_OK;

    if (out_cnt == NULL)
    {
        sensor_satus = SENSOR_ERR_PARAM;
    }
    else
    {
        *out_cnt = g_sensor.fault_count;
    }

    return sensor_satus;
}

/**
 * @brief  Get the latched status (root cause) stored when entering FAULT.
 *
 * @note   This is the error that caused the transition into the latched FAULT state.
 * @note   This function does not perform any I2C transaction.
 */
SensorStatus_t Sensor_GetLatchedStatus(SensorStatus_t *out)
{
    SensorStatus_t sensor_status = SENSOR_OK;
    
    if (out == NULL) 
    {
        sensor_status = SENSOR_ERR_PARAM;
    }
    else
    {
        *out = g_sensor.latched_status;
    }
    return sensor_status;
}

/**
 * @brief  Request a recovery from a latched FAULT state.
 *
 * @note   This is the only allowed exit from the latched FAULT state.
 * @note   Recovery typically clears the consecutive failure counter and resets
 *         the internal state machine back to IDLE.
 */
SensorStatus_t Sensor_RequestRecovery(void)
{
    SensorStatus_t sensor_status = SENSOR_OK;

    if (g_sensor.state == SENSOR_STATE_UNINIT)
    {
        sensor_status = SENSOR_ERR_INIT;
    }
    else if (g_sensor.state != SENSOR_STATE_FAULT)
    {
        /* pas en FAULT: rien à recover, mais commande acceptée */
        return SENSOR_OK;
    }
    else
    {
        g_sensor.consecutive_failures = 0U;
        g_sensor.sample_cnt = 0U;

        /* Invalidate the last value to force the app to wait */
        g_sensor.last_temp_x10 = SENSOR_INVALID_TEMP_X10;
        g_sensor.age_ticks = UINT16_MAX; /* force stale/invalid */

        g_sensor.state = SENSOR_STATE_IDLE;
    }
    
    return sensor_status;
}

