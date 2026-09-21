# 🐣 Quail Incubator (Arduino IoT Cloud)

A networked incubator controller for quail eggs, built on the
[Arduino IoT Cloud](https://create.arduino.cc/cloud). The board reads a
temperature/humidity sensor and drives four relays (heater, mister, fan and
egg-turner) to hold the eggs at target conditions, reporting live state to a
Cloud dashboard. It shares the incubation timeline with the companion
**Quail Egg Manager** Streamlit app (18-day hatch, day-15 lockdown).

## Files

| File | Purpose |
|------|---------|
| `incubator.ino` | Main sketch: sensing + control logic |
| `thingProperties.h` | Cloud variable declarations & registration |
| `arduino_secrets.h` | Wi-Fi credentials (placeholders — do not commit real ones) |

## Hardware

- An Arduino IoT Cloud–compatible board (Nano 33 IoT, MKR WiFi 1010, ESP32, …)
- A **DHT22 (AM2302)** temperature/humidity sensor
- A 4-channel relay board for: heater, mister/humidifier, circulation fan,
  egg-turner motor

### Egg-turner motor (mains AC)

This build uses a **TY-50AF synchronous gear motor** — **AC 110–120 V**,
50/60 Hz, ≤4 W, ~2.5–3 rpm, bidirectional. It is a continuous-rotation turner:
the motor drives the tray until a mechanical end-stop, then auto-reverses (or a
cam carries it around), so **direction is handled by the mechanism, not the
Arduino** (it is a 2-wire motor). The controller only switches its line power
on/off.

> ⚠️ **Mains safety.** The turner relay switches **110–120 V AC**. Use a relay
> module rated for mains (≥250 VAC, e.g. 10 A), switch the motor's **line**
> conductor through it, and keep all mains wiring fully isolated and insulated
> from the Arduino's low-voltage side. The Arduino provides only a dry contact
> and never touches mains voltage. If you are not comfortable wiring mains,
> get someone qualified to do it.

### Default pin map (edit at the top of `incubator.ino`)

| Signal | Pin |
|--------|-----|
| DHT22 data | `D2` |
| Heater relay | `D3` |
| Mister relay | `D4` |
| Fan relay | `D5` |
| Turner relay | `D6` |

Most low-cost relay boards are **active-low**; the sketch assumes this via
`RELAY_ACTIVE_LOW true`. Set it to `false` if your relays energise on HIGH.

## How it works

- **Auto mode** (`autoMode = true`, the default): the board regulates
  everything.
  - **Heater** cycles with hysteresis around `tempSetpoint`
    (default **37.5 °C / ~99.5 °F**).
  - **Mister** cycles with hysteresis around `humiditySetpoint`
    (default **50 %RH**).
  - **Fan** runs continuously for even, forced-air circulation.
  - **Turner** runs on an interval — the motor is powered for a set run-time
    (default **4 min**, tune `TURN_RUN_MS`) every **4 h** (`TURN_INTERVAL_S`),
    long enough to guarantee a full side-to-side traverse — and **stops at
    lockdown (day 15)**. At lockdown the humidity target is automatically raised
    to **65 %RH**.
- **Manual mode** (`autoMode = false`): the `heater`, `mister`, `fan` and
  `turner` dashboard toggles drive their relays directly.
- **Turn Now** (`turnNow`): triggers one immediate timed turn run in any mode.
- **Reset Cycle** (`resetCycle`): restarts the incubation clock at day 1.
- **Out of range** (`outOfRange`): set when temperature or humidity drifts more
  than 1 °C / 10 %RH from target, or when the sensor read fails.
- **Fail-safe**: if the sensor read fails, the heater and mister are forced off
  and only the fan runs, so a bad sensor can never cook the eggs.

Day counting uses the Cloud/RTC time, so it survives reboots as long as the
board keeps its network time. Setpoints are clamped to safe ranges
(temp 30–40 °C, humidity 20–90 %) whenever changed from the dashboard.

## Setup

1. In the Arduino IoT Cloud editor, create/open the **Quail Incubator** Thing
   with the variables listed in `thingProperties.h`.
2. Copy this sketch's logic into that Thing (or use these files with
   `arduino-cli`), and set your Wi-Fi credentials in `arduino_secrets.h`.
3. Install the **Adafruit DHT sensor library** (and its dependency, the Adafruit
   Unified Sensor library).
4. Flash the board, then build a dashboard with widgets bound to each variable
   (gauges for temperature/humidity, switches for the relays, buttons for
   `turnNow` / `resetCycle`).

> ⚠️ Always verify readings against a calibrated thermometer/hygrometer before
> trusting the incubator with real eggs.
