#!/bin/bash
# ANTLR Parser Regeneration Script
# This script regenerates all ANTLR parsers with proper dependencies

set -e  # Exit on error

ANTLR_JAR="/opt/homebrew/Cellar/antlr/4.13.2/antlr-4.13.2-complete.jar"
JAVA="/opt/homebrew/Cellar/openjdk/25.0.1/bin/java"
GRAMMAR_DIR="/Users/karthikt/Ideas/FluidLoom/fluidloom/src/parsing/grammars"
OUTPUT_DIR="/Users/karthikt/Ideas/FluidLoom/fluidloom/generated/parsers/src/parsing/grammars"

echo "=== ANTLR Parser Regeneration ==="
echo "Grammar Dir: $GRAMMAR_DIR"
echo "Output Dir: $OUTPUT_DIR"
echo ""

# Step 1: Generate ExpressionLexer
echo "[1/6] Generating ExpressionLexer..."
$JAVA -jar $ANTLR_JAR -Dlanguage=Cpp -o $OUTPUT_DIR $GRAMMAR_DIR/FluidLoomExpressionLexer.g4
echo "✓ ExpressionLexer generated"

# Step 2: Generate ExpressionParser
echo "[2/6] Generating ExpressionParser..."
$JAVA -jar $ANTLR_JAR -Dlanguage=Cpp -visitor -o $OUTPUT_DIR -lib $OUTPUT_DIR $GRAMMAR_DIR/FluidLoomExpressionParser.g4 2>&1 | grep -v "warning" || true
echo "✓ ExpressionParser generated"

# Step 3: Generate KernelLexer
echo "[3/6] Generating KernelLexer..."
$JAVA -jar $ANTLR_JAR -Dlanguage=Cpp -o $OUTPUT_DIR $GRAMMAR_DIR/FluidLoomKernelLexer.g4
echo "✓ KernelLexer generated"

# Step 4: Generate KernelParser
echo "[4/6] Generating KernelParser..."
$JAVA -jar $ANTLR_JAR -Dlanguage=Cpp -visitor -o $OUTPUT_DIR -lib $OUTPUT_DIR $GRAMMAR_DIR/FluidLoomKernelParser.g4 2>&1 | grep -v "warning" || true
echo "✓ KernelParser generated"

# Step 5: Generate SimulationLexer
echo "[5/6] Generating SimulationLexer..."
$JAVA -jar $ANTLR_JAR -Dlanguage=Cpp -o $OUTPUT_DIR $GRAMMAR_DIR/FluidLoomSimulationLexer.g4
echo "✓ SimulationLexer generated"

# Step 6: Generate SimulationParser
echo "[6/6] Generating SimulationParser..."
$JAVA -jar $ANTLR_JAR -Dlanguage=Cpp -visitor -o $OUTPUT_DIR -lib $OUTPUT_DIR $GRAMMAR_DIR/FluidLoomSimulationParser.g4 2>&1 | grep -v "warning" || true
echo "✓ SimulationParser generated"

echo ""
echo "=== All parsers regenerated successfully ==="
echo ""
echo "Generated files:"
ls -1 $OUTPUT_DIR/FluidLoom*Parser.cpp 2>/dev/null | wc -l | xargs echo "  Parser files:"
ls -1 $OUTPUT_DIR/FluidLoom*Lexer.cpp 2>/dev/null | wc -l | xargs echo "  Lexer files:"
echo ""
echo "Next steps:"
echo "  1. cd build"
echo "  2. cmake --build . --target fluidloom-run"
echo "  3. cd .. && ./build/src/parsing/fluidloom-run examples/LidDrivenCavity/lid_driven_cavity.fl"
