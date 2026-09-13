# Bill of materials

## Receiver (gondola) — the flying part

| Qty | Part | Notes |
|----:|------|-------|
| 1 | ESP32 dev module | Any ESP32-WROOM DevKit. See **weight** below — a bare module or an ESP32-C3 super-mini is much lighter. |
| 2 | **DRV8833** dual H-bridge breakout | Each chip drives 2 motors. One drives L+R thrusters, the second drives the vertical motor (one channel spare). |
| 3 | Small brushed DC motors | Coreless "8520"/"7×16 mm" or N20 motors with light props/props-on-shaft. Match to your envelope's lift. |
| 3 | Propellers | Sized to the motors; light plastic. |
| 1 | 1S LiPo, ~150–500 mAh | Size for your lift budget; more mAh = more weight. |
| 1 | LiPo protection/charge board (optional) | e.g. TP4056 for charging; add low-voltage cutoff to protect the cell. |
| — | Light silicone wire (28–30 AWG), heat-shrink, tape | Keep it light. |

## Transmitter (handheld) — stays on the ground

| Qty | Part | Notes |
|----:|------|-------|
| 1 | ESP32 dev module | Weight doesn't matter here. |
| 2 | 2-axis analog thumb joysticks | KY-023 / PS2-style modules. We use 3 of the 4 axes. |
| 1 | Momentary push button | ARM toggle (wired to GND). |
| 1 | Power source | USB power bank, or 2×18650 + regulator. |
| — | Perfboard / 3D-printed case (optional) | For a real "controller" feel. |

## ⚖️ Weight is everything on a blimp

Helium lifts roughly **1 gram per liter** of envelope volume (at sea level,
minus the envelope's own weight). So your **entire flying payload** — ESP32 +
2× DRV8833 + 3 motors + props + battery + wire — must be under your envelope's
net lift. Weigh everything and size the envelope to match.

Rough flying-side weights (verify with a scale):

| Item | Approx. weight |
|---|---|
| ESP32-WROOM DevKit (full board) | ~9–10 g |
| **Bare ESP32-WROOM module** | ~3 g |
| **ESP32-C3 super-mini board** | ~3–4 g |
| DRV8833 breakout (×2) | ~1–2 g each |
| Coreless motor + prop (×3) | ~2–5 g each |
| 1S 150–260 mAh LiPo | ~4–8 g |
| Wire / tape / gondola | ~3–8 g |

A full ESP32 DevKit build lands around **35–55 g**, which needs a **large**
envelope (roughly a 90–110 cm foil "party" blimp, or several joined). If your
envelope is small, switch the flying board to a **bare WROOM module or an
ESP32-C3 super-mini** — the firmware is the same, only the GPIO pin numbers in
`receiver.ino` need updating for the C3's pinout.

**Trim to neutral buoyancy:** add small ballast (tape, putty) until the blimp
neither rises nor sinks on its own — then remove a hair so it's *just* slightly
heavy. The vertical motor then only nudges altitude, which is far easier to fly.
