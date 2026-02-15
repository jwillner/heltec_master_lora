# FSD — LoRa Master–Client Timeslot-Protokoll (v0.1)

## 1. Ziel
Ein LoRa **Master** fragt mehrere **Clients** in einem festen **Zeitfenster (Frame)** ab.  
Der Master sendet pro Client ein Token/Request. Jeder Client darf **nur in seinem Timeslot** antworten:
- **OK** (keine Neuigkeiten)
- oder **Status/Update** (wenn es Neuigkeiten gibt)

Der Master liefert eine **Referenzzeit**, mit der Clients ihre lokale Uhr **synchronisieren**, sodass die Timeslots zuverlässig eingehalten werden.

**Initialparameter (v0.1):**
- Frame-Dauer: **5 Minuten (300 s)**
- Anzahl Timeslots: **10**
- Client i antwortet in Timeslot i (0..9)
- Master pollt alle 10 Clients pro Frame (oder eine Teilmenge)

---

## 2. Begriffe & Grundkonzept

### 2.1 Frame und Timeslots
- Ein **Frame** ist ein wiederkehrendes Zeitfenster fester Länge (hier 300 s).
- Ein Frame ist in **N Timeslots** geteilt (hier 10).
- Jeder Timeslot hat Länge:  
  `slot_len_ms = frame_len_ms / N`  
  → 300000 ms / 10 = **30000 ms (30 s)**

### 2.2 Referenzzeit (Master Time)
Der Master stellt die Referenzzeit bereit als:
- `master_epoch_ms` (Monotoner Zeitstempel in ms; muss nicht Unix-Zeit sein)
- plus `frame_id` bzw. `frame_start_epoch_ms`

Clients berechnen daraus:
- aktuellen Frame
- ihren Slot-Start
- Zeitpunkt zum Senden innerhalb des Slots

---

## 3. Anforderungen (Functional Requirements)

### 3.1 Master
1. Master startet Frames zyklisch alle 5 Minuten.
2. Master sendet zu Beginn jedes Client-Slots einen **POLL** (Token/Request) an den jeweiligen Client.
3. Master enthält in jedem POLL:
   - Referenzzeitdaten zur Synchronisation
   - Frame-Parameter (Frame-Länge, Slot-Anzahl)
   - Slot-Index des adressierten Clients
4. Master hört während des Slot-Fensters auf eine Antwort.
5. Master akzeptiert:
   - `OK` (keine Neuigkeiten)
   - `STATUS` (Client liefert Nutzdaten)
6. Master erkennt Timeouts, CRC/Parsing-Fehler, Duplicate-Frames und loggt Ergebnisse.

### 3.2 Client
1. Client empfängt POLL vom Master.
2. Client synchronisiert lokale Zeitbasis (Offset/Drift-Modell v0.1 minimal).
3. Client sendet Antwort **nur innerhalb seines Slots** (mit Guard-Zeiten).
4. Wenn keine Neuigkeiten: sendet `OK`.
5. Wenn Neuigkeiten: sendet `STATUS` mit Payload.
6. Client ignoriert POLLs, die nicht an ihn adressiert sind.

---

## 4. Nicht-Funktionale Anforderungen
- Robust gegen verlorene Pakete (LoRa typisch).
- Keine harte Echtzeit, aber Slot-Grenzen sollen i.d.R. eingehalten werden.
- Minimaler Overhead (kleine Pakete).
- Erweiterbar (mehr Clients, mehr Slots, variable Frame-Länge).

---

## 5. Systemparameter (v0.1 Default)
| Parameter | Wert |
|---|---:|
| Frame-Länge | 300 s |
| Frame-Länge ms | 300000 ms |
| Timeslots | 10 |
| Slot-Länge | 30 s |
| Guard vor/nach Slot | 500 ms (empfohlen) |
| Antwort-Delay im Slot | zufällig 100..300 ms (Kollisionsvermeidung bei Sonderfällen) |
| LoRa SF/BW/CR | Projekt-abhängig (vorerst wie bestehendes P2P Setup) |

**Guard-Zeiten:**  
Client sendet in: `[slot_start + guard_pre, slot_end - guard_post]`

---

## 6. Adressierung & Rollen

### 6.1 Device IDs
- Master hat `master_id` (1 Byte oder 2 Byte)
- Client hat `client_id` (1 Byte) in Bereich 0..9 (für 10 Slots)
- Slot-Index == client_id (v0.1)

### 6.2 Broadcast vs. Unicast
- POLL ist **unicast** an einen Client (oder “logical unicast” via client_id im Payload).
- Optional: Master kann zusätzlich zu Frame-Start ein **SYNC_BROADCAST** senden (v0.2), v0.1 nicht nötig.

