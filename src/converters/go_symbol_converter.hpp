//=============================================================================
// 🐹 Go Symbol Converter
//
// Phase 4.5: Go専用のUniversalSymbol変換レイヤー
// AnalysisResult ⇔ SymbolTable の相互変換
//=============================================================================

#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include "nekocode/types.hpp"
#include "nekocode/universal_symbol.hpp"
#include "nekocode/symbol_table.hpp"

namespace nekocode {

//=============================================================================
// 🔄 Go Symbol Converter Class
//=============================================================================

class GoSymbolConverter {
private:
    // ID生成の重複管理
    std::unordered_set<std::string> used_ids_;
    std::unordered_map<std::string, int> id_counters_;
    
    // Go固有のコンテキスト
    struct GoContext {
        Language language = Language::GO;
        std::string current_package;  // 現在のpackage
        bool has_interfaces = false;  // interface定義の有無
        bool has_goroutines = false;  // goroutine使用の有無
        bool has_channels = false;    // channel使用の有無
    };
    
    // Package解析用
    struct PackageInfo {
        std::string name;           // package名
        std::string full_name;      // 完全修飾package名
        LineNumber start_line;
        std::vector<std::string> imports;        // importされたpackage
        std::vector<std::string> child_types;   // 子type
        std::vector<std::string> child_funcs;   // 子function
    };
    
    // ユニークID生成
    std::string generate_unique_id(const std::string& base);
    
    // Go特有の解析
    std::vector<PackageInfo> parse_packages(const AnalysisResult& result);
    bool is_package_class(const ClassInfo& class_info);
    bool is_interface_class(const ClassInfo& class_info);
    bool is_struct_class(const ClassInfo& class_info);
    bool is_receiver_method(const FunctionInfo& func_info);
    std::string extract_package_name(const std::string& class_name);
    std::string extract_interface_name(const std::string& class_name);
    std::string extract_receiver_type(const FunctionInfo& func_info);
    
    // シンボル変換
    UniversalSymbolInfo convert_package_to_symbol(const PackageInfo& pkg_info, GoContext& context);
    UniversalSymbolInfo convert_interface_to_symbol(const ClassInfo& class_info, GoContext& context);
    UniversalSymbolInfo convert_struct_to_symbol(const ClassInfo& class_info, GoContext& context);
    UniversalSymbolInfo convert_function_to_symbol(const FunctionInfo& func_info, GoContext& context);
    UniversalSymbolInfo convert_method_to_symbol(const FunctionInfo& method, const std::string& parent_id, GoContext& context);
    UniversalSymbolInfo convert_member_to_symbol(const MemberVariable& member, const std::string& parent_id, GoContext& context);
    
    // メタデータ構築
    std::unordered_map<std::string, std::string> build_package_metadata(const PackageInfo& pkg_info, GoContext& context);
    std::unordered_map<std::string, std::string> build_interface_metadata(const ClassInfo& class_info, GoContext& context);
    std::unordered_map<std::string, std::string> build_struct_metadata(const ClassInfo& class_info, GoContext& context);
    std::unordered_map<std::string, std::string> build_function_metadata(const FunctionInfo& func_info, GoContext& context);
    std::unordered_map<std::string, std::string> build_method_metadata(const FunctionInfo& method, GoContext& context);
    
public:
    GoSymbolConverter() = default;
    ~GoSymbolConverter() = default;
    
    // AnalysisResult → SymbolTable 変換
    SymbolTable convert_from_analysis_result(const AnalysisResult& result);
    
    // SymbolTable → AnalysisResult 変換（将来用）
    AnalysisResult convert_to_analysis_result(const SymbolTable& table);
};

} // namespace nekocode