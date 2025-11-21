# FluidLoom Module 16: Complete Test Suite Guide

## Overview

Module 16 provides comprehensive testing infrastructure for FluidLoom, including system tests, performance benchmarks, and automated CI/CD validation.

## Quick Reference

### Build and Run Tests

```bash
# Configure with tests enabled
cmake -B build -DFLUIDLOOM_ENABLE_TESTS=ON -DFLUIDLOOM_BACKEND=MOCK
cmake --build build -j$(nproc)

# Run all tests
cd build && ctest

# Run specific test category
ctest -R '^system_'        # System tests
ctest -R '^perf_'          # Performance benchmarks
ctest -R '^validation_'    # Validation tests
```

### Test Categories

| Category | Location | Purpose | Count |
|----------|----------|---------|-------|
| **System Tests** | `tests/system/` | End-to-end validation | 3 |
| **Performance** | `tests/perf/` | Microbenchmarks | 3 |
| **Validation** | `tests/validation/` | Correctness checks | 4 |
| **Unit Tests** | `tests/unit/` | Component tests | 35+ |

## System Tests

### 1. Lid-Driven Cavity
**File**: `tests/system/lid_driven_cavity/lid_driven_cavity_test.cpp`

Validates against Ghia et al. (1982) benchmark:
- Centerline velocity profiles
- Convergence to steady state
- Multi-GPU weak scaling

**Run**: `./tests/system/lid_driven_cavity_test`

### 2. Taylor-Green Vortex
**File**: `tests/system/taylor_green_vortex/taylor_green_test.cpp`

Analytical validation:
- Kinetic energy decay: dE/dt = -2νE
- Vorticity conservation: ∇·ω = 0
- AMR enstrophy capture > 95%

**Run**: `./tests/system/taylor_green_test`

### 3. Wind Tunnel
**File**: `tests/system/wind_tunnel/wind_tunnel_test.cpp`

Complex geometry validation:
- STL voxelization watertightness
- Moving rotor mass conservation < 1e-5
- Dynamic AMR tracking

**Run**: `./tests/system/wind_tunnel_test`

## Performance Benchmarks

### Rebuild Pipeline
**Target**: < 10ms @ 1M cells

```bash
./tests/perf/rebuild_benchmark --benchmark_filter=1000000
```

### Halo Exchange
**Target**: > 90% overlap efficiency

```bash
./tests/perf/halo_benchmark --benchmark_filter=4
```

### Scaling
**Target**: > 95% weak scaling @ 4 GPUs

```bash
./tests/perf/scaling_test --gtest_filter=*WeakScaling*
```

## Validation Tests

### Memory Safety
Leak detection and exception safety:
```bash
./tests/system/memory/memory_safety_test
```

### Checkpoint Restart
HDF5 roundtrip validation:
```bash
./tests/restart/checkpoint_restart_test
```

### Trace Validation
Chrome Trace Event format:
```bash
./tests/validation/trace_validation_test
```

### Cross-Backend
Bit reproducibility:
```bash
./tests/cross_backend/cross_backend_test
```

## CI/CD Pipeline

### GitHub Actions Workflow
**File**: `.github/workflows/system_tests.yml`

4 phases:
1. Unit tests (GCC/Clang, Valgrind, coverage)
2. System tests (all 3 benchmarks)
3. Performance benchmarks (regression detection)
4. Memory safety (AddressSanitizer)

### Regression Detection

```bash
python3 ci/scripts/regression_detector.py \
  --baseline ci/perf_baseline.db \
  --new "build/*_results.json" \
  --threshold 0.05 \
  --output regression_report.md
```

## Performance Gates

All metrics in `ci/configs/performance_gates.json`:

| Metric | Target | Unit |
|--------|--------|------|
| Hash rebuild 1M | < 10 | ms |
| Halo overlap | > 90 | % |
| Weak scaling 4 GPU | > 95 | % |
| Memory usage 1M | < 1500 | MB |

## Analytical Validators

### Taylor-Green Solution
```cpp
#include "tests/validation/analytical/TaylorGreenSolution.h"

double u, v, w;
TaylorGreenSolution::computeVelocity(x, y, z, t, Re, u, v, w);
double E = TaylorGreenSolution::computeKineticEnergy(t, Re);
```

### Lid-Driven Cavity (Ghia Benchmark)
```cpp
#include "tests/validation/analytical/LidDrivenCavitySolution.h"

auto u_data = LidDrivenCavitySolution::getUCenterline();
double error = LidDrivenCavitySolution::computeL2Error("velocity_x", computed);
```

## Test Harness API

```cpp
#include "tools/test_harness/TestHarness.h"

TestHarness harness(BackendType::MOCK, true);

TestResult result = harness.runSimulation(
    simulation_script,
    max_steps,
    fields_to_validate,
    gold_data_dir
);

EXPECT_TRUE(result.passed);
EXPECT_LT(result.l2_error, 0.01);
```

## Troubleshooting

### Tests Fail to Build
- Ensure GoogleTest is installed: `sudo apt-get install libgtest-dev`
- For benchmarks: `sudo apt-get install libbenchmark-dev`

### Memory Tests Fail
- Run with Valgrind: `valgrind --leak-check=full ./test_executable`
- Check ASan output for detailed leak reports

### Performance Regression
- Check `regression_report.md` for specific metrics
- Compare against baseline in `ci/perf_baseline.db`

## Contributing

When adding new tests:
1. Follow existing test structure
2. Add to appropriate CMakeLists.txt
3. Update this documentation
4. Ensure tests pass in CI

## Status

**Module 16**: ✅ Complete (95%)

**Implemented**:
- ✅ Test infrastructure (TestHarness, GoldDataManager)
- ✅ System tests (3/3)
- ✅ Performance benchmarks (3/3)
- ✅ Validation tests (4/4)
- ✅ CI/CD pipeline
- ✅ Documentation

**Remaining**:
- Integration with runtime execution
- GPU backend validation
- Gold data generation
