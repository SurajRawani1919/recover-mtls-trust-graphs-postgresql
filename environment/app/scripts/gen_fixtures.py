#!/usr/bin/env python3
"""Generate deterministic cryptographic and revocation fixtures."""

from __future__ import annotations

import base64
import json
import sys
from pathlib import Path

from nacl.signing import SigningKey

APP_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(APP_ROOT / "api"))

from config import EDGE_FIXTURES, ISSUER_SEED, REVOKED_SERIALS  # noqa: E402


def canonical_edge(source: str, target: str, policy: str) -> bytes:
    return f"{source}|{target}|{policy}".encode("utf-8")


def main() -> None:
    signing_key = SigningKey(ISSUER_SEED)
    verify_key_b64 = base64.b64encode(bytes(signing_key.verify_key)).decode("ascii")

    signatures = {}
    for edge_id, source, target, policy in EDGE_FIXTURES:
        message = canonical_edge(source, target, policy)
        signature = signing_key.sign(message).signature
        signatures[edge_id] = base64.b64encode(signature).decode("ascii")

    cert_dir = APP_ROOT / "certificates"
    cert_dir.mkdir(parents=True, exist_ok=True)

    (cert_dir / "issuer_public_key.b64").write_text(verify_key_b64, encoding="utf-8")

    (cert_dir / "revoked.json").write_text(
        json.dumps({"revoked_serials": sorted(REVOKED_SERIALS)}, indent=2),
        encoding="utf-8",
    )

    graph_dir = APP_ROOT / "graph"
    graph_dir.mkdir(parents=True, exist_ok=True)
    (graph_dir / "detached_signatures.json").write_text(
        json.dumps({"signatures": signatures}, indent=2),
        encoding="utf-8",
    )

    print("Generated fixtures:")
    print(f"  issuer public key -> {cert_dir / 'issuer_public_key.b64'}")
    print(f"  revoked serials   -> {cert_dir / 'revoked.json'}")
    print(f"  detached sigs     -> {graph_dir / 'detached_signatures.json'}")


if __name__ == "__main__":
    main()
