# Taylor-Green Vortex System Test

## Overview

3D Taylor-Green vortex decay test with analytical solution validation.

## Test Cases

1. **Kinetic Energy Decay**
   - Validate energy decay rate: dE/dt = -2νE
   - Compare against analytical solution
   - Target: L₂ error < 1%

2. **Vorticity Conservation**
   - Verify divergence-free condition: ∇·ω = 0
   - Target: |∇·ω| < 1e-10

3. **AMR Refinement Enstrophy Capture**
   - Refine on vorticity magnitude
   - Capture > 95% of total enstrophy
   - Validate 2:1 balance constraint

4. **Temporal Convergence**
   - Test with dt, dt/2, dt/4
   - Verify 2nd order convergence

## Configuration

- Domain: [0, 2π] × [0, 2π] × [0, 2π]
- Reynolds number: 50
- Initial condition: u = sin(x)cos(y)cos(z), v = -cos(x)sin(y)cos(z), w = 0
- Viscosity: ν = 1/Re
