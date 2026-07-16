# Recover mTLS Trust Graphs: Cryptographic Trust Policy Recovery

This is a **Security & Cryptography** task: implementing a fail-closed **access-control enforcement pipeline** for a mutual-TLS (mTLS) service mesh, not a data-loading exercise.

## Threat model

Security operations exported the mTLS service-trust authorization graph (which services may present which certificates, and which service-to-service trust edges are cryptographically authorized) as GraphML. The export is **corrupted**, and corruption of an authorization graph is a security incident, not a formatting nuisance: a revoked certificate, a forged/stripped Ed25519 signature, or a stale duplicate service record could each cause an attacker-controlled or de-authorized service to be silently granted mTLS trust if the graph is used as-is. Until the graph is reconstructed **and every node and edge is independently re-verified against the PKI verifier**, it **must not** be used for any access-control decision. Your pipeline is the security control that stands between this untrusted export and the production trust store (PostgreSQL) — it must reject, not guess, whenever it cannot cryptographically prove authorization.

## Environment

Corrupted graph fragments live under `/app/graph/`. Detached edge signatures are in `/app/graph/detached_signatures.json`. The Flask verifier API is in `/app/api/` (port **5001**). PostgreSQL DDL is at `/app/database/schema.sql`. Certificate fixtures are under `/app/certificates/`. Pipeline stubs are under `/app/pipeline/`. Helper scripts are under `/app/scripts/`. The report JSON schema is at `/app/schema/report.schema.json`.

Fetch the live GraphML XSD at runtime from `http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd` (use HTTP; HTTPS has a hostname mismatch on that host).

## Trust graph reconstruction (prerequisite for verification)

A structurally invalid or ambiguous graph cannot be meaningfully security-checked, so reconstruct it first: merge split fragments (`head.graphml` + `tail.graphml`) into one GraphML document. Inject missing `<key>` declarations **before** `<graph>`. Deduplicate nodes by `service_id`: keep the higher `version`; on tie, keep the later document-order node (an unresolved duplicate is an integrity hazard — it creates ambiguity about which certificate serial is authoritative for a service). Reattach detached Ed25519 signatures from `detached_signatures.json` onto matching edges as `<data key="d_sig">`; an edge with no signature has no cryptographic authorization and cannot be trusted.

| Key id | Scope | Attribute | Meaning |
|--------|-------|-----------|---------|
| `d_svc` | node | `service_id` | Stable service identifier |
| `d_label` | node | `label` | Human-readable name |
| `d_serial` | node | `cert_serial` | Certificate serial for revocation checks |
| `d_version` | node | `version` | Duplicate resolution |
| `d_policy` | edge | `policy` | Authorization policy string |
| `d_sig` | edge | `signature` | Base64 Ed25519 signature |

## Cryptographic verification (the security-critical path)

Nothing in the reconstructed GraphML — labels, policy strings, even the presence of a signature field — is trustworthy on its own. Every claim must be independently verified through the PKI verifier before it can affect the trust store. Start services with `/app/scripts/start_services.sh`, then call these JSON endpoints on port **5001**:

| Endpoint | Method | Request | Response |
|----------|--------|---------|----------|
| `/health` | GET | — | `{"status":"ok"}` |
| `/verify/key` | POST | `{"cert_serial":"<serial>"}` | `{"valid":true,"issuer_key_id":"<id>"}` or `{"valid":false}` — is this certificate serial issued by the trusted PKI? |
| `/verify/revocation` | POST | `{"cert_serial":"<serial>"}` | `{"revoked":true}` or `{"revoked":false}` — has this certificate been revoked (e.g. compromised key, decommissioned service)? |
| `/verify/edge` | POST | `{"source":"<service_id>","target":"<service_id>","policy":"<policy>","signature":"<base64>"}` | `{"valid":true}` or `{"valid":false}` — does the Ed25519 signature actually authorize this trust relationship? |

Edge canonical signed payload (UTF-8, no trailing newline): `{source_service_id}|{target_service_id}|{policy}`.

## Security requirements (fail-closed enforcement)

Implement `/app/bin/recover_graph` from `/app/pipeline/` as a **C++17** binary enforcing these invariants:

- **Zero implicit trust**: a node is verified only if `/verify/key` returns valid **and** `/verify/revocation` returns not-revoked. A revoked or unrecognized certificate must never be treated as authorized, regardless of what the GraphML claims.
- **No orphaned authorization**: an edge is verified only if `/verify/edge` confirms its signature **and** both its source and target are already-verified nodes. A cryptographically valid signature between an unverified/revoked endpoint is still rejected.
- **Fail closed on persistence**: after reconstructing `/app/output/repaired.graphml` and passing live XSD validation, persist **every verified record and no unverified record** into the `services(service_id, label, cert_serial, issuer_key_id)` and `trust_edges(edge_id, source_service_id, target_service_id, policy, signature)` tables via `libpq`. A record that fails any check must be absent from the database, not merely flagged.
- **Auditable outcome**: write `/app/output/report.json` conforming to `/app/schema/report.schema.json`, recording XSD validation status, the repair actions applied (`merge_fragments`, `inject_keys`, `deduplicate_nodes`, `reattach_signatures`), and which nodes/edges were verified vs. rejected — this is the security audit trail for the recovery.

Report `status` is `"success"` when XSD passed and at least one verified node loaded without hard failure; `"partial"` when XSD passed but verification rejected some records; `"failed"` when repair or XSD failed. Do not vendor the XSD (fetch it live). Exit **0** on `success`/`partial` and non-zero on `failed`.

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
