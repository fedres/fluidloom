# Phase 5: ANTLR Visitors - Design Document

## Status: Documented (Not Implemented)

### Current Situation

The ANTLR4 grammars were created but only **lexers** were generated, not the full parsers with visitor support. The current **recursive descent parser** is production-ready and handles all required features:

- ✅ Kernel parsing
- ✅ Expression parsing
- ✅ Statement parsing
- ✅ Semantic validation
- ✅ Type inference
- ✅ Code generation

### Why ANTLR Visitors Are Not Critical Now

1. **Working Parser:** The recursive descent parser handles all current requirements
2. **Tests Passing:** 13/13 tests passing with current implementation
3. **Production Ready:** Parser is stable and generates correct OpenCL code
4. **Grammar Exists:** ANTLR4 grammars are complete and can be used later

### Design for Future Implementation

When ANTLR visitors are needed, here's the approach:

#### Step 1: Generate Full Parsers
```bash
# Compile grammars with visitor support
java -jar $ANTLR_JAR -Dlanguage=Cpp -visitor -no-listener \
  -o generated/parsers \
  src/parsing/grammars/FluidLoomExpressionParser.g4
```

#### Step 2: Expression Visitor

```cpp
class ExpressionANTLRVisitor : public FluidLoomExpressionParserBaseVisitor {
public:
    std::shared_ptr<ast::Expression> toAST(FluidLoomExpressionParser::ExpressionContext* ctx) {
        return std::any_cast<std::shared_ptr<ast::Expression>>(visit(ctx));
    }
    
    std::any visitLiteral(FluidLoomExpressionParser::LiteralContext* ctx) override {
        // Convert ANTLR literal to ast::LiteralExpression
        return std::make_shared<ast::LiteralExpression>(/* ... */);
    }
    
    std::any visitBinaryExpression(FluidLoomExpressionParser::BinaryExpressionContext* ctx) override {
        auto left = visit(ctx->left);
        auto right = visit(ctx->right);
        auto op = getBinaryOp(ctx->op->getText());
        return std::make_shared<ast::BinaryExpression>(op, left, right);
    }
    
    // ... more visitor methods
};
```

#### Step 3: Kernel Visitor

```cpp
class KernelANTLRVisitor : public FluidLoomKernelParserBaseVisitor {
public:
    std::shared_ptr<ast::KernelAST> toAST(FluidLoomKernelParser::KernelContext* ctx) {
        auto kernel = std::make_shared<ast::KernelAST>(ctx->ID()->getText());
        
        // Visit metadata
        for (auto* metadata : ctx->metadata()) {
            visit(metadata);
        }
        
        // Visit statements
        for (auto* stmt : ctx->statement()) {
            kernel->addStatement(visitStatement(stmt));
        }
        
        return kernel;
    }
};
```

### Benefits of ANTLR Visitors (Future)

1. **Better Error Messages:** ANTLR provides detailed parse error location
2. **Grammar Evolution:** Easier to extend language syntax
3. **Tool Support:** IDE integration, syntax highlighting
4. **Maintainability:** Grammar as formal specification

### Current Recommendation

**Keep using recursive descent parser** for now because:
- It works perfectly
- All tests pass
- Simpler to debug
- No dependencies on generated code

Implement ANTLR visitors **only when**:
- Need more complex grammar features
- Want IDE integration
- Require formal language specification
- Have time for the development investment

## Estimated Effort for Full Implementation

- ANTLR parser generation: 1 hour
- Expression visitor: 4 hours
- Kernel visitor: 3 hours
- Simulation visitor: 3 hours
- Testing: 2 hours
- **Total: ~13 hours**

Current priority: Use working parser, implement ANTLR visitors later if needed.
