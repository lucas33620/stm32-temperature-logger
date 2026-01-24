/**
 * @file scheduler.c
 * @brief Cooperative scheduler triggered by timer tick
 * @copyright
 * © 2025 SYLORIA — MIT License — BAQUEY Lucas (contact@syloria.fr)
 */

#include "scheduler.h"

/* ----Define ----*/
#define SCHEDULER_MIN_PERIOD_TICKS   (1U)

/* ----Static variables ----*/
static uint32_t        tick_period = 0U;   /* configuration */
static uint32_t        tick_count  = 0U;   /* ISR-only counter */
static volatile uint8_t tick_flag  = 0U;   /* ISR <-> main flag */

/* ---- API ----*/
/* Initisalisation du scheduler */
SchedulerStatus_t Scheduler_Init(uint32_t period_tick)
{
    SchedulerStatus_t status = SCHEDULER_OK;

    if (period_tick >= SCHEDULER_MIN_PERIOD_TICKS)
    {
        tick_period = period_tick;
    }
    else
    {
        tick_period = SCHEDULER_MIN_PERIOD_TICKS;
        status = SCHEDULER_ERR_PARAM;
    }

    tick_count = 0U;
    tick_flag  = 0U;

    return status;
}

/* Appel depuis l'ISR du TIMER */
void Scheduler_OnTick(void)
{
    if (tick_period != 0U)
    {
        tick_count += 1U;

        if (tick_count >= tick_period)
        {
            tick_flag = 1U;
            tick_count = 0U;
        }
    }
}

/* Appel dans la boucle principal */
void Scheduler_Process(void)
{
    if (tick_flag != 0U)
    {
        tick_flag = 0U;
        Scheduler_Task();
    }
}

/* -- API -- */
#if defined(__GNUC__)
#define SCHEDULER_WEAK __attribute__((weak))
#else
#define SCHEDULER_WEAK
#endif

SCHEDULER_WEAK void Scheduler_Task(void)
{
    /* Default empty task; may be overridden by application */
}
