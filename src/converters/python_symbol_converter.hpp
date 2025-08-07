//=============================================================================
// 🐍 Python Symbol Converter
//
// Phase 4.2: Python専用のUniversalSymbol変換レイヤー
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
// 🔄 Python Symbol Converter Class
//=============================================================================

class PythonSymbolConverter {
private:
    // ID生成の重複管理
    std::unordered_set<std::string> used_ids_;
    std::unordered_map<std::string, int> id_counters_;
    
    // Python固有のコンテキスト
    struct PythonContext {
        Language language = Language::PYTHON;
        bool is_package = false;  // __init__.pyがあるかどうか
        bool has_main_guard = false;  // if __name__ == "__main__"
        std::string module_name;  // モジュール名
    };
    
    // ユニークID生成
    std::string generate_unique_id(const std::string& base);
    
    // シンボル変換
    UniversalSymbolInfo convert_class_to_symbol(const ClassInfo& class_info, PythonContext& context);
    UniversalSymbolInfo convert_function_to_symbol(const FunctionInfo& func_info, PythonContext& context);
    UniversalSymbolInfo convert_method_to_symbol(const FunctionInfo& method, const std::string& parent_id, PythonContext& context);
    UniversalSymbolInfo convert_member_to_symbol(const MemberVariable& member, const std::string& parent_id, PythonContext& context);
    
    // メタデータ構築
    std::unordered_map<std::string, std::string> build_class_metadata(const ClassInfo& class_info, PythonContext& context);
    std::unordered_map<std::string, std::string> build_function_metadata(const FunctionInfo& func_info, PythonContext& context);
    std::unordered_map<std::string, std::string> build_method_metadata(const FunctionInfo& method, PythonContext& context);
    
public:
    PythonSymbolConverter() = default;
    ~PythonSymbolConverter() = default;
    
    // AnalysisResult → SymbolTable 変換
    SymbolTable convert_from_analysis_result(const AnalysisResult& result);
    
    // SymbolTable → AnalysisResult 変換（将来用）
    AnalysisResult convert_to_analysis_result(const SymbolTable& table);
};

} // namespace nekocode