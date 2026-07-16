#!/usr/bin/env python3
"""Validate submission zip matches Edition 2 CI expectations."""

from __future__ import annotations

import ast
import re
import sys
import zipfile
from pathlib import Path

REQUIRED = [
    "task.toml",
    "instruction.md",
    "environment/Dockerfile",
    "environment/.dockerignore",
    "environment/test-requirements.txt",
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

DOCKERIGNORE_REQUIRED = [
    ".git",
    "__pycache__",
    ".pytest_cache",
    ".mypy_cache",
    ".ruff_cache",
    "node_modules",
]

VALID_SUBCATEGORIES = {
    "api_integration",
    "db_interaction",
    "long_context",
    "tool_specific",
    "ui_building",
}

VALID_CODEBASE_SIZES = {"large", "minimal", "small"}

TEST_FUNCTIONS = [
    "test_repaired_graph_exists",
    "test_report_matches_schema",
    "test_report_status",
    "test_report_repairs",
    "test_report_counts",
    "test_services_table",
    "test_trust_edges_table",
    "test_edge_signature_reattached",
]


def check_test_docstrings(source: str) -> list[str]:
    errors: list[str] = []
    tree = ast.parse(source)
    for node in tree.body:
        if isinstance(node, ast.FunctionDef) and node.name.startswith("test_"):
            doc = ast.get_docstring(node)
            if not doc or not doc.strip():
                errors.append(f"tests/test_outputs.py:{node.name} missing docstring")
    return errors


def main() -> int:
    zip_path = Path(
        sys.argv[1] if len(sys.argv) > 1 else "recover-mtls-trust-graphs-postgresql-submission.zip"
    )
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
        if re.search(r"\[agent\][\s\S]*timeout_sec = 3600", toml):
            errors.append("agent timeout still 3600")
        if "timeout_sec = 1800.0" not in toml:
            errors.append("agent timeout not 1800")
        if "number_of_milestones = 0" not in toml:
            errors.append("number_of_milestones must be 0 for non-milestone task")
        if 'category = "security"' not in toml:
            errors.append('task.toml category should be "security"')

        subcats_match = re.search(r"subcategories\s*=\s*\[([^\]]*)\]", toml)
        if subcats_match:
            declared = {
                s.strip().strip('"') for s in subcats_match.group(1).split(",") if s.strip()
            }
            invalid = declared - VALID_SUBCATEGORIES
            if invalid:
                errors.append(
                    f"task.toml invalid subcategories {sorted(invalid)} "
                    f"(must be one of: {sorted(VALID_SUBCATEGORIES)})"
                )

        size_match = re.search(r'codebase_size\s*=\s*"([^"]*)"', toml)
        if size_match and size_match.group(1) not in VALID_CODEBASE_SIZES:
            errors.append(
                f"task.toml invalid codebase_size '{size_match.group(1)}' "
                f"(must be one of: {sorted(VALID_CODEBASE_SIZES)})"
            )

        sh = zf.read("tests/test.sh").decode()
        if "-rA" not in sh:
            errors.append("pytest missing -rA")
        if 'rc=$?' not in sh or 'if [ "$rc" -eq 0 ]' not in sh:
            errors.append("test.sh missing rc reward pattern")
        if not re.search(r"rc=\$\?\s*\nif \[ \"\$rc\" -eq 0 \]", sh):
            errors.append(
                "test.sh: rc=$? must be immediately followed by "
                'if [ "$rc" -eq 0 ]; no intervening lines (e.g. set -e) allowed'
            )
        if "pip install" in sh:
            errors.append("test.sh still runs pip install")
        if not sh.rstrip().endswith("fi"):
            errors.append("test.sh must end with reward if/fi block")

        dockerfile = zf.read("environment/Dockerfile").decode()
        if "@sha256:" not in dockerfile:
            errors.append("Dockerfile not digest-pinned")
        if "COPY app/" in dockerfile and "COPY app/api/requirements.txt" not in dockerfile:
            errors.append("Dockerfile should copy requirements before app/ for layer caching")
        first_from = next(
            (line for line in dockerfile.splitlines() if line.strip().startswith("FROM")), ""
        )
        if "ubuntu:24.04@sha256:" in first_from and "public.ecr.aws/docker/library/" not in first_from:
            errors.append(
                "Dockerfile FROM must use canonical registry reference "
                "'public.ecr.aws/docker/library/ubuntu:24.04@sha256:...'"
            )

        dockerignore = zf.read("environment/.dockerignore").decode()
        for entry in DOCKERIGNORE_REQUIRED:
            if entry not in dockerignore:
                errors.append(f".dockerignore missing {entry}")

        app_py = zf.read("environment/app/api/app.py").decode()
        if "EDGE_FIXTURES" in app_py:
            errors.append("unused EDGE_FIXTURES import in app.py")

        instruction = zf.read("instruction.md").decode()
        for path in (
            "/app/output/repaired.graphml",
            "/app/output/report.json",
            "/app/bin/recover_graph",
            "/app/schema/report.schema.json",
        ):
            if path not in instruction:
                errors.append(f"instruction.md missing absolute path {path}")

        test_py = zf.read("tests/test_outputs.py").decode()
        errors.extend(check_test_docstrings(test_py))
        for fn in TEST_FUNCTIONS:
            if f"def {fn}" not in test_py:
                errors.append(f"tests/test_outputs.py missing {fn}")

        if "\r\n" in sh or "\r\n" in toml:
            errors.append("CRLF line endings detected in critical shell/toml files")

    if errors:
        print("VALIDATION FAILED:")
        for e in errors:
            print(f"  - {e}")
        return 1

    print(f"VALIDATION PASSED: {zip_path} ({zip_path.stat().st_size / 1024:.1f} KB)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
