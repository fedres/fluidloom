#!/bin/bash
set -e

cd "$(dirname "$0")/.."

echo "Running FluidLoom performance benchmarks..."

mkdir -p results

docker run --rm \
  -v "$(pwd)/..:/workspace" \
  -v "$(pwd)/results:/workspace/build/results" \
  fluidloom:test \
  bash -c "cd /workspace/build && \
    ./tests/benchmark/benchmark_transport --benchmark_format=json --benchmark_out=results/transport_benchmark.json && \
    cat results/transport_benchmark.json"

echo "Benchmarks complete! Results in docker/results/"
