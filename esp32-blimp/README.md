# Phone-Controlled ESP32 Blimp

A remote-control **blimp/airship** flown from your **phone**. A single ESP32 on
the gondola creates its own WiFi hotspot and serves a touchscreen controller to
your phone's browser — no app to install, no transmitter to build. The ESP32
drives three small brushed DC motors through H-bridge motor drivers.

```
   ┌──────────────┐   join WiFi        ┌───────────────────────────┐
   │  Your phone  │  "Blimp-01"        │  ESP32 on the blimp       │
   │  browser UI  │ ◀──── hotspot ────▶│  • WiFi access point      │
   │  2 touch     │  WebSocket ~20 Hz  │  • serves control page    │
   │  joysticks   │                    │  • DRV8833 ×2 → 3 motors  │
   │  + ARM       │                    │    left / right / vertical│
   └──────────────┘                    └───────────────────────────┘
```

## How it flies

The two horizontal thrusters do double duty via **differential thrust**:

| You want to…        | Left thruster | Right thruster |
|---------------------|---------------|----------------|
| Go forward          | forward       | forward        |
| Go backward         | reverse       | reverse        |
| Turn right (yaw)    | forward       | reverse        |
| Turn left (yaw)     | reverse       | forward        |

The mixing math lives in `blimp/blimp.ino`: `left = forward + yaw`,
`right = forward - yaw`. The third motor pushes the blimp **up or down**.
Because helium already provides most of the lift, the vertical motor only trims
altitude — balance your ballast so the blimp is very slightly heavy.

### Phone controls

- **Left joystick** — up/down = throttle (forward/back), left/right = turn.
- **Right joystick** — up/down = altitude (ascend/descend).
- **ARM button** — motors won't spin until you press it (turns red when armed).

Both sticks spring back to center when you lift your finger, so releasing =
stop / hover.

A **battery voltage** pill sits in the top bar. It turns amber below ~3.6 V and
red below ~3.4 V — land and recharge when it goes red to avoid over-draining
the 1S cell (and browning out the ESP32 mid-flight).

## Repository layout

```
esp32-blimp/
├── README.md
├── blimp/
│   └── blimp.ino       ← the only firmware; flash this to the ESP32
└── docs/
    ├── parts.md        ← bill of materials + weight budget
    └── wiring.md       ← pin-by-pin wiring tables
```

## Quick start

1. **Install the toolchain.** Arduino IDE with the **esp32 by Espressif**
   boards package (this code targets **core 3.x**).
2. **Install two libraries** (Arduino IDE → *Tools → Manage Libraries…*):
   - **ESP Async WebServer** (by ESP32Async / me-no-dev)
   - **Async TCP** (by ESP32Async)
3. **Wire it up.** Follow [`docs/wiring.md`](docs/wiring.md); parts are in
   [`docs/parts.md`](docs/parts.md).
4. **Flash.** Open `blimp/blimp.ino`, select your ESP32 board, upload. Open the
   Serial Monitor at **115200** — it prints the hotspot name and
   `http://192.168.4.1`.
5. **Fly.**
   - On your phone, join the WiFi network **`Blimp-01`** (password
     **`flyblimp`** — change both at the top of `blimp.ino`).
   - Your phone may warn *"network has no internet — stay connected?"* → tap
     **stay connected**.
   - Open a browser to **`http://192.168.4.1`**.
   - Press **ARM**, then fly with the two joysticks.

## Safety & failsafe (built in)

- **Arming:** motors never spin until you press ARM in the browser.
- **Signal-loss failsafe:** if the ESP32 hears nothing for 500 ms (phone
  locks, browser closes, out of range) it cuts all motors automatically.
- **Soft start:** a slew-rate limiter ramps motor changes so the blimp doesn't
  lurch.
- **Props spin!** Test with props off first, and disarm before handling.

## Range & where to fly

WiFi hotspot range is roughly **30–50 m** outdoors with line of sight, less
through walls. That's a great fit for a blimp, which flies best **indoors** in
still air anyway (gyms, large rooms, atriums). Outdoors, fly only in near-calm
conditions — a light breeze will push a blimp around more than the motors can
correct.

## Tuning cheat-sheet

| Symptom | Fix | Where |
|---|---|---|
| A motor runs backwards | flip its `*_REVERSE` flag | `blimp.ino` |
| Twitchy / hard to hover | raise the `expo` value in the page JS | `blimp.ino` |
| Motor whines but won't spin | raise `MOTOR_DEADSTART` | `blimp.ino` |
| Feels sluggish to respond | raise `SLEW_PER_UPDATE` | `blimp.ino` |
| Coasts too far on release | raise `SLEW_PER_UPDATE` | `blimp.ino` |
| Want a different hotspot name/password | edit `AP_SSID` / `AP_PASS` | `blimp.ino` |
| Battery reading is off vs. a multimeter | tweak `BATT_CAL` (or `BATT_R1`/`R2`) | `blimp.ino` |

## Notes for Arduino-ESP32 core 2.x

This code uses **core 3.x** LEDC APIs (`ledcAttach(pin, freq, res)` +
`ledcWrite(pin, duty)`). On core 2.x, use `ledcSetup(ch, freq, res)` +
`ledcAttachPin(pin, ch)` and write by channel number instead. Upgrading to core
3.x is the easier path.
