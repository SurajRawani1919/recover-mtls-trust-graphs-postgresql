#!/usr/bin/env bash
set -euo pipefail

mkdir -p /app/bin
cmake -S /app/pipeline -B /app/pipeline/build
cmake --build /app/pipeline/build --target recover_graph
cp /app/pipeline/build/recover_graph /app/bin/recover_graph
