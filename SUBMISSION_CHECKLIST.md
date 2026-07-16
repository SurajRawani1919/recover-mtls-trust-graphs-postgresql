# Pre-Submission Checklist — recover-mtls-trust-graphs-postgresql

**Official reference:** [Snorkel Submission Checklist](https://snorkel-ai.github.io/Terminus-EC-Training-stateful/portal/docs/submitting-tasks/submission-checklist)

Status as of **2026-07-15**. Use before every upload.

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
| Difficulty target: < 80% pass rate | ❌ | **Both agents passed 100% on run 1 — recalibrate before Hard approval** |

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
| `tests/test.sh` — pytest with pre-installed deps + reward file | ✅ `pytest -rA`, `rc=$?` block, no pip |
| `tests/test_outputs.py` — pytest with docstrings | ✅ 8 tests with docstrings |

### Rubric

| Item | Status |
|------|--------|
| Rubric drafted and aligned to task | ✅ See `RUBRIC.md` |
| ≥3 negative-reward criteria (-1) | ✅ N1–N4 in `RUBRIC.md` |
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
| Category classifier (security, not data-processing) | ⚠️ Confirm on re-upload |
| Dockerfile warnings (multi-stage, lockfile) | ⚠️ Acceptable carve-out — agent compiles C++ at runtime |

### LLMaJ Checks

| Check | Status |
|-------|--------|
| behavior_in_task_description | ✅ |
| behavior_in_tests | ✅ |
| informative_test_docstrings | ✅ |
| anti_cheating_measures | ✅ |
| structured_data_schema | ✅ |
| file_reference_mentioned | ✅ |

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

| Metric | Value |
|--------|-------|
| Worst-model pass rate | **100%** (1/1) |
| Best-model pass rate | **100%** (1/1) |
| Declared difficulty | **Hard** |
| **Verdict** | ❌ Hard requires ≤80% worst-model pass rate — run 2–3 more trials or change to Medium |

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
