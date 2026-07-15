#!/usr/bin/env python3
"""Flask verifier API for mTLS trust graph recovery."""

from __future__ import annotations

import base64

from flask import Flask, jsonify, request
from nacl.exceptions import BadSignatureError
from nacl.signing import SigningKey, VerifyKey

from config import (
    EDGE_FIXTURES,
    ISSUER_KEY_ID,
    ISSUER_SEED,
    KNOWN_SERIALS,
    REVOKED_SERIALS,
    VERIFIER_HOST,
    VERIFIER_PORT,
)

app = Flask(__name__)

_SIGNING_KEY = SigningKey(ISSUER_SEED)
_VERIFY_KEY: VerifyKey = _SIGNING_KEY.verify_key


def canonical_edge(source: str, target: str, policy: str) -> bytes:
    return f"{source}|{target}|{policy}".encode("utf-8")


def sign_edge(source: str, target: str, policy: str) -> str:
    message = canonical_edge(source, target, policy)
    signature = _SIGNING_KEY.sign(message).signature
    return base64.b64encode(signature).decode("ascii")


@app.get("/health")
def health():
    return jsonify({"status": "ok"})


@app.post("/verify/key")
def verify_key():
    payload = request.get_json(silent=True) or {}
    cert_serial = payload.get("cert_serial")
    if not cert_serial or cert_serial not in KNOWN_SERIALS:
        return jsonify({"valid": False, "issuer_key_id": None})
    return jsonify({"valid": True, "issuer_key_id": ISSUER_KEY_ID})


@app.post("/verify/revocation")
def verify_revocation():
    payload = request.get_json(silent=True) or {}
    cert_serial = payload.get("cert_serial")
    if not cert_serial:
        return jsonify({"revoked": True, "error": "missing cert_serial"}), 400
    return jsonify({"revoked": cert_serial in REVOKED_SERIALS})


@app.post("/verify/edge")
def verify_edge():
    payload = request.get_json(silent=True) or {}
    source = payload.get("source")
    target = payload.get("target")
    policy = payload.get("policy")
    signature_b64 = payload.get("signature")

    if not all([source, target, policy, signature_b64]):
        return jsonify({"valid": False, "error": "missing fields"}), 400

    try:
        signature = base64.b64decode(signature_b64)
        message = canonical_edge(source, target, policy)
        _VERIFY_KEY.verify(message, signature)
        return jsonify({"valid": True})
    except (BadSignatureError, ValueError):
        return jsonify({"valid": False})


if __name__ == "__main__":
    app.run(host=VERIFIER_HOST, port=VERIFIER_PORT)
