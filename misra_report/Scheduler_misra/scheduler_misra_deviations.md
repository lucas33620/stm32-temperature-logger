# Dérogations MISRA — Scheduler

## SCH-DEV-001 — Callback `Scheduler_Task()` avec liaison faible (weak)
- **Règle / outil concerné :** MISRA-C:2012 R8.7 (fonction non utilisée / devrait être `static`) + avertissement Cppcheck `staticFunction`.
- **Écart :** `Scheduler_Task()` est exposée publiquement mais peut ne pas être référencée directement dans tous les TU (translation units) selon l’architecture.
- **Justification :** `Scheduler_Task()` est un **point d’extension applicatif (callback)** volontairement public, destiné à être **surchargé côté application** sans modifier le module scheduler.
- **Mesure de maîtrise :**
  - Implémentation par défaut **vide** fournie (safe default).
  - Intégration vérifiée par test fonctionnel (ex. trace UART / breakpoint) confirmant l’appel réel depuis `Scheduler_Process()`.

## SCH-DEV-002 — Avertissements `unusedFunction` sur API scheduler (analyse partielle)
- **Règle / outil concerné :** Cppcheck `unusedFunction` (information / style).
- **Écart :** `Scheduler_Init()`, `Scheduler_OnTick()`, `Scheduler_Process()` signalées “non utilisées” lors d’une analyse sur fichier isolé.
- **Justification :** Ces fonctions sont appelées **depuis d’autres unités** à partir de `scheduler.h`:
  - `Scheduler_Init()` et `Scheduler_Process()` depuis `main.c` (boucle principale),
  - `Scheduler_OnTick()` depuis l’ISR timer (`stm32f4xx_it.c` ou callback HAL).
- **Mesure de maîtrise :**
  - Lancement de l’analyse statique sur **l’ensemble du projet** (mêmes flags et include paths) pour éliminer les faux positifs.
  - Revue d’intégration confirmant les points d’appel.

## SCH-DEV-003 — Lecture de `tick_period` en ISR (hypothèse d’initialisation avant IT)
- **Règle / risque ciblé :** Concurrence potentielle (variable configurée hors ISR, lue en ISR).
- **Écart :** `tick_period` est configuré dans `Scheduler_Init()` et lu dans `Scheduler_OnTick()`.
- **Justification :** Conception imposant que `Scheduler_Init()` soit exécutée **avant l’activation des interruptions du timer** (séquence d’initialisation système).
- **Mesure de maîtrise :**
  - Séquence d’init documentée (ordre d’appel) dans `main.c`.
  - Option de garde dans l’ISR (`if (tick_period != 0U)`) pour éviter tout comportement indéfini.
