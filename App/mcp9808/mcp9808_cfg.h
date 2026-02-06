/**
 * @file    syloria_config.h
 * @brief   Project configuration constants (timing, retries, thresholds).
 * @company Syloria
 * @author  Lucas Baquey
 *
 * @copyright MIT License
 * Summary: Permission is granted, free of charge, to any person obtaining a copy
 * of this software to deal in the Software without restriction, including the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies.
 * The software is provided "as is", without warranty of any kind.
 */

#ifndef MCP9808_CFG_H
#define MCP9808_CFG_H

/**  
 *  @brief MCP9808 temperature sensor configuration
 * * @note :
 *  - Scheduler_Task period is SENSOR_TICK_MS (cooperative tick).
 *  - Sampling is performed every SENSOR_SAMPLE_DIV ticks.
 *  - A sample becomes "stale" after SENSOR_STALE_MS without a valid update.
 *  
 */

/* Base tick period (Scheduler_Task call period), in milliseconds */
#define SENSOR_TICK_MS                 (10U)

/* Stale threshold, in milliseconds (data older than this is considered unusable) */
#define SENSOR_STALE_MS                (1000U)

/* Downsampling factor: sample every N ticks (N=10 => 100 ms for 10 ms tick) */
#define SENSOR_SAMPLE_DIV              (10U)

/* Max consecutive acquisition failures before latching FAULT */
#define SENSOR_MAX_CONSEC_FAIL         (3U)

/* I2C timeout per transaction, in milliseconds (must be < SENSOR_TICK_MS ideally) */
#define SENSOR_I2C_TIMEOUT_MS          (3U)

/* Number of I2C "device ready" trials (HAL_I2C_IsDeviceReady) */
#define SENSOR_I2C_TRIALS              (1U)

/* ---- Derived constants ---- */
#define SENSOR_STALE_TICKS             ((SENSOR_STALE_MS) / (SENSOR_TICK_MS))

/* ---- Compile-time checks (build-time safety) ---- */
#if ((SENSOR_STALE_MS % SENSOR_TICK_MS) != 0U)
#error "SENSOR_STALE_MS must be a multiple of SENSOR_TICK_MS to avoid truncation"
#endif

/* ============================================================================
 *  Future project-wide configuration sections can be added here
 *  e.g., watchdog, logging, diagnostics, communication timeouts, etc.
 * ========================================================================== */

#endif /* SYLORIA_CONFIG_H */
