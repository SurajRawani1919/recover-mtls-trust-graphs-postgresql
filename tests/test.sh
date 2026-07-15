#!/usr/bin/env bash
set -euo pipefail

cd /app
mkdir -p /logs/verifier

set +e
pytest -rA /tests/test_outputs.py
rc=$?
set -e

if [ "$rc" -eq 0 ]; then
  echo 1 > /logs/verifier/reward.txt
else
  echo 0 > /logs/verifier/reward.txt
fi
