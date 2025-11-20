# ANTLR Integration Status

## Current State

### ✅ What Works
1. **AMR System**: Fully functional with GPU compaction
2. **Runtime Integration**: `fluidloom-run` executable works with stub builder
3. **Execution Graph**: Minimal DAG with `AdaptMeshNode`
4. **Demo**: Successfully runs with 512-cell mesh

### ⚠️ ANTLR Parser Generation - Blocked

**Issue**: Grammar files have errors preventing ANTLR generation

**Error Details**:
```
error(126): FluidLoomKernelParser.g4:16:6: cannot create implicit token for string literal in non-combined grammar: 'void'
error(126): FluidLoomKernelParser.g4:17:6: cannot create implicit token for string literal in non-combined grammar: 'float'
...
```

**Root Cause**: The parser grammars (`FluidLoomKernelParser.g4`, `FluidLoomSimulationParser.g4`) use string literals like `'void'`, `'float'`, etc. directly in rules. In split lexer/parser grammars, these must be defined as tokens in the lexer first.

## What's Installed

- ✅ ANTLR jar: `/opt/homebrew/Cellar/antlr/4.13.2/antlr-4.13.2-complete.jar`
- ✅ ANTLR C++ runtime: `/opt/homebrew/lib/libantlr4-runtime.dylib`
- ✅ Java: `/opt/homebrew/Cellar/openjdk/25.0.1/bin/java`
- ✅ CMake FindANTLR4: Configured correctly

## Steps to Fix

### Option 1: Fix Grammar Files (Recommended)

1. **Update `FluidLoomExpressionLexer.g4`** to define all missing tokens:
   ```antlr
   VOID: 'void';
   FLOAT_TYPE: 'float';
   DOUBLE: 'double';
   INT: 'int';
   BOOL: 'bool';
   VECTOR: 'vector';
   IMPLICIT: 'implicit';
   AT: 'at';
   SCALE: 'scale';
   ROTATION: 'rotation';
   SURFACE_MATERIAL: 'surface_material';
   ```

2. **Update parser grammars** to use token names instead of string literals:
   ```antlr
   // Before:
   typeSpec: 'void' | 'float' | 'double' | 'int' | 'bool' | 'vector';
   
   // After:
   typeSpec: VOID | FLOAT_TYPE | DOUBLE | INT | BOOL | VECTOR;
   ```

3. **Regenerate parsers**:
   ```bash
   cd /Users/karthikt/Ideas/FluidLoom/fluidloom
   /opt/homebrew/Cellar/openjdk/25.0.1/bin/java -jar \
     /opt/homebrew/Cellar/antlr/4.13.2/antlr-4.13.2-complete.jar \
     -Dlanguage=Cpp -visitor -o generated/parsers \
     -lib src/parsing/grammars \
     src/parsing/grammars/FluidLoomExpressionLexer.g4 \
     src/parsing/grammars/FluidLoomKernelLexer.g4 \
     src/parsing/grammars/FluidLoomKernelParser.g4 \
     src/parsing/grammars/FluidLoomSimulationLexer.g4 \
     src/parsing/grammars/FluidLoomSimulationParser.g4
   ```

4. **Update `SimulationBuilder.cpp`** to use generated parsers instead of stub

### Option 2: Use Existing Recursive Descent Parser

The project already has a working recursive descent parser in `src/parsing/Parser.cpp`. You could:

1. Extend it to handle simulation-level constructs
2. Skip ANTLR entirely for now
3. Focus on AMR functionality (which is complete)

## Current Workaround

The stub implementation in `SimulationBuilder.cpp` creates the execution graph programmatically:
- Initializes 512-cell mesh
- Marks center region for refinement
- Creates `AdaptMeshNode`
- Executes successfully

This demonstrates that the AMR integration works - only the `.fl` file parsing is missing.

## Recommendation

For immediate AMR demonstration, the current stub is sufficient. For production use, fix the grammar files as described in Option 1.
