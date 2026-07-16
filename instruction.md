# Recover mTLS Trust Graphs: Cryptographic Trust Policy Recovery

This is a **Security & Cryptography** task. Security operations exported an **mTLS service-trust authorization graph** as GraphML. The archive is **cryptographically corrupted** and must not be used for access-control decisions until you implement a **fail-closed C++ recovery pipeline** that repairs the graph, validates **Ed25519** edge signatures, checks **X.509 certificate revocation**, verifies trust material through the bundled Flask PKI verifier API, and only then persists authorized trust relationships.

## Environment

Corrupted graph fragments live under `/app/graph/`. Detached edge signatures are in `/app/graph/detached_signatures.json`. The Flask verifier API is in `/app/api/` (port **5001**). PostgreSQL DDL is at `/app/database/schema.sql`. Certificate fixtures are under `/app/certificates/`. Pipeline stubs are under `/app/pipeline/`. Helper scripts are under `/app/scripts/`. The report JSON schema is at `/app/schema/report.schema.json`.

Fetch the live GraphML XSD at runtime from `http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd` (use HTTP; HTTPS has a hostname mismatch on that host).

## Corruption to repair

Merge split fragments (`head.graphml` + `tail.graphml`) into one GraphML document. Inject missing `<key>` declarations **before** `<graph>`. Deduplicate nodes by `service_id`: keep the higher `version`; on tie, keep the later document-order node. Reattach detached Ed25519 signatures from `detached_signatures.json` onto matching edges as `<data key="d_sig">`.

| Key id | Scope | Attribute | Meaning |
|--------|-------|-----------|---------|
| `d_svc` | node | `service_id` | Stable service identifier |
| `d_label` | node | `label` | Human-readable name |
| `d_serial` | node | `cert_serial` | Certificate serial for revocation checks |
| `d_version` | node | `version` | Duplicate resolution |
| `d_policy` | edge | `policy` | Authorization policy string |
| `d_sig` | edge | `signature` | Base64 Ed25519 signature |

## Verifier API

Start services with `/app/scripts/start_services.sh`, then call these JSON endpoints on port **5001**:

| Endpoint | Method | Request | Response |
|----------|--------|---------|----------|
| `/health` | GET | — | `{"status":"ok"}` |
| `/verify/key` | POST | `{"cert_serial":"<serial>"}` | `{"valid":true,"issuer_key_id":"<id>"}` or `{"valid":false}` |
| `/verify/revocation` | POST | `{"cert_serial":"<serial>"}` | `{"revoked":true}` or `{"revoked":false}` |
| `/verify/edge` | POST | `{"source":"<service_id>","target":"<service_id>","policy":"<policy>","signature":"<base64>"}` | `{"valid":true}` or `{"valid":false}` |

Edge canonical signed payload (UTF-8, no trailing newline): `{source_service_id}|{target_service_id}|{policy}`.

## Pipeline requirements

Implement `/app/bin/recover_graph` from `/app/pipeline/`. The pipeline must merge and repair the graph; write `/app/output/repaired.graphml`; fetch and pass XSD validation; for each node call `/verify/key` and `/verify/revocation` (only non-revoked nodes with valid keys are verified); for each edge call `/verify/edge` (ingest only when signature is valid and both endpoints are verified nodes); persist every verified record — and no unverified record — into the `services(service_id, label, cert_serial, issuer_key_id)` and `trust_edges(edge_id, source_service_id, target_service_id, policy, signature)` tables using `libpq`; write `/app/output/report.json` conforming to `/app/schema/report.schema.json`.

Report `status` is `"success"` when XSD passed and at least one verified node loaded without hard failure; `"partial"` when XSD passed but verification rejected some records; `"failed"` when repair or XSD failed. List repair actions (`merge_fragments`, `inject_keys`, `deduplicate_nodes`, `reattach_signatures`) when applicable. Use **C++17**, do not vendor the XSD, do not insert unverified records, exit **0** on `success`/`partial` and non-zero on `failed`.

## Build and run

```bash
/app/scripts/build.sh
/app/scripts/start_services.sh
/app/bin/recover_graph \
  --fragments-dir /app/graph/fragments \
  --detached-signatures /app/graph/detached_signatures.json \
  --output-graph /app/output/repaired.graphml \
  --output-report /app/output/report.json \
  --verifier-url http://127.0.0.1:5001 \
  --database-url postgresql://mtls:mtls@127.0.0.1:5432/mtls_trust
```

## Expected behavior

A correct run persists into PostgreSQL only the services that have a valid issuer key and are not revoked, and only the edges whose Ed25519 signature verifies and whose source and target are both persisted services. Any node or edge failing a check must be absent from the database and reported in `verification.rejected_nodes` / `verification.rejected_edges`.
