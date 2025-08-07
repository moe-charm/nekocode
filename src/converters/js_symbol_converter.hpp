//=============================================================================
// 🟨 JavaScript/TypeScript Symbol Converter
//
// Phase 4.1: JavaScript/TypeScript専用のUniversalSymbol変換レイヤー
// AnalysisResult ⇔ SymbolTable の相互変換
//=============================================================================

#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include "nekocode/types.hpp"
#include "nekocode/universal_symbol.hpp"
#include "nekocode/symbol_table.hpp"

namespace nekocode {

//=============================================================================
// 🔄 JavaScript/TypeScript Symbol Converter Class
//=============================================================================

class JSSymbolConverter {
private:
    // ID生成の重複管理
    std::unordered_set<std::string> used_ids_;
    std::unordered_map<std::string, int> id_counters_;
    
    // JavaScript/TypeScript固有のコンテキスト
    struct JSContext {
        Language language;  // JavaScript or TypeScript
        bool is_module = false;  // ES6 module形式かどうか
        bool is_commonjs = false;  // CommonJS形式かどうか
    };
    
    // ユニークID生成
    std::string generate_unique_id(const std::string& base);
    
    // シンボル変換
    UniversalSymbolInfo convert_class_to_symbol(const ClassInfo& class_info, JSContext& context);
    UniversalSymbolInfo convert_function_to_symbol(const FunctionInfo& func_info, JSContext& context);
    UniversalSymbolInfo convert_method_to_symbol(const FunctionInfo& method, const std::string& parent_id, JSContext& context);
    UniversalSymbolInfo convert_member_to_symbol(const MemberVariable& member, const std::string& parent_id, JSContext& context);
    
    // メタデータ構築
    std::unordered_map<std::string, std::string> build_class_metadata(const ClassInfo& class_info, JSContext& context);
    std::unordered_map<std::string, std::string> build_function_metadata(const FunctionInfo& func_info, JSContext& context);
    std::unordered_map<std::string, std::string> build_method_metadata(const FunctionInfo& method, JSContext& context);
    
public:
    JSSymbolConverter() = default;
    ~JSSymbolConverter() = default;
    
    // AnalysisResult → SymbolTable 変換
    SymbolTable convert_from_analysis_result(const AnalysisResult& result);
    
    // SymbolTable → AnalysisResult 変換（将来用）
    AnalysisResult convert_to_analysis_result(const SymbolTable& table);
};

} // namespace nekocode