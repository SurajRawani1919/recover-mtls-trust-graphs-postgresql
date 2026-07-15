"""Verifier API configuration."""

from pathlib import Path

APP_ROOT = Path(__file__).resolve().parent.parent
CERT_DIR = APP_ROOT / "certificates"

# Deterministic 32-byte issuer seed for reproducible Ed25519 signatures.
ISSUER_SEED = b"mtls_trust_graph_issuer_seed_012"  # exactly 32 bytes for Ed25519

ISSUER_KEY_ID = "platform-issuer-v1"
VERIFIER_HOST = "0.0.0.0"
VERIFIER_PORT = 5001

REVOKED_SERIALS = {"LEGACY-0099"}

KNOWN_SERIALS = {
    "AUTH-0001": {"service_id": "svc-auth-01", "label": "Auth Service"},
    "PAY-0002": {"service_id": "svc-pay-02", "label": "Payment Service"},
    "NOTIFY-0003": {"service_id": "svc-notify-03", "label": "Notify Service"},
    "LEGACY-0099": {"service_id": "svc-legacy-99", "label": "Legacy Gateway"},
}

EDGE_FIXTURES = [
    ("e1", "svc-auth-01", "svc-pay-02", "allow:read"),
    ("e2", "svc-pay-02", "svc-notify-03", "allow:write"),
    ("e3", "svc-auth-01", "svc-legacy-99", "allow:admin"),
    ("e4", "svc-notify-03", "svc-auth-01", "allow:ping"),
]
