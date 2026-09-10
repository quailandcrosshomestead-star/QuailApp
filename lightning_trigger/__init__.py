"""Lightning trigger: detect lightning in a camera feed and fire a DSLR.

See ``README_LIGHTNING.md`` for usage.  Public entry points:

    python -m lightning_trigger --help

Programmatic use:

    from lightning_trigger.detector import FlashDetector
"""

from .detector import DetectionResult, FlashDetector

__all__ = ["FlashDetector", "DetectionResult"]
__version__ = "0.1.0"
