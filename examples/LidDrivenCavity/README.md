# Lid-Driven Cavity AMR Example

## Description
2D lid-driven cavity flow with adaptive mesh refinement based on vorticity magnitude.

## Configuration
- **Domain**: 128×128 base grid
- **Refinement levels**: L=0 to L=3
- **Reynolds number**: 1000
- **Lattice**: D2Q9

## Refinement Criterion
Cells are marked for refinement/coarsening based on vorticity magnitude:
- `|vorticity| > 0.5`: Refine (split cell)
- `|vorticity| < 0.1`: Coarsen (merge siblings)
- Otherwise: Keep current level

## Running the Example
```bash
cd /Users/karthikt/Ideas/FluidLoom/fluidloom
./build/src/parsing/fluidloom-run examples/LidDrivenCavity/lid_driven_cavity.fl
```

## Output
VTK files are written to `examples/LidDrivenCavity/output/` every 500 timesteps.

## Expected Behavior
- Initial uniform mesh (512 cells)
- Adaptive refinement near lid and vortex cores
- Mesh adaptation every 100 timesteps
- Final output includes density, velocity, and vorticity fields
