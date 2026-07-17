# Recover mTLS Trust Graphs: Cryptographic Trust Policy Recovery

**Security & Cryptography task.** Implement a fail-closed **C++17 access-control enforcement binary** (`/app/bin/recover_graph`) that reconstructs a corrupted mTLS service-trust graph and admits a service or trust edge into the production trust store only after independently re-verifying it cryptographically. Nothing in the corrupted export — labels, policy strings, signature fields — is trusted until verified.

## Security requirements (fail-closed invariants)

- **Zero implicit trust.** A node is authorized only if `/verify/key` returns valid **and** `/verify/revocation` returns not-revoked.
- **No orphaned authorization.** An edge is authorized only if `/verify/edge` confirms its signature **and** both endpoints are already-authorized nodes.
- **Fail closed on the trust store.** Admit every authorized record and exclude every unauthorized record from PostgreSQL — never admit on ambiguity or partial verification.
- **Auditable decision trail.** Record which records were authorized vs. rejected, and why, in `/app/output/report.json`.
- Use **C++17**. Do not vendor the XSD. Exit **0** on `success`/`partial`, non-zero on `failed`.

## Environment

Corrupted graph fragments: `/app/graph/`. Detached edge signatures: `/app/graph/detached_signatures.json`. Verifier API source: `/app/api/` (port **5001**). Trust store DDL: `/app/database/schema.sql`. Certificate fixtures: `/app/certificates/`. Pipeline stubs: `/app/pipeline/`. Helper scripts: `/app/scripts/`. Report schema: `/app/schema/report.schema.json`.

Fetch the live GraphML XSD at runtime from `http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd` over HTTP (not HTTPS).

## Verifier API

Start with `/app/scripts/start_services.sh`. JSON endpoints on port **5001**:

| Endpoint | Method | Request | Response |
|----------|--------|---------|----------|
| `/health` | GET | — | `{"status":"ok"}` |
| `/verify/key` | POST | `{"cert_serial":"<serial>"}` | `{"valid":true,"issuer_key_id":"<id>"}` or `{"valid":false}` |
| `/verify/revocation` | POST | `{"cert_serial":"<serial>"}` | `{"revoked":true}` or `{"revoked":false}` |
| `/verify/edge` | POST | `{"source":"<service_id>","target":"<service_id>","policy":"<policy>","signature":"<base64>"}` | `{"valid":true}` or `{"valid":false}` |

Edge canonical signed payload: `{source_service_id}|{target_service_id}|{policy}` (UTF-8, no trailing newline).

## Graph reconstruction (required before verification is meaningful)

Merge `head.graphml` + `tail.graphml` into one document. Inject missing `<key>` declarations before `<graph>`: `d_svc`/`d_label`/`d_serial`/`d_version` (node), `d_policy`/`d_sig` (edge). Deduplicate nodes by `service_id`, keeping the higher `d_version` (tie: later document order). Reattach detached Ed25519 signatures from `detached_signatures.json` onto matching edges as `<data key="d_sig">`. Write the result to `/app/output/repaired.graphml` and validate it against the live XSD.

## Trust store admission

On successful XSD validation, run verification and admit only authorized records via `libpq` into:

- `services(service_id, label, cert_serial, issuer_key_id)`
- `trust_edges(edge_id, source_service_id, target_service_id, policy, signature)`

Write `/app/output/report.json` conforming to `/app/schema/report.schema.json`: `status` (`success` if XSD passed and ≥1 node authorized; `partial` if XSD passed but some records rejected; `failed` if repair/XSD failed), `repairs` actions applied (`merge_fragments`, `inject_keys`, `deduplicate_nodes`, `reattach_signatures`), and authorized/rejected node and edge lists.

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
