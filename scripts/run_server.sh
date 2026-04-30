#!/bin/bash

set -e

echo "Starting server container..."

docker compose up --build server
