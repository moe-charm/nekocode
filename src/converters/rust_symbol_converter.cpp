//=============================================================================
// 🦀 Rust Symbol Converter Implementation
//
// Phase 3: Rust専用変換レイヤーの実装
// Phase 1,2の成果を最大限活用
//=============================================================================

#include "rust_symbol_converter.hpp"
#include <algorithm>
#include <sstream>

namespace nekocode {

//=============================================================================
// 主要変換メソッド
//=============================================================================

SymbolTable RustSymbolConverter::convert_from_analysis_result(const AnalysisResult& result) {
    SymbolTable table;
    
    // マッピング用（旧ClassInfo → 新symbol_id）
    std::unordered_map<std::string, std::string> struct_name_to_id;
    
    // ========== 1. structを変換 ==========
    for (const auto& cls : result.classes) {
        UniversalSymbolInfo struct_sym = convert_struct(cls);
        struct_sym.symbol_id = generate_unique_id("struct_" + cls.name);
        struct_sym.start_line = cls.start_line;
        struct_sym.end_line = cls.end_line;
        
        // structのメタデータ設定
        struct_sym.metadata["language"] = "rust";
        struct_sym.metadata["kind"] = "struct";
        
        // IDマッピングを保存
        struct_name_to_id[cls.name] = struct_sym.symbol_id;
        
        // ========== 2. メンバ変数を子要素として追加 ==========
        for (const auto& var : cls.member_variables) {
            UniversalSymbolInfo var_sym = convert_member_var(var, struct_sym.symbol_id);
            var_sym.symbol_id = generate_unique_id("field_" + cls.name + "_" + var.name);
            var_sym.parent_id = struct_sym.symbol_id;
            
            // メンバ変数のメタデータ
            var_sym.metadata["access_modifier"] = var.access_modifier;
            var_sym.metadata["type"] = var.type;
            if (var.is_static) {
                var_sym.metadata["is_static"] = "true";
            }
            if (var.is_const) {
                var_sym.metadata["is_const"] = "true";
            }
            
            struct_sym.child_ids.push_back(var_sym.symbol_id);
            table.add_symbol(var_sym);
        }
        
        // ========== 3. メソッドを子要素として追加（Phase 1,2の成果活用）==========
        for (const auto& method : cls.methods) {
            UniversalSymbolInfo method_sym = convert_method(method, struct_sym.symbol_id);
            method_sym.symbol_id = generate_unique_id("method_" + cls.name + "_" + method.name);
            method_sym.parent_id = struct_sym.symbol_id;
            method_sym.qualified_name = build_qualified_name(cls.name, method.name);
            
            // Phase 2のmetadataをそのまま引き継ぎ
            inherit_metadata(method_sym, method);
            
            // 追加のメタデータ
            if (!method_sym.metadata.count("parent_struct")) {
                method_sym.metadata["parent_struct"] = cls.name;
            }
            
            struct_sym.child_ids.push_back(method_sym.symbol_id);
            table.add_symbol(method_sym);
        }
        
        // structをテーブルに追加
        table.add_symbol(struct_sym);
    }
    
    // ========== 4. 独立関数を変換 ==========
    for (const auto& func : result.functions) {
        UniversalSymbolInfo func_sym = convert_function(func);
        func_sym.symbol_id = generate_unique_id("function_" + func.name);
        
        // 独立関数のメタデータ
        func_sym.metadata["language"] = "rust";
        if (func.is_async) {
            func_sym.metadata["is_async"] = "true";
        }
        
        // Phase 2でmetadataがあれば引き継ぎ
        for (const auto& [key, value] : func.metadata) {
            func_sym.metadata[key] = value;
        }
        
        table.add_symbol(func_sym);
    }
    
    // ========== 5. trait定義があれば変換（将来拡張）==========
    // TODO: trait定義の解析が実装されたらここに追加
    
    return table;
}

//=============================================================================
// 逆変換（SymbolTable → AnalysisResult）
//=============================================================================

AnalysisResult RustSymbolConverter::convert_to_analysis_result(const SymbolTable& symbols) {
    AnalysisResult result;
    
    // structシンボルを探してClassInfoに変換
    auto structs = symbols.find_by_type(SymbolType::STRUCT);
    for (const auto& struct_sym : structs) {
        ClassInfo cls = symbol_to_class(struct_sym, symbols);
        result.classes.push_back(cls);
    }
    
    // 独立関数シンボルを探してFunctionInfoに変換
    auto functions = symbols.find_by_type(SymbolType::FUNCTION);
    for (const auto& func_sym : functions) {
        FunctionInfo func = symbol_to_function(func_sym);
        result.functions.push_back(func);
    }
    
    // その他の情報は既存のAnalysisResultから継承される想定
    
    return result;
}

//=============================================================================
// 変換ヘルパーメソッド
//=============================================================================

UniversalSymbolInfo RustSymbolConverter::convert_struct(const ClassInfo& cls) {
    UniversalSymbolInfo sym;
    sym.symbol_type = SymbolType::STRUCT;
    sym.name = cls.name;
    sym.start_line = cls.start_line;
    sym.end_line = cls.end_line;
    
    // Rust特有のメタデータ
    sym.metadata["language"] = "rust";
    
    // 将来的にはderive属性なども追加可能
    // sym.metadata["derives"] = "#[derive(Debug, Clone)]";
    
    return sym;
}

UniversalSymbolInfo RustSymbolConverter::convert_method(
    const FunctionInfo& method, 
    const std::string& parent_struct_id) {
    
    UniversalSymbolInfo sym;
    sym.symbol_type = SymbolType::METHOD;
    sym.name = method.name;
    sym.parent_id = parent_struct_id;
    sym.start_line = method.start_line;
    sym.end_line = method.end_line;
    
    // パラメータ情報
    sym.parameters = method.parameters;
    
    // 複雑度情報があれば設定
    sym.complexity = method.complexity;
    
    return sym;
}

UniversalSymbolInfo RustSymbolConverter::convert_function(const FunctionInfo& func) {
    UniversalSymbolInfo sym;
    sym.symbol_type = SymbolType::FUNCTION;
    sym.name = func.name;
    sym.start_line = func.start_line;
    sym.end_line = func.end_line;
    
    // パラメータ情報
    sym.parameters = func.parameters;
    
    // 複雑度情報
    sym.complexity = func.complexity;
    
    return sym;
}

UniversalSymbolInfo RustSymbolConverter::convert_member_var(
    const MemberVariable& var, 
    const std::string& parent_struct_id) {
    
    UniversalSymbolInfo sym;
    sym.symbol_type = SymbolType::MEMBER_VAR;
    sym.name = var.name;
    sym.parent_id = parent_struct_id;
    sym.start_line = var.declaration_line;
    sym.end_line = var.declaration_line;  // 通常1行
    
    // 型情報をメタデータに保存
    sym.metadata["type"] = var.type;
    sym.metadata["access_modifier"] = var.access_modifier;
    
    return sym;
}

//=============================================================================
// 逆変換ヘルパー
//=============================================================================

ClassInfo RustSymbolConverter::symbol_to_class(
    const UniversalSymbolInfo& symbol, 
    const SymbolTable& table) {
    
    ClassInfo cls;
    cls.name = symbol.name;
    cls.start_line = symbol.start_line;
    cls.end_line = symbol.end_line;
    
    // 子要素を取得
    auto children = table.get_children(symbol.symbol_id);
    for (const auto& child : children) {
        if (child.symbol_type == SymbolType::METHOD) {
            FunctionInfo method = symbol_to_function(child);
            cls.methods.push_back(method);
        } else if (child.symbol_type == SymbolType::MEMBER_VAR) {
            MemberVariable var = symbol_to_member_var(child);
            cls.member_variables.push_back(var);
        }
    }
    
    return cls;
}

FunctionInfo RustSymbolConverter::symbol_to_function(const UniversalSymbolInfo& symbol) {
    FunctionInfo func;
    func.name = symbol.name;
    func.start_line = symbol.start_line;
    func.end_line = symbol.end_line;
    func.parameters = symbol.parameters;
    func.complexity = symbol.complexity;
    
    // metadataを復元
    func.metadata = symbol.metadata;
    
    // is_asyncフラグの復元
    if (symbol.metadata.count("is_async") && symbol.metadata.at("is_async") == "true") {
        func.is_async = true;
    }
    
    return func;
}

MemberVariable RustSymbolConverter::symbol_to_member_var(const UniversalSymbolInfo& symbol) {
    MemberVariable var;
    var.name = symbol.name;
    var.declaration_line = symbol.start_line;
    
    // メタデータから型情報を復元
    if (symbol.metadata.count("type")) {
        var.type = symbol.metadata.at("type");
    }
    if (symbol.metadata.count("access_modifier")) {
        var.access_modifier = symbol.metadata.at("access_modifier");
    }
    if (symbol.metadata.count("is_static") && symbol.metadata.at("is_static") == "true") {
        var.is_static = true;
    }
    if (symbol.metadata.count("is_const") && symbol.metadata.at("is_const") == "true") {
        var.is_const = true;
    }
    
    return var;
}

//=============================================================================
// ユーティリティ
//=============================================================================

std::string RustSymbolConverter::generate_unique_id(const std::string& base) {
    std::string id = base;
    
    // 特殊文字を置換
    for (char& c : id) {
        if (!std::isalnum(c) && c != '_') {
            c = '_';
        }
    }
    
    // 重複チェック
    if (used_ids_.find(id) != used_ids_.end()) {
        int& counter = id_counters_[base];
        do {
            counter++;
            id = base + "_" + std::to_string(counter);
        } while (used_ids_.find(id) != used_ids_.end());
    }
    
    used_ids_.insert(id);
    return id;
}

std::string RustSymbolConverter::build_qualified_name(
    const std::string& parent, 
    const std::string& name) {
    
    if (parent.empty()) {
        return name;
    }
    return parent + "::" + name;
}

void RustSymbolConverter::inherit_metadata(
    UniversalSymbolInfo& symbol, 
    const FunctionInfo& func) {
    
    // Phase 2で設定されたmetadataをそのまま引き継ぎ
    for (const auto& [key, value] : func.metadata) {
        symbol.metadata[key] = value;
    }
    
    // 基本的なメタデータも設定
    symbol.metadata["language"] = "rust";
    
    if (func.is_async) {
        symbol.metadata["is_async"] = "true";
    }
    
    if (func.is_arrow_function) {
        symbol.metadata["is_closure"] = "true";  // Rustではclosure
    }
}

} // namespace nekocode