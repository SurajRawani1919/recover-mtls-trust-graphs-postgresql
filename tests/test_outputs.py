"""Verify the mTLS trust-graph recovery pipeline output: repaired GraphML,
report.json schema/content, the PostgreSQL services/trust_edges tables, and
that the pipeline actually performed a compiled C++ run against the live
verifier API rather than shortcutting to a hardcoded answer.
"""

import json
import os
import re
import xml.etree.ElementTree as ET
from pathlib import Path

import jsonschema
import psycopg2
import pytest

APP_ROOT = Path("/app")
OUTPUT_GRAPH = APP_ROOT / "output" / "repaired.graphml"
OUTPUT_REPORT = APP_ROOT / "output" / "report.json"
REPORT_SCHEMA = APP_ROOT / "schema" / "report.schema.json"
RECOVER_BINARY = APP_ROOT / "bin" / "recover_graph"
VERIFIER_LOG = Path("/tmp/verifier.log")
DATABASE_URL = os.environ.get(
    "DATABASE_URL", "postgresql://mtls:mtls@127.0.0.1:5432/mtls_trust"
)

GRAPHML_NS = "http://graphml.graphdrawing.org/xmlns"
NS = {"g": GRAPHML_NS}
REQUIRED_KEY_IDS = {"d_svc", "d_label", "d_serial", "d_version", "d_policy", "d_sig"}

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


@pytest.fixture(scope="module")
def graphml_tree() -> ET.ElementTree:
    assert OUTPUT_GRAPH.exists(), "repaired.graphml was not created"
    return ET.parse(str(OUTPUT_GRAPH))


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


def test_binary_is_compiled_elf():
    """/app/bin/recover_graph must be a real compiled native binary (anti-cheating:
    rules out a Python/bash script masquerading as the required C++17 pipeline)."""
    assert RECOVER_BINARY.exists(), "recover_graph binary was not built"
    header = RECOVER_BINARY.read_bytes()[:4]
    assert header == b"\x7fELF", "recover_graph is not a native ELF executable"


def test_verifier_api_was_called():
    """Pipeline must call the Flask verifier endpoints for every node and eligible
    edge (anti-cheating: rules out reading config.py's fixture constants directly
    and writing the expected rows/report without ever contacting the API)."""
    assert VERIFIER_LOG.exists(), "verifier API log not found; API may never have started"
    log = VERIFIER_LOG.read_text(encoding="utf-8", errors="ignore")
    key_calls = len(re.findall(r"POST /verify/key", log))
    revocation_calls = len(re.findall(r"POST /verify/revocation", log))
    edge_calls = len(re.findall(r"POST /verify/edge", log))
    assert key_calls >= 4, f"expected >=4 /verify/key calls (one per node), got {key_calls}"
    assert revocation_calls >= 4, (
        f"expected >=4 /verify/revocation calls (one per node), got {revocation_calls}"
    )
    assert edge_calls >= 3, f"expected >=3 /verify/edge calls, got {edge_calls}"


def test_graphml_keys_declared_before_graph(graphml_tree):
    """All six GraphML <key> declarations must exist as direct children of
    <graphml> preceding the <graph> element, not injected after it."""
    root = graphml_tree.getroot()
    children = list(root)
    graph_index = next(
        i for i, el in enumerate(children) if el.tag == f"{{{GRAPHML_NS}}}graph"
    )
    key_ids_before_graph = {
        el.get("id")
        for el in children[:graph_index]
        if el.tag == f"{{{GRAPHML_NS}}}key"
    }
    missing = REQUIRED_KEY_IDS - key_ids_before_graph
    assert not missing, f"missing key declarations before <graph>: {missing}"


def test_graphml_nodes_deduplicated(graphml_tree):
    """Duplicate svc-auth-01 nodes must be collapsed to a single node retaining
    the higher version, not left as two separate <node> elements."""
    graph = graphml_tree.getroot().find("g:graph", NS)
    service_ids = [
        data.text
        for node in graph.findall("g:node", NS)
        for data in node.findall('g:data[@key="d_svc"]', NS)
    ]
    assert service_ids.count("svc-auth-01") == 1, (
        "expected exactly one svc-auth-01 node after deduplication, "
        f"found {service_ids.count('svc-auth-01')}"
    )

    versions_for_auth = [
        data.text
        for node in graph.findall("g:node", NS)
        if any(d.text == "svc-auth-01" for d in node.findall('g:data[@key="d_svc"]', NS))
        for data in node.findall('g:data[@key="d_version"]', NS)
    ]
    assert "2" in versions_for_auth, "deduplication must keep the higher-version svc-auth-01 node"


def test_graphml_passes_independent_xsd_validation():
    """Independently re-validate the repaired GraphML against the live XSD rather
    than trusting the pipeline's self-reported xsd_validation.passed flag."""
    from lxml import etree

    xsd_url = "http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd"
    schema_doc = etree.parse(xsd_url)
    schema = etree.XMLSchema(schema_doc)
    doc = etree.parse(str(OUTPUT_GRAPH))
    assert schema.validate(doc), f"independent XSD validation failed: {schema.error_log}"
