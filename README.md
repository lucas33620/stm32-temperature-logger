# 📡 Mini Station Température & Data Logger — STM32

Mini-station embarquée permettant de mesurer une température, l’enregistrer périodiquement dans une mémoire non volatile, et récupérer l’historique des mesures au format JSON via UART.

Ce projet démontre une **architecture modulaire claire**, des **drivers bas niveau** (I2C, SPI, UART), un **scheduler coopératif simple**, et une **interface CLI série**, représentatifs d’un mini-système embarqué industrialisable.

---

## 🎯 Objectifs

- Mesurer périodiquement la température  
  (capteur interne ADC ou capteur externe I2C)
- Stocker chaque mesure dans une **SPI Flash (QSPI capable) W25Q128 – 16 MB**, utilisée en **mode SPI 1-bit**
- Fournir une interface série **UART CLI** pour interroger la station
- Exporter les données sous forme **JSON** (`GET`, `LAST`, `CLEAR`)
- Démontrer une architecture claire et évolutive  
  (**Sensor / Logger / Scheduler / CLI**)

---

## 🗂 Arborescence du projet

/Core
  /Inc
    main.h
  /Src
    main.c

/HalPort
  i2c_hal.h
  i2c_hal.c
  critical_hal.h
  critical_hal.c

/Modules
  /mcp9808
    mcp9808.h
    mcp9808.c
    mcp9808_types.h 
    mcp9808_cfg.h
    mcp9808_port.h
    mcp9808_port_stm32_hal.c
    README.md

/App
  /mcp9808
    app_mcp9808.h
    app_mcp9808.c
  app_main.h
  app_main.c
  app_cfg.h
  app_tasks.c

/Services
  /scheduler
    scheduler.h
    scheduler.c

---

## 🛠 Matériel utilisé

- **STM32F4** (compatible HAL)
- **Capteur de température** :
  - LM75 (I2C)
  - broches : SCL : PB8 | SDA : PB9
- **Mémoire non volatile** :
  - SPI Flash **W25Q128 – 16 MB**  
    (composant QSPI, utilisé ici en **SPI 1-bit**)
  - broches : SCK : PA5 | MISO : PA6 | MOSI : PA7 | PA4 en GPIO_Output
- **UART vers PC** (3.3 V) broche PD8 : RX | PD9 : TX
- **Alimentation 3.3 V**

---

## 🧪 Mise en route

### 1️⃣ Compiler & flasher
Projet compatible **STM32CubeIDE** (HAL).

### 2️⃣ Lancer un terminal série
- 115200 bauds  
- 8N1  

### 3️⃣ Taper une commande CLI
GET
LAST
CLEAR

Les données sont renvoyées via l’UART au format texte / JSON simplifié.
