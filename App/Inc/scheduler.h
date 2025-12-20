/**
 * @file scheduler.c
 * @brief Cooperative scheduler triggered by timer tick
 * @copyright
 * © 2025 Syloria — MIT
 */


#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

/* Initisalisation du scheduler */
void Scheduler_Init(uint32_t periode_ms);

/* Appel depuis l'ISR du TIMER */
void Scheduler_OnTick(void);

/* Appel dans la boucle principal */
void Scheduler_Process(void);

#endif
