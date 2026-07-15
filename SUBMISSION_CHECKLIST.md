# Pre-Submission Checklist — recover-mtls-trust-graphs-postgresql

Status as of **2026-07-15**. Use before every upload.

---

## Task Design

| Item | Status | Notes |
|------|--------|-------|
| Problem statement clear | ✅ | Security-focused `instruction.md` |
| Requirements explicit | ✅ | Pipeline steps, API table, absolute `/app/` paths |
| Absolute paths used | ✅ | `/app/output/repaired.graphml`, `/app/bin/recover_graph`, etc. |
| Output files named | ✅ | `repaired.graphml`, `report.json` |
| Data schemas specified | ✅ | `report.schema.json`, GraphML key table |
| Difficulty target < 80% pass | ❌ | **Both agents passed 100% on run 1 — recalibrate before Hard approval** |

---

## Required Files (non-milestone, `number_of_milestones = 0`)

| File | Status |
|------|--------|
| `task.toml` | ✅ All Edition 2 metadata fields |
| `environment/Dockerfile` | ✅ Digest-pinned base, pinned deps, test deps preinstalled |
| `instruction.md` | ✅ |
| `solution/solve.sh` | ✅ Deterministic oracle wrapper |
| `tests/test.sh` | ✅ `pytest -rA`, reward block, no pip in tests |
| `tests/test_outputs.py` | ✅ 8 tests with docstrings |

---

## Rubric

| Item | Status |
|------|--------|
| Rubric drafted | ✅ See `RUBRIC.md` |
| ≥3 negative criteria (-1) | ✅ N1–N4 in `RUBRIC.md` |
| Paste into Experts platform UI | ⏳ Manual step after upload |

---

## Quality Standards

| Item | Status |
|------|--------|
| Requirements have tests | ✅ |
| Tests verify behavior (outputs, DB, schema) | ✅ |
| Python pytest | ✅ |
| Anti-cheating (no answers in image) | ✅ solution/tests not in Docker context |
| Tests check behavior not implementation | ✅ |
| Test docstrings | ✅ Added |

---

## Automated Checks

| Check | Status |
|-------|--------|
| Oracle `stb harbor run -a oracle` | ✅ Mean 1.000 |
| ZIP flat layout (`task.toml` at root) | ✅ Use `scripts/create-submission-zip.py` |
| `validate-submission-zip.py` | ✅ Local pre-check |
| CI static checks (metadata, test.sh, digest) | ✅ Fixed in latest zip |
| Category classifier | ⚠️ Reworked for security; confirm on re-upload |
| Dockerfile warnings (multi-stage, lockfile) | ⚠️ Acceptable carve-out: agent must compile C++ at runtime |

---

## Real Agent Testing

| Model | Run 1 | Run 2 | Run 3 |
|-------|-------|-------|-------|
| GPT-5.5 | **PASS (1.0)** | ⏳ | ⏳ |
| Claude Opus 4.8 | **PASS (1.0)** | ⏳ | ⏳ |

**Worst-model pass rate:** 100% (1/1)  
**Best-model pass rate:** 100% (1/1)  
**Declared difficulty:** Hard  
**Recommendation:** Run 2 more trials each; if still >80%, increase difficulty or change to Medium.

---

## Submission Method

| Step | Status |
|------|--------|
| Create flat ZIP (not nested folder) | ✅ `recover-mtls-trust-graphs-postgresql-submission.zip` on Desktop |
| Upload to Experts platform | ⏳ |
| Project access / enrollment | ❌ Confirm `stb projects list` shows your project |
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
