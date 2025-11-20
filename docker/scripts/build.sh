#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Building FluidLoom Docker image..."
docker build -t fluidloom:test -f Dockerfile ..

echo "Build complete! Image: fluidloom:test"
