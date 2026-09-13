# ESP-NOW RC Blimp

A two-ESP32 remote-control **blimp/airship**: a handheld **transmitter** with
joysticks talks to a **receiver** on the gondola over
[ESP-NOW](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html)
(low-latency, connectionless, no WiFi router needed). The receiver drives three
small brushed DC motors through H-bridge motor drivers.

```
   ┌─────────────────────┐        ESP-NOW         ┌──────────────────────┐
   │  TRANSMITTER (hand)  │      2.4 GHz radio     │  RECEIVER (gondola)  │
   │                      │  ~50 packets / second  │                      │
   │  Left stick  X → yaw │ ─────────────────────▶ │  DRV8833 ×2 ──▶ 3 DC │
   │  Left stick  Y → fwd │                        │   motors:            │
   │  Right stick Y → up  │                        │   • left thruster    │
   │  ARM button          │                        │   • right thruster   │
   │  ESP32 dev module    │                        │   • vertical motor   │
   └─────────────────────┘                        └──────────────────────┘
```

## How it flies

The two horizontal thrusters do double duty via **differential thrust**:

| You want to…        | Left thruster | Right thruster |
|---------------------|---------------|----------------|
| Go forward          | forward       | forward        |
| Go backward         | reverse       | reverse        |
| Turn right (yaw)    | forward       | reverse        |
| Turn left (yaw)     | reverse       | forward        |

The mixing math lives in `receiver/receiver.ino`:
`left = forward + yaw`, `right = forward - yaw`. The third motor pushes the
blimp **up or down**. Because helium already provides most of the lift, the
vertical motor only needs to trim altitude — trim your ballast so the blimp is
very slightly heavy ("neutrally buoyant" plus a gram or two).

## Repository layout

```
esp32-blimp/
├── README.md
├── transmitter/
│   ├── transmitter.ino     ← flash to the handheld ESP32
│   └── protocol.h          ← shared packet definition (keep identical!)
├── receiver/
│   ├── receiver.ino        ← flash to the gondola ESP32
│   └── protocol.h          ← shared packet definition (keep identical!)
└── docs/
    ├── parts.md            ← bill of materials + weight budget
    └── wiring.md           ← pin-by-pin wiring tables
```

> `transmitter/protocol.h` and `receiver/protocol.h` must stay **byte-for-byte
> identical**. If you edit one, copy it to the other.

## Quick start

1. **Install the toolchain.** Arduino IDE with the **esp32 by Espressif**
   boards package (this code targets **core 3.x**), or PlatformIO with
   `platform = espressif32`.
2. **Wire it up.** Follow [`docs/wiring.md`](docs/wiring.md) and gather the
   parts in [`docs/parts.md`](docs/parts.md).
3. **Flash the receiver.** Open `receiver/receiver.ino`, select your ESP32 dev
   board, upload. Open the Serial Monitor at **115200** and note the printed
   `MAC:` line (only needed if you later want to pair to one specific blimp).
4. **Flash the transmitter.** Open `transmitter/transmitter.ino`, upload to the
   second ESP32. Leave the joysticks centered while it boots — it calibrates
   stick center at startup.
5. **Fly.** Power both boards. The receiver LED fast-blinks until it hears the
   transmitter, then slow-blinks (safe). Press the **ARM** button on the
   transmitter (its LED goes solid) and the motors are live.

## Safety & failsafe (built in)

- **Arming:** motors never spin until you press ARM on the transmitter. Press
  again to disarm.
- **Signal-loss failsafe:** if the receiver hears nothing for `FAILSAFE_MS`
  (400 ms), it cuts all motors automatically.
- **Soft start:** a slew-rate limiter ramps motor changes so the blimp doesn't
  lurch and to protect the light structure and battery.
- **Props spin!** Always test with props off first, and disarm before handling.

## Pairing (optional)

By default the transmitter **broadcasts** (`peerMac = FF:FF:FF:FF:FF:FF`), which
just works with a single blimp and needs no configuration. To lock a
transmitter to one specific blimp (e.g. flying several at once), copy the
receiver's printed MAC into `peerMac[]` in `transmitter.ino`. Both boards must
also share the same `BLIMP_WIFI_CHANNEL` (set in `protocol.h`).

## Tuning cheat-sheet

| Symptom | Fix | Where |
|---|---|---|
| A motor runs backwards | flip its `*_REVERSE` flag | `receiver.ino` |
| A stick moves the wrong way | flip its `INV_*` flag | `transmitter.ino` |
| Twitchy / hard to hover | raise `EXPO`, raise `DEADBAND` | `transmitter.ino` |
| Motor whines but won't spin | raise `MOTOR_DEADSTART` | `receiver.ino` |
| Feels sluggish to respond | raise `SLEW_PER_UPDATE` | `receiver.ino` |
| Coasts too far on stick release | raise `SLEW_PER_UPDATE` | `receiver.ino` |
| Turns too aggressively | scale `yaw` down in the mixer | `receiver.ino` |

## Notes for Arduino-ESP32 core 2.x

This code uses the **core 3.x** APIs (`ledcAttach(pin, freq, res)` and the
`esp_now_recv_info_t` receive callback). If you are on core 2.x:

- Replace `ledcAttach(pin, freq, res)` with `ledcSetup(ch, freq, res)` +
  `ledcAttachPin(pin, ch)` and use `ledcWrite(ch, duty)` with channel numbers.
- Change the receive callback signature to
  `void onRecv(const uint8_t *mac, const uint8_t *data, int len)`.

Upgrading to core 3.x is the easier path.
