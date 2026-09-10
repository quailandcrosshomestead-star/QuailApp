"""Camera trigger backends.

A "trigger" is the action taken the moment a flash is detected.  The default
fires a tethered DSLR through the ``gphoto2`` command-line tool, but the same
interface covers a dry run (for testing without hardware) and an arbitrary
shell command (for anything gphoto2 cannot do, e.g. toggling a GPIO pin or an
opto-isolated shutter cable on a Raspberry Pi).

Triggers fire on a background worker thread so a slow camera never stalls the
detection loop.  If a fire is requested while the previous one is still running
it is dropped rather than queued -- when catching lightning, the freshest
moment matters more than a backlog.
"""

from __future__ import annotations

import shutil
import subprocess
import threading
import time
from abc import ABC, abstractmethod
from pathlib import Path


class Trigger(ABC):
    """Base class: fire the camera asynchronously, never blocking the caller."""

    def __init__(self) -> None:
        self._lock = threading.Lock()
        self._busy = False
        self.fire_count = 0
        self.drop_count = 0

    @abstractmethod
    def _capture(self, tag: str) -> None:
        """Perform the actual (blocking) capture.  Runs on a worker thread."""

    def fire(self, tag: str) -> bool:
        """Request a capture.  Returns True if started, False if dropped.

        ``tag`` is a short identifier (typically a timestamp) used to name
        saved files.
        """
        with self._lock:
            if self._busy:
                self.drop_count += 1
                return False
            self._busy = True
            self.fire_count += 1

        def _run() -> None:
            try:
                self._capture(tag)
            except Exception as exc:  # noqa: BLE001 - never kill the worker
                print(f"[trigger] capture failed: {exc}")
            finally:
                with self._lock:
                    self._busy = False

        threading.Thread(target=_run, name="capture", daemon=True).start()
        return True

    def close(self) -> None:  # pragma: no cover - overridden if needed
        """Release any held resources."""


class DryRunTrigger(Trigger):
    """Logs instead of capturing.  Useful for testing the full pipeline."""

    def _capture(self, tag: str) -> None:
        print(f"[dry-run] would capture photo (tag={tag})")


class GPhoto2Trigger(Trigger):
    """Fires a tethered camera via the ``gphoto2`` CLI.

    Modes
    -----
    ``trigger``  (default): ``gphoto2 --trigger-capture``.  Fastest -- the
        camera writes the image to its own card and we do not wait for a
        download, so we are ready for the next strike almost immediately.
    ``download``: ``gphoto2 --capture-image-and-download`` and save into
        ``output_dir``.  Slower but the photo lands on your disk right away.
    """

    def __init__(
        self,
        output_dir: Path,
        mode: str = "trigger",
        gphoto2_path: str = "gphoto2",
        extra_args: list[str] | None = None,
    ) -> None:
        super().__init__()
        if mode not in ("trigger", "download"):
            raise ValueError("mode must be 'trigger' or 'download'")
        if shutil.which(gphoto2_path) is None:
            raise FileNotFoundError(
                f"'{gphoto2_path}' not found on PATH. Install gphoto2 "
                "(e.g. `sudo apt install gphoto2` / `brew install gphoto2`) "
                "or run with --trigger dry-run to test without a camera."
            )
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.mode = mode
        self.gphoto2_path = gphoto2_path
        self.extra_args = extra_args or []

    def _capture(self, tag: str) -> None:
        if self.mode == "trigger":
            cmd = [self.gphoto2_path, "--trigger-capture", *self.extra_args]
        else:
            filename = str(self.output_dir / f"lightning_{tag}.%C")
            cmd = [
                self.gphoto2_path,
                "--capture-image-and-download",
                "--filename",
                filename,
                *self.extra_args,
            ]

        start = time.monotonic()
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=30,
        )
        elapsed = (time.monotonic() - start) * 1000
        if result.returncode != 0:
            raise RuntimeError(
                f"gphoto2 exited {result.returncode}: "
                f"{result.stderr.strip() or result.stdout.strip()}"
            )
        print(f"[gphoto2] captured (tag={tag}, {elapsed:.0f} ms)")


class CommandTrigger(Trigger):
    """Runs an arbitrary shell command on each strike.

    The command string may contain ``{tag}``, which is substituted with the
    strike's timestamp tag.  Use this for GPIO-driven shutter cables, custom
    capture scripts, IFTTT webhooks via curl, etc.
    """

    def __init__(self, command: str) -> None:
        super().__init__()
        if not command.strip():
            raise ValueError("command must be a non-empty string")
        self.command = command

    def _capture(self, tag: str) -> None:
        cmd = self.command.replace("{tag}", tag)
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=30)
        if result.returncode != 0:
            raise RuntimeError(
                f"command exited {result.returncode}: "
                f"{result.stderr.strip() or result.stdout.strip()}"
            )
        print(f"[command] ran capture command (tag={tag})")
