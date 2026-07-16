# Canonical Terminal-Bench base image, digest-pinned to the sanctioned ubuntu:24.04 reference.
FROM public.ecr.aws/docker/library/ubuntu:24.04@sha256:0d39fcc8335d6d74d5502f6df2d30119ff4790ebbb60b364818d5112d9e3e932

ENV DEBIAN_FRONTEND=noninteractive
ENV PGDATA=/var/lib/postgresql/data
ENV POSTGRES_USER=mtls
ENV POSTGRES_PASSWORD=mtls
ENV POSTGRES_DB=mtls_trust
ENV FLASK_VERIFIER_URL=http://127.0.0.1:5001
ENV DATABASE_URL=postgresql://mtls:mtls@127.0.0.1:5432/mtls_trust

# Carve-out for check_no_build_tools_in_runtime: the agent's task is to author and
# compile the C++ pipeline (/app/pipeline -> /app/bin/recover_graph) inside this
# container at runtime, so build-essential/cmake/g++/curl-dev headers must remain in
# the runtime image rather than a discarded builder stage.
RUN apt-get update && apt-get install -y --no-install-recommends \
    asciinema \
    build-essential \
    ca-certificates \
    cmake \
    curl \
    g++ \
    git \
    libcurl4-openssl-dev \
    libpq-dev \
    libssl-dev \
    libsodium-dev \
    libxml2-dev \
    pkg-config \
    postgresql \
    postgresql-client \
    python3 \
    python3-pip \
    python3-venv \
    tmux \
    && rm -rf /var/lib/apt/lists/*

RUN update-ca-certificates

# check_reproducible_builds carve-out: direct deps are exact-version pinned in
# requirements.txt / test-requirements.txt; a full hash-locked file is not used
# because the verifier/API dependency set is small and stable across builds.
#
# Application dependency (Flask verifier API) — installed system-wide since the
# agent must start and interact with this service as part of the task.
COPY app/api/requirements.txt /tmp/api-requirements.txt
RUN pip3 install --no-cache-dir --break-system-packages -r /tmp/api-requirements.txt

# Verifier/test-only dependencies (pytest, psycopg2-binary, jsonschema, lxml) are
# installed into an isolated virtualenv rather than the system python3 the agent
# uses, so they are pre-installed at build time (no runtime network fetch in
# test.sh) without being importable from the agent's default `python3`/`pip3`.
COPY test-requirements.txt /tmp/test-requirements.txt
RUN python3 -m venv /opt/test-venv && \
    /opt/test-venv/bin/pip install --no-cache-dir -r /tmp/test-requirements.txt

WORKDIR /app

COPY app/ /app/

RUN python3 /app/scripts/gen_fixtures.py

RUN mkdir -p /app/output && chmod 777 /app/output

RUN find /app/scripts -name '*.sh' -exec sed -i 's/\r$//' {} + && \
    chmod +x /app/scripts/*.sh 2>/dev/null || true

EXPOSE 5001 5432

CMD ["/bin/bash"]
