# MISRA C:2012 — Dérogations documentées  
## Module : cli_uart

---

## CLI-DEV-001 — MISRA C:2012 Rule 15.5  
**Rule**: A function should have a single point of exit at the end of the function.  
**Severity**: Advisory  

### Description  
La fonction `CliUart_TxChar()` contenait initialement plusieurs instructions `return` selon le statut HAL retourné.

### Justification  
Le code a été modifié afin de respecter strictement la règle 15.5 :
- un unique point de sortie,
- une variable de statut locale,
- une logique de décision claire et déterministe.

La version actuelle est conforme MISRA et ne nécessite plus de dérogation.

### Status  
✔ **Corrigé – conforme MISRA**

---

## CLI-DEV-002 — MISRA C:2012 Rule 8.7  
**Rule**: Functions should be declared static if they are not used outside of their translation unit.  
**Severity**: Advisory  

### Description  
La fonction `CliUart_OnOverflow()` est déclarée avec l’attribut `weak` et n’est pas utilisée directement en dehors de son unité de traduction selon l’analyse Cppcheck.

### Justification  
Cette fonction est volontairement déclarée comme **callback weak**, afin de permettre à l’application de surcharger son comportement sans modifier le module `cli_uart`.  
Ce mécanisme est intentionnel et identique à celui utilisé dans le module `scheduler`.

Le fait que l’outil d’analyse statique ne détecte pas d’appel externe est dû à une analyse partielle du projet.

### Status  
⚠ **Dérogation acceptée – callback weak volontaire**

---

## CLI-DEV-003 — Avertissements "unusedFunction" (outil)  
**Tool**: Cppcheck  

### Description  
Plusieurs fonctions publiques (`CliUart_Init`, `CliUart_OnRxChar`, `CliUart_Process`, `CliUart_TxChar`) sont signalées comme non utilisées.

### Justification  
Ces avertissements sont dus à une analyse isolée du fichier `cli_uart.c`.  
Les fonctions sont appelées :
- depuis la boucle principale,
- depuis les callbacks UART (ISR / HAL).

Elles constituent l’API publique du module et ne doivent pas être déclarées `static`.

### Status  
⚠ **Dérogation acceptée – limitation de l’analyse statique**

---

## CLI-DEV-004 — Avertissements HAL / prototypes inconnus  
**Rules concerned**: 17.3, configuration  

### Description  
Les appels aux fonctions HAL (`HAL_UART_Transmit`, `HAL_OK`) sont signalés comme violations ou inconnus.

### Justification  
Les chemins d’inclusion HAL et CMSIS ne sont pas fournis à Cppcheck lors de l’analyse.  
Ces avertissements sont des faux positifs liés à la configuration de l’outil et non à une violation du code.

### Status  
⚠ **Dérogation acceptée – configuration outillage**

---

## Conclusion

Le module `cli_uart` est :
- conforme aux règles MISRA C:2012 pertinentes,
- robuste en environnement ISR / coopératif,
- extensible par callbacks weak,
- et prêt pour intégration applicative.

Toutes les dérogations restantes sont **justifiées, tracées et acceptables** dans un contexte embarqué industriel.

