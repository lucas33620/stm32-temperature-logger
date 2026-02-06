/**
 * @file scheduler.c
 * @brief Cooperative scheduler triggered by timer tick (demo MCP9808)
 * @copyright
 * © 2025 SYLORIA — MIT License — BAQUEY Lucas (contact@syloria.eu)
 */

#include "scheduler.h"
#include <stdint.h>

/* ---- Define ---- */
#define SCHEDULER_MIN_PERIOD_TICKS   (1U)

/* En démo: on borne le rattrapage pour éviter un "death loop" si une tâche bloque */
#define SCHEDULER_MAX_PENDING        (3U)
#define SCHEDULER_MAX_DRAIN_PER_CALL (1U)  /* 1 = plus stable pour démo */

/* ---- Static variables ---- */
static uint32_t tick_period = SCHEDULER_MIN_PERIOD_TICKS;
static uint32_t tick_count  = 0U;

/* pending = nombre de tâches à exécuter */
static volatile uint32_t pending = 0U;

/* overrun = on a raté au moins un tick car pending était déjà plein */
static volatile uint8_t scheduler_overrun = 0U;

/* ---- API ---- */
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
    pending = 0U;
    scheduler_overrun = 0U;

    return status;
}

/* Called from TIMER ISR */
void Scheduler_OnTick(void)
{
    tick_count++;

    if (tick_count >= tick_period)
    {
        tick_count = 0U;

        /* Saturation: si pending est plein, on latch un overrun */
        if (pending < (uint32_t)SCHEDULER_MAX_PENDING)
        {
            pending++;
        }
        else
        {
            scheduler_overrun = 1U;
        }
    }
}

/* Called from main loop */
void Scheduler_Process(void)
{
    uint32_t drained = 0U;

    /* Drain borné: exécute au plus N tâches par appel */
    while ((pending != 0U) && (drained < (uint32_t)SCHEDULER_MAX_DRAIN_PER_CALL))
    {
        /* décrément atomique simple (volatile) */
        pending--;
        drained++;

        Scheduler_Task();
    }
}

/* ---- Optional demo helpers ---- */
uint8_t Scheduler_GetOverrunFlag(void)
{
    return scheduler_overrun;
}

void Scheduler_ClearOverrunFlag(void)
{
    scheduler_overrun = 0U;
}

uint32_t Scheduler_GetPending(void)
{
    return pending;
}

/* ---- Weak task ---- */
#if defined(__GNUC__)
#define SCHEDULER_WEAK __attribute__((weak))
#else
#define SCHEDULER_WEAK
#endif

SCHEDULER_WEAK void Scheduler_Task(void)
{
    /* Override in app: call Sensor_Tick() here */
}
