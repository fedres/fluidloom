# Module 16: Integration & System Test Suite

Complete test infrastructure for FluidLoom production readiness validation.

## Quick Start

### Build Tests

```bash
cd /Users/karthikt/Ideas/FluidLoom/fluidloom
mkdir -p build
cd build

# Configure with tests enabled
cmake .. -DFLUIDLOOM_ENABLE_TESTS=ON -DFLUIDLOOM_BACKEND=MOCK

# Build all tests
cmake --build . --parallel $(nproc)
```

### Run System Tests

```bash
# Run all system tests
ctest -R '^(lid_driven_cavity|taylor_green|wind_tunnel)'

# Run specific test
./tests/system/lid_driven_cavity_test
./tests/system/taylor_green_test
./tests/system/wind_tunnel_test
```

### Run Performance Benchmarks

```bash
# Requires Google Benchmark: sudo apt-get install libbenchmark-dev

# Run all benchmarks
make run_benchmarks

# Run specific benchmark
./tests/perf/rebuild_benchmark
./tests/perf/halo_benchmark
```

### Run Regression Detection

```bash
python3 ci/scripts/regression_detector.py \
  --baseline ci/perf_baseline.db \
  --new "build/*_results.json" \
  --threshold 0.05 \
  --output regression_report.md
```

## Test Structure

```
tests/
├── system/              # End-to-end system tests
│   ├── lid_driven_cavity/
│   ├── taylor_green_vortex/
│   └── wind_tunnel/
├── perf/                # Performance benchmarks
│   ├── rebuild/
│   ├── halo/
│   └── scaling/
├── validation/          # Analytical validators
│   └── analytical/
└── tools/
    └── test_harness/    # Universal test framework
```

## Components

### Test Harness
- Backend-agnostic testing (MOCK/OpenCL/ROCm/CUDA)
- Memory leak detection
- Gold data comparison
- Benchmarking framework

### System Tests
1. **Lid-Driven Cavity**: Ghia 1982 benchmark validation
2. **Taylor-Green Vortex**: Analytical energy decay
3. **Wind Tunnel**: STL geometry + moving rotor

### Performance Benchmarks
- Hash rebuild: < 10ms @ 1M cells
- Halo overlap: > 90% efficiency
- Weak scaling: > 95% @ 4 GPUs

### CI/CD Pipeline
- 4-phase GitHub Actions workflow
- Automated regression detection
- Memory safety validation (ASan/Valgrind)

## Performance Gates

All metrics defined in `ci/configs/performance_gates.json`:
- Hash rebuild 1M: < 10ms
- Halo overlap: > 90%
- Weak scaling 4 GPU: > 95%
- Memory usage 1M: < 1500 MB

## Documentation

- [Test Harness API](tools/test_harness/README.md)
- [Analytical Validators](tests/validation/README.md)
- [Performance Benchmarks](tests/perf/README.md)
- [System Tests](tests/system/*/README.md)

## Status

**Module 16**: ~85% complete ✅

Implemented:
- ✅ Test infrastructure
- ✅ System tests (3/3)
- ✅ Performance benchmarks
- ✅ CI/CD pipeline
- ✅ Regression detection

Remaining:
- ⏳ Integration with runtime execution
- ⏳ Full GPU backend testing
- ⏳ Cross-backend validation
