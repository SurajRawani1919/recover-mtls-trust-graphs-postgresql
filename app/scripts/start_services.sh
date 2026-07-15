#!/usr/bin/env bash
set -euo pipefail

/app/scripts/init_db.sh

if ! curl -sf http://127.0.0.1:5001/health >/dev/null 2>&1; then
  python3 /app/api/app.py >/tmp/verifier.log 2>&1 &
fi

for _ in $(seq 1 30); do
  if curl -sf http://127.0.0.1:5001/health >/dev/null 2>&1; then
    echo "Verifier API ready"
    exit 0
  fi
  sleep 1
done

echo "Verifier API failed to start" >&2
exit 1
