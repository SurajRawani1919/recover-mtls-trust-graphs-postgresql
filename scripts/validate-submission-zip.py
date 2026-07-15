#!/usr/bin/env python3
"""Validate submission zip matches Edition 2 CI expectations."""

from __future__ import annotations

import sys
import zipfile
from pathlib import Path

REQUIRED = [
    "task.toml",
    "instruction.md",
    "environment/Dockerfile",
    "tests/test.sh",
    "tests/test_outputs.py",
    "solution/solve.sh",
]

TOML_REQUIRED = [
    "author_name",
    "author_email",
    "subcategories",
    "codebase_size",
    "number_of_milestones",
    "languages",
    "expert_time_estimate_min",
    "junior_time_estimate_min",
    "allow_internet",
]


def main() -> int:
    zip_path = Path(sys.argv[1] if len(sys.argv) > 1 else "recover-mtls-trust-graphs-postgresql-submission.zip")
    if not zip_path.exists():
        print(f"Missing zip: {zip_path}", file=sys.stderr)
        return 1

    errors: list[str] = []
    with zipfile.ZipFile(zip_path) as zf:
        names = set(zf.namelist())
        if any(n.startswith("recover-mtls-trust-graphs-postgresql/") for n in names):
            errors.append("nested folder prefix in zip")

        for req in REQUIRED:
            if req not in names:
                errors.append(f"missing {req}")

        toml = zf.read("task.toml").decode()
        for field in TOML_REQUIRED:
            if field not in toml:
                errors.append(f"task.toml missing {field}")
        if "timeout_sec = 3600" in toml:
            errors.append("agent timeout still 3600")
        if "timeout_sec = 1800.0" not in toml:
            errors.append("agent timeout not 1800")

        sh = zf.read("tests/test.sh").decode()
        if "-rA" not in sh:
            errors.append("pytest missing -rA")
        if 'rc=$?' not in sh or 'if [ "$rc" -eq 0 ]' not in sh:
            errors.append("test.sh missing rc reward pattern")
        if "pip install" in sh:
            errors.append("test.sh still runs pip install")

        dockerfile = zf.read("environment/Dockerfile").decode()
        if "@sha256:" not in dockerfile:
            errors.append("Dockerfile not digest-pinned")

        app_py = zf.read("environment/app/api/app.py").decode()
        if "EDGE_FIXTURES" in app_py:
            errors.append("unused EDGE_FIXTURES import")

    if errors:
        print("VALIDATION FAILED:")
        for e in errors:
            print(f"  - {e}")
        return 1

    print(f"VALIDATION PASSED: {zip_path} ({zip_path.stat().st_size / 1024:.1f} KB)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
