#include "fluidloom/parsing/symbol_table/SymbolTable.h"

namespace fluidloom {
namespace parsing {
namespace symbol_table {

// Scope implementation

bool Scope::addSymbol(const Symbol& symbol) {
    // Check if symbol already exists in this scope
    if (symbols.find(symbol.name) != symbols.end()) {
        return false;  // Symbol already defined
    }
    
    symbols[symbol.name] = symbol;
    return true;
}

std::optional<Symbol> Scope::lookup(const std::string& name) const {
    // Check this scope
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        return it->second;
    }
    
    // Check parent scope
    if (parent) {
        return parent->lookup(name);
    }
    
    return std::nullopt;
}

std::optional<Symbol> Scope::lookupLocal(const std::string& name) const {
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        return it->second;
    }
    return std::nullopt;
}

// SymbolTable implementation

SymbolTable::SymbolTable() {
    global_scope = std::make_unique<Scope>(0, nullptr);
    current_scope = global_scope.get();
    
    // Add built-in functions
    Symbol sqrt_func("sqrt", SymbolType::FUNCTION);
    sqrt_func.parameter_types = {SymbolType::FLOAT};
    sqrt_func.return_type = SymbolType::FLOAT;
    global_scope->addSymbol(sqrt_func);
    
    Symbol pow_func("pow", SymbolType::FUNCTION);
    pow_func.parameter_types = {SymbolType::FLOAT, SymbolType::FLOAT};
    pow_func.return_type = SymbolType::FLOAT;
    global_scope->addSymbol(pow_func);
    
    Symbol abs_func("abs", SymbolType::FUNCTION);
    abs_func.parameter_types = {SymbolType::FLOAT};
    abs_func.return_type = SymbolType::FLOAT;
    global_scope->addSymbol(abs_func);
}

void SymbolTable::enterScope() {
    int new_level = current_scope->getLevel() + 1;
    auto new_scope = std::make_unique<Scope>(new_level, current_scope);
    current_scope = new_scope.get();
    all_scopes.push_back(std::move(new_scope));
}

void SymbolTable::exitScope() {
    if (current_scope->getParent()) {
        current_scope = current_scope->getParent();
    }
}

bool SymbolTable::addSymbol(const Symbol& symbol) {
    return current_scope->addSymbol(symbol);
}

std::optional<Symbol> SymbolTable::lookup(const std::string& name) const {
    return current_scope->lookup(name);
}

bool SymbolTable::addField(const std::string& name, int num_components) {
    Symbol field(name, SymbolType::FIELD);
    field.is_field = true;
    field.num_components = num_components;
    return addSymbol(field);
}

bool SymbolTable::addKernel(const std::string& name) {
    Symbol kernel(name, SymbolType::KERNEL);
    return addSymbol(kernel);
}

bool SymbolTable::addVariable(const std::string& name, SymbolType type) {
    Symbol var(name, type);
    return addSymbol(var);
}

bool SymbolTable::exists(const std::string& name) const {
    return lookup(name).has_value();
}

void SymbolTable::reset() {
    all_scopes.clear();
    global_scope = std::make_unique<Scope>(0, nullptr);
    current_scope = global_scope.get();
}

} // namespace symbol_table
} // namespace parsing
} // namespace fluidloom
