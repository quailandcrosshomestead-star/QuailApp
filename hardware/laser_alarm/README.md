# 🔦 ESP32-C3 Laser Tripwire Security Alarm

A simple, reliable laser tripwire for a coop entrance, gate, doorway, or feed
room. A laser points across the opening onto a photoresistor (LDR). When
something breaks the beam, a buzzer sounds. Everything runs on the ESP32-C3 —
**no WiFi or internet required**, so it works anywhere you have power.

---

## 🧰 Parts list

| Part | Notes | Qty |
|------|-------|-----|
| **ESP32-C3** dev board | SuperMini or DevKitM-1 both fine | 1 |
| **Laser diode module** | KY-008 (5 V, red dot) is ideal — has its own resistor | 1 |
| **Photoresistor (LDR)** | Any GL5516 / GL5528 type | 1 |
| **10 kΩ resistor** | For the LDR voltage divider | 1 |
| **Buzzer** | Active buzzer (recommended) or passive piezo | 1 |
| Breadboard + jumper wires | | — |
| USB-C cable + 5 V USB power | Phone charger works great | 1 |

> **Laser safety:** Use a low-power (≤5 mW, Class 2/3R) red laser like the KY-008.
> Never aim it at eyes — mount it at ankle/knee height across a doorway, or angle
> it low across the coop threshold.

---

## 🔌 Wiring

### 1. Laser diode (KY-008)
| KY-008 pin | ESP32-C3 |
|------------|----------|
| `S` (signal) | **GPIO4** |
| middle (VCC) | **5V** (or 3V3) |
| `-` (GND) | **GND** |

*(Bare laser diode instead of KY-008? Put a 100 Ω resistor in series with its +
lead, drive it from GPIO4, other leg to GND.)*

### 2. LDR light sensor — voltage divider
```
   3V3 ──[ LDR ]──┬──[ 10kΩ ]── GND
                  │
                GPIO2   (analog input)
```
- LDR between **3V3** and **GPIO2**
- 10 kΩ resistor between **GPIO2** and **GND**

With the laser hitting the LDR, GPIO2 reads **high**. When the beam is broken the
LDR's resistance climbs and the reading **drops** — that's the trip.

### 3. Buzzer
| Buzzer | ESP32-C3 |
|--------|----------|
| `+` / signal | **GPIO5** |
| `-` / GND | **GND** |

Active buzzer: leave `PASSIVE_BUZZER = false` (default).
Passive piezo: set `PASSIVE_BUZZER = true` in the sketch for a wailing siren.

### 4. Reset / status (built-in, no wiring)
- **BOOT button (GPIO9)** — press to silence a sounding alarm and re-arm.
- **Onboard LED (GPIO8)** — blinks while the alarm is active.

### Pin summary
| Function | GPIO |
|----------|------|
| Laser drive | 4 |
| LDR analog in | 2 |
| Buzzer | 5 |
| Status LED (onboard) | 8 |
| Reset button (onboard BOOT) | 9 |

---

## 💻 Flashing the firmware

1. Install the **Arduino IDE** (or use PlatformIO / `arduino-cli`).
2. In Arduino IDE: **File → Preferences → Additional Boards Manager URLs**, add:
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. **Tools → Board → Boards Manager** → install **esp32** by Espressif.
4. Select board: **Tools → Board → ESP32C3 Dev Module** (or your specific board).
5. Open `laser_alarm.ino`, plug in the board, pick the port, click **Upload**.
   - If upload fails, hold **BOOT**, tap **RESET**, release **BOOT**, retry.

---

## ✅ Using it

1. Mount the laser so its dot lands squarely on the LDR across the opening.
2. Power on. On boot it **auto-calibrates for ~1 second** — keep the path clear
   and the beam on the sensor. Two beeps = **armed**.
3. Break the beam → the alarm sounds. It **latches on** and auto-silences after
   30 seconds, or press **BOOT** to reset immediately.

### Tuning (open the Serial Monitor at 115200 baud)
The board prints `light=` and `threshold=` values once per second while armed.

| Want to... | Change (top of the sketch) |
|------------|----------------------------|
| Make it **less** sensitive (fewer false trips) | lower `TRIP_FRACTION` (e.g. `0.45`) |
| Make it **more** sensitive | raise `TRIP_FRACTION` (e.g. `0.75`) |
| Ignore bugs/dust flicker | raise `TRIP_CONFIRM_MS` (e.g. `100`) |
| Alarm stays on longer / forever | raise `ALARM_DURATION_MS` (`0` = never auto-stop) |

---

## 🧩 Troubleshooting

| Symptom | Fix |
|---------|-----|
| `[WARN] Baseline is very low` | Laser not hitting the LDR, or LDR/laser mis-wired. Re-align, re-check divider. |
| Alarm never trips | Beam is bright enough that broken beam still > threshold — raise `TRIP_FRACTION`, or shade the LDR from room/sun light. |
| Constant false alarms | Ambient light changing, or beam wobbling. Shield the LDR in a short tube, lower `TRIP_FRACTION`, raise `TRIP_CONFIRM_MS`. |
| Buzzer just clicks / stays silent | You likely have a **passive** piezo — set `PASSIVE_BUZZER = true`. |
| Sun/daylight blinds the sensor | Put the LDR inside a black straw/heat-shrink tube pointed only at the laser. |

---

## 🚀 Where to take it next
Your ESP32-C3 has WiFi, so this can grow into:
- A **phone push notification** on trip (ntfy / Telegram) for a remote coop.
- Logging trip events into the **QuailApp** as a security history tab.

Say the word and I'll add either.
