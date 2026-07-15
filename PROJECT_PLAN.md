# PROJECT_PLAN.md

## Recover mTLS Trust Graphs into PostgreSQL with C++

---

## 1. Task Overview

This TerminalBench Edition 2 (non-milestone) task asks the agent to build a **C++ recovery pipeline** that ingests corrupted mTLS service-trust graphs encoded as GraphML (XML), repairs structural and cryptographic defects, validates the result against the official GraphML schema, verifies trust relationships through a provided Flask verifier API, and loads only verified records into PostgreSQL using `libpq` bulk `COPY`.

The task sits at the intersection of **security/cryptography**, **API integration**, and **tool-specific systems work** (C++, PostgreSQL, XML schema validation, Ed25519 signature verification). The agent must reason about corrupted artifacts, external verification services, and efficient database ingestion—not just parse a clean file.

**Why this matters for the task design:** A naïve XML parser or a script that skips verification would fail. The pipeline must demonstrate end-to-end recovery: repair → schema-validate → cryptographically verify → persist → report.

---

## 2. Problem Statement

An operations team exported mTLS service-trust graphs as GraphML documents. During transfer and archival, the artifacts became **corrupted** and **incomplete**:

- XML records were **split** across fragments or malformed boundaries.
- **Key declarations** required for GraphML data attributes were missing.
- **Duplicate service nodes** appeared with conflicting metadata.
- **Ed25519 signatures** on service-to-service authorization edges were **detached** or no longer bound to their edge payloads.

Without a recovery pipeline, these graphs cannot be trusted for authorization decisions, audit trails, or compliance reporting. Manual repair is error-prone and does not scale.

Additionally, trust cannot be assumed from structure alone. Each service identity must be checked against:

- Issuer **public keys**
- **Certificate revocation** status
- **Signed trust-edge** validity (Ed25519 over canonical edge material)

Only after repair, schema validation, and API verification should data enter PostgreSQL.

**Why this framing:** The problem is not “parse GraphML”—it is **recover trustworthiness** of a security-critical graph under real-world corruption and cryptographic constraints.

---

## 3. Objective

Build a complete, runnable recovery system that:

1. **Repairs** corrupted GraphML input into a well-formed, deduplicated graph representation.
2. **Fetches** the live GraphML XSD from `graphml.graphdrawing.org` at runtime and **validates** repaired XML against it (schema is intentionally **not** vendored in the repo).
3. **Verifies** nodes and edges via the provided Flask verifier API (keys, revocation, trust-edge signatures).
4. **Ingests** only verified nodes and edges into PostgreSQL using **`libpq` `COPY`** for bulk load performance.
5. **Emits** a **schema-validated JSON recovery report** summarizing repairs, verification outcomes, and load statistics.

Success is measured by: Docker build, oracle/solution pass, automated tests, target model difficulty, CI green, and rubric completion.

**Why these objectives:** Each step gates the next—unrepaired XML cannot validate; unvalidated XML should not be verified; unverified records must not be copied into the database; the report provides auditable proof of recovery.

---

## 4. Domain

**Security & Cryptography**

This task requires understanding:

- **mTLS service identity** and certificate-chain concepts (issuer keys, revocation).
- **Ed25519** detached signatures on authorization edges and how canonicalization affects verification.
- **Trust graphs** as security artifacts (who may call whom, under what credentials).
- **Fail-closed verification**—reject or skip unverifiable records rather than silently ingesting them.

**Why Security & Cryptography:** The core difficulty is not file I/O; it is correctly handling cryptographic verification workflows and treating the graph as a security policy artifact.

---

## 5. Task Subtypes

### API Integration

The C++ pipeline must integrate with a **Flask verifier API** over HTTP. Expected interaction patterns include:

- Fetching or validating **issuer public keys** for service certificates.
- Checking **revocation status** for presented certificates.
- Submitting **trust-edge payloads and signatures** for Ed25519 verification.

The pipeline must handle API failures, malformed responses, and partial verification results without corrupting database state.

**Why:** Real security systems delegate trust decisions to verifiers, HSMs, or policy services. Agents must implement robust client logic, not assume local-only checks.

### Tool Specific

The task mandates specific tools and interfaces:

- **C++** as the primary recovery/orchestration language.
- **PostgreSQL** with **`libpq`** and **`COPY`** for ingestion.
- **GraphML / XML** parsing and **XSD** validation (live schema fetch).
- **Flask** (Python) verifier service provided in the environment.

**Why:** Tool-specific constraints test whether the agent can work within a prescribed stack (C++ + libpq + external API), not swap in arbitrary alternatives.

---

## 6. Technologies Required

