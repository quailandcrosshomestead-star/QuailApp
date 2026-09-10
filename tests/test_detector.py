"""Tests for the flash-detection logic (no camera/OpenCV needed)."""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from lightning_trigger.detector import FlashDetector  # noqa: E402


def _feed(detector, values, start=0.0, dt=1 / 30):
    """Feed a list of brightness values; return list of DetectionResults."""
    results = []
    t = start
    for v in values:
        results.append(detector.update(v, t))
        t += dt
    return results


def test_no_trigger_on_steady_scene():
    det = FlashDetector(warmup_frames=5, min_delta=8.0)
    results = _feed(det, [50.0] * 100)
    assert not any(r.triggered for r in results)


def test_no_trigger_on_gradual_change():
    # Slow dusk: brightness drifts down over many frames -> never a spike.
    det = FlashDetector(warmup_frames=5, min_delta=8.0)
    ramp = [80.0 - i * 0.2 for i in range(200)]
    results = _feed(det, ramp)
    assert not any(r.triggered for r in results)


def test_triggers_on_sudden_flash():
    det = FlashDetector(warmup_frames=10, min_delta=8.0, cooldown_seconds=0.0)
    # 30 calm frames, then one bright flash frame, then calm again.
    values = [40.0] * 30 + [200.0] + [40.0] * 10
    results = _feed(det, values)
    fired = [i for i, r in enumerate(results) if r.triggered]
    assert fired == [30], f"expected one trigger at the flash frame, got {fired}"


def test_no_trigger_during_warmup():
    det = FlashDetector(warmup_frames=50, min_delta=8.0)
    values = [40.0] * 10 + [200.0] + [40.0] * 10  # flash lands during warmup
    results = _feed(det, values)
    assert not any(r.triggered for r in results)


def test_cooldown_collapses_flicker_into_one_shot():
    # A strike that flickers bright across several consecutive frames.
    det = FlashDetector(warmup_frames=10, min_delta=8.0, cooldown_seconds=1.0)
    values = [40.0] * 20 + [210.0, 60.0, 205.0, 70.0, 200.0] + [40.0] * 10
    # dt of 1/30s means all flickers fall inside the 1s cooldown.
    results = _feed(det, values)
    assert sum(1 for r in results if r.triggered) == 1


def test_zero_cooldown_fires_each_return_stroke():
    det = FlashDetector(warmup_frames=10, min_delta=8.0, cooldown_seconds=0.0)
    values = [40.0] * 20 + [210.0, 40.0, 205.0, 40.0, 200.0] + [40.0] * 5
    results = _feed(det, values)
    assert sum(1 for r in results if r.triggered) == 3


def test_baseline_not_poisoned_by_flash():
    det = FlashDetector(warmup_frames=10, min_delta=8.0, cooldown_seconds=0.0)
    _feed(det, [50.0] * 30)
    baseline_before = det.baseline
    _feed(det, [230.0], start=100.0)  # a single flash
    # Baseline should barely move because spike frames are excluded.
    assert abs(det.baseline - baseline_before) < 1.0


def test_reset_restarts_warmup():
    det = FlashDetector(warmup_frames=5)
    _feed(det, [50.0] * 20)
    det.reset()
    assert det.baseline == 0.0
    # First frame after reset seeds baseline and is flagged warming_up.
    r = det.update(60.0, 0.0)
    assert r.warming_up and not r.triggered


def test_invalid_params_rejected():
    for kwargs in (
        {"baseline_alpha": 0.0},
        {"baseline_alpha": 1.0},
        {"sensitivity": -1},
        {"min_delta": -1},
        {"warmup_frames": 0},
        {"cooldown_seconds": -1},
    ):
        try:
            FlashDetector(**kwargs)
        except ValueError:
            pass
        else:  # pragma: no cover
            raise AssertionError(f"expected ValueError for {kwargs}")
