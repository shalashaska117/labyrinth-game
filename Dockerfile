FROM gcc:13

WORKDIR /app

RUN apt-get update \
    && apt-get install -y --no-install-recommends libhiredis-dev \
    && rm -rf /var/lib/apt/lists/*

COPY . .

RUN make clean || true
RUN make release

CMD ["./server_app", "8080"]
