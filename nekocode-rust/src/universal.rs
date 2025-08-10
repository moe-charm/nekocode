//=============================================================================
// 🦀 NekoCode Rust - Universal AST Module
//
// Universal AST system for cross-language operations
//=============================================================================

use crate::types::Language;
use serde::{Serialize, Deserialize};
use std::collections::HashMap;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum SymbolType {
    // Rust-focused
    Struct,
    Trait,
    ImplBlock,
    Method,
    Function,
    MemberVar,
    
    // Other languages
    Class,
    Interface,
    Enum,
    Namespace,
    Module,
    Package,
    
    // Common elements
    Constructor,
    Destructor,
    Property,
    Parameter,
    Variable,
    Constant,
    
    Unknown,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct UniversalSymbol {
    pub symbol_type: SymbolType,
    pub name: String,
    pub qualified_name: String,
    pub start_line: u32,
    pub end_line: u32,
    pub symbol_id: String,
    pub parent_id: String,
    pub child_ids: Vec<String>,
    pub metadata: HashMap<String, String>,
    pub parameters: Vec<String>,
}

impl UniversalSymbol {
    pub fn new(symbol_type: SymbolType, name: String) -> Self {
        Self {
            symbol_type,
            name: name.clone(),
            qualified_name: name,
            start_line: 0,
            end_line: 0,
            symbol_id: String::new(),
            parent_id: String::new(),
            child_ids: Vec::new(),
            metadata: HashMap::new(),
            parameters: Vec::new(),
        }
    }

    pub fn generate_id(&mut self, sequence: usize) {
        self.symbol_id = format!("{:?}_{}_{}",
            self.symbol_type,
            self.name,
            sequence
        ).to_lowercase();
    }
}

pub struct SymbolTable {
    symbols: HashMap<String, UniversalSymbol>,
}

impl SymbolTable {
    pub fn new() -> Self {
        Self {
            symbols: HashMap::new(),
        }
    }

    pub fn add_symbol(&mut self, symbol: UniversalSymbol) {
        self.symbols.insert(symbol.symbol_id.clone(), symbol);
    }

    pub fn get_symbol(&self, symbol_id: &str) -> Option<&UniversalSymbol> {
        self.symbols.get(symbol_id)
    }

    pub fn get_all_symbols(&self) -> &HashMap<String, UniversalSymbol> {
        &self.symbols
    }
}