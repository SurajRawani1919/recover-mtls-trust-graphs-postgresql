# Experts Platform Rubric — Recover mTLS Trust Graphs into PostgreSQL

Copy these fields into the **Terminus-2nd-Edition** submission rubric on the Snorkel Experts platform after `stb submissions create` succeeds.

---

## Task metadata (match `task.toml`)

| Field | Value |
|-------|-------|
| **Task name** | `snorkel/recover-mtls-trust-graphs-postgresql` |
| **Difficulty** | Hard |
| **Domain** | Security & Cryptography |
| **Subtypes** | API Integration, Tool Specific |
| **Category** | security |

---

## Task summary (short description)

Agent builds a C++ recovery pipeline that repairs corrupted mTLS GraphML trust graphs (split XML, missing keys, duplicate nodes, detached Ed25519 signatures), validates against the live GraphML XSD, verifies nodes/edges via a Flask API, bulk-loads verified records into PostgreSQL with libpq COPY, and emits a schema-valid JSON recovery report.

---

## Oracle / verifier expectations

- **Oracle reward:** 1.0 (8/8 pytest tests pass)
- **Verified services in DB:** `svc-auth-01`, `svc-pay-02`, `svc-notify-03`
- **Verified edges in DB:** `e1`, `e2`, `e4`
- **Rejected (must NOT load):** service `svc-legacy-99`, edge `e3`
- **Outputs:** `/app/output/repaired.graphml`, `/app/output/report.json`

---

## Rubric criteria (suggested)

### 1. Graph repair (structural)

- Merges split fragments (`head.graphml` + `tail.graphml`) into one valid GraphML document.
- Injects missing `<key>` declarations before `<graph>` so all `<data key="...">` references resolve.
- Deduplicates nodes by `service_id`: keeps higher `version`; on tie, keeps later document order.
- Reattaches detached Ed25519 signatures from `detached_signatures.json` to matching edges as `<data key="d_sig">`.

### 2. XSD validation

- Downloads GraphML XSD at runtime from `http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd` (HTTP, not HTTPS).
- Repaired graph passes XSD validation; report records `xsd_validation.passed: true`.

### 3. Flask verifier integration

- Starts PostgreSQL and verifier API via `/app/scripts/start_services.sh`.
- Calls `/verify/key`, `/verify/revocation`, `/verify/edge` before loading data.
- Fail-closed: revoked/invalid nodes and edges are rejected, not loaded.

### 4. PostgreSQL ingest

- Uses libpq **COPY** (not row-by-row INSERT) for bulk load into `services` and `trust_edges`.
- Only verified records appear in the database.

### 5. Recovery report

- Writes `/app/output/report.json` conforming to `/app/schema/report.schema.json`.
- Documents repair actions, verification counts, rejections, and DB load stats.

### 6. Pipeline deliverable

- Implements `/app/bin/recover_graph` C++ binary built from `/app/pipeline/`.
- Accepts CLI flags for fragments dir, detached signatures, output paths, DB URL, verifier URL.

---

## Difficulty calibration targets

Run each model **2–3 times** locally before final submission:

| Model | Target pass rate (hard task) |
|-------|------------------------------|
| `@openai/gpt-5.5` | ~0–40% (most runs should fail or partial) |
| `@anthropic/claude-opus-4-8` | ~0–40% |

**Oracle must always pass (1.0).** If agents pass too often, increase difficulty (more corruption edge cases, stricter verifier rules, additional rejection cases).

### Local calibration results (2026-07-15)

| Model | Run 1 reward | Runtime |
|-------|--------------|---------|
| `@openai/gpt-5.5` | **1.0** | ~2m 35s |
| `@anthropic/claude-opus-4-8` | **1.0** | ~10m |

Both agents passed on the first trial. For a **hard** task, consider increasing difficulty before final platform submission (e.g., additional corruption, stricter COPY verification, more rejection cases). Re-run 2–3 trials per model after changes.

---

## Common failure modes (for reviewers)

- Keys inserted after `<graph>` → XSD failure.
- Using HTTPS for XSD URL → SSL hostname mismatch.
- Loading revoked `svc-legacy-99` or invalid edge `e3`.
- INSERT instead of COPY for PostgreSQL bulk load.
- Not starting Flask verifier / PostgreSQL before pipeline run.
- Stub pipeline left unimplemented (`recover_graph` exits without outputs).

---

## Handling time

Suggested platform entry: **180 minutes** (3 hours) — includes C++ pipeline, XML repair, API integration, libpq COPY, and JSON report.
