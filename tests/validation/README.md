# Analytical Solution Validators

This directory contains analytical solution implementations for validation of FluidLoom system tests.

## Available Validators

### TaylorGreenSolution.h
3D Taylor-Green vortex analytical solution:
- Velocity field computation
- Kinetic energy decay
- Vorticity field
- Enstrophy calculation

**Usage**:
```cpp
#include "tests/validation/analytical/TaylorGreenSolution.h"

double u, v, w;
TaylorGreenSolution::computeVelocity(x, y, z, t, Re, u, v, w);

double E = TaylorGreenSolution::computeKineticEnergy(t, Re);
```

### LidDrivenCavitySolution.h
Ghia et al. (1982) benchmark data for lid-driven cavity:
- U-velocity centerline (Re=1000)
- V-velocity centerline (Re=1000)
- Interpolation utilities
- L2 error computation

**Usage**:
```cpp
#include "tests/validation/analytical/LidDrivenCavitySolution.h"

auto u_data = LidDrivenCavitySolution::getUCenterline();
double u_ref = LidDrivenCavitySolution::interpolate(u_data, 0.5);
double error = LidDrivenCavitySolution::computeL2Error("velocity_x", computed);
```

## Adding New Validators

1. Create header file in `analytical/`
2. Implement as header-only (static methods)
3. Add documentation in this README
4. Update CMakeLists.txt if needed (currently interface library)
