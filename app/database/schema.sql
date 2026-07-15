-- PostgreSQL bootstrap schema for verified mTLS trust graph ingest.

CREATE TABLE IF NOT EXISTS services (
    service_id TEXT PRIMARY KEY,
    label TEXT NOT NULL,
    cert_serial TEXT NOT NULL,
    issuer_key_id TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS trust_edges (
    edge_id TEXT PRIMARY KEY,
    source_service_id TEXT NOT NULL REFERENCES services(service_id),
    target_service_id TEXT NOT NULL REFERENCES services(service_id),
    policy TEXT NOT NULL,
    signature TEXT NOT NULL,
    UNIQUE (source_service_id, target_service_id, policy)
);

CREATE INDEX IF NOT EXISTS idx_trust_edges_source ON trust_edges(source_service_id);
CREATE INDEX IF NOT EXISTS idx_trust_edges_target ON trust_edges(target_service_id);
