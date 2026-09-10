"""Lightning flash detection.

The detector is deliberately free of any camera or OpenCV dependency: it works
on a single scalar brightness value per frame.  That keeps the core algorithm
fast, portable and easy to unit-test without hardware.

Algorithm
---------
Lightning appears in a video feed as a sudden, short-lived spike in overall
brightness against an otherwise slowly-changing background (dusk, moving
clouds, city glow).  We therefore:

1. Track a slow-moving baseline of "normal" brightness with an exponential
   moving average (EMA).
2. Track how much the brightness typically wanders using an EMA of the
   absolute deviation from that baseline (a robust stand-in for std-dev).
3. Flag a frame as a flash when its brightness jumps above the baseline by
   more than both an absolute floor and an adaptive, noise-scaled threshold.
4. Freeze the baseline while a flash is happening so a bright bolt does not
   poison the "normal" estimate, and apply a short cooldown so the many
   flickers of one strike do not each fire the camera.
"""

from __future__ import annotations

from dataclasses import dataclass


@dataclass
class DetectionResult:
    """Outcome of feeding one frame's brightness to the detector."""

    brightness: float
    baseline: float
    delta: float          # brightness - baseline
    threshold: float      # the delta required to trigger on this frame
    triggered: bool       # True only on the frame that fires the camera
    warming_up: bool      # True while the baseline is still being established


class FlashDetector:
    """Detects sudden brightness spikes characteristic of lightning.

    Parameters
    ----------
    baseline_alpha:
        Smoothing factor for the brightness baseline EMA (0-1).  Higher =
        slower to adapt.  0.9 tracks gradual light changes without chasing a
        flash.
    sensitivity:
        Multiplier on the measured brightness noise.  The adaptive part of the
        threshold is ``sensitivity * noise``.  Lower = more sensitive (more
        triggers), higher = stricter.
    min_delta:
        Absolute brightness jump (on the same 0-255 scale as the input) that
        must always be exceeded, regardless of how quiet the scene is.  Stops
        a dead-calm night from firing on sensor noise.
    warmup_frames:
        Number of frames used to seed the baseline before any triggering is
        allowed.
    cooldown_seconds:
        Minimum time between two triggers.  One strike flickers many times;
        this collapses that into a single fire.  Set to 0 to fire on every
        qualifying frame (e.g. to catch individual return strokes).
    """

    def __init__(
        self,
        baseline_alpha: float = 0.9,
        sensitivity: float = 4.0,
        min_delta: float = 8.0,
        warmup_frames: int = 20,
        cooldown_seconds: float = 0.5,
    ) -> None:
        if not 0.0 < baseline_alpha < 1.0:
            raise ValueError("baseline_alpha must be between 0 and 1 (exclusive)")
        if sensitivity < 0:
            raise ValueError("sensitivity must be >= 0")
        if min_delta < 0:
            raise ValueError("min_delta must be >= 0")
        if warmup_frames < 1:
            raise ValueError("warmup_frames must be >= 1")
        if cooldown_seconds < 0:
            raise ValueError("cooldown_seconds must be >= 0")

        self.baseline_alpha = baseline_alpha
        self.sensitivity = sensitivity
        self.min_delta = min_delta
        self.warmup_frames = warmup_frames
        self.cooldown_seconds = cooldown_seconds

        self._baseline: float | None = None
        self._noise: float = 0.0
        self._frames_seen = 0
        self._last_trigger_time: float | None = None

    @property
    def baseline(self) -> float:
        """Current brightness baseline (0.0 before the first frame)."""
        return self._baseline or 0.0

    def reset(self) -> None:
        """Forget all history and start warming up again."""
        self._baseline = None
        self._noise = 0.0
        self._frames_seen = 0
        self._last_trigger_time = None

    def update(self, brightness: float, now: float) -> DetectionResult:
        """Feed one frame's mean brightness and get a detection result.

        ``now`` is a monotonic timestamp in seconds (e.g. ``time.monotonic()``)
        used only for the cooldown; passing it in keeps this class free of any
        implicit clock so tests stay deterministic.
        """
        self._frames_seen += 1

        # First frame seeds the baseline directly.
        if self._baseline is None:
            self._baseline = brightness
            return DetectionResult(
                brightness=brightness,
                baseline=self._baseline,
                delta=0.0,
                threshold=float("inf"),
                triggered=False,
                warming_up=True,
            )

        delta = brightness - self._baseline
        threshold = max(self.min_delta, self.sensitivity * self._noise)
        warming_up = self._frames_seen <= self.warmup_frames

        is_spike = delta > threshold
        triggered = False

        if is_spike and not warming_up and self._cooldown_elapsed(now):
            triggered = True
            self._last_trigger_time = now

        # Only fold the frame back into the baseline/noise estimates when it is
        # *not* a spike, so a flash never contaminates "normal".
        if not is_spike:
            a = self.baseline_alpha
            self._baseline = a * self._baseline + (1.0 - a) * brightness
            self._noise = a * self._noise + (1.0 - a) * abs(delta)

        return DetectionResult(
            brightness=brightness,
            baseline=self._baseline,
            delta=delta,
            threshold=threshold,
            triggered=triggered,
            warming_up=warming_up,
        )

    def _cooldown_elapsed(self, now: float) -> bool:
        if self._last_trigger_time is None:
            return True
        return (now - self._last_trigger_time) >= self.cooldown_seconds
