# Recover mTLS Trust Graphs into PostgreSQL with C++

Operations exported an mTLS service-trust graph as GraphML, but the archive is corrupted. Build a **C++ recovery pipeline** that repairs the artifact, validates it against the live GraphML XSD, verifies trust material through the bundled Flask verifier API, and loads only verified records into PostgreSQL with **`libpq` `COPY`**.

## What you are given

- Corrupted graph fragments under `/app/graph/`
- Detached edge signatures at `/app/graph/detached_signatures.json`
- Flask verifier API source in `/app/api/` (start it on port **5001**)
- PostgreSQL bootstrap DDL at `/app/database/schema.sql`
- Certificate and revocation fixtures under `/app/certificates/`
- Pipeline stubs under `/app/pipeline/`
- Helper scripts under `/app/scripts/`
- Report JSON schema at `/app/schema/report.schema.json`

The official GraphML XSD is **not** vendored. Your pipeline must download it at runtime from:

`http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd`

Note: this host serves a valid XSD over HTTP; its HTTPS certificate does not match the hostname.

## Corruption you must repair

1. **Split XML records** — the graph is split across `/app/graph/fragments/head.graphml` and `/app/graph/fragments/tail.graphml`. Merge them into one document.
2. **Missing key declarations** — `<data key="...">` elements reference keys that are not declared in `<key>` elements. Infer and inject the required `<key>` declarations.
3. **Duplicate service nodes** — the same `service_id` appears more than once. Deduplicate deterministically: keep the node with the **higher `version` attribute**; if versions tie, keep the node that appears **later** in document order.
4. **Detached Ed25519 signatures** — authorization edge signatures are stored in `detached_signatures.json` instead of on the edge. Reattach each signature to the matching edge `id` in a `<data key="d_sig">` element.

## GraphML data model

Nodes (`<node>`) carry:

| Key id | Attribute | Meaning |
|--------|-----------|---------|
| `d_svc` | `service_id` | Stable service identifier |
| `d_label` | `label` | Human-readable name |
| `d_serial` | `cert_serial` | Certificate serial used for revocation checks |
| `d_version` | `version` | Integer used for duplicate resolution |

Edges (`<edge>`) carry:

| Key id | Attribute | Meaning |
|--------|-----------|---------|
| `d_policy` | `policy` | Authorization policy string |
| `d_sig` | `signature` | Base64 Ed25519 signature (reattach from sidecar) |

## Verification API

Start the verifier before running your pipeline:

```bash
/app/scripts/start_services.sh
```

Endpoints (JSON bodies, JSON responses):

### `GET /health`

Returns `{"status":"ok"}` when ready.

### `POST /verify/key`

Request:

```json
{ "cert_serial": "AUTH-0001" }
```

Response:

```json
{ "valid": true, "issuer_key_id": "platform-issuer-v1" }
```

Unknown serials return `"valid": false`.

### `POST /verify/revocation`

Request:

```json
{ "cert_serial": "LEGACY-0099" }
```

Response:

```json
{ "revoked": true }
```

Active certificates return `"revoked": false`.

### `POST /verify/edge`

Canonical signed payload (UTF-8, no trailing newline):

```
{source_service_id}|{target_service_id}|{policy}
```

Request:

```json
{
  "source": "svc-auth-01",
  "target": "svc-pay-02",
  "policy": "allow:read",
  "signature": "<base64>"
}
```

Response:

```json
{ "valid": true }
```

## Pipeline requirements

Implement the recovery binary at `/app/pipeline/` and install it as `/app/bin/recover_graph`.

Your pipeline must:

1. Merge fragments and apply all repairs listed above.
2. Write repaired GraphML to `/app/output/repaired.graphml`.
3. Fetch the live GraphML XSD and validate `repaired.graphml`. Record results in the report.
4. For each node: call `/verify/key` and `/verify/revocation`. Only **non-revoked** nodes with `valid: true` key lookup are **verified nodes**.
5. For each edge: call `/verify/edge`. Only ingest edges where:
   - signature verification returns `valid: true`, **and**
   - both endpoint service IDs are verified nodes.
6. Bulk-load verified rows into PostgreSQL using **`PQexec` with `COPY ... FROM STDIN`** (not row-by-row `INSERT`).
   - Table `services(service_id, label, cert_serial, issuer_key_id)`
   - Table `trust_edges(edge_id, source_service_id, target_service_id, policy, signature)`
7. Write `/app/output/report.json` conforming to `/app/schema/report.schema.json`.

## Report semantics

- `status` is `"success"` only if XSD validation passed, at least one verified node was loaded, and no hard I/O/DB failure occurred.
- `status` is `"partial"` if XSD passed but some nodes/edges were rejected by verification.
- `status` is `"failed"` if repair or XSD validation failed.
- `repairs` must list each repair category applied at least once when that corruption was present.
- `database.nodes_loaded` and `database.edges_loaded` must match actual `COPY` counts.

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

## Constraints

- Primary implementation language: **C++17** (libxml2, libcurl, libpq, libsodium or OpenSSL for local helpers if needed).
- Do not vendor the GraphML XSD.
- Do not insert unverified nodes or edges.
- Use `COPY` for database ingest.
- Exit code **0** on `success` or `partial`; **non-zero** on `failed`.

## Expected verified outcome (sanity check)

After a correct run against the bundled fixtures:

- **3** services loaded (`svc-auth-01`, `svc-pay-02`, `svc-notify-03`)
- **3** trust edges loaded (`e1`, `e2`, `e4`)
- `svc-legacy-99` must **not** appear in `services`
- Edge `e3` must **not** appear in `trust_edges` (revoked endpoint)
