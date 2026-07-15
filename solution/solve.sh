#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ -d "/solution/pipeline" ]; then
  PIPELINE_SRC="/solution/pipeline"
else
  PIPELINE_SRC="${SCRIPT_DIR}/pipeline"
fi

cp -r "${PIPELINE_SRC}/." /app/pipeline/
chmod +x /app/scripts/*.sh
/app/scripts/run_pipeline.sh
