# Recover mTLS Trust Graphs into PostgreSQL with C++

TerminalBench Edition 2 (non-milestone) task for Snorkel Terminus.

**Task ID:** `snorkel/recover-mtls-trust-graphs-postgresql`  
**Difficulty:** Hard | **Domain:** Security & Cryptography

## Summary

Agents build a C++ recovery pipeline that repairs corrupted GraphML mTLS trust graphs, validates against the live GraphML XSD, verifies nodes and edges through a Flask API, and bulk-loads verified records into PostgreSQL via `libpq COPY`.

## Repository layout

```
.
├── instruction.md              # Agent prompt
├── task.toml                   # Harness metadata
├── PROJECT_PLAN.md             # Full design & status
├── RUBRIC.md                   # Experts platform rubric (copy-paste)
├── EXPERTS_PLATFORM.md         # Submission & platform steps
│
├── environment/                # Harbor Docker build context
│   ├── Dockerfile
│   ├── .dockerignore
│   └── app/                    # Copied to /app/ in the container
│       ├── graph/              # Corrupted fixtures + fragments
│       ├── api/                # Flask verifier (port 5001)
│       ├── database/           # schema.sql
│       ├── certificates/       # Generated at image build
│       ├── pipeline/           # C++ stubs for the agent
│       ├── schema/             # report.schema.json
│       ├── scripts/            # build, init_db, start_services, run_pipeline
│       └── output/             # repaired.graphml, report.json (runtime)
│
├── solution/                   # Oracle (not visible to agents at runtime)
│   ├── solve.sh
│   └── pipeline/               # Reference C++ implementation
│
├── tests/
│   ├── test.sh
│   └── test_outputs.py
│
└── scripts/                    # Local Snorkel CLI helpers (not in image)
    ├── run-harness.sh          # oracle / gpt / claude / submit / verify
    └── load-stb-env.sh.example # SNORKEL_API_KEY template
```

> **Harbor note:** Docker builds only from `environment/`. Task files live at `environment/app/` on disk and appear as `/app/` inside the container.

## Local validation (WSL + Docker)

Prerequisites: Docker Desktop, Ubuntu WSL, `stb` CLI, `docker compose` v2.

```bash
wsl -d Ubuntu-24.04
cd /mnt/c/Users/rawan/OneDrive/Desktop/recover-mtls-trust-graphs-postgresql
source scripts/load-stb-env.sh   # SNORKEL_API_KEY only

./scripts/run-harness.sh verify   # Docker + auth + zip dry-run
./scripts/run-harness.sh oracle   # Must return Mean 1.000
./scripts/run-harness.sh gpt        # Optional difficulty calibration
./scripts/run-harness.sh claude   # Optional difficulty calibration
```

Manual Docker build (same context Harbor uses):

```bash
cd environment
docker build -t recover-mtls-test .
```

## Oracle workflow

1. `solution/solve.sh` copies `solution/pipeline/` → `/app/pipeline/`
2. `/app/scripts/build.sh` compiles `recover_graph`
3. `/app/scripts/start_services.sh` starts PostgreSQL + Flask verifier
4. Pipeline repairs graph → XSD validate → API verify → `COPY` ingest → JSON report
5. `tests/test.sh` runs 8 pytest checks; reward written to `/logs/verifier/reward.txt`

Fixtures are generated at image build time via `environment/app/scripts/gen_fixtures.py`.

## Expected oracle outcome

| Check | Expected |
|-------|----------|
| Verified services in DB | `svc-auth-01`, `svc-pay-02`, `svc-notify-03` |
| Verified edges in DB | `e1`, `e2`, `e4` |
| Rejected service | `svc-legacy-99` (revoked) |
| Rejected edge | `e3` (revoked endpoint) |
| Outputs | `/app/output/repaired.graphml`, `/app/output/report.json` |
| Oracle reward | **1.0** (8/8 tests) |

## Submission

See **`EXPERTS_PLATFORM.md`** for full steps. Summary:

1. Enroll in **Terminus-2nd-Edition** on the Experts platform
2. `stb login` → `stb projects list` must show your project
3. `./scripts/run-harness.sh submit Terminus-2nd-Edition 180`
4. Paste rubric from **`RUBRIC.md`** on the platform

## Project status (~90% complete)

| Area | Status |
|------|--------|
| Task development | ✅ Complete |
| Oracle + local Harbor validation | ✅ Mean 1.000 |
| Agent runs (GPT-5.5, Claude Opus 4.8) | ✅ Both 1.000 on first trial |
| GitHub | ✅ Pushed to `main` |
| Platform submission + rubric paste | ⏳ Blocked on project access |

## Links

- **GitHub:** https://github.com/SurajRawani1919/recover-mtls-trust-graphs-postgresql
- **Live XSD (runtime fetch):** `http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd`
