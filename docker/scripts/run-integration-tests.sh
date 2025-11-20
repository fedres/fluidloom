#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Running FluidLoom integration tests with 2 MPI ranks..."

docker run --rm \
  -v "$(pwd)/..:/workspace" \
  fluidloom:test \
  bash -c "cd /workspace/build && mpirun -np 2 --allow-run-as-root ./tests/integration/transport/test_two_gpu_exchange"

echo "Integration tests complete!"
