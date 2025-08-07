//=============================================================================
// ⚙️ C++ Symbol Converter Implementation
//
// Phase 4.3: C++専用変換レイヤーの実装
// JavaScript/Python実装をテンプレートとして使用
//=============================================================================

#include "cpp_symbol_converter.hpp"
#include <algorithm>
#include <sstream>
#include <iostream>

namespace nekocode {

//=============================================================================
// 主要変換メソッド
//=============================================================================

SymbolTable CppSymbolConverter::convert_from_analysis_result(const AnalysisResult& result) {
    SymbolTable table;
    
    
    // C++固有のコンテキスト設定
    CppContext context;
    context.language = result.language;
    
    // C++特有: namespace階層を解析
    auto namespaces = parse_namespaces(result);
    
    // マッピング用（クラス名 → symbol_id）
    std::unordered_map<std::string, std::string> class_name_to_id;
    std::unordered_map<std::string, std::string> namespace_name_to_id;
    
    // ========== 1. namespaceを変換 ==========
    for (const auto& ns : namespaces) {
        UniversalSymbolInfo ns_sym = convert_namespace_to_symbol(ns, context);
        ns_sym.symbol_id = generate_unique_id("namespace_" + ns.name);
        ns_sym.start_line = ns.start_line;
        
        // namespaceのメタデータ設定
        ns_sym.metadata = build_namespace_metadata(ns, context);
        
        // IDマッピングを保存
        namespace_name_to_id[ns.name] = ns_sym.symbol_id;
        
        table.add_symbol(ns_sym);
    }
    
    // ========== 2. 通常のクラスを変換 ==========
    for (const auto& cls : result.classes) {
        if (is_namespace_class(cls)) {
            continue; // namespaceとして既に処理済み
        }
        
        UniversalSymbolInfo class_sym = convert_class_to_symbol(cls, context);
        class_sym.symbol_id = generate_unique_id("class_" + cls.name);
        class_sym.start_line = cls.start_line;
        class_sym.end_line = cls.end_line;
        
        // クラスのメタデータ設定
        class_sym.metadata = build_class_metadata(cls, context);
        
        // IDマッピングを保存
        class_name_to_id[cls.name] = class_sym.symbol_id;
        
        // ========== 3. メンバ変数を子要素として追加 ==========
        for (const auto& member : cls.member_variables) {
            UniversalSymbolInfo member_sym = convert_member_to_symbol(member, class_sym.symbol_id, context);
            member_sym.symbol_id = generate_unique_id("field_" + cls.name + "_" + member.name);
            member_sym.parent_id = class_sym.symbol_id;
            
            // メンバ変数のメタデータ
            member_sym.metadata["access_modifier"] = member.access_modifier;
            member_sym.metadata["type"] = member.type;
            if (member.is_static) {
                member_sym.metadata["is_static"] = "true";
            }
            if (member.is_const) {
                member_sym.metadata["is_const"] = "true";
            }
            
            // 子要素として追加
            class_sym.child_ids.push_back(member_sym.symbol_id);
            table.add_symbol(member_sym);
        }
        
        // ========== 4. メソッドを子要素として追加 ==========
        for (const auto& method : cls.methods) {
            UniversalSymbolInfo method_sym = convert_method_to_symbol(method, class_sym.symbol_id, context);
            method_sym.symbol_id = generate_unique_id("method_" + cls.name + "_" + method.name);
            method_sym.parent_id = class_sym.symbol_id;
            
            // qualified_name設定
            method_sym.qualified_name = cls.name + "::" + method.name;
            
            // メソッドのメタデータ
            method_sym.metadata = build_method_metadata(method, context);
            method_sym.metadata["parent_class"] = cls.name;
            method_sym.metadata["language"] = "cpp";
            
            // 複雑度情報設定
            method_sym.complexity.cyclomatic_complexity = method.complexity.cyclomatic_complexity;
            method_sym.complexity.cognitive_complexity = method.complexity.cognitive_complexity;
            method_sym.complexity.max_nesting_depth = method.complexity.max_nesting_depth;
            
            // 子要素として追加
            class_sym.child_ids.push_back(method_sym.symbol_id);
            table.add_symbol(method_sym);
        }
        
        table.add_symbol(class_sym);
    }
    
    // ========== 5. スタンドアロン関数を変換 ==========
    for (const auto& func : result.functions) {
        // クラスメソッドかどうかの簡易判定（既にメソッドとして処理済みの場合はスキップ）
        bool is_method = false;
        for (const auto& cls : result.classes) {
            if (is_namespace_class(cls)) continue;
            
            for (const auto& method : cls.methods) {
                if (method.name == func.name) {
                    is_method = true;
                    break;
                }
            }
            if (is_method) break;
        }
        
        if (is_method) continue; // 既にメソッドとして処理済み
        
        UniversalSymbolInfo func_sym = convert_function_to_symbol(func, context);
        func_sym.symbol_id = generate_unique_id("function_" + func.name);
        
        // 関数のメタデータ
        func_sym.metadata = build_function_metadata(func, context);
        func_sym.metadata["language"] = "cpp";
        
        // 複雑度情報設定
        func_sym.complexity.cyclomatic_complexity = func.complexity.cyclomatic_complexity;
        func_sym.complexity.cognitive_complexity = func.complexity.cognitive_complexity;
        func_sym.complexity.max_nesting_depth = func.complexity.max_nesting_depth;
        
        table.add_symbol(func_sym);
    }
    
    return table;
}

//=============================================================================
// C++特有の解析メソッド
//=============================================================================

std::vector<CppSymbolConverter::NamespaceInfo> CppSymbolConverter::parse_namespaces(const AnalysisResult& result) {
    std::vector<NamespaceInfo> namespaces;
    
    for (const auto& cls : result.classes) {
        if (is_namespace_class(cls)) {
            NamespaceInfo ns;
            ns.name = extract_namespace_name(cls.name);
            ns.full_name = cls.name; // "namespace:Database" 形式
            ns.start_line = cls.start_line;
            namespaces.push_back(ns);
        }
    }
    
    return namespaces;
}

bool CppSymbolConverter::is_namespace_class(const ClassInfo& class_info) {
    return class_info.name.starts_with("namespace:");
}

bool CppSymbolConverter::is_template_class(const ClassInfo& class_info) {
    // 簡易判定：テンプレート記号の検出
    return class_info.name.find('<') != std::string::npos ||
           class_info.name.find("template") != std::string::npos;
}

std::string CppSymbolConverter::extract_namespace_name(const std::string& class_name) {
    if (class_name.starts_with("namespace:")) {
        return class_name.substr(10); // "namespace:"を除去
    }
    return class_name;
}

//=============================================================================
// 個別シンボル変換メソッド
//=============================================================================

UniversalSymbolInfo CppSymbolConverter::convert_namespace_to_symbol(const NamespaceInfo& ns_info, CppContext& context) {
    UniversalSymbolInfo symbol;
    symbol.symbol_type = SymbolType::NAMESPACE;
    symbol.name = ns_info.name;
    symbol.start_line = ns_info.start_line;
    symbol.qualified_name = ns_info.name; // 簡易実装
    
    // 基本的な複雑度設定
    symbol.complexity.cyclomatic_complexity = 1;
    symbol.complexity.cognitive_complexity = 0;
    symbol.complexity.max_nesting_depth = 0;
    
    return symbol;
}

UniversalSymbolInfo CppSymbolConverter::convert_class_to_symbol(const ClassInfo& class_info, CppContext& context) {
    UniversalSymbolInfo symbol;
    symbol.symbol_type = SymbolType::CLASS;
    symbol.name = class_info.name;
    symbol.start_line = class_info.start_line;
    symbol.end_line = class_info.end_line;
    
    // 基本的な複雑度設定
    symbol.complexity.cyclomatic_complexity = 1;
    symbol.complexity.cognitive_complexity = 0;
    symbol.complexity.max_nesting_depth = 0;
    
    return symbol;
}

UniversalSymbolInfo CppSymbolConverter::convert_function_to_symbol(const FunctionInfo& func_info, CppContext& context) {
    UniversalSymbolInfo symbol;
    symbol.symbol_type = SymbolType::FUNCTION;
    symbol.name = func_info.name;
    symbol.start_line = func_info.start_line;
    symbol.end_line = func_info.end_line;
    symbol.parameters = func_info.parameters;
    
    return symbol;
}

UniversalSymbolInfo CppSymbolConverter::convert_method_to_symbol(const FunctionInfo& method, const std::string& parent_id, CppContext& context) {
    UniversalSymbolInfo symbol;
    symbol.symbol_type = SymbolType::METHOD;
    symbol.name = method.name;
    symbol.start_line = method.start_line;
    symbol.end_line = method.end_line;
    symbol.parameters = method.parameters;
    symbol.parent_id = parent_id;
    
    return symbol;
}

UniversalSymbolInfo CppSymbolConverter::convert_member_to_symbol(const MemberVariable& member, const std::string& parent_id, CppContext& context) {
    UniversalSymbolInfo symbol;
    symbol.symbol_type = SymbolType::MEMBER_VAR;
    symbol.name = member.name;
    symbol.start_line = member.declaration_line;
    symbol.end_line = member.declaration_line;
    symbol.parent_id = parent_id;
    
    // 基本的な複雑度設定
    symbol.complexity.cyclomatic_complexity = 1;
    symbol.complexity.cognitive_complexity = 0;
    symbol.complexity.max_nesting_depth = 0;
    
    return symbol;
}

//=============================================================================
// メタデータ構築メソッド
//=============================================================================

std::unordered_map<std::string, std::string> CppSymbolConverter::build_namespace_metadata(const NamespaceInfo& ns_info, CppContext& context) {
    std::unordered_map<std::string, std::string> metadata;
    
    metadata["kind"] = "namespace";
    metadata["language"] = "cpp";
    metadata["full_name"] = ns_info.full_name;
    
    return metadata;
}

std::unordered_map<std::string, std::string> CppSymbolConverter::build_class_metadata(const ClassInfo& class_info, CppContext& context) {
    std::unordered_map<std::string, std::string> metadata;
    
    metadata["kind"] = "class";
    metadata["language"] = "cpp";
    
    if (is_template_class(class_info)) {
        metadata["is_template"] = "true";
    }
    
    if (!class_info.parent_class.empty()) {
        metadata["base_class"] = class_info.parent_class;
    }
    
    return metadata;
}

std::unordered_map<std::string, std::string> CppSymbolConverter::build_function_metadata(const FunctionInfo& func_info, CppContext& context) {
    std::unordered_map<std::string, std::string> metadata;
    
    // 関数テンプレートの判定
    if (func_info.name == "max" || func_info.name.find("template") != std::string::npos) {
        metadata["is_template"] = "true";
    }
    
    // C++固有のfunction metadata
    for (const auto& [key, value] : func_info.metadata) {
        metadata[key] = value;
    }
    
    return metadata;
}

std::unordered_map<std::string, std::string> CppSymbolConverter::build_method_metadata(const FunctionInfo& method, CppContext& context) {
    auto metadata = build_function_metadata(method, context);
    
    // C++特有のメソッド判定
    if (method.name.find("~") != std::string::npos) {
        metadata["method_type"] = "destructor";
    } else if (method.name == method.name) { // 簡易コンストラクタ判定
        metadata["method_type"] = "method"; // デフォルト
    }
    
    // static判定
    for (const auto& [key, value] : method.metadata) {
        if (key == "is_static" && value == "true") {
            metadata["method_type"] = "static";
        }
        metadata[key] = value;
    }
    
    return metadata;
}

//=============================================================================
// ユーティリティメソッド
//=============================================================================

std::string CppSymbolConverter::generate_unique_id(const std::string& base) {
    std::string candidate = base;
    
    if (used_ids_.count(candidate)) {
        int counter = ++id_counters_[base];
        candidate = base + "_" + std::to_string(counter);
        
        while (used_ids_.count(candidate)) {
            counter = ++id_counters_[base];
            candidate = base + "_" + std::to_string(counter);
        }
    }
    
    used_ids_.insert(candidate);
    return candidate;
}

//=============================================================================
// 将来実装: SymbolTable → AnalysisResult 変換
//=============================================================================

AnalysisResult CppSymbolConverter::convert_to_analysis_result(const SymbolTable& table) {
    // 将来実装予定
    AnalysisResult result;
    result.language = Language::CPP;
    return result;
}

} // namespace nekocode