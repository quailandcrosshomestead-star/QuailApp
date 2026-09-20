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
  - **Turner** pulses every 4 hours (12 s per pulse), and **stops at lockdown
    (day 15)**. At lockdown the humidity target is automatically raised to
    **65 %RH**.
- **Manual mode** (`autoMode = false`): the `heater`, `mister`, `fan` and
  `turner` dashboard toggles drive their relays directly.
- **Turn Now** (`turnNow`): triggers one immediate turn pulse in any mode.
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
