# Wiring guide

There is only **one board to wire** now — the ESP32 on the blimp. Your phone is
the controller, so there's no transmitter to build. All pins below match the
defaults at the top of `blimp/blimp.ino`; change the code if you use other GPIOs.

---

## ESP32 + 2× DRV8833 + 3 motors

### DRV8833 #1 → Left & Right thrusters

| DRV8833 #1 pin | Connects to | Purpose |
|----------------|-------------|---------|
| AIN1 | ESP32 **GPIO25** | Left thruster PWM A |
| AIN2 | ESP32 **GPIO26** | Left thruster PWM B |
| BIN1 | ESP32 **GPIO27** | Right thruster PWM A |
| BIN2 | ESP32 **GPIO14** | Right thruster PWM B |
| AOUT1/AOUT2 | Left motor terminals | — |
| BOUT1/BOUT2 | Right motor terminals | — |
| VCC (logic) | ESP32 **3V3** | Logic supply |
| VM (motor) | Battery **+** (1S LiPo) | Motor supply |
| GND | Common ground | Tie ESP32 GND + battery − together |
| nSLEEP | ESP32 **GPIO13** | Enable/failsafe (or tie to 3V3) |

### DRV8833 #2 → Vertical motor

| DRV8833 #2 pin | Connects to | Purpose |
|----------------|-------------|---------|
| AIN1 | ESP32 **GPIO32** | Vertical PWM A |
| AIN2 | ESP32 **GPIO33** | Vertical PWM B |
| AOUT1/AOUT2 | Vertical motor terminals | — |
| BIN1/BIN2/BOUT* | *(unused, spare channel)* | free for a 4th motor later |
| VCC (logic) | ESP32 **3V3** | Logic supply |
| VM (motor) | Battery **+** | Motor supply |
| GND | Common ground | Same ground as everything else |
| nSLEEP | ESP32 **GPIO13** | Share with DRV8833 #1, or tie to 3V3 |

### Battery voltage sense (for the phone readout)

A 1S LiPo reaches 4.2 V — above the ESP32's ~3.3 V ADC limit — so read it
through a **two-resistor divider** that halves the voltage. Use two equal
resistors (100 kΩ each works well and wastes almost no current).

```
   Battery +  ──[ R1 100kΩ ]──┬──[ R2 100kΩ ]── GND
                              │
                          GPIO34 (BATT_PIN, ADC1)
```

| Connection | Notes |
|-----------|-------|
| Battery **+** → R1 → sense node | R1 = 100 kΩ (top) |
| sense node → **GPIO34** | ADC1 input-only pin; must be ADC1 (ADC2 is dead while WiFi runs) |
| sense node → R2 → **GND** | R2 = 100 kΩ (bottom) |

Tap R1 at the **raw battery +** (same node as the DRV8833 `VM`), before any
boost/regulator, so you read true cell voltage. If your reading is a little off
versus a multimeter, adjust `BATT_CAL` in `blimp.ino`. Using different resistor
values? Set `BATT_R1` / `BATT_R2` to match.

### Power

- **1S LiPo (3.7 V)** → both DRV8833 `VM` pins and the ESP32.
- Feed the ESP32 from the 1S cell into its **3V3 pin** (bypasses the onboard
  regulator; works while the cell is above ~3.3 V) **or** use a tiny boost
  converter to 5 V into `VIN`/`5V` for steadier operation as the cell drains.
- **All grounds must be common:** ESP32 GND, both DRV8833 GNDs, battery −.

> GPIO25/26/27/14/32/33 are all valid PWM outputs; GPIO13 drives `nSLEEP`.
> Avoid GPIO6–11 (flash) and GPIO34–39 (input-only) for motor pins. Because
> there are no joysticks anymore, the ADC-pin restrictions no longer apply.

---

## First-power checklist

1. **Props OFF.** Flash `blimp.ino`. The Serial Monitor (115200) prints the
   hotspot name and `http://192.168.4.1`.
2. On your phone, join WiFi **`Blimp-01`** (password `flyblimp`). Tap **stay
   connected** if it warns there's no internet.
3. Open a browser to **`http://192.168.4.1`** — you should see two joysticks
   and an ARM button; the status reads *connected*.
4. Press **ARM** (button turns red). With props still off, nudge each stick and
   confirm each motor spins the correct way. Fix any with the `*_REVERSE`
   flags in `blimp.ino`.
5. **DISARM**, fit props, balance the blimp to near-neutral buoyancy (slightly
   heavy), then fly.

### Status LED (onboard, GPIO2)
- **Fast blink** — no phone connected (failsafe, motors off)
- **Slow blink** — phone connected but disarmed (safe)
- **Solid** — armed, motors live