---

## 7. Nachrichtenformat (Binary, little-endian)

### 7.1 Paket-Header (gemeinsam)
Alle Frames beginnen mit:

| Feld | Typ | Größe | Beschreibung |
|---|---|---:|---|
| preamble | u16 | 2 | 0xA55A (Erkennung) |
| version | u8 | 1 | 0x01 |
| msg_type | u8 | 1 | siehe 7.2 |
| master_id | u8 | 1 | ID des Masters |
| client_id | u8 | 1 | Ziel-Client (bei POLL) oder Sender-Client (bei Antworten) |
| frame_id | u16 | 2 | Laufende Frame-Nummer (rollover ok) |
| seq | u16 | 2 | Nachrichten-Sequence (pro Sender) |
| payload_len | u8 | 1 | Länge Payload in Bytes |
| payload | bytes | n | msg-spezifisch |
| crc16 | u16 | 2 | CRC über alles außer crc16 (optional wenn LoRa CRC genutzt wird; v0.1 optional) |

**Hinweis:** Wenn die SX1262/LoRa PHY bereits CRC aktiviert hat, kann `crc16` in v0.1 entfallen. Dann muss `crc16` Feld raus, und `payload_len` reicht.

### 7.2 Message Types
| msg_type | Name | Richtung |
|---:|---|---|
| 0x01 | POLL | Master → Client |
| 0x02 | OK | Client → Master |
| 0x03 | STATUS | Client → Master |
| 0x04 | NACK | Client → Master (optional v0.1) |

---

## 8. POLL Payload (Master → Client)

| Feld | Typ | Größe | Beschreibung |
|---|---|---:|---|
| master_epoch_ms | u32 | 4 | monotone Master-Zeit in ms (Modulo 2^32 ok) |
| frame_len_ms | u32 | 4 | z.B. 300000 |
| slot_count | u8 | 1 | z.B. 10 |
| slot_index | u8 | 1 | 0..9 (soll == client_id sein) |
| slot_len_ms | u32 | 4 | z.B. 30000 (redundant, aber praktisch) |
| flags | u8 | 1 | bitfield (z.B. ack_req, reserved) |

**Interpretation:**  
`master_epoch_ms` ist die Masterzeit zum Zeitpunkt des Sendens (t_tx).  
Client nutzt Empfangszeit `t_rx_local` und setzt Offset:

`offset_ms = master_epoch_ms - t_rx_local`  (Modulo/Wrap beachten)

Damit kann Client masterbasierte Zeit approximieren:
`master_now_ms ≈ local_now_ms + offset_ms`

---

## 9. OK Payload (Client → Master)
Minimalantwort ohne Neuigkeiten:

| Feld | Typ | Größe | Beschreibung |
|---|---|---:|---|
| client_epoch_ms | u32 | 4 | lokale Zeit (oder master-synced Zeit) zum Sendezeitpunkt |
| status_code | u8 | 1 | 0 = ok |
| rssi_last | i8 | 1 | optional: letzter RSSI (falls verfügbar) |
| snr_last | i8 | 1 | optional: letzter SNR (falls verfügbar) |

---

## 10. STATUS Payload (Client → Master)
Antwort mit Nutzdaten. v0.1 definiert generisch:

| Feld | Typ | Größe | Beschreibung |
|---|---|---:|---|
| client_epoch_ms | u32 | 4 | Zeit beim Senden |
| data_type | u8 | 1 | 0x01=state, 0x02=sensor, 0x03=event, ... |
| data_len | u8 | 1 | Länge von data |
| data | bytes | n | Nutzdaten |

**Beispiele `data`:**
- `state`: z.B. bitfield (door open, motion, battery low)
- `event`: event_id + value
- `sensor`: strukturierte Messwerte (später versionieren)

---

## 11. Zeitsynchronisation (v0.1)

### 11.1 Einfaches Offset-Modell
Client berechnet bei jedem gültigen POLL:
- `offset_ms_new = master_epoch_ms - local_rx_ms`

Dann glättet Client den Offset (um Jitter zu reduzieren):
- `offset_ms = offset_ms * 0.8 + offset_ms_new * 0.2` (Beispiel)

### 11.2 Slot-Berechnung
Client berechnet:
- `frame_start_master_ms = master_epoch_ms - (master_epoch_ms % frame_len_ms)`  
  (Frame-Grenze auf Masterzeit)
- `slot_start = frame_start_master_ms + slot_index * slot_len_ms`
- `slot_end = slot_start + slot_len_ms`

