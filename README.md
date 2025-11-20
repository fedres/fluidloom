# FluidLoom

**High-Performance Adaptive Mesh Refinement (AMR) Framework for Computational Fluid Dynamics**

FluidLoom is a GPU-accelerated CFD framework featuring dynamic adaptive mesh refinement, multi-GPU support, and a domain-specific language for simulation scripting.

## Key Features

- **Adaptive Mesh Refinement (AMR)**: Dynamic octree-based mesh adaptation with Hilbert space-filling curves
- **Multi-GPU Support**: Halo exchange and MPI-based communication for distributed computing
- **Domain-Specific Language**: Custom `.fl` scripting language with ANTLR-based parser
- **OpenCL Backend**: GPU-accelerated kernels with automatic code generation
- **Modular Architecture**: Clean separation of concerns across 14 modules

## Architecture

```
FluidLoom Framework
├── Parsing & Code Generation (Module 10)
│   ├── ANTLR4 grammar for .fl files
│   ├── Kernel extraction & compilation
│   └── OpenCL code generation
├── Adaptive Mesh Refinement (Module 11)
│   ├── Split/Merge/Balance engines
│   └── Hilbert-based cell ordering
├── Multi-GPU Communication (Modules 5-7)
│   ├── Ghost range builder
│   ├── Halo exchange
│   └── MPI transport layer
├── Runtime System (Module 9)
│   ├── Execution graph
│   ├── Dependency tracking
│   └── Topological scheduler
└── Field Management (Module 8)
    ├── Field registry
    └── SOA field manager
```

## Quick Start

### Prerequisites
- C++17 compiler (GCC 9+, Clang 10+)
- CMake 3.18+
- OpenCL 1.2+
- ANTLR4 C++ runtime
- MPI (optional, for multi-GPU)

### Build

```bash
git clone https://github.com/fedres/fluidloom.git
cd fluidloom
mkdir build && cd build
cmake ..
make -j8
```

### Run Tests

```bash
ctest --output-on-failure
```

## Current Status

**Module Completion:**
- ✅ Module 5-7: Multi-GPU Communication (100%)
- ✅ Module 8: Field Management (100%)
- ✅ Module 9: Runtime System (100%)
- 🔄 Module 10: DSL Parser (85% - kernel generation working)
- ✅ Module 11: AMR Engine (100%)

**Working Features:**
- Zero-error ANTLR parsing of `.fl` simulation files
- Kernel extraction and OpenCL code generation
- 5 kernels successfully compiled (LBM collision, streaming, boundary conditions)
- Complete AMR engine with split/merge/balance
- Halo exchange for multi-GPU simulations
- Execution graph with dependency tracking

## Technology Stack

- **Language**: C++17
- **Build System**: CMake
- **GPU**: OpenCL 1.2+
- **Parser**: ANTLR4
- **Communication**: MPI
- **Testing**: Google Test

## Project Structure

```
fluidloom/
├── src/
│   ├── parsing/        # ANTLR grammars & code generation
│   ├── adaptation/     # AMR split/merge/balance
│   ├── halo/          # Multi-GPU halo exchange
│   ├── runtime/       # Execution graph & scheduler
│   ├── core/          # Field registry, backends
│   └── transport/     # MPI/GPU communication
├── include/fluidloom/
├── tests/
│   ├── unit/
│   ├── integration/
│   └── benchmark/
└── benchmarks/
```

## License

This software is free for **non-commercial use only**. See [LICENSE](LICENSE) for details.

For commercial licensing inquiries, please contact: genrex3@gmail.com

## Contributing

This is a research project. Contributions are welcome for non-commercial purposes.

## Author

**fedres** (genrex3@gmail.com)

## Acknowledgments

Built with ANTLR4 for parsing and OpenCL for GPU acceleration.
