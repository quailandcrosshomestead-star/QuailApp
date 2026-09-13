# Bill of materials

You control this blimp from your **phone**, so there's only one board to
build — the ESP32 on the gondola. No second ESP32, no joystick modules, no
transmitter.

## What you already have (salvaged from the old drone) ✅

| Qty | Part | Notes |
|----:|------|-------|
| 1 | **1S LiPo battery — 3.7 V 600 mAh** | Slide-in "brick" pack; solder leads to its contacts. Powers the ESP32 + motors. |
| 3 | **Brushed coreless motors** (you have 4 — one spare) | Silver cylinders from the quadcopter. Direction is set electrically, so any 3 work. |
| 3 | **Propellers** | Reuse the drone props; lighten if needed. |
| — | Wire + small connectors | Harvested from the drone. |
| — | *(optional)* nav LEDs | The little bulbs on wires, if you want a status light. |

> Not reusable — recycle: the drone's flight-controller board, camera/FPV board,
> antenna, and the original gamepad. The ESP32 replaces all of it.

## What to buy

| Qty | Part | Notes |
|----:|------|-------|
| **1** | ESP32 dev module | ESP32-WROOM DevKit with an onboard PCB antenna (avoid the "-32U" external-antenna version). See **weight** below. |
| **2** | **DRV8833** dual H-bridge breakout | One drives the L+R thrusters, the second drives the vertical motor (one channel spare). |
| **1** | TP4056 charger (with DW01A protection) | To recharge the salvaged 1S cell safely. |
| **1 kit** | Dupont jumper wires | For connecting the DRV8833 boards and bench-testing. |
| **1–2** | 36" round foil balloon (+ helium) | The envelope. See lift math below. |
| — | Solder + iron, USB cable to match the ESP32 | To assemble and flash. |

**No longer needed (vs. the old two-ESP32 plan):** the second ESP32, the 2
joystick modules, and the push-button kit — the controls are all on your phone.

## ⚖️ Weight is everything on a blimp

Helium lifts roughly **1 gram per liter** of envelope volume (minus the
envelope's own weight). Your **entire flying payload** — ESP32 + 2× DRV8833 +
3 motors + props + battery + wire — must be under your envelope's net lift.
Weigh everything and size the envelope to match.

Rough flying-side weights (verify with a scale):

| Item | Approx. weight |
|---|---|
| ESP32-WROOM DevKit (full board) | ~9–10 g |
| **Bare ESP32-WROOM module** (lighter option) | ~3 g |
| **ESP32-C3 super-mini board** (lighter option) | ~3–4 g |
| DRV8833 breakout (×2) | ~1–2 g each |
| Salvaged coreless motor + prop (×3) | ~3–5 g each |
| Salvaged 1S 600 mAh LiPo | ~15–20 g |
| Wire / tape / gondola | ~3–8 g |

A full ESP32 DevKit build lands around **45–60 g**. A 36" round foil balloon
inflates to ~28" ≈ **~180 L → ~150 g net lift**, so a single one comfortably
carries this build. If you switch to a smaller envelope, drop the flying board
to a **bare WROOM module or ESP32-C3 super-mini** (same firmware; just update
the GPIO pin numbers in `blimp.ino` for the C3 pinout) and use a smaller
150–300 mAh cell to shed weight.

**Trim to neutral buoyancy:** add small ballast (tape, putty) until the blimp
neither rises nor sinks on its own — then remove a hair so it's *just* slightly
heavy. The vertical motor then only nudges altitude, which is far easier to fly.
