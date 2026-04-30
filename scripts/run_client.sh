#!/bin/bash

set -e

IP=${1:-server}
PORT=${2:-5000}

echo "Starting client connected to $IP:$PORT..."

docker compose run --rm client "$IP" "$PORT"
