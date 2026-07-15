#!/usr/bin/env bash
set -euo pipefail

export PGDATA="${PGDATA:-/var/lib/postgresql/data}"
export POSTGRES_USER="${POSTGRES_USER:-mtls}"
export POSTGRES_PASSWORD="${POSTGRES_PASSWORD:-mtls}"
export POSTGRES_DB="${POSTGRES_DB:-mtls_trust}"

PG_VERSION="$(ls /usr/lib/postgresql | sort -nr | head -1)"
export PATH="/usr/lib/postgresql/${PG_VERSION}/bin:/usr/bin:${PATH}"

mkdir -p "$PGDATA"
chown -R postgres:postgres "$PGDATA"

if [ ! -f "$PGDATA/PG_VERSION" ]; then
  su - postgres -c "PATH=${PATH} initdb -D '${PGDATA}' --auth=trust"
  echo "host all all 127.0.0.1/32 trust" >> "$PGDATA/pg_hba.conf"
fi

if ! su - postgres -c "PATH=${PATH} pg_isready -q"; then
  su - postgres -c "PATH=${PATH} pg_ctl -D '${PGDATA}' -o '-c listen_addresses=127.0.0.1' -w start"
fi

for _ in $(seq 1 30); do
  if su - postgres -c "PATH=${PATH} pg_isready -q"; then
    break
  fi
  sleep 1
done

su - postgres -c "PATH=${PATH} psql -tc \"SELECT 1 FROM pg_roles WHERE rolname='${POSTGRES_USER}'\"" | grep -q 1 || \
  su - postgres -c "PATH=${PATH} psql -c \"CREATE USER ${POSTGRES_USER} WITH PASSWORD '${POSTGRES_PASSWORD}';\""

su - postgres -c "PATH=${PATH} psql -tc \"SELECT 1 FROM pg_database WHERE datname='${POSTGRES_DB}'\"" | grep -q 1 || \
  su - postgres -c "PATH=${PATH} psql -c \"CREATE DATABASE ${POSTGRES_DB} OWNER ${POSTGRES_USER};\""

psql "postgresql://${POSTGRES_USER}:${POSTGRES_PASSWORD}@127.0.0.1:5432/${POSTGRES_DB}" \
  -f /app/database/schema.sql
