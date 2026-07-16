"""Verify the mTLS trust-graph recovery pipeline output: repaired GraphML,
report.json schema/content, and the PostgreSQL services/trust_edges tables.
"""

import json
import os
from pathlib import Path

import jsonschema
import psycopg2
import pytest

APP_ROOT = Path("/app")
OUTPUT_GRAPH = APP_ROOT / "output" / "repaired.graphml"
OUTPUT_REPORT = APP_ROOT / "output" / "report.json"
REPORT_SCHEMA = APP_ROOT / "schema" / "report.schema.json"
DATABASE_URL = os.environ.get(
    "DATABASE_URL", "postgresql://mtls:mtls@127.0.0.1:5432/mtls_trust"
)

EXPECTED_SERVICES = {
    "svc-auth-01",
    "svc-pay-02",
    "svc-notify-03",
}
EXPECTED_EDGES = {"e1", "e2", "e4"}
REJECTED_SERVICE = "svc-legacy-99"
REJECTED_EDGE = "e3"


@pytest.fixture(scope="module")
def report() -> dict:
    assert OUTPUT_REPORT.exists(), "report.json was not created"
    return json.loads(OUTPUT_REPORT.read_text(encoding="utf-8"))


@pytest.fixture(scope="module")
def report_schema() -> dict:
    return json.loads(REPORT_SCHEMA.read_text(encoding="utf-8"))


@pytest.fixture(scope="module")
def db_conn():
    conn = psycopg2.connect(DATABASE_URL)
    yield conn
    conn.close()


def test_repaired_graph_exists():
    """Repaired GraphML must be written to /app/output/repaired.graphml with expected service content."""
    assert OUTPUT_GRAPH.exists(), "repaired.graphml was not created"
    content = OUTPUT_GRAPH.read_text(encoding="utf-8")
    assert "<graphml" in content
    assert "svc-auth-01" in content
    assert "allow:read" in content


def test_report_matches_schema(report, report_schema):
    """Recovery report JSON must validate against /app/schema/report.schema.json."""
    jsonschema.validate(instance=report, schema=report_schema)


def test_report_status(report):
    """Report must record successful XSD validation against the live GraphML schema URL."""
    assert report["status"] in {"success", "partial"}
    assert report["xsd_validation"]["passed"] is True
    assert "graphml.graphdrawing.org" in report["xsd_validation"]["schema_url"]


def test_report_repairs(report):
    """Report must document all four corruption repair categories applied to the fixtures."""
    actions = {entry["action"] for entry in report["repairs"]}
    assert "merge_fragments" in actions
    assert "inject_keys" in actions
    assert "deduplicate_nodes" in actions
    assert "reattach_signatures" in actions


def test_report_counts(report):
    """Report verification and database counts must match the expected verified and rejected fixtures."""
    assert report["verification"]["nodes_verified"] == 3
    assert report["verification"]["edges_verified"] == 3
    assert report["database"]["nodes_loaded"] == 3
    assert report["database"]["edges_loaded"] == 3
    assert REJECTED_SERVICE in report["verification"]["rejected_nodes"]
    assert REJECTED_EDGE in report["verification"]["rejected_edges"]


def test_services_table(db_conn):
    """PostgreSQL services table must contain exactly the three verified, non-revoked services."""
    with db_conn.cursor() as cur:
        cur.execute("SELECT service_id FROM services ORDER BY service_id")
        rows = {row[0] for row in cur.fetchall()}
    assert rows == EXPECTED_SERVICES
    assert REJECTED_SERVICE not in rows


def test_trust_edges_table(db_conn):
    """PostgreSQL trust_edges table must contain exactly the three verified edges and exclude revoked edge e3."""
    with db_conn.cursor() as cur:
        cur.execute("SELECT edge_id FROM trust_edges ORDER BY edge_id")
        rows = {row[0] for row in cur.fetchall()}
    assert rows == EXPECTED_EDGES
    assert REJECTED_EDGE not in rows


def test_edge_signature_reattached():
    """Repaired GraphML must reattach detached Ed25519 signatures as d_sig data elements on edges."""
    content = OUTPUT_GRAPH.read_text(encoding="utf-8")
    assert 'key="d_sig"' in content
