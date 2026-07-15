#!/usr/bin/env bash
set -euo pipefail

/app/scripts/build.sh
/app/scripts/start_services.sh

/app/bin/recover_graph \
  --fragments-dir /app/graph/fragments \
  --detached-signatures /app/graph/detached_signatures.json \
  --output-graph /app/output/repaired.graphml \
  --output-report /app/output/report.json \
  --verifier-url "${FLASK_VERIFIER_URL:-http://127.0.0.1:5001}" \
  --database-url "${DATABASE_URL:-postgresql://mtls:mtls@127.0.0.1:5432/mtls_trust}"
