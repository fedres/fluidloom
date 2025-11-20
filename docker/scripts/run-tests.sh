#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "=== FluidLoom Docker Test Suite ==="
echo ""

# Build
echo "Step 1: Building Docker image..."
./scripts/build.sh
echo ""

# Unit tests
echo "Step 2: Running unit tests..."
docker run --rm -v "$(pwd)/..:/workspace" fluidloom:test \
  bash -c "cd /workspace/build && ctest --output-on-failure -R '.*Unit.*'"
echo ""

# Integration tests
echo "Step 3: Running integration tests (2 MPI ranks)..."
./scripts/run-integration-tests.sh
echo ""

# Benchmarks
echo "Step 4: Running performance benchmarks..."
./scripts/run-benchmarks.sh
echo ""

echo "=== All tests complete! ==="
