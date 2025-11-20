# FluidLoom Multi-GPU Testing with Docker

This directory contains Docker infrastructure for testing FluidLoom's multi-GPU capabilities using MPI.

## Prerequisites

- Docker Desktop or Docker Engine
- Docker Compose (optional)
- At least 8GB RAM allocated to Docker

## Quick Start

```bash
cd docker

# Build the Docker image
./scripts/build.sh

# Run integration tests (2 MPI ranks simulating 2 GPUs)
./scripts/run-integration-tests.sh

# Run performance benchmarks
./scripts/run-benchmarks.sh

# Run all tests
./scripts/run-tests.sh
```

## Testing Approach

We simulate a two-GPU environment using:
1. **MPI with 2 ranks**: Each rank represents one GPU
2. **MockBackend**: Simulates GPU operations
3. **CPU-based OpenCL**: Uses OpenCL CPU devices if available

## Performance Notes

Docker measurements may differ from real hardware:
- Latency targets (< 1ms) may be higher due to containerization
- P2P tests use fallback paths without real GPUs
- Overlap measurements validate algorithmic correctness

## Real GPU Testing (NVIDIA only)

```bash
docker run --gpus all -v $(pwd)/..:/workspace fluidloom:test \
  mpirun -np 2 ./build/tests/integration/transport/test_two_gpu_exchange
```

Requires: NVIDIA Container Toolkit
