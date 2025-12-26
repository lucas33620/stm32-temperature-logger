/**
 * @file sensor.h
 * @brief fournir une température en °C à partir du module LM75 en I2C
 * @copyright
 * © 2025 Syloria — MIT
 */

#ifndef SENSORTEMP_H
#define SENSORTEMP_H
#include <stdint.h>

/*Datasheet LM75*/
#define SENSOR_LM75_MIN_ADDR   		(0x48U)
#define SENSOR_LM75_DEFAULT_ADDR   	(0x48U) /* LM75 I2C address is 7-bit.*/
#define SENSOR_LM75_MAX_ADDR 		(0x4FU)
#define SENSOR_LM75_REG_TEMP		(0x00U)

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
SensorStatus_t  Sensor_SetAddress(uint8_t addr);

#endif /* SENSORTEMP_H */
