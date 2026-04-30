#!/bin/bash

set -e

echo "Building Docker images..."

docker compose down --remove-orphans
docker compose build

echo "Docker build completed successfully."
