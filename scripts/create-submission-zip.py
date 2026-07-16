#!/usr/bin/env python3
"""Create submission zip with task files at archive root (CI-compatible layout)."""

from __future__ import annotations

import shutil
import sys
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUT = ROOT / "recover-mtls-trust-graphs-postgresql-submission.zip"

REQUIRED = [
    "task.toml",
    "instruction.md",
    "environment/Dockerfile",
    "tests/test.sh",
    "tests/test_outputs.py",
    "solution/solve.sh",
]

INCLUDE_FILES = [
    "instruction.md",
    "task.toml",
    ".gitattributes",
    "README.md",
    "PROJECT_PLAN.md",
    "RUBRIC.md",
    "SUBMISSION_CHECKLIST.md",
    "EXPERTS_PLATFORM.md",
]

INCLUDE_DIRS = ["environment", "solution", "tests"]

SKIP_PARTS = {".git", "__pycache__", ".pytest_cache", "jobs", "my-task-name"}


def should_skip(path: Path) -> bool:
    return any(part in SKIP_PARTS for part in path.parts)


def main() -> int:
    out = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else DEFAULT_OUT

    with tempfile.TemporaryDirectory() as tmp:
        staging = Path(tmp)
        for d in INCLUDE_DIRS:
            src = ROOT / d
            dst = staging / d
            shutil.copytree(src, dst, ignore=shutil.ignore_patterns("__pycache__", "*.pyc"))
        for name in INCLUDE_FILES:
            src = ROOT / name
            if src.exists():
                shutil.copy2(src, staging / name)
        for rel in REQUIRED:
            if not (staging / rel).exists():
                print(f"Missing required path: {rel}", file=sys.stderr)
                return 1

        if out.exists():
            out.unlink()
        with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as zf:
            for file_path in sorted(staging.rglob("*")):
                if file_path.is_file():
                    arcname = file_path.relative_to(staging).as_posix()
                    zf.write(file_path, arcname)

    size_kb = out.stat().st_size / 1024
    print(f"Created: {out}")
    print(f"Size: {size_kb:.2f} KB")
    print("\nZip root entries:")
    with zipfile.ZipFile(out) as zf:
        roots = sorted({n.split("/")[0] for n in zf.namelist() if n})
        for r in roots:
            print(f"  {r}/")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
