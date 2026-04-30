#!/bin/bash

set -e

IP=${1:-server}
PORT=${2:-5000}
NUM_CLIENTS=${3:-3}

echo "Starting $NUM_CLIENTS Docker clients connected to $IP:$PORT..."

pids=()

for ((i=1; i<=NUM_CLIENTS; i++)); do
    echo "Starting client $i..."
    docker compose run --rm client "$IP" "$PORT" &
    pids+=($!)
done

echo "All clients started. Press Ctrl+C to stop."

for pid in "${pids[@]}"; do
    wait "$pid"
done
