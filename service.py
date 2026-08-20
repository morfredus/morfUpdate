#!/usr/bin/env python3
"""Install or inspect the local morfUpdate service through morfdeploy."""

from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "third_party" / "morf"))

try:
    from morfdeploy.cli import main
except ImportError as exc:
    print(f"Cannot load the vendored deployment core: {exc}", file=sys.stderr)
    raise SystemExit(2) from exc

if __name__ == "__main__":
    sys.exit(main([*sys.argv[1:], "--repo", str(HERE)]))
