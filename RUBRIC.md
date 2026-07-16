# Experts Platform Rubric — Recover mTLS Trust Graphs into PostgreSQL

Copy into the **Terminus-2nd-Edition** (or **terminus-project-v2**) submission rubric UI. Include **at least three negative-reward criteria (-1)**.

---

## Task metadata

| Field | Value |
|-------|-------|
| **Task name** | `snorkel/recover-mtls-trust-graphs-postgresql` |
| **Difficulty** | Hard |
| **Domain** | Security & Cryptography |
| **Subcategories** | Cryptography, API Integration |
| **Category** | security |
| **Handling time** | 180 minutes |

---

## Positive criteria (+1 each when satisfied)

| # | Criterion |
|---|-----------|
| 1 | Merges split GraphML fragments into one valid document and injects missing `<key>` elements before `<graph>`. |
| 2 | Deduplicates service nodes by `service_id` using higher `version`, with tie-break by later document order. |
| 3 | Reattaches detached Ed25519 signatures from `detached_signatures.json` to edges as `<data key="d_sig">`. |
| 4 | Fetches live GraphML XSD over HTTP and passes XSD validation; report records `xsd_validation.passed: true`. |
| 5 | Calls Flask verifier endpoints (`/verify/key`, `/verify/revocation`, `/verify/edge`) before persistence. |
| 6 | Loads only verified rows into PostgreSQL via `libpq` (bulk `COPY FROM STDIN` preferred). |
| 7 | Writes `/app/output/report.json` conforming to `/app/schema/report.schema.json`. |
| 8 | Produces `/app/output/repaired.graphml` and `/app/bin/recover_graph` built from C++ pipeline code. |

---

## Negative criteria (-1 each when violated)

| # | Criterion | Penalty |
|---|-----------|---------|
| N1 | Loads revoked service `svc-legacy-99` or invalid edge `e3` into the database, or omits fail-closed verification. | **-1** |
| N2 | Uses HTTPS for the GraphML XSD URL, inserts `<key>` elements after `<graph>`, or skips live XSD validation. | **-1** |
| N3 | Persists records without using `libpq`, or leaves pipeline stubs unimplemented with no outputs. | **-1** |
| N4 | Report counts or repair actions do not match actual pipeline behavior (e.g., missing required repair categories). | **-1** |

---

## Oracle / verifier expectations

- Oracle reward: **1.0** (8/8 pytest tests)
- Verified services: `svc-auth-01`, `svc-pay-02`, `svc-notify-03`
- Verified edges: `e1`, `e2`, `e4`
- Rejected: service `svc-legacy-99`, edge `e3`

---

## Difficulty note

Local agent runs (2026-07-15): GPT-5.5 and Claude Opus 4.8 both scored **1.0** on first trial. For **Hard** difficulty, re-run 2–3 trials per model; if pass rate stays above 80%, increase task difficulty or revise difficulty classification before final approval.