Client sendet, wenn:
`master_now_ms in [slot_start + guard_pre, slot_end - guard_post]`

---

## 12. Ablauf (Sequenz)

### 12.1 Pro Frame (Master)
Für slot_index 0..9:
1. Master sendet `POLL(client_id=slot_index, frame_id=current_frame)`
2. Master wechselt in RX und wartet bis:
   - Antwort kommt, oder
   - Timeout = (slot_len_ms - guard_post) (z.B. 28 s)
3. Master protokolliert Ergebnis.

### 12.2 Client-Reaktion
1. Client ist in RX (oder wacht zyklisch auf; v0.1 immer RX ok).
2. Client empfängt `POLL`, prüft:
   - preamble, version
   - client_id == eigene ID
3. Client aktualisiert Zeitsync.
4. Client wartet bis innerhalb Slot-Fenster (falls POLL früh kommt).
5. Client sendet `OK` oder `STATUS`.

---

## 13. Timeouts, Retries, Duplicate Handling

### 13.1 Timeouts (Master)
- Keine Antwort bis Slot-Ende ⇒ `missed`
- Optional v0.2: Retry innerhalb Slot (z.B. 1 Wiederholung nach 2 s)

### 13.2 Duplicate/Out-of-Frame (Master)
- Master akzeptiert Antworten nur, wenn `frame_id == current_frame_id`
- Antworten mit anderem frame_id loggen, aber ignorieren.

### 13.3 Duplicate/Old POLLs (Client)
Client merkt sich:
- `last_frame_id`
- `last_seq_master`
und ignoriert:
- POLLs mit `frame_id < last_frame_id` (Modulo beachten)
- oder exakt gleiche `(frame_id, seq)` mehrfach

---

## 14. Kollisionsvermeidung
Da pro Slot nur ein Client senden soll, sind Kollisionen selten.
Trotzdem:
- Client verzögert innerhalb Slot zufällig `rand(100..300ms)` vor dem Senden
- Guard-Zeiten einhalten

---

## 15. Logging & Telemetrie (empfohlen)
### 15.1 Master Log pro Slot
- frame_id, slot_index, poll_tx_time, response_rx_time
- result: OK/STATUS/timeout/parse_error
- rssi/snr (wenn verfügbar)

### 15.2 Client Log
- letzte Sync-Parameter (offset_ms)
- letzte frame_id/seq
- gesendeter Typ (OK/STATUS) + reason

---

## 16. Konfigurierbarkeit
`config.h` (v0.1) soll mindestens enthalten:
- `MASTER_ID`
- `CLIENT_ID` (bei Client-FW)
- `FRAME_LEN_MS` (Default 300000)
- `SLOT_COUNT` (Default 10)
- `GUARD_PRE_MS`, `GUARD_POST_MS`
- optional `ENABLE_CRC16`
- LoRa PHY Settings (Frequenz, SF, BW, CR, TX Power)

---

## 17. Akzeptanzkriterien (v0.1)
1. Master sendet innerhalb 5 Minuten genau 10 POLLs (je Slot 1).
2. Jeder Client antwortet nur in seinem Slot.
3. Wenn Client keine Neuigkeiten hat, sendet er OK, Master zeigt OK an.
4. Wenn Client Neuigkeiten hat, sendet er STATUS mit Payload, Master empfängt und loggt.
5. Zeitsync funktioniert so, dass Client-Sendungen nicht systematisch außerhalb Slot landen (unter normalen LoRa-Latenzen).

---

## 18. Offene Punkte / v0.2 Ideen
- Dynamische Client-Liste (mehr als 10, Slot-Zuordnung via Table)
- Broadcast-SYNC am Frame-Start
- Retry-Mechanismus
- Kompensation von Drift (lineares Drift-Modell)
- Security: simple auth tag / rolling code / AES (wenn nötig)
- Duty-cycle / Sleep-Mode: Client schläft und wacht kurz vor seinem Slot auf

---
## 19. Beispielwerte (v0.1)
- Frame 300000 ms, Slots 10, Slot 30000 ms
- Guard 500/500 ms
- Client 3 sendet im Zeitfenster:
  `frame_start + 3*30000 + 500` bis `frame_start + 4*30000 - 500`

---
## 20. Minimaler Implementierungsfahrplan (Micro-Steps)
1. Master: Zeitbasis + Slot-Schleife + POLL senden (ohne Antworten auszuwerten)
2. Client: POLL empfangen + ID check + sofort OK senden (ohne Timeslot-Delay)
3. Client: Timeslot-Delay + Guard + OK im Slot
4. Master: RX + OK empfangen + log
5. STATUS Payload + einfache “Neuigkeit”-Flag Simulation

