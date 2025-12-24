# Dérogations MISRA — Sensor

## SEN-DEV-001 — Avertissements `unusedFunction` / `staticFunction` (Cppcheck)
- **Règle / outil concerné :** MISRA-C:2012 R8.7 (objet/fonction à liaison externe non référencé dans l’unité analysée) + avertissements Cppcheck `unusedFunction` et `staticFunction`.
- **Écart :** `Sensor_Init()`, `Sensor_ReadTemperature()` et `Sensor_SetAddress()` peuvent être signalées comme “non utilisées” ou “devraient être `static`” lors d’une analyse partielle (fichier `sensor.c` analysé isolément).
- **Justification :** Ces fonctions constituent l’**API publique** du module capteur et sont appelées par d’autres unités de traduction (ex. `main.c`, `cli_uart.c`, tests d’intégration). Les rendre `static` casserait l’interface et l’architecture modulaire.
- **Mesure de maîtrise :**
  - Exécuter l’analyse statique sur **l’ensemble du projet** (toutes unités de compilation) avec les chemins d’inclusion et définitions de build corrects.
  - Vérifier l’usage réel par test d’intégration (ex. commande CLI déclenchant `Sensor_ReadTemperature()` / séquence d’init appelant `Sensor_Init()`).

## SEN-DEV-002 — Avertissements `missingInclude` / `misra-config` (Cppcheck)
- **Règle / outil concerné :** Avertissements Cppcheck `missingInclude`, `missingIncludeSystem` et erreurs de configuration `misra-config` (incomplétude d’analyse).
- **Écart :** Certains symboles HAL/STM32 (`stm32f4xx_hal.h`, `HAL_OK`, `HAL_TIMEOUT`, `HAL_I2C_*`, constantes) peuvent être “inconnus” si Cppcheck n’est pas configuré avec les bons include paths et defines.
- **Justification :** Écart lié à la **configuration d’outillage** (chemins d’en-têtes HAL/CMSIS et macros de compilation) et non à un défaut du code. Les diagnostics MISRA associés peuvent être des **faux positifs** tant que la configuration n’est pas complète.
- **Mesure de maîtrise :**
  - Configurer Cppcheck avec les include paths HAL/CMSIS du projet et les macros (ex. `USE_HAL_DRIVER`, `STM32F429xx`).
  - Relancer l’audit MISRA après configuration et conserver le report complet (analyse non “incomplete”).
