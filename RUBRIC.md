# Experts Platform Rubric — Recover mTLS Trust Graphs into PostgreSQL

Copy into the **Terminus-2nd-Edition** (or **terminus-project-v2**) submission rubric UI. Includes **five negative-reward criteria** (N1–N5, values -2/-3/-5 — compliant with the "-1, -2, -3, or -5, never -4" rule).

---

## Task metadata

| Field | Value |
|-------|-------|
| **Task name** | `snorkel/recover-mtls-trust-graphs-postgresql` |
| **Difficulty** | Hard |
| **Domain** | Security & Cryptography |
| **Subcategories** | api_integration, tool_specific |
| **Category** | security |
| **Handling time** | 180 minutes |

---

## Positive criteria

| # | Criterion | Points |
|---|-----------|--------|
| 1 | Agent merges `head.graphml` and `tail.graphml` fragments into a single well-formed GraphML document. | +3 |
| 2 | Agent injects missing `<key>` declarations (`d_svc`, `d_label`, `d_serial`, `d_version`, `d_policy`, `d_sig`) before the `<graph>` element. | +3 |
| 3 | Agent deduplicates nodes by `service_id`, keeping the higher `version` (or later document-order on tie). | +3 |
| 4 | Agent reattaches detached Ed25519 signatures from `detached_signatures.json` onto matching edges as `<data key="d_sig">`. | +2 |
| 5 | Agent fetches the GraphML XSD at runtime from `http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd` (not vendored locally). | +2 |
| 6 | Agent performs XSD validation of the repaired GraphML document. | +2 |
| 7 | Agent calls the Flask verifier API `/verify/key` and `/verify/revocation` for each node to determine verification status. | +3 |
| 8 | Agent calls the Flask verifier API `/verify/edge` for each edge and only accepts edges with valid signatures where both endpoints are verified nodes. | +3 |
| 9 | Agent implements fail-closed persistence: only verified records are inserted into PostgreSQL (`svc-legacy-99` and edge `e3` are absent from the database). | +5 |
| 10 | Agent persists exactly 3 services (`svc-auth-01`, `svc-pay-02`, `svc-notify-03`) and 3 edges (`e1`, `e2`, `e4`) in the database. | +3 |
| 11 | Agent generates `/app/output/report.json` conforming to the report schema with correct `status` field and `repairs` array. | +2 |
| 12 | Agent updates `CMakeLists.txt` to compile all pipeline modules and link required libraries (libxml2, libpq or equivalent, curl). | +2 |
| 13 | Agent starts required services (PostgreSQL, Flask verifier) before running the pipeline. | +1 |
| 14 | Agent explores the existing codebase structure (stubs, graph data, schema, scripts) before implementing the solution. | +1 |
| 15 | Agent verifies the final database state by querying PostgreSQL after pipeline execution. | +1 |

---

## Negative criteria

| # | Criterion | Points |
|---|-----------|--------|
| N1 | Agent inserts unverified or revoked records into the database (violates fail-closed requirement). | -5 |
| N2 | Agent hardcodes or vendors the GraphML XSD instead of fetching it at runtime. | -2 |
| N3 | Agent fails to handle node deduplication correctly (keeps wrong version or both copies). | -3 |
| N4 | Agent does not call the Flask verifier API and bypasses cryptographic verification. | -5 |
| N5 | Agent produces a `report.json` that does not conform to the required schema. | -2 |

---

## Oracle / verifier expectations

- Oracle reward: **1.0** (8/8 pytest tests)
- Verified services: `svc-auth-01`, `svc-pay-02`, `svc-notify-03`
- Verified edges: `e1`, `e2`, `e4`
- Rejected: service `svc-legacy-99`, edge `e3`

---

## Difficulty note

Local agent runs (2026-07-15): GPT-5.5 and Claude Opus 4.8 both scored **1.0** on first trial. For **Hard** difficulty, re-run 2–3 trials per model; if pass rate stays above 80%, increase task difficulty or revise difficulty classification before final approval.
