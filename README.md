# 📡 Mini Station Température & Data Logger — STM32

Mini-station embarquée permettant de mesurer une température, l’enregistrer périodiquement en EEPROM I2C, et récupérer l’historique des mesures au format JSON via UART.

Ce projet démontre une architecture modulaire propre, des drivers bas niveau (ADC, I2C, EEPROM), un scheduler simple, et une interface UART type CLI — un ensemble représentatif d’un mini-système embarqué industrialisable.

---

## 🎯 Objectifs

- Mesurer périodiquement la température (capteur interne ADC ou capteur externe I2C)
- Stocker chaque mesure dans une EEPROM I2C (24C32 / 24C64)
- Fournir une interface série (UART CLI) pour interroger la station
- Exporter les données sous forme JSON (`GET`, `LAST`, `CLEAR`)
- Démontrer une architecture claire et évolutive (Sensor / Logger / Scheduler / CLI)

---

## 🗂 Arborescence du projet
/App
  /Inc
    sensor.h
    logger.h
    cli_uart.h
    scheduler.h

  /Src
    main.c
    sensor.c
    logger.c
    cli_uart.c
    scheduler.c

---

## 🛠 Matériel utilisé

- STM32F4 / STM32F1 (compatible HAL)
- Capteur de température : TMP102 / LM75 (I2C) ou capteur interne ADC
- EEPROM 24C32 ou 24C64 (I2C)
- UART vers PC (3.3V)
- Alimentation 3.3V

---

## 🧪 Mise en route
1. Compiler & flasher
Projet compatible STM32CubeIDE / CMake HAL.

2. Lancer un terminal série
115200 bauds
8N1

3. Taper une commande
GET
LAST
CLEAR