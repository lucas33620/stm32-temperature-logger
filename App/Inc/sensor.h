/**
 * @file sensor.h
 * @brief fournir une température en °C à partir du module LM75 en I2C
 * @copyright
 * © 2025 Syloria — MIT
 */

#ifndef SENSOR_H
#define SENSOR_H
#include <stdint.h>
#include <stddef.h>
#include "stm32f4xx_hal.h"

extern I2C_HandleTypeDef hi2c1; /*hi2c1 est généré par CubeMX*/



typedef enum
{
    SENSOR_OK = 0,
    SENSOR_ERR_INIT,	/*Erreur d'initialisation du capteur*/
    SENSOR_ERR_PARAM,	/*Erreur de parametres instancié */
    SENSOR_ERR_I2C,		/*Erreur du bus I2C*/
    SENSOR_ERR_TIMEOUT	/*Erreur de receptin de donnee*/
} SensorStatus_t;

/* Initialise le module capteur */
SensorStatus_t Sensor_Init(uint8_t addr);

/*	Lit la temperature en degC */
SensorStatus_t Sensor_ReadTemperature(float * out_celsius);

/* Configure l'adresse I2C 7-bit du LM75 a tout moment */
void Sensor_SetAddress(uint8_t addr);

#endif /* SENSOR_H */