| Technology | Role | Why needed |
|------------|------|------------|
| **C++17+** | Main recovery pipeline | Task requirement; orchestrates repair, validation, API calls, DB load, reporting |
| **libxml2 / Xerces / similar** | XML parse + XSD validate | Repair and schema-validate GraphML after fetch |
| **libcurl / cpp-httplib** | HTTP client | Call Flask verifier API from C++ |
| **libsodium / OpenSSL** | Ed25519 handling | Align edge signature verification with API expectations; local pre-checks if needed |
| **libpq (PostgreSQL C API)** | Database access | Required `COPY`-based bulk ingest |
| **PostgreSQL 15+** | Persistence | Store verified service nodes and trust edges |
| **Python 3 + Flask** | Verifier API | Provided service for keys, revocation, trust-edge verification |
| **GraphML / XML** | Input/output format | Standard graph interchange for mTLS service graphs |
| **JSON** | Recovery report | Machine-readable audit output |
| **Docker** | Reproducible environment | TerminalBench harness builds and runs the task container |

**Build tooling (to define later):** `CMake` or `Makefile`, `g++`/`clang++`, `pkg-config` for libpq and XML libraries.

---

## 7. Project Folder Structure

TerminalBench Edition 2 non-milestone layout (already scaffolded):

```
.
├── instruction.md          # Agent-facing task prompt (not written yet)
├── task.toml               # Harness metadata (not written yet)
├── PROJECT_PLAN.md         # This document
├── README.md               # Contributor/oracle documentation (placeholder)
│
├── environment/
│   ├── Dockerfile          # Container image definition (not written yet)
│   └── .dockerignore
│
├── app/
│   ├── graph/              # Corrupted + repaired GraphML artifacts
│   ├── schema/             # Optional local schema helpers (live XSD fetched at runtime)
│   ├── api/                # Flask verifier service
│   ├── database/           # PostgreSQL bootstrap SQL, seed data
│   ├── certificates/       # Test certs, keys, revocation fixtures
│   ├── scripts/            # Build, run, bootstrap helpers
│   └── output/             # Runtime outputs: repaired.graphml, report.json
│
├── solution/
│   └── solve.sh            # Oracle entrypoint (not written yet)
│
└── tests/
    ├── test.sh             # Test runner (not written yet)
    └── test_outputs.py     # Pytest assertions (not written yet)
```

### Folder purpose

| Path | Purpose |
|------|---------|
| `app/graph/` | Holds corrupted input GraphML and generated repaired GraphML |
| `app/schema/` | Supporting schema utilities; **not** the canonical XSD (fetched live) |
| `app/api/` | Flask verifier: `/keys`, `/revocation`, `/verify-edge` (exact routes TBD) |
| `app/database/` | `schema.sql`, table definitions for nodes/edges, indexes |
| `app/certificates/` | PEM/DER fixtures, issuer material, revoked cert lists |
| `app/scripts/` | `build.sh`, `run_pipeline.sh`, DB init scripts |
| `app/output/` | Writable directory for `repaired.graphml` and `report.json` |
| `environment/` | Docker build context for C++, Python, PostgreSQL client libs |
| `solution/` | Reference oracle that demonstrates full recovery |
| `tests/` | Validates repair, verification, DB contents, and report schema |

---

## 8. Input Files

Files to create or bundle during development (paths indicative):

| Input | Location (proposed) | Description |
|-------|---------------------|-------------|
| **Corrupted GraphML** | `app/graph/graph_corrupted.graphml` | Primary artifact with split XML, missing keys, duplicate nodes, detached signatures |
| **GraphML fragments** | `app/graph/fragments/` | Optional shard files if split-record corruption is file-based |
| **Service certificates** | `app/certificates/*.pem` | Leaf and issuer certs referenced by graph nodes |
| **Issuer keys / trust material** | `app/certificates/issuers/` | Public keys the Flask API uses for verification |
| **Revocation fixtures** | `app/certificates/revoked.json` or CRL | Drives revocation API responses |
| **Edge signature payloads** | Embedded in GraphML or sidecar | Detached Ed25519 signatures to reattach during repair |
| **PostgreSQL bootstrap** | `app/database/schema.sql` | Tables for `services` (nodes) and `trust_edges` (edges) |
| **Flask verifier config** | `app/api/config.py` | Port, cert paths, canonicalization rules |

**Live fetch (not an input file on disk):**

- **GraphML XSD** from `https://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd` (or current canonical URL)—downloaded at pipeline runtime.

**Why each input exists:** Corruption types must be reproducible in test fixtures; certs and API config enable cryptographic verification; DB schema defines the ingest target.

---

## 9. Output Files

| Output | Location (proposed) | Description |
|--------|---------------------|-------------|
| **Repaired GraphML** | `app/output/repaired.graphml` | XSD-valid, deduplicated graph with signatures reattached |
| **Recovery report (JSON)** | `app/output/report.json` | Schema-validated summary: repairs applied, verify pass/fail counts, DB load stats, errors |
| **PostgreSQL rows** | DB tables | Verified nodes and edges only—no rejected records |
| **Pipeline logs** | `stdout` / `app/output/pipeline.log` (optional) | Human-readable trace for debugging and rubric |

