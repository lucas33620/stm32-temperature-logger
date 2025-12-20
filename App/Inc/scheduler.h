/**
 * @file scheduler.h
 * @brief Cooperative scheduler triggered by timer tick
 * @copyright
 * © 2025 Syloria — MIT
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

typedef enum
{
    SCHEDULER_OK = 0,
    SCHEDULER_ERR_PARAM
} SchedulerStatus_t;

/* period_tick: number of timer ticks between task executions */
SchedulerStatus_t Scheduler_Init(uint32_t period_tick);

/* Called from timer ISR */
void Scheduler_OnTick(void);

/* Called from main loop */
void Scheduler_Process(void);

/* API callback executed when period elapsed */
void Scheduler_Task(void);

#endif
