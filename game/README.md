# 🚀 Star Squadron

A self-contained, retro-style arcade shooter in the spirit of *Galaga*. Pilot a lone
fighter and hold the line against a swarm of alien invaders.

## Play it

Just open `index.html` in any modern browser (double-click it). No build step, no
dependencies, no internet required — it's a single HTML file.

## Controls

| Action | Keyboard | Touch |
| ------ | -------- | ----- |
| Move   | ← → (or A / D) | drag anywhere |
| Fire   | SPACE | tap |
| Pause  | P | — |
| Start / Retry | any key | tap |

## Features

- Classic Galaga-style **formation** of enemies that sways, plus **dive-bombing**
  attackers that swoop toward you (worth 2× points to shoot mid-dive).
- Three alien tiers with increasing hit points; tougher aliens fill the top rows.
- Escalating **waves** — more frequent and more numerous dives each round.
- **Score, lives, and high score** (saved in your browser via `localStorage`).
- Procedural **sound effects** via the Web Audio API — no audio files needed.
- **Responsive** canvas that scales to the window, with keyboard and touch support.

Everything lives in one file (`index.html`) — HTML, CSS, and JavaScript with zero
external assets — so it's easy to host, share, or drop into any page.
