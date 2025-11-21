# Performance Benchmarking Framework

## Overview

Microbenchmarks and scaling tests for FluidLoom performance validation.

## Benchmark Categories

### 1. Rebuild Benchmarks (`rebuild/`)
- Hash table rebuild pipeline
- Sort performance (target: < 5ms for 1M cells)
- Compaction (target: < 2ms)
- Hash build (target: < 3ms)
- **Total target: < 10ms for 1M cells**

### 2. Halo Benchmarks (`halo/`)
- Pack/unpack bandwidth
- Overlap efficiency (target: > 90%)
- MPI latency breakdown
- PCIe bandwidth (target: > 10 GB/s for PCIe 3.0)

### 3. Fusion Benchmarks (`fusion/`)
- Kernel launch count reduction
- Register pressure impact
- Performance improvement measurement

### 4. Scaling Benchmarks (`scaling/`)
- Weak scaling: 1, 2, 4, 8 GPUs
- Strong scaling: fixed problem size
- Efficiency curves
- Communication/compute ratio

## Running Benchmarks

```bash
# Build performance binaries
cmake -B build_perf -DFLUIDLOOM_ENABLE_BENCHMARKS=ON
cmake --build build_perf

# Run specific benchmark
./build_perf/tests/perf/rebuild/rebuild_benchmark

# Run all benchmarks
ctest -R '^perf_'
```

## Performance Gates

All benchmarks must meet targets defined in `ci/configs/performance_gates.json`.
Regression threshold: 5% degradation triggers CI failure.
