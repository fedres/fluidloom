#pragma once
// Symbol table for semantic analysis

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <optional>

namespace fluidloom {
namespace parsing {
namespace symbol_table {

/**
 * @brief Type information for symbols
 */
enum class SymbolType {
    UNKNOWN,
    FLOAT,
    DOUBLE,
    INT,
    BOOL,
    VECTOR_3,
    VECTOR_19,
    FIELD,
    KERNEL,
    FUNCTION,
    LAMBDA
};

/**
 * @brief Symbol information
 */
struct Symbol {
    std::string name;
    SymbolType type;
    bool is_const = false;
    bool is_field = false;
    int scope_level = 0;
    
    // For fields
    int num_components = 0;
    
    // For functions/kernels
    std::vector<SymbolType> parameter_types;
    SymbolType return_type = SymbolType::UNKNOWN;
    
    Symbol() : type(SymbolType::UNKNOWN) {}
    Symbol(std::string n, SymbolType t) : name(std::move(n)), type(t) {}
};

/**
 * @brief Scope for symbol resolution
 */
class Scope {
private:
    std::unordered_map<std::string, Symbol> symbols;
    Scope* parent = nullptr;
    int level = 0;
    
public:
    explicit Scope(int lvl = 0, Scope* p = nullptr) : parent(p), level(lvl) {}
    
    /**
     * @brief Add a symbol to this scope
     */
    bool addSymbol(const Symbol& symbol);
    
    /**
     * @brief Look up a symbol in this scope or parent scopes
     */
    std::optional<Symbol> lookup(const std::string& name) const;
    
    /**
     * @brief Look up a symbol only in this scope (not parent)
     */
    std::optional<Symbol> lookupLocal(const std::string& name) const;
    
    /**
     * @brief Get scope level
     */
    int getLevel() const { return level; }
    
    /**
     * @brief Get parent scope
     */
    Scope* getParent() const { return parent; }
};

/**
 * @brief Symbol table for managing scopes and symbols
 */
class SymbolTable {
private:
    std::unique_ptr<Scope> global_scope;
    Scope* current_scope = nullptr;
    std::vector<std::unique_ptr<Scope>> all_scopes;
    
public:
    SymbolTable();
    
    /**
     * @brief Enter a new scope
     */
    void enterScope();
    
    /**
     * @brief Exit current scope
     */
    void exitScope();
    
    /**
     * @brief Add a symbol to current scope
     */
    bool addSymbol(const Symbol& symbol);
    
    /**
     * @brief Look up a symbol
     */
    std::optional<Symbol> lookup(const std::string& name) const;
    
    /**
     * @brief Add a field symbol
     */
    bool addField(const std::string& name, int num_components);
    
    /**
     * @brief Add a kernel symbol
     */
    bool addKernel(const std::string& name);
    
    /**
     * @brief Add a variable symbol
     */
    bool addVariable(const std::string& name, SymbolType type);
    
    /**
     * @brief Check if a symbol exists
     */
    bool exists(const std::string& name) const;
    
    /**
     * @brief Get current scope
     */
    Scope* getCurrentScope() const { return current_scope; }
    
    /**
     * @brief Get global scope
     */
    Scope* getGlobalScope() const { return global_scope.get(); }
    
    /**
     * @brief Reset symbol table
     */
    void reset();
};

} // namespace symbol_table
} // namespace parsing
} // namespace fluidloom