### Recovery report (expected fields, to finalize in instruction)

- `task`, `timestamp`, `input_file`
- `repairs`: list of actions (dedupe, key injection, signature reattach, fragment merge)
- `xsd_validation`: `passed`, `schema_url`, `errors`
- `verification`: per-node and per-edge results from Flask API
- `database`: `nodes_loaded`, `edges_loaded`, `copy_duration_ms`
- `status`: `success` | `partial` | `failed`

**Why:** Outputs must be independently testable—tests will assert file existence, JSON schema, row counts, and absence of unverified data in PostgreSQL.

---

## 10. Expected Workflow

```
┌─────────────────┐
│ Corrupted       │
│ GraphML input   │
└────────┬────────┘
         │
         ▼
┌─────────────────┐     Fetch live XSD from graphml.graphdrawing.org
│ Repair pipeline │◄────────────────────────────────────────────┐
│ (C++)           │                                              │
└────────┬────────┘                                              │
         │                                                       │
         ▼                                                       │
┌─────────────────┐                                              │
│ XSD validate    │──────────────────────────────────────────────┘
│ repaired XML    │
└────────┬────────┘
         │
         ▼
┌─────────────────┐     HTTP: keys, revocation, edge verify
│ Flask verifier  │◄─────────────────────────────────────────────┐
│ API             │                                              │
└────────┬────────┘                                              │
         │                                                       │
         ▼                                                       │
┌─────────────────┐                                              │
│ Filter verified │                                              │
│ nodes & edges   │                                              │
└────────┬────────┘                                              │
         │                                                       │
         ▼                                                       │
┌─────────────────┐     libpq COPY                               │
│ PostgreSQL      │◄─────────────────────────────────────────────┘
│ ingest          │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ JSON recovery   │
│ report          │
└─────────────────┘
```

### Step-by-step

1. **Bootstrap** — Start PostgreSQL, apply `schema.sql`, start Flask verifier API.
2. **Load input** — Read corrupted GraphML from `app/graph/`.
3. **Repair** — Merge split records, inject missing `<key>` declarations, deduplicate service nodes (deterministic merge rules), reattach Ed25519 signatures to canonical edge serialization.
4. **Fetch XSD** — Download GraphML schema from `graphml.graphdrawing.org`; cache in memory or temp file for validation pass.
5. **Validate** — Run XSD validation on repaired GraphML; abort or mark failed if invalid.
6. **Verify nodes** — For each service node, call API for issuer key resolution and revocation check.
7. **Verify edges** — For each authorization edge, call API to verify Ed25519 signature over canonical payload.
8. **Ingest** — `COPY` verified rows into PostgreSQL staging/final tables via libpq.
9. **Report** — Write `report.json` with repair log, validation result, verification tallies, and load metrics.
10. **Exit** — Non-zero exit code on hard failure; partial success policy to be defined in instruction/tests.

**Why this order:** Schema validation before API calls avoids verifying structurally invalid graphs; API verification before `COPY` prevents polluting the database; report is always last so it reflects final state.

---

## 11. Components to Build

### 11.1 C++ Recovery Pipeline (`app/scripts/` + compiled binary)

**Responsibilities:**

- GraphML repair engine (XML DOM or SAX with repair pass)
- Live XSD fetch + validation
- HTTP client for Flask verifier
- libpq `COPY` ingest
- JSON report writer

**Why:** Central orchestrator; satisfies the “C++ pipeline” requirement from the task screenshot.

### 11.2 Graph Repair Module

**Handles:**

- **Split records:** concatenate or merge fragment boundaries into valid `<graphml>` document
- **Missing keys:** infer or restore `<key id="..." for="node|edge" attr.name="..."/>` declarations from data usage
- **Duplicate nodes:** collapse by stable service ID (e.g., `service_id` or certificate fingerprint); merge attributes with documented precedence
- **Detached signatures:** locate signature blobs, bind to edge `id` + canonical JSON/XML payload, store in GraphML data elements

**Why:** Each corruption type is an explicit test scenario; repair logic must be deterministic for oracle/tests.

### 11.3 XSD Validation Module

**Handles:**

- HTTP GET of official GraphML XSD at runtime
- Validate repaired output; surface errors in report

**Why:** Task explicitly requires live schema fetch—not a vendored copy—to test network + validation integration.

### 11.4 Flask Verifier API (`app/api/`)

