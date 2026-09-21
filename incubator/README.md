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

- An **ESP32-WROOM 4-channel relay board** with an onboard AC/DC power supply
  (the integrated board pictured in this build). Any Arduino IoT Cloud–compatible
  board works, but the pin map and flashing steps below are for this ESP32 board.
- A **USB-to-serial (CH340/CP2102) adapter** to flash it — this board has no USB
  port of its own.
- A **DHT22 (AM2302)** temperature/humidity sensor
- Loads for the four relays: heater, mister/humidifier, circulation fan and the
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

### Pin map (ESP32 relay board — edit at the top of `incubator.ino`)

| Signal | ESP32 GPIO | Relay channel |
|--------|-----------|---------------|
| Heater | `GPIO32` | CH1 |
| Mister | `GPIO33` | CH2 |
| Fan | `GPIO25` | CH3 |
| Turner | `GPIO26` | CH4 |
| DHT22 data | `GPIO4` | — (any free header GPIO) |

Wire each load to the screw terminals of the matching relay channel (use the
**NO** + **COM** contacts so the load is off when the relay is de-energised).

This board is driven **active-high** (`RELAY_ACTIVE_LOW false` in the sketch:
GPIO HIGH = relay energised). **Verify this before wiring the heater** — see the
polarity test below.

## Board setup (ESP32)

### 1. Flash it over the USB-serial adapter

This board has no USB port, so you upload through the CH340/CP2102 adapter.

1. **Set the adapter to 3.3 V logic** (jumper), not 5 V — 5 V can damage the ESP32.
2. Wire adapter → board:
   - `TXD` (adapter) → `RX` (ESP32)
   - `RXD` (adapter) → `TX` (ESP32)
   - `GND` → `GND`
   - `3V3` → `3V3` (powers the ESP32 for flashing)
3. **Do not apply AC mains while flashing** — power the logic from the adapter's
   3.3 V only.
4. Put the ESP32 in download mode: hold **IO0/BOOT** to GND, tap reset (or
   power up), then release IO0.
5. In the Arduino IDE: install the **esp32 by Espressif** boards package, select
   **ESP32 Dev Module**, and set upload speed to **115200** (drop lower if the
   CH340 is flaky). Install the **ArduinoIoTCloud**, **Arduino_ConnectionHandler**,
   and **Adafruit DHT** (+ **Adafruit Unified Sensor**) libraries.
6. Upload. When it's done, disconnect IO0 from GND and reset.

### 2. Verify relay polarity BEFORE wiring anything dangerous

With **no heater or mains connected**, power the board and watch the relay LEDs /
listen for clicks:

- At boot, **all four relays must read OFF** (LEDs off, no click). If any relay
  is energised at boot, flip `RELAY_ACTIVE_LOW` in the sketch, re-flash, and
  re-check.
- Toggle `heater` on from the dashboard: only **CH1** should energise. Repeat for
  mister/fan/turner (CH2/CH3/CH4).

Only once the OFF-at-boot behaviour and channel mapping are confirmed should you
wire the heater and the AC turner motor.

## How it works

- **Auto mode** (`autoMode = true`, the default): the board regulates
  everything.
  - **Heater** cycles with hysteresis around `tempSetpoint`
    (default **37.5 °C / ~99.5 °F**).
  - **Mister** cycles with hysteresis around `humiditySetpoint`
    (default **50 %RH**).
  - **Fan** runs continuously for even, forced-air circulation.
  - **Turner** runs on an interval — the motor is powered for a set run-time
    (default **12 s**, tune `TURN_RUN_MS`) every **3 h** (`TURN_INTERVAL_S`) —
    and **stops at lockdown (day 15)**. At lockdown the humidity target is
    automatically raised to **65 %RH**.
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

### 3. Register the ESP32 in Arduino IoT Cloud

An ESP32 is a "manually configured" device in IoT Cloud:

1. **Devices → Add → Third-party device → ESP32** (pick ESP32 Dev Module). You'll
   be given a **Device ID** and a **Secret Key** — save the Secret Key now, it's
   shown only once.
2. Put the **Device ID** in `DEVICE_LOGIN_NAME` (in `thingProperties.h`) and the
   **Secret Key** in `SECRET_DEVICE_KEY` (in `arduino_secrets.h`), along with your
   2.4 GHz Wi-Fi name/password (the ESP32 has no 5 GHz radio).
3. Create/open the **Quail Incubator** Thing with the variables listed in
   `thingProperties.h`, and associate it with this device.
4. Build a dashboard with widgets bound to each variable (gauges for
   temperature/humidity, switches for the relays, buttons for `turnNow` /
   `resetCycle`).

> ⚠️ Always verify readings against a calibrated thermometer/hygrometer before
> trusting the incubator with real eggs.
