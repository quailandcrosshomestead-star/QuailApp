"""Main loop: read frames, detect flashes, fire the camera."""

from __future__ import annotations

import csv
import signal
import time
from datetime import datetime
from pathlib import Path

from .camera import Camera, region_brightness, _import_cv2
from .config import Config, parse_args
from .detector import FlashDetector
from .trigger import CommandTrigger, DryRunTrigger, GPhoto2Trigger, Trigger


def build_trigger(config: Config) -> Trigger:
    if config.trigger == "dry-run":
        return DryRunTrigger()
    if config.trigger == "command":
        assert config.command is not None  # guaranteed by parse_args
        return CommandTrigger(config.command)
    return GPhoto2Trigger(
        output_dir=config.output_dir,
        mode=config.capture_mode,
    )


def _log_strike(log_file: Path, when: datetime, result) -> None:
    new_file = not log_file.exists()
    with log_file.open("a", newline="") as fh:
        writer = csv.writer(fh)
        if new_file:
            writer.writerow(["timestamp", "brightness", "baseline", "delta", "threshold"])
        writer.writerow([
            when.isoformat(timespec="milliseconds"),
            f"{result.brightness:.2f}",
            f"{result.baseline:.2f}",
            f"{result.delta:.2f}",
            f"{result.threshold:.2f}",
        ])


def run(config: Config) -> int:
    detector = FlashDetector(
        baseline_alpha=config.baseline_alpha,
        sensitivity=config.sensitivity,
        min_delta=config.min_delta,
        warmup_frames=config.warmup_frames,
        cooldown_seconds=config.cooldown,
    )
    trigger = build_trigger(config)

    stopping = {"flag": False}

    def _handle_signal(signum, frame):  # noqa: ARG001
        stopping["flag"] = True

    signal.signal(signal.SIGINT, _handle_signal)
    signal.signal(signal.SIGTERM, _handle_signal)

    cv2 = _import_cv2() if config.preview else None

    print(
        f"Watching source {config.source!r} | trigger={config.trigger} | "
        f"cooldown={config.cooldown}s | Ctrl-C to stop."
    )

    strikes = 0
    frames = 0
    started = time.monotonic()

    with Camera(config.source, config.width, config.height) as cam:
        while not stopping["flag"]:
            try:
                frame = cam.read()
            except RuntimeError as exc:
                print(f"[camera] {exc}")
                break

            frames += 1
            brightness = region_brightness(frame, config.roi)
            result = detector.update(brightness, time.monotonic())

            if config.verbose:
                state = "warmup" if result.warming_up else "armed "
                print(
                    f"[{state}] b={result.brightness:6.2f} "
                    f"base={result.baseline:6.2f} d={result.delta:+6.2f} "
                    f"thr={result.threshold:6.2f}"
                )

            if result.triggered:
                strikes += 1
                now = datetime.now()
                tag = now.strftime("%Y%m%d_%H%M%S_") + f"{now.microsecond // 1000:03d}"
                started_ok = trigger.fire(tag)
                status = "FIRED" if started_ok else "busy, dropped"
                print(
                    f"⚡ Strike #{strikes} at {now.strftime('%H:%M:%S.%f')[:-3]} "
                    f"(brightness {result.brightness:.1f} vs baseline "
                    f"{result.baseline:.1f}) -> {status}"
                )
                if config.log_file:
                    _log_strike(config.log_file, now, result)

            if config.preview and cv2 is not None:
                label = f"strikes: {strikes}  brightness: {brightness:.0f}"
                cv2.putText(frame, label, (10, 30), cv2.FONT_HERSHEY_SIMPLEX,
                            0.8, (0, 255, 255), 2)
                if result.triggered:
                    cv2.rectangle(frame, (0, 0), (frame.shape[1] - 1, frame.shape[0] - 1),
                                  (0, 0, 255), 8)
                cv2.imshow("lightning_trigger", frame)
                if cv2.waitKey(1) & 0xFF == ord("q"):
                    break

        if config.preview and cv2 is not None:
            cv2.destroyAllWindows()

    trigger.close()
    elapsed = time.monotonic() - started
    fps = frames / elapsed if elapsed > 0 else 0.0
    print(
        f"\nStopped. {frames} frames in {elapsed:.1f}s ({fps:.1f} fps), "
        f"{strikes} strike(s) detected, {trigger.fire_count} fired, "
        f"{trigger.drop_count} dropped."
    )
    return 0


def main(argv: list[str] | None = None) -> int:
    config = parse_args(argv)
    try:
        return run(config)
    except (FileNotFoundError, RuntimeError, ImportError) as exc:
        print(f"Error: {exc}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
