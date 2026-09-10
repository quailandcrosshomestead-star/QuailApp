# 🎮 Quail Galaga

A self-contained, retro-style arcade shooter in the spirit of *Galaga* — themed for the
homestead. You pilot a quail gunship and defend the covey from waves of diving hawks.

## Play it

Two ways:

1. **Standalone** — just open `quail-galaga.html` in any modern browser (double-click it).
   No build step, no dependencies, no internet required.
2. **Inside the app** — run the Streamlit app and open the **🎮 Quail Galaga** tab:
   ```bash
   streamlit run app.py
   ```

## Controls

| Action | Keyboard | Touch |
| ------ | -------- | ----- |
| Move   | ← → (or A / D) | drag anywhere |
| Fire   | SPACE | tap |
| Pause  | P | — |
| Start / Retry | any key | tap |

## Features

- Classic Galaga-style **formation** of enemies that sways, plus **dive-bombing** attackers
  that swoop toward you (worth 2× points to shoot mid-dive).
- Three hawk tiers with increasing hit points; tougher hawks fill the top rows.
- Escalating **waves** — more frequent and more numerous dives each round.
- **Score, lives, and high score** (saved in your browser via `localStorage`).
- Procedural **sound effects** via the Web Audio API — no audio files needed.
- **Responsive** canvas that scales to the window and supports touch devices.

Everything lives in one HTML file (`quail-galaga.html`) — HTML, CSS, and JavaScript with
zero external assets — so it's easy to host, share, or drop into any page.
