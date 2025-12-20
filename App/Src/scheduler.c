/**
 * @file scheduler.c
 * @brief Cooperative scheduler triggered by timer tick
 * @copyright
 * © 2025 Syloria — MIT
 */


#include "scheduler.h"

static volatile uint32_t tick_count  = 0U;
static uint32_t          tick_period = 0U;
static volatile uint8_t  tick_flag   = 0U;

void Scheduler_Init(uint32_t period_ticks)
{
    tick_count = 0U;
    tick_flag  = 0U;

    if (period_ticks > 0U)
    {
        tick_period = period_ticks;
    }
    else
    {
        tick_period = 1U;
    }
}

/* Called from timer ISR */
void Scheduler_OnTick(void)
{
    tick_count++;

    if (tick_count >= tick_period)
    {
        tick_flag  = 1U;
        tick_count = 0U;
    }
}

void Scheduler_Process(void)
{
    if (tick_flag == 1U)
    {
        tick_flag = 0U;

        /*Scheduled action */
    }
}
