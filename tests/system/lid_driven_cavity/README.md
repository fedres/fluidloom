# Lid-Driven Cavity System Test

## Overview

2D lid-driven cavity flow at Re=1000, validated against the Ghia et al. (1982) benchmark.

## Test Cases

1. **Centerline Profile Validation**
   - Compare u-velocity along vertical centerline (x=0.5)
   - Compare v-velocity along horizontal centerline (y=0.5)
   - Target: L₂ error < 2%, L∞ error < 5%

2. **Convergence to Steady State**
   - Monitor kinetic energy evolution
   - Verify convergence within 50,000 steps
   - Target: dE/dt < 1e-6

3. **Multi-GPU Scaling**
   - Weak scaling: 1M cells per GPU
   - Test configurations: 1, 2, 4 GPUs
   - Target: efficiency > 90%

## Gold Data

Reference data from Ghia et al. (1982):
- `ghia_u_centerline.csv`: U-velocity at x=0.5
- `ghia_v_centerline.csv`: V-velocity at y=0.5

## Configuration

- Domain: [0,1] × [0,1]
- Base resolution: 128 × 128
- Reynolds number: 1000
- Lid velocity: 1.0 m/s
- Viscosity: 0.001 m²/s
