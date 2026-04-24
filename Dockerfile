# ---- Build stage ----
FROM gcc:13-bookworm AS builder

WORKDIR /app

COPY Makefile .
COPY src/ src/

RUN make all

# ---- Runtime stage ----
FROM debian:bookworm-slim AS server

RUN apt-get update && apt-get install -y --no-install-recommends \
    libc6 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/bin/server /usr/local/bin/server

EXPOSE 8080

CMD ["server"]

# ---- Client runtime ----
FROM debian:bookworm-slim AS client

RUN apt-get update && apt-get install -y --no-install-recommends \
    libc6 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/bin/client /usr/local/bin/client

ENTRYPOINT ["client"]
