//=============================================================================
// 🟨 JavaScript/TypeScript Symbol Converter Implementation
//
// Phase 4.1: JavaScript/TypeScript専用変換レイヤーの実装
// Rust実装をテンプレートとして使用
//=============================================================================

#include "js_symbol_converter.hpp"
#include <algorithm>
#include <sstream>

namespace nekocode {

//=============================================================================
// 主要変換メソッド
//=============================================================================

SymbolTable JSSymbolConverter::convert_from_analysis_result(const AnalysisResult& result) {
    SymbolTable table;
    
    // JavaScript/TypeScript固有のコンテキスト設定
    JSContext context;
    context.language = result.language;
    context.is_module = !result.exports.empty() || !result.imports.empty();
    context.is_commonjs = false;  // 詳細な検出は将来実装
    
    // CommonJS検出（簡易版）
    for (const auto& imp : result.imports) {
        if (imp.type == ImportType::COMMONJS_REQUIRE) {
            context.is_commonjs = true;
            break;
        }
    }
    
    // マッピング用（旧ClassInfo → 新symbol_id）
    std::unordered_map<std::string, std::string> class_name_to_id;
    
    // ========== 1. クラスを変換 ==========
    for (const auto& cls : result.classes) {
        UniversalSymbolInfo class_sym = convert_class_to_symbol(cls, context);
        class_sym.symbol_id = generate_unique_id("class_" + cls.name);
        class_sym.start_line = cls.start_line;
        class_sym.end_line = cls.end_line;
        
        // クラスのメタデータ設定
        class_sym.metadata = build_class_metadata(cls, context);
        
        // IDマッピングを保存
        class_name_to_id[cls.name] = class_sym.symbol_id;
        
        // ========== 2. メンバ変数を子要素として追加 ==========
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
        
        // ========== 3. メソッドを子要素として追加 ==========
        for (const auto& method : cls.methods) {
            UniversalSymbolInfo method_sym = convert_method_to_symbol(method, class_sym.symbol_id, context);
            method_sym.symbol_id = generate_unique_id("method_" + cls.name + "_" + method.name);
            method_sym.parent_id = class_sym.symbol_id;
            
            // qualified_name設定
            method_sym.qualified_name = cls.name + "::" + method.name;
            
            // メソッドのメタデータ
            method_sym.metadata = build_method_metadata(method, context);
            method_sym.metadata["parent_class"] = cls.name;
            method_sym.metadata["language"] = (context.language == Language::JAVASCRIPT) ? "javascript" : "typescript";
            
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
    
    // ========== 4. スタンドアロン関数を変換 ==========
    for (const auto& func : result.functions) {
        UniversalSymbolInfo func_sym = convert_function_to_symbol(func, context);
        func_sym.symbol_id = generate_unique_id("function_" + func.name);
        
        // 関数のメタデータ
        func_sym.metadata = build_function_metadata(func, context);
        func_sym.metadata["language"] = (context.language == Language::JAVASCRIPT) ? "javascript" : "typescript";
        
        // 複雑度情報設定
        func_sym.complexity.cyclomatic_complexity = func.complexity.cyclomatic_complexity;
        func_sym.complexity.cognitive_complexity = func.complexity.cognitive_complexity;
        func_sym.complexity.max_nesting_depth = func.complexity.max_nesting_depth;
        
        table.add_symbol(func_sym);
    }
    
    return table;
}

//=============================================================================
// 個別シンボル変換メソッド
//=============================================================================

UniversalSymbolInfo JSSymbolConverter::convert_class_to_symbol(const ClassInfo& class_info, JSContext& context) {
    UniversalSymbolInfo symbol;
    symbol.symbol_type = SymbolType::CLASS;
    symbol.name = class_info.name;
    symbol.start_line = class_info.start_line;
    symbol.end_line = class_info.end_line;
    
    // 基本的な複雑度設定
    symbol.complexity.cyclomatic_complexity = 1;  // クラス自体の基本複雑度
    symbol.complexity.cognitive_complexity = 0;
    symbol.complexity.max_nesting_depth = 0;
    
    return symbol;
}

UniversalSymbolInfo JSSymbolConverter::convert_function_to_symbol(const FunctionInfo& func_info, JSContext& context) {
    UniversalSymbolInfo symbol;
    symbol.symbol_type = SymbolType::FUNCTION;
    symbol.name = func_info.name;
    symbol.start_line = func_info.start_line;
    symbol.end_line = func_info.end_line;
    symbol.parameters = func_info.parameters;
    
    return symbol;
}

UniversalSymbolInfo JSSymbolConverter::convert_method_to_symbol(const FunctionInfo& method, const std::string& parent_id, JSContext& context) {
    UniversalSymbolInfo symbol;
    symbol.symbol_type = SymbolType::METHOD;
    symbol.name = method.name;
    symbol.start_line = method.start_line;
    symbol.end_line = method.end_line;
    symbol.parameters = method.parameters;
    symbol.parent_id = parent_id;
    
    return symbol;
}

UniversalSymbolInfo JSSymbolConverter::convert_member_to_symbol(const MemberVariable& member, const std::string& parent_id, JSContext& context) {
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

std::unordered_map<std::string, std::string> JSSymbolConverter::build_class_metadata(const ClassInfo& class_info, JSContext& context) {
    std::unordered_map<std::string, std::string> metadata;
    
    metadata["kind"] = "class";
    metadata["language"] = (context.language == Language::JAVASCRIPT) ? "javascript" : "typescript";
    
    if (!class_info.parent_class.empty()) {
        metadata["extends"] = class_info.parent_class;
    }
    
    if (context.is_module) {
        metadata["module_type"] = "es6";
    } else if (context.is_commonjs) {
        metadata["module_type"] = "commonjs";
    }
    
    return metadata;
}

std::unordered_map<std::string, std::string> JSSymbolConverter::build_function_metadata(const FunctionInfo& func_info, JSContext& context) {
    std::unordered_map<std::string, std::string> metadata;
    
    if (func_info.is_async) {
        metadata["is_async"] = "true";
    }
    
    if (func_info.is_arrow_function) {
        metadata["function_type"] = "arrow";
    } else {
        metadata["function_type"] = "regular";
    }
    
    // JavaScript/TypeScript固有のメタデータをfunc_info.metadataから取得
    for (const auto& [key, value] : func_info.metadata) {
        metadata[key] = value;
    }
    
    return metadata;
}

std::unordered_map<std::string, std::string> JSSymbolConverter::build_method_metadata(const FunctionInfo& method, JSContext& context) {
    auto metadata = build_function_metadata(method, context);
    
    metadata["method_type"] = "instance";  // デフォルト
    
    // static/privateなどの判定は既存のmetadataから
    for (const auto& [key, value] : method.metadata) {
        if (key == "is_static" && value == "true") {
            metadata["method_type"] = "static";
        }
        if (key == "access_modifier") {
            metadata["access_modifier"] = value;
        }
        metadata[key] = value;
    }
    
    return metadata;
}

//=============================================================================
// ユーティリティメソッド
//=============================================================================

std::string JSSymbolConverter::generate_unique_id(const std::string& base) {
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

AnalysisResult JSSymbolConverter::convert_to_analysis_result(const SymbolTable& table) {
    // 将来実装予定
    AnalysisResult result;
    result.language = Language::JAVASCRIPT;  // デフォルト
    return result;
}

} // namespace nekocode