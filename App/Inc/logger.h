/**
 * @file logger.h
 * @brief Logger de mesures de temperature en SPI Flash (W25Q128, SPI 1-bit)
 * @copyright
 * © 2025 SYLORIA — MIT License
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>

/* Flash SPI — Parametres fixes  */
/* Capacites W25Q128 */
#define LOGGER_FLASH_TOTAL_SIZE_BYTES     (16UL * 1024UL * 1024UL) /* 16 MB */
#define LOGGER_FLASH_SECTOR_SIZE_BYTES    (4096UL)                 /* 4 KB */
#define LOGGER_FLASH_PAGE_SIZE_BYTES      (256UL)

/* Zone reservee au logger V1*/
#define LOGGER_FLASH_BASE_ADDR            (0x00000000UL)
#define LOGGER_FLASH_MAX_SIZE_BYTES       (LOGGER_FLASH_TOTAL_SIZE_BYTES)

/* Entree de log (taille logique = 6 octets, stockee sur 8 octets) */
typedef struct
{
    int16_t  temp_x10;	/* Format temperature 23.1°C = 231 */
    uint32_t sample_id;	/* Compteur */
} LoggerEntry_t;

typedef enum
{
    LOGGER_OK = 0,
    LOGGER_ERR_PARAM,
    LOGGER_ERR_SPI,
    LOGGER_ERR_TIMEOUT,
    LOGGER_ERR_FULL,
    LOGGER_ERR_EMPTY
} LoggerStatus_t;

/* Initialisation du logger */
LoggerStatus_t Logger_Init(void);

/* Ajoute une mesure de temperature au logger */
LoggerStatus_t Logger_AddSample(int16_t temp_x10);

/* Lit la derniere entree enregistree. */
LoggerStatus_t Logger_GetLast(LoggerEntry_t * out_entry);

/* Lit une entree par index. */
LoggerStatus_t Logger_GetEntry(uint32_t index, LoggerEntry_t * out_entry);

/* Retourne le nombre total d'entrees enregistrees. */
uint32_t Logger_GetCount(void);

/*Efface toute la zone logger (erase complet).
 * @note Operation lente (erase secteurs).
 */
LoggerStatus_t Logger_Clear(void);

#endif /* LOGGER_H */
