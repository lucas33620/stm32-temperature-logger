/**
 * @file    sensor_temp.h
 * @brief   Sensor management module for MCP9808.
 * @company Syloria
 * @author  Lucas Baquey
 * * @copyright MIT License
 * * Summary: Permission is granted, free of charge, to any person obtaining a copy
 * of this software to deal in the Software without restriction, including the rights
 * to use, copy, modify, merge, publish, distribute, sublicense and/or sell copies.
 * The software is provided "as is", without warranty of any kind.
 * 
 * @note SensorState_t = internal FSM (acquisition + FAULT latch). 
 *       SensorStatus_t = API result/error codes.
 *       last_status = last attempt result; latched_status = sticky cause in FAULT until Sensor_RequestRecovery().
 *       Data can be valid but stale; getters are non-blocking and may return SENSOR_ERR_STALE.
 */


#ifndef SENSOR_TEMP_H
#define SENSOR_TEMP_H

/** @section Include */

#include <stdint.h>
#include <limits.h>
#include "mcp9808_cfg.h"

/** @section Define*/

#define SENSOR_MCP9808_ADDR_MIN      (0x18U) /* The final address is 0x18 + A2 + A1 + A0.  */
#define SENSOR_MCP9808_ADDR_MAX      (0x1FU)  
#define SENSOR_MCP9808_REG_TA        (0x05U)   /* Ambient Temperature (TA) */

/** @section Typedef */

/**
 * @brief Sensor module status codes.
 *
 * * @note :
 * - These codes represent the module's internal state/health and last events.
 * - Validity of the cached temperature also depends on "stale" management.
 */
typedef enum
{
    SENSOR_OK = 0,       /**< No error. */
    SENSOR_ERR_INIT,     /**< Module not initialized or internal invalid state. */
    SENSOR_ERR_PARAM,    /**< Invalid parameter (NULL pointer, invalid address, etc.). */
    SENSOR_ERR_I2C,      /**< I2C communication error (NACK, bus error, etc.). */
    SENSOR_ERR_STALE,    /**< Last cached temperature is too old (stale): no valid sample received within the allowed age threshold. */
    SENSOR_ERR_TIMEOUT   /**< I2C communication timeout. */
} SensorStatus_t;

/** @section Global Functions */

/**
 * @brief  Initialize the MCP9808 module and internal context.
 *
 * @param  addr_7bit  7-bit I2C address of the MCP9808 (0x18..0x1F).
 *
 * @return SENSOR_OK on success or an error code if parameters are invalid or
 *         initial communication check fails.
 *
 * @note   Prerequisite: the I2C peripheral must be initialized before calling.
 * @note   This function may perform a bounded communication check (device ready).
 */
SensorStatus_t Sensor_Init(uint8_t addr_7bit);

/**
 * @brief  Periodic function called from the cooperative scheduler.
 *
 * @return Cached module status after progressing the internal state machine.
 *
 * @note   Call this at a fixed period (e.g., every SENSOR_TICK_MS).
 * @note   The module performs sampling every SENSOR_SAMPLE_DIV ticks
 *         (e.g., 10 ticks @ 10ms => 100ms).
 * @note   When a FAULT is latched, this function performs no acquisition until
 *         Sensor_RequestRecovery() is called.
 */
SensorStatus_t Sensor_Tick(void);

/**
 * @brief  Set (update) the MCP9808 7-bit I2C address used by the module.
 *
 * @param  addr_7bit  New 7-bit address (0x18..0x1F).
 *
 * @return SENSOR_OK on success, SENSOR_ERR_PARAM if address invalid,
 *         or SENSOR_ERR_I2C / SENSOR_ERR_TIMEOUT if a bounded readiness check fails
 *         (implementation dependent).
 *
 * @note   This is a configuration setter. It must not be called from hard real-time
 *         critical sections unless you accept its bounded I2C check cost.
 */
SensorStatus_t Sensor_SetAddress(uint8_t addr_7bit);

/**
 * @brief  Get the last recorded module status (cached).
 *
 * @param  out_status  Output pointer receiving the cached SensorStatus_t.
 *
 * @return SENSOR_OK on success, SENSOR_ERR_PARAM if out_status is NULL.
 *
 * @note   This function does not perform any I2C transaction.
 * @note   This status typically reflects the last acquisition attempt and/or
 *         the current latched FAULT condition.
 */
SensorStatus_t Sensor_GetLastStatus(SensorStatus_t* out_status);

/**
 * @brief  Get the age (in scheduler ticks) since the last valid temperature update.
 *
 * @param  out_age  Output pointer receiving the age in ticks.
 *
 * @return SENSOR_OK on success, SENSOR_ERR_PARAM if out_age is NULL,
 *         SENSOR_ERR_INIT if no valid measurement has ever been produced (implementation dependent).
 *
 * @note   The application can compare this value against SENSOR_STALE_TICKS
 *         to detect stale data (data too old).
 */
SensorStatus_t Sensor_GetAgeTicks(uint16_t* out_age);

/**
 * @brief  Get the last valid temperature (cached) in fixed-point format (°C * 10).
 *
 * @param  out_temp_x10  Output pointer receiving temperature in °C*10.
 *
 * @return SENSOR_OK if a valid, non-stale cached value is available,
 *         SENSOR_ERR_PARAM if out_temp_x10 is NULL,
 *         SENSOR_ERR_INIT if no valid cached value is available (never sampled,
 *                         invalidated, or stale beyond the configured threshold).
 *
 * @note   Getter only: this function never performs any I2C transaction.
 * @note   This function reports data usability (valid + freshness). It does not
 *         guarantee that the last acquisition attempt was successful: check
 *         Sensor_GetLastStatus() for health/diagnostics.
 * @note   Example: 255 means 25.5°C. This avoids float usage in safety-critical paths.
 */
SensorStatus_t Sensor_GetLastTemperature_x10(int16_t * out_temp_x10);

/** @section Safety / Diagnostic Functions */

/**
 * @brief  Get the total number of acquisition faults detected since initialization.
 *
 * @param  out_cnt  Output pointer receiving the fault count (saturating counter).
 *
 * @return SENSOR_OK on success, SENSOR_ERR_PARAM if out_cnt is NULL.
 *
 * @note   The counter is saturating (stops incrementing at UINT16_MAX).
 * @note   Useful for diagnostics / health monitoring.
 */
SensorStatus_t Sensor_GetFaultCount(uint16_t* out_cnt);

/**
 * @brief  Get the latched status (root cause) stored when entering FAULT.
 *
 * @param  out  Output pointer receiving the latched SensorStatus_t.
 *
 * @return SENSOR_OK on success, SENSOR_ERR_PARAM if out is NULL.
 *
 * @note   This is the error that caused the transition into the latched FAULT state.
 * @note   This function does not perform any I2C transaction.
 */
SensorStatus_t Sensor_GetLatchedStatus(SensorStatus_t *out);

/**
 * @brief  Request a recovery from a latched FAULT state.
 *
 * @return SENSOR_OK if recovery request accepted, SENSOR_ERR_INIT if module not initialized.
 *
 * @note   This is the only allowed exit from the latched FAULT state.
 * @note   Recovery typically clears the consecutive failure counter and resets
 *         the internal state machine back to IDLE.
 */
SensorStatus_t Sensor_RequestRecovery(void);

#endif /* SENSOR_TEMP_H */
