"""Command-line configuration for the lightning trigger app."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path


@dataclass
class Config:
    source: str | int
    width: int
    height: int
    roi: tuple[float, float, float, float] | None
    baseline_alpha: float
    sensitivity: float
    min_delta: float
    warmup_frames: int
    cooldown: float
    trigger: str          # "gphoto2" | "dry-run" | "command"
    capture_mode: str     # "trigger" | "download"  (gphoto2 only)
    command: str | None
    output_dir: Path
    log_file: Path | None
    preview: bool
    verbose: bool


def _roi(value: str) -> tuple[float, float, float, float]:
    parts = value.split(",")
    if len(parts) != 4:
        raise argparse.ArgumentTypeError("ROI must be 'x,y,w,h' as fractions 0-1")
    try:
        x, y, w, h = (float(p) for p in parts)
    except ValueError:
        raise argparse.ArgumentTypeError("ROI values must be numbers")
    return (x, y, w, h)


def _source(value: str) -> str | int:
    # A bare integer means a local camera index; anything else is a URL/path.
    return int(value) if value.isdigit() else value


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="lightning_trigger",
        description=(
            "Watch a camera feed for lightning and fire a tethered DSLR the "
            "moment a strike is detected."
        ),
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    src = p.add_argument_group("video source (the feed we watch)")
    src.add_argument(
        "--source", type=_source, default=0,
        help="Camera index (0,1,...) or a video file / RTSP-HTTP stream URL.",
    )
    src.add_argument("--width", type=int, default=0, help="Requested capture width (0 = default).")
    src.add_argument("--height", type=int, default=0, help="Requested capture height (0 = default).")
    src.add_argument(
        "--roi", type=_roi, default=None, metavar="X,Y,W,H",
        help="Watch only this region (fractions 0-1), e.g. 0,0,1,0.5 for the top half (sky).",
    )

    det = p.add_argument_group("detection tuning")
    det.add_argument("--baseline-alpha", type=float, default=0.9, help="Baseline smoothing (higher = slower).")
    det.add_argument("--sensitivity", type=float, default=4.0, help="Noise multiplier (lower = more sensitive).")
    det.add_argument("--min-delta", type=float, default=8.0, help="Minimum brightness jump (0-255 scale).")
    det.add_argument("--warmup-frames", type=int, default=20, help="Frames to learn the baseline before arming.")
    det.add_argument("--cooldown", type=float, default=0.5, help="Seconds between shots (0 = every flicker).")

    trg = p.add_argument_group("camera trigger (what we fire)")
    trg.add_argument(
        "--trigger", choices=["gphoto2", "dry-run", "command"], default="gphoto2",
        help="How to capture: tethered DSLR, a no-op test, or a shell command.",
    )
    trg.add_argument(
        "--capture-mode", choices=["trigger", "download"], default="trigger",
        help="gphoto2 only: 'trigger' (fast, saves to card) or 'download' (saves to disk).",
    )
    trg.add_argument(
        "--command", default=None,
        help="For --trigger command: shell command to run; '{tag}' is replaced with the timestamp.",
    )

    out = p.add_argument_group("output")
    out.add_argument("--output-dir", type=Path, default=Path("captures"), help="Where downloaded photos are saved.")
    out.add_argument("--log-file", type=Path, default=None, help="Append a CSV row per strike to this file.")
    out.add_argument("--preview", action="store_true", help="Show a live preview window (needs a display).")
    out.add_argument("--verbose", action="store_true", help="Print per-frame brightness diagnostics.")

    return p


def parse_args(argv: list[str] | None = None) -> Config:
    args = build_parser().parse_args(argv)
    if args.trigger == "command" and not args.command:
        build_parser().error("--trigger command requires --command")
    return Config(
        source=args.source,
        width=args.width,
        height=args.height,
        roi=args.roi,
        baseline_alpha=args.baseline_alpha,
        sensitivity=args.sensitivity,
        min_delta=args.min_delta,
        warmup_frames=args.warmup_frames,
        cooldown=args.cooldown,
        trigger=args.trigger,
        capture_mode=args.capture_mode,
        command=args.command,
        output_dir=args.output_dir,
        log_file=args.log_file,
        preview=args.preview,
        verbose=args.verbose,
    )
