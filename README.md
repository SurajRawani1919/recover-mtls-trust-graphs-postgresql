# Recover mTLS Trust Graphs into PostgreSQL with C++

TerminalBench Edition 2 (non-milestone) task for Snorkel Terminus.

## Summary

Agents build a C++ recovery pipeline that repairs corrupted GraphML mTLS trust graphs, validates against the live GraphML XSD, verifies nodes and edges through a Flask API, and bulk-loads verified records into PostgreSQL via `libpq COPY`.

## Layout

- `instruction.md` — agent prompt
- `task.toml` — harness metadata
- `environment/Dockerfile` — container image
- `app/` — task environment (fixtures, API, DB schema, pipeline stubs)
- `solution/` — oracle (`solve.sh` + reference C++ pipeline)
- `tests/` — verifier (`test.sh`, `test_outputs.py`)

## Local notes

The Docker image generates deterministic cryptographic fixtures at build time via `app/scripts/gen_fixtures.py`.

The oracle copies `solution/pipeline/` into `/app/pipeline/`, builds `recover_graph`, starts PostgreSQL and the verifier API, and runs the full pipeline.

## Expected oracle outcome

- 3 verified services loaded
- 3 verified trust edges loaded
- `svc-legacy-99` and edge `e3` rejected
- `app/output/repaired.graphml` and `app/output/report.json` produced
