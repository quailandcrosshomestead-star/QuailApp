"""Video source used to *watch* for lightning.

This is the fast preview feed (a webcam, a capture card fed by the DSLR's HDMI
out, or a Raspberry Pi camera) -- not the DSLR that takes the final photo.  We
only need brightness out of it, as quickly as possible.

OpenCV is imported lazily so the rest of the package (and its tests) work
without it installed.
"""

from __future__ import annotations

from typing import Optional


def _import_cv2():
    try:
        import cv2  # noqa: PLC0415
    except ImportError as exc:  # pragma: no cover - depends on environment
        raise ImportError(
            "OpenCV is required to read the camera. Install it with "
            "`pip install opencv-python` (or opencv-python-headless on a "
            "server/Raspberry Pi)."
        ) from exc
    return cv2


def region_brightness(frame, roi: Optional[tuple[float, float, float, float]] = None) -> float:
    """Mean brightness (0-255) of ``frame``, optionally within an ROI.

    ``roi`` is ``(x, y, w, h)`` in fractions of frame size (0-1), letting you
    watch just the sky and ignore a bright horizon or a streetlight.
    """
    cv2 = _import_cv2()
    if roi is not None:
        h, w = frame.shape[:2]
        x0, y0, rw, rh = roi
        x1 = int(max(0, min(1, x0)) * w)
        y1 = int(max(0, min(1, y0)) * h)
        x2 = int(max(0, min(1, x0 + rw)) * w)
        y2 = int(max(0, min(1, y0 + rh)) * h)
        if x2 <= x1 or y2 <= y1:
            raise ValueError(f"ROI {roi} is empty for a {w}x{h} frame")
        frame = frame[y1:y2, x1:x2]
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    return float(gray.mean())


class Camera:
    """Thin wrapper over ``cv2.VideoCapture`` with a context manager."""

    def __init__(self, source: str | int = 0, width: int = 0, height: int = 0) -> None:
        self.source = source
        self.width = width
        self.height = height
        self._cap = None

    def open(self) -> "Camera":
        cv2 = _import_cv2()
        self._cap = cv2.VideoCapture(self.source)
        if not self._cap.isOpened():
            raise RuntimeError(
                f"Could not open camera source {self.source!r}. "
                "Check the index/URL and that nothing else is using it."
            )
        if self.width:
            self._cap.set(cv2.CAP_PROP_FRAME_WIDTH, self.width)
        if self.height:
            self._cap.set(cv2.CAP_PROP_FRAME_HEIGHT, self.height)
        return self

    def read(self):
        """Return the next frame (BGR ndarray) or raise if the feed ended."""
        if self._cap is None:
            raise RuntimeError("Camera is not open; call open() first")
        ok, frame = self._cap.read()
        if not ok:
            raise RuntimeError("Failed to read a frame from the camera")
        return frame

    def release(self) -> None:
        if self._cap is not None:
            self._cap.release()
            self._cap = None

    def __enter__(self) -> "Camera":
        return self.open()

    def __exit__(self, *exc) -> None:
        self.release()
