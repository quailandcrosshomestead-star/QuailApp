# ⚡ Lightning Trigger

Watch a camera feed for lightning and fire a **tethered DSLR** the instant a
strike is detected. Point a fast preview camera at the sky; when the frame
suddenly brightens, the app trips your DSLR's shutter through
[`gphoto2`](http://www.gphoto.org/).

> This lives alongside the Quail Egg Tracker (`app.py`) in the same repo but is
> a completely separate tool.

## How it works

```
 preview camera ──▶  brightness  ──▶  FlashDetector  ──▶  Trigger  ──▶  DSLR
 (webcam / HDMI       per frame        (spike vs.          (gphoto2)     shutter
  capture / Pi cam)                     adaptive baseline)
```

You use **two cameras**:

1. **The watcher** — any cheap, fast video source (a USB webcam, an HDMI
   capture card fed by the DSLR's clean HDMI out, or a Raspberry Pi camera).
   We only read brightness from it, as fast as possible.
2. **The shooter** — your DSLR, connected by USB and controlled by `gphoto2`,
   which takes the actual high-resolution photo.

The detector keeps a slow-moving **baseline** of "normal" brightness and fires
only when a frame jumps above it by more than an adaptive, noise-scaled
threshold — so drifting clouds and dusk don't trip it, but a bolt does.

### A note on shutter lag ⏱️

No software trigger reading a video feed can beat a DSLR's mechanical shutter
lag for the *first* flash. Lightning helps you here: strikes flicker with
multiple return strokes over ~100–500 ms, and storms fire repeatedly. Two
proven ways to use this tool:

- **Fast trigger mode** (default): `--capture-mode trigger` fires
  `gphoto2 --trigger-capture`, which returns almost instantly and lets the
  camera write to its own card. You stay armed for the next stroke.
- **Continuous exposures**: put the DSLR in bulb/continuous long exposures
  (e.g. 2–4 s at f/8, ISO 100) so the shutter is open much of the time, and
  use the log/preview to confirm which exposures caught a strike.

## Install

```bash
pip install -r requirements-lightning.txt      # opencv-python
# and the gphoto2 CLI (not a pip package):
sudo apt install gphoto2      # Debian/Ubuntu/Raspberry Pi
brew install gphoto2          # macOS
```

Verify your DSLR is detected and supports triggering:

```bash
gphoto2 --auto-detect
gphoto2 --trigger-capture      # should take a photo
```

If your camera isn't recognized, close any other app using it (macOS Photos,
Android File Transfer, etc.) and re-plug it.

## Run

Test the whole pipeline with **no camera and no hardware** first:

```bash
python -m lightning_trigger --trigger dry-run --source 0 --verbose
```

Real run against webcam index 0, firing the DSLR, watching only the top half of
the frame (the sky), logging every strike:

```bash
python -m lightning_trigger \
    --source 0 \
    --roi 0,0,1,0.5 \
    --trigger gphoto2 --capture-mode trigger \
    --log-file strikes.csv
```

Download each shot straight to disk instead of the card:

```bash
python -m lightning_trigger --capture-mode download --output-dir captures/
```

Fire something other than a DSLR (GPIO shutter cable, custom script, webhook):

```bash
python -m lightning_trigger --trigger command \
    --command "python fire_shutter_gpio.py --tag {tag}"
```

Press **Ctrl-C** (or `q` in the preview window) to stop.

## Tuning

Run with `--verbose` and watch the `b` (brightness), `base` (baseline) and
`thr` (threshold) columns to dial these in for your scene:

| Flag | Meaning | If you get… |
|------|---------|-------------|
| `--sensitivity` | Noise multiplier for the adaptive threshold. | **Missed strikes?** lower it (e.g. `3`). **False fires?** raise it (e.g. `6`). |
| `--min-delta` | Absolute brightness jump always required (0–255). | Raise on a noisy/dark feed to stop sensor-noise fires. |
| `--baseline-alpha` | How fast "normal" adapts (higher = slower). | Lower slightly if fast-moving clouds cause fires. |
| `--cooldown` | Seconds between shots. | `0` fires on every return stroke; raise to get one shot per strike. |
| `--roi X,Y,W,H` | Region to watch, as fractions 0–1. | Use `0,0,1,0.5` to ignore a bright horizon/streetlight. |
| `--warmup-frames` | Frames spent learning the baseline before arming. | Raise if the first seconds after start misfire. |

## Files

| Path | What |
|------|------|
| `lightning_trigger/detector.py` | Pure-Python flash detection (no OpenCV) |
| `lightning_trigger/camera.py`   | OpenCV video-source wrapper + brightness |
| `lightning_trigger/trigger.py`  | DSLR / dry-run / shell-command backends |
| `lightning_trigger/config.py`   | CLI arguments |
| `lightning_trigger/app.py`      | Main detect-and-fire loop |
| `tests/test_detector.py`        | Detection unit tests (`pytest tests/`) |

## Safety

Storm photography puts you and your gear near lightning. Shoot from indoors, a
vehicle, or a safe covered distance — never operate exposed equipment in the
open during an active storm.
