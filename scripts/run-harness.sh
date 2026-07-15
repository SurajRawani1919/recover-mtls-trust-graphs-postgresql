#!/usr/bin/env bash
# Run Snorkel Harbor workflows from WSL. Usage:
#   source scripts/load-stb-env.sh
#   ./scripts/run-harness.sh verify
#   ./scripts/run-harness.sh oracle
#   ./scripts/run-harness.sh gpt
#   ./scripts/run-harness.sh claude
#   ./scripts/run-harness.sh submit Terminus-2nd-Edition 180
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
export PATH="$HOME/.local/bin:/usr/bin:/bin:$PATH"

if [ -f "$ROOT/scripts/load-stb-env.sh" ]; then
  set -a
  # shellcheck source=/dev/null
  source "$ROOT/scripts/load-stb-env.sh"
  set +a
fi

require_snorkel_key() {
  if [ -z "${SNORKEL_API_KEY:-}" ]; then
    echo "SNORKEL_API_KEY is not set." >&2
    echo "Fix: run 'stb login' in WSL, or export SNORKEL_API_KEY in scripts/load-stb-env.sh" >&2
    exit 1
  fi
}

refresh_ai_keys() {
  if ! stb keys refresh; then
    echo "Warning: stb keys refresh failed; using cached credentials from stb keys show" >&2
  fi
  eval "$(stb keys show | grep '^export')"
}

pre_submit_cleanup() {
  rm -rf "$ROOT/jobs" "$ROOT/my-task-name"
  echo "Removed jobs/ and my-task-name/ from submission folder."
}

verify_environment() {
  echo "== Docker Compose =="
  docker compose version
  docker ps >/dev/null
  echo "OK"

  require_snorkel_key
  echo
  echo "== Snorkel auth =="
  stb keys refresh >/dev/null && echo "stb keys refresh: OK" || echo "stb keys refresh: FAILED"

  echo
  echo "== Project access =="
  if projects="$(stb projects list 2>&1)"; then
    if [ -n "$projects" ] && [ "$projects" != "No projects found." ]; then
      echo "$projects"
      echo "OK — account has project assignments"
    else
      echo "No projects found."
      echo "BLOCKED — your Snorkel account is not assigned to Terminus-2nd-Edition."
      echo "Fix: Experts platform → request access, then run 'stb login' with that account's API key."
      echo "Verify: stb projects list should show Terminus-2nd-Edition"
    fi
  else
    echo "$projects"
  fi

  echo
  echo "== Submission zip size (dry-run) =="
  pre_submit_cleanup
  stb submissions create . -p Terminus-2nd-Edition --dry-run --time 180 | tail -5
}

cmd="${1:-verify}"
project="${2:-Terminus-2nd-Edition}"
time_min="${3:-180}"

case "$cmd" in
  verify)
    verify_environment
    ;;
  oracle)
    require_snorkel_key
    stb harbor run -a oracle -p .
    ;;
  gpt)
    require_snorkel_key
    refresh_ai_keys
    stb harbor run -m @openai/gpt-5.5 -p .
    ;;
  claude)
    require_snorkel_key
    refresh_ai_keys
    stb harbor run -m @anthropic/claude-opus-4-8 -p .
    ;;
  submit)
    require_snorkel_key
    pre_submit_cleanup
    stb submissions create . -p "$project" --time "$time_min"
    ;;
  *)
    echo "Usage: $0 {verify|oracle|gpt|claude|submit} [project-name] [time-minutes]" >&2
    exit 1
    ;;
esac
