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
