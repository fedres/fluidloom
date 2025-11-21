# Wind Tunnel with Moving Geometry System Test

## Overview

Complex test case with STL geometry voxelization and moving objects.

## Test Cases

1. **STL Geometry Watertightness**
   - Voxelize cylinder.stl
   - Verify no fluid cells inside solid
   - Check surface normal consistency > 99%

2. **Moving Rotor Mass Conservation**
   - Static cylinder + moving rotor
   - Update geometry every 100 steps
   - Target: mass conservation < 1e-5 relative error

3. **Dynamic AMR Tracking**
   - AMR follows moving rotor
   - Tracking lag < 10 cells
   - Material IDs persist through refinement

4. **Staggered Rebuild Integrity**
   - Rapid geometry updates (every 10 steps)
   - Verify hash map integrity
   - Cell count variance < 5%

## Configuration

- Domain: [0, 2] × [0, 1] × [0, 0.5]
- Static geometry: cylinder.stl at (0.5, 0.5, 0.25)
- Moving geometry: Rotor on circular path
- Update frequency: 100 steps
