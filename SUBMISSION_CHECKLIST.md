# Pre-Submission Checklist — recover-mtls-trust-graphs-postgresql

**Official reference:** Snorkel Task Requirements & Submission Checklist (Terminus EC Training portal, login-gated — full text pasted into this project's chat history on 2026-07-16).

Status as of **2026-07-16**. Use before every upload.

---

## Pre-Submission Verification

### Task Design

| Item | Status | Notes |
|------|--------|-------|
| Problem statement is clear and unambiguous | ✅ | Security-focused `instruction.md` |
| All requirements are explicitly stated | ✅ | Pipeline steps, API table, absolute `/app/` paths |
| Uses absolute paths (e.g., `/app/file.txt`) | ✅ | `/app/output/repaired.graphml`, `/app/bin/recover_graph`, etc. |
| Output files are named in instructions | ✅ | `repaired.graphml`, `report.json` |
| Data schemas are fully specified | ✅ | `report.schema.json`, GraphML key table |
| Difficulty target: < 80% pass rate | ❌ | Client review (2026-07-16): TRIVIAL (5/5 both terminus-claude-opus-4-6 and terminus-gpt5-2). Fixed the biggest known cause — instruction.md named the exact expected answer (services/edges) and used real fixture values in API examples; both replaced with rule-based/generic text. Re-run agents to confirm improvement. |
| Instruction does not give Answers/Hints | ✅ (fixed 2026-07-16) | Removed "Expected verified outcome" section that named literal service/edge IDs; genericized API example payloads |

### Required Files

**Always required:**

| File | Status |
|------|--------|
| `task.toml` — complete configuration with all required sections | ✅ All Edition 2 metadata fields |
| `environment/Dockerfile` — digest-pinned base, pinned deps, builds successfully | ✅ |

**Non-milestone (`number_of_milestones = 0`):**

| File | Status |
|------|--------|
| `instruction.md` | ✅ |
| `solution/solve.sh` — deterministic oracle wrapper | ✅ |
| `tests/test.sh` — pytest with pre-installed deps + reward file | ✅ `pytest -rA` via isolated `/opt/test-venv`, `rc=$?` block, no pip |
| `tests/test_outputs.py` — pytest with docstrings | ✅ 13 tests with docstrings (5 added 2026-07-16 for anti-cheating) |

### Rubric

| Item | Status |
|------|--------|
| Rubric drafted and aligned to task | ✅ See `RUBRIC.md` — replaced with client's point-weighted rubric (2026-07-16) |
| ≥3 negative-reward criteria | ✅ N1–N5 |
| Negative values are -1/-2/-3/-5 (never -4) | ✅ -5, -2, -3, -5, -2 |
| Paste into Experts platform UI after upload | ⏳ Manual step |

### Quality Standards

| Item | Status |
|------|--------|
| All requirements have corresponding tests | ✅ |
| Tests verify described requirements (explicit + implicit) | ✅ outputs, DB, schema |
| Tests written in Python and run with pytest | ✅ |
| Anti-cheating measures (no hints or exposed answers) | ✅ solution/tests not in Docker context |
| Tests check behavior, not implementation | ✅ |
| Complies with Quality Guidelines | ✅ |

---

## Automated Checks

### Oracle Agent

```bash
stb harbor run -a oracle -p .
```

| Check | Status |
|-------|--------|
| Oracle agent PASSES | ✅ Mean **1.000** |

### CI Checks (run on upload)

| Check | Status |
|-------|--------|
| Flat ZIP layout (`task.toml` at root, not nested folder) | ✅ |
| `validate-submission-zip.py` local pre-check | ✅ PASSED (42.1 KB) |
| `task.toml` metadata fields | ✅ Fixed |
| Agent timeout ≤ 1800 | ✅ |
| `tests/test.sh` — `-rA`, reward block, no pip | ✅ |
| Dockerfile digest-pinned | ✅ |
| `.dockerignore` required exclusions | ✅ |
| Category classifier (security, not data-processing) | ❌→✅ Fixed 2026-07-16: CodeBuild run flagged `Predicted category 'data-processing' (confidence 0.9) is blocked`. Cause: de-hinting instruction.md (removing the named answer) stripped most of the security framing, leaving mostly ETL-flavored verbs (merge/deduplicate/persist/report). Rewrote instruction.md with an explicit threat-model paragraph and reframed every section around verification/enforcement rather than data movement, without reintroducing the removed answer. |
| Dockerfile warnings (multi-stage, lockfile) | ⚠️ Acceptable carve-out — agent compiles C++ at runtime |
| Test-only deps (pytest, psycopg2-binary, lxml) not in agent-visible runtime | ✅ Fixed 2026-07-16 — moved to isolated `/opt/test-venv`, not on system `python3`/`pip3` |
| `tmux` + `asciinema` installed | ✅ Added 2026-07-16 (defensive; not required by any observed Edition-2 CodeBuild log so far) |

**Note on 2026-07-16 client review report:** a "REVIEW REPORT" section in that feedback flagged `task.toml` missing `version = "2.0"` and a "non-canonical base image" warning, citing generic Terminal-Bench (t-bench) conventions (`ghcr.io/laude-institute/t-bench/*` base images). Our actual Snorkel Edition-2 CodeBuild runs have explicitly validated `schema_version = "1.3"` with no complaint and confirmed `ubuntu:24.04` (pinned to the canonical `public.ecr.aws/docker/library/` digest) as **sanctioned**. This suggests that section of the review used a template for a different (open-source Terminal-Bench) task format. Added `version = "2.0"` as a harmless alias field in `task.toml` anyway; did not change the base image given the real CI's explicit sanction.

### LLMaJ Checks

| Check | Status |
|-------|--------|
| behavior_in_task_description | ✅ |
| behavior_in_tests | ✅ |
| informative_test_docstrings | ✅ |
| anti_cheating_measures | ✅ Strengthened 2026-07-16: verifier-API-call-count test, ELF-binary test, structural GraphML checks, independent XSD re-validation, isolated test-venv, de-hinted instruction.md |
| structured_data_schema | ✅ |
| file_reference_mentioned | ✅ |
| test_deps_in_image (flagged ❌ in 2026-07-16 review) | ✅ Fixed — pytest/psycopg2-binary/jsonschema/lxml now installed into `/opt/test-venv`, invisible to the agent's default `python3` |

---

## Real Agent Testing

Run against GPT-5.5:

```bash
stb harbor run -m @openai/gpt-5.5 -p .
```

| Run | Result |
|-----|--------|
| Run 1 | **PASS (1.0)** |
| Run 2 | ⏳ |
| Run 3 | ⏳ |

Run against Claude Opus 4.8:

```bash
stb harbor run -m @anthropic/claude-opus-4-8 -p .
```

| Run | Result |
|-----|--------|
| Run 1 | **PASS (1.0)** |
| Run 2 | ⏳ |
| Run 3 | ⏳ |

### Difficulty Calculation

Official guidance: accuracy is measured out of **5 attempts** per frontier model (minimum 2–3 acceptable for local iteration).

| Metric | Value |
|--------|-------|
| Worst-model pass rate | **100%** (1/1) |
| Best-model pass rate | **100%** (1/1) |
| Declared difficulty | **Hard** |
| **Verdict** | ❌ Hard requires ≤20% accuracy on best OR worst model; task must score <80% on worst model to be accepted at all. Run 3–4 more trials per model, or change difficulty to Medium/Easy |

---

## Submission Method

| Step | Status |
|------|--------|
| Created ZIP of files (not folder) | ✅ `recover-mtls-trust-graphs-postgresql-submission.zip` on Desktop |
| All required files included in ZIP | ✅ |
| Upload to **terminus-project-v2** on Snorkel Expert Platform | ⏳ |
| Project access / enrollment (`stb projects list`) | ❌ Confirm enrollment |
| Metadata filled in platform UI | ⏳ |
| Rubric pasted from `RUBRIC.md` | ⏳ |

---

## Commands

```bash
# Regenerate + validate zip
python3 scripts/create-submission-zip.py ~/Desktop/recover-mtls-trust-graphs-postgresql-submission.zip
python3 scripts/validate-submission-zip.py ~/Desktop/recover-mtls-trust-graphs-postgresql-submission.zip

# Local checks
./scripts/run-harness.sh oracle
./scripts/run-harness.sh gpt      # repeat 2-3x
./scripts/run-harness.sh claude   # repeat 2-3x
```

---

## Blockers before final approval

1. **Difficulty calibration** — Hard tasks require ≤80% worst-model pass rate; currently 100%.
2. **Platform project access** — `stb submissions create` needs Terminus project enrollment.
3. **Rubric UI** — Paste `RUBRIC.md` criteria (including N1–N4 negatives) on Experts platform.
