# FluidLoom DSL Parser

## Overview
The parsing module provides ANTLR4-based parsers for FluidLoom's domain-specific language (DSL) files:
- `fields.fl` - Field definitions with metadata
- `lattices.fl` - Lattice stencils and constants

## Grammar Reference

### Fields Grammar (`fields.fl`)

#### Field Definition
```
field <name>: <type_spec> {
    <parameters>
}
```

#### Type Specifications
- `scalar <data_type>` - Single value per cell
- `vector[N] <data_type>` - N-component vector

#### Data Types
- `float` - 32-bit floating point
- `double` - 64-bit floating point
- `int` - 32-bit integer
- `uint8` - 8-bit unsigned integer

#### Parameters
- `default_value: <value>` - Initial value
- `halo_depth: <0-2>` - Ghost cell layers
- `visualization: <true|false>` - Include in output
- `solid_scheme: "<scheme>"` - Boundary treatment
- `averaging_rule: <rule>` - Interpolation method
- `is_mask: <true|false>` - Flag field

#### Averaging Rules
- `arithmetic` - Simple average
- `volume_weighted` - Weighted by cell volume
- `min_value` - Minimum value
- `max_value` - Maximum value

#### Solid Schemes
- `"zero"` - Set to zero in solid
- `"bounce_back"` - LBM bounce-back
- `"neumann"` - Neumann boundary
- `"skip"` - Don't allocate in solid

### Lattices Grammar (`lattices.fl`)

#### Lattice Definition
```
export lattice <name> {
    stencil: [<vectors>],
    weights: [<numbers>],
    cs2: <number>,
    num_populations: <int>
}
```

#### Constant Definition
```
export float <name> = <value>;
export int <name> = <value>;
```

## Usage

### Command Line Tool
```bash
fluidloom-parse [options] <fields.fl> <lattices.fl>

Options:
  -o <dir>          Output directory (default: ./generated)
  --validate-only   Only validate, don't generate code
  -h, --help        Show help message
```

### Example
```bash
fluidloom-parse -o build/generated examples/fields.fl examples/lattices.fl
```

### Programmatic API
```cpp
#include "parsing/visitors/FieldsVisitor.h"
#include "parsing/visitors/LatticesVisitor.h"
#include "parsing/codegen/OpenCLPreambleGenerator.h"

// Parse fields
fluidloom::parsing::FieldsVisitor fields_visitor;
fields_visitor.parseFile("fields.fl");

// Parse lattices
fluidloom::parsing::LatticesVisitor lattices_visitor;
lattices_visitor.parseFile("lattices.fl");

// Generate OpenCL preamble
fluidloom::parsing::OpenCLPreambleGenerator generator;
generator.generateToFile("generated/fluidloom_preamble.cl");
```

## Error Handling

Parse errors include line and column information:
```
Line 5:12 - Halo depth must be <= 2
Line 8:1 - Duplicate field name: density
```

## Validation Rules

### Fields
- Field names must be unique
- Halo depth: 0-2
- Vector size: 1-32
- No reserved OpenCL keywords

### Lattices
- Weights must sum to 1.0 (±1e-6)
- Stencil vectors must have opposites for symmetric lattices
- cs2 must be > 0

## Generated Output

### OpenCL Preamble (`fluidloom_preamble.cl`)
Contains:
- Constant definitions (`#define TAU 0.6f`)
- Lattice definitions (`#define D3Q19_Q 19`)
- Weight arrays (`__constant float D3Q19_weights[19] = {...}`)
- Opposite indices (`__constant uchar D3Q19_opposite[19] = {...}`)

## Integration

### With FieldRegistry
Fields are automatically registered in `FieldRegistry` during parsing.

### With SOAFieldManager
Use `FieldRegistry` to query field metadata for allocation.

### With Kernel Code Generation
Include the generated preamble in kernel files:
```c
#include "fluidloom_preamble.cl"

__kernel void my_kernel() {
    // Use D3Q19_weights, TAU, etc.
}
```

## Performance

Typical parsing times:
- 10 fields: < 50ms
- 2 lattices: < 20ms
- Preamble generation (1000 lines): < 10ms

## Examples

See `examples/fields.fl` and `examples/lattices.fl` for complete examples.
