#!/usr/bin/env bash
set -uo pipefail

cd /app
mkdir -p /logs/verifier

/opt/test-venv/bin/python3 -m pytest -rA /tests/test_outputs.py
rc=$?
if [ "$rc" -eq 0 ]; then
  echo 1 > /logs/verifier/reward.txt
else
  echo 0 > /logs/verifier/reward.txt
fi
