# Démo MCP9808 — Résumé des tests (UART)

## Contexte
- Scheduler_Task : 10 ms
- Échantillonnage capteur : 100 ms (sample_div = 10)
- Seuil STALE : 1000 ms
- FAULT : latched (verrouillé) jusqu’à action de recovery explicite

---

## 1) Test nominal (succès)
**Log :**
- `Sensor init OK`
- `TEMP_NOT_AVAILABLE / st_temp=1` au démarrage (pas encore de mesure valide)
- Puis valeurs stables autour de `20.9°C` à `21.0°C`

**Conclusion :**
- Communication I2C OK
- Conversion température OK
- Mesure cohérente avec température ambiante (~21°C)
- Le module ne “fabrique” pas de valeur au boot (comportement safety)

---

## 2) Test débranchement en cours d’exécution
**Log :**
- Dernière mesure : `T=21.2C`
- Puis `FAULT latched` + `latched=3`

**Conclusion :**
- Détection panne I2C correcte
- FAULT verrouillé (latched) correctement, pas de reprise automatique

---

## 3) Test changement d’adresse (capteur non joignable)
**Log :**
- `Sensor init OK` puis (après changement) `Sensor init FAIL`
- `TEMP_NOT_AVAILABLE / st_temp=1`

**Conclusion :**
- Init détecte correctement une adresse invalide / capteur absent
- Pas de mesure disponible (comportement attendu)

---

## 4) Test STALE via période d’échantillonnage trop longue (sample_div = 200)
**Log :**
- `Sensor init OK`
- 1ère mesure : `T=21.9C`
- Puis `st_temp=5` (STALE) entre deux mesures
- Nouvelle mesure : `T=21.9C`, puis à nouveau `st_temp=5`

**Conclusion :**
- STALE fonctionne : la donnée devient “trop vieille” si la période de mesure (2 s) dépasse le seuil STALE (1 s)
- Le log texte doit distinguer `STALE` vs `TEMP_NOT_AVAILABLE` (amélioration démo)

---

## Statut global
✅ Nominal OK  
✅ FAULT latched OK (débranchement)  
✅ Init FAIL OK (adresse invalide)  
✅ STALE OK (période > seuil)

## Test restant à valider :
- Recovery
