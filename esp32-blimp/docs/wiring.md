# Wiring guide

All pins below match the defaults in the sketches. If you use different GPIOs,
update the `static const int` pin definitions at the top of the `.ino` files.

---

## Receiver (gondola) — ESP32 + 2× DRV8833

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

### Power (receiver)

- **1S LiPo (3.7 V)** → both DRV8833 `VM` pins and the ESP32.
- Feed the ESP32 from the 1S cell into its **3V3 pin** (bypasses the onboard
  regulator; works while the cell is above ~3.3 V) **or** use a tiny boost
  converter to 5 V into `VIN`/`5V` for steadier operation as the cell drains.
- **All grounds must be common:** ESP32 GND, both DRV8833 GNDs, battery −.

> Notes: GPIO25/26/27/14/32/33 are all valid PWM outputs. GPIO13 is used for
> `nSLEEP`. Avoid GPIO6–11 (flash) and GPIO34–39 (input-only) for motor pins.

---

## Transmitter (handheld) — ESP32 + 2 joysticks + button

> **Critical:** ESP-NOW uses the WiFi radio, which disables ADC2. The joystick
> axes **must** use ADC1 pins: **32, 33, 34, 35, 36, 39**.

| Joystick signal | ESP32 pin | Axis |
|-----------------|-----------|------|
| Left stick **VRy** | **GPIO34** | Forward / back |
| Left stick **VRx** | **GPIO35** | Yaw (turn) |
| Right stick **VRy** | **GPIO32** | Vertical up / down |
| Both sticks **VCC** | **3V3** | — |
| Both sticks **GND** | **GND** | — |
| ARM button | **GPIO4** → other leg to **GND** | Toggle arm (uses internal pull-up) |

- Right-stick VRx and the joystick push-switches are unused (free for trims or
  extra features later).
- Leave both sticks **centered** at power-up — the transmitter samples stick
  center during boot.

---

## First-power checklist

1. **Props OFF.** Flash both boards.
2. Power the receiver; its LED should **fast-blink** (no link yet).
3. Power the transmitter; the receiver LED should switch to a **slow blink**
   (linked, disarmed).
4. Press **ARM** — transmitter LED goes solid, receiver LED goes solid.
5. With props still off, verify each motor spins the right way for each stick.
   Fix direction with the `*_REVERSE` (receiver) or `INV_*` (transmitter) flags.
6. Disarm, fit props, balance the blimp to near-neutral buoyancy, then fly.
