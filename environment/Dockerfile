FROM public.ecr.aws/docker/library/ubuntu:24.04@sha256:0d39fcc8335d6d74d5502f6df2d30119ff4790ebbb60b364818d5112d9e3e932

ENV DEBIAN_FRONTEND=noninteractive
ENV PGDATA=/var/lib/postgresql/data
ENV POSTGRES_USER=mtls
ENV POSTGRES_PASSWORD=mtls
ENV POSTGRES_DB=mtls_trust
ENV FLASK_VERIFIER_URL=http://127.0.0.1:5001
ENV DATABASE_URL=postgresql://mtls:mtls@127.0.0.1:5432/mtls_trust

RUN apt-get update && apt-get install -y --no-install-recommends \
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
    && rm -rf /var/lib/apt/lists/*

RUN update-ca-certificates

COPY app/api/requirements.txt /tmp/api-requirements.txt
COPY test-requirements.txt /tmp/test-requirements.txt

RUN pip3 install --no-cache-dir --break-system-packages \
    -r /tmp/api-requirements.txt \
    -r /tmp/test-requirements.txt

WORKDIR /app

COPY app/ /app/

RUN python3 /app/scripts/gen_fixtures.py

RUN mkdir -p /app/output && chmod 777 /app/output

RUN find /app/scripts -name '*.sh' -exec sed -i 's/\r$//' {} + && \
    chmod +x /app/scripts/*.sh 2>/dev/null || true

EXPOSE 5001 5432

CMD ["/bin/bash"]