**Endpoints (proposed):**

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/health` | GET | Liveness for harness |
| `/verify/key` | POST | Resolve issuer public key for a certificate |
| `/verify/revocation` | POST | Return revoked or active status |
| `/verify/edge` | POST | Verify Ed25519 signature over edge canonical form |

**Why:** Decouples cryptographic policy from C++; allows tests to mock or inspect verification behavior; matches “API Integration” subtype.

### 11.5 PostgreSQL Layer (`app/database/`)

**Handles:**

- DDL for service nodes (cert metadata, labels, timestamps)
- DDL for trust edges (source, target, signature, policy attributes)
- Constraints: unique service IDs, foreign keys, no duplicate edges
- Staging tables optional for `COPY` then merge

**Why:** `libpq COPY` requires well-defined column order and delimiters; schema is the contract for tests.

### 11.6 Certificate Fixtures (`app/certificates/`)

**Handles:**

- Valid leaf certificates for services
- Revoked certificate for negative tests
- Issuer key pairs for API verification

**Why:** Revocation and key lookup cannot be tested without realistic material.

### 11.7 Build & Run Scripts (`app/scripts/`)

**Handles:**

- `build.sh` — compile C++ with libpq, XML, HTTP deps
- `init_db.sh` — create DB, run migrations
- `run_pipeline.sh` — env vars, paths, execute recovery binary

**Why:** Oracle and agents need a single entry pattern; Docker CMD will invoke these.

### 11.8 Harness Artifacts (later, not in this phase)

| Artifact | Purpose |
|----------|---------|
| `instruction.md` | Agent prompt with corruption details and API contracts |
| `task.toml` | Time limits, resources, metadata |
| `environment/Dockerfile` | Pin tool versions |
| `solution/solve.sh` | Reference implementation |
| `tests/test.sh`, `tests/test_outputs.py` | Automated grading |

---

## 12. Development Roadmap

### Phase 1 — Foundation (no agent-facing content yet)

- [ ] Finalize PostgreSQL schema (`app/database/schema.sql`)
- [ ] Create corrupted GraphML fixture with all four defect types
- [ ] Implement Flask verifier API with fixture-driven responses
- [ ] Add certificate/key/revocation test data

**Exit criteria:** API and DB can start independently; fixtures are hand-inspectable.

### Phase 2 — C++ Pipeline Core

- [ ] CMake/Makefile and dependency wiring (libpq, XML, HTTP)
- [ ] GraphML repair module with unit-testable functions
- [ ] Live XSD fetch + validation pass
- [ ] Write `repaired.graphml` to `app/output/`

**Exit criteria:** Repaired file validates against live GraphML XSD.

### Phase 3 — Verification Integration

- [ ] HTTP client wrappers for Flask endpoints
- [ ] Node verification loop (key + revocation)
- [ ] Edge verification loop (Ed25519 via API)
- [ ] Filter: only verified records proceed to ingest

**Exit criteria:** Pipeline skips or flags revoked/invalid edges per fixture design.

### Phase 4 — PostgreSQL Ingest

- [ ] libpq connection management
- [ ] `COPY FROM` for nodes and edges (CSV or binary format)
- [ ] Transaction boundaries and error handling

**Exit criteria:** Row counts match expected verified subset; no duplicate keys.

### Phase 5 — Reporting

- [ ] Define JSON report schema
- [ ] Populate repair log, validation, verification, DB metrics
- [ ] Validate report JSON against schema before exit

**Exit criteria:** `app/output/report.json` is complete and testable.

### Phase 6 — Container & Harness

- [ ] `environment/Dockerfile` with C++, Python, PostgreSQL client
- [ ] `instruction.md`, `task.toml`
- [ ] `solution/solve.sh` oracle
- [ ] `tests/test.sh` + `tests/test_outputs.py`

**Exit criteria:** Docker build succeeds; oracle passes; CI green; difficulty targets met.

### Phase 7 — Rubric & Hardening

- [ ] Rubric aligned to repair, verify, COPY, and report behaviors
- [ ] Edge-case review: network failure on XSD fetch, API timeout, partial graph
- [ ] README for local reproduction

**Exit criteria:** Task ready for Snorkel Terminus Edition 2 submission.

---

## Open Decisions (to resolve before implementation)

1. **Exact GraphML data model** — attribute names for `service_id`, cert PEM, edge signature, policy fields.
2. **Duplicate merge policy** — which node wins when two duplicates differ only in non-critical metadata.
3. **Partial success semantics** — ingest verified subset vs. all-or-nothing transaction.
4. **Canonical edge payload** — byte-exact format for Ed25519 verification (critical for API contract).
5. **Report JSON schema** — formal JSON Schema file location (`app/schema/report.schema.json`?).

---

## Approval Gate

This document is **planning only**. No Dockerfile, `instruction.md`, `task.toml`, solution, or tests have been written.

**Awaiting approval before proceeding to Phase 1 implementation.**
