//=============================================================================
// ⚙️ C++ Symbol Converter
//
// Phase 4.3: C++専用のUniversalSymbol変換レイヤー
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
// 🔄 C++ Symbol Converter Class
//=============================================================================

class CppSymbolConverter {
private:
    // ID生成の重複管理
    std::unordered_set<std::string> used_ids_;
    std::unordered_map<std::string, int> id_counters_;
    
    // C++固有のコンテキスト
    struct CppContext {
        Language language = Language::CPP;
        std::vector<std::string> namespace_stack;  // 現在のnamespace階層
        bool has_templates = false;  // テンプレート使用の有無
    };
    
    // Namespace解析用
    struct NamespaceInfo {
        std::string name;           // namespace名
        std::string full_name;      // 完全修飾namespace名
        LineNumber start_line;
        std::vector<std::string> child_namespaces; // 子namespace
        std::vector<std::string> child_classes;    // 子class
    };
    
    // ユニークID生成
    std::string generate_unique_id(const std::string& base);
    
    // C++特有の解析
    std::vector<NamespaceInfo> parse_namespaces(const AnalysisResult& result);
    bool is_namespace_class(const ClassInfo& class_info);
    bool is_template_class(const ClassInfo& class_info);
    std::string extract_namespace_name(const std::string& class_name);
    
    // シンボル変換
    UniversalSymbolInfo convert_namespace_to_symbol(const NamespaceInfo& ns_info, CppContext& context);
    UniversalSymbolInfo convert_class_to_symbol(const ClassInfo& class_info, CppContext& context);
    UniversalSymbolInfo convert_function_to_symbol(const FunctionInfo& func_info, CppContext& context);
    UniversalSymbolInfo convert_method_to_symbol(const FunctionInfo& method, const std::string& parent_id, CppContext& context);
    UniversalSymbolInfo convert_member_to_symbol(const MemberVariable& member, const std::string& parent_id, CppContext& context);
    
    // メタデータ構築
    std::unordered_map<std::string, std::string> build_namespace_metadata(const NamespaceInfo& ns_info, CppContext& context);
    std::unordered_map<std::string, std::string> build_class_metadata(const ClassInfo& class_info, CppContext& context);
    std::unordered_map<std::string, std::string> build_function_metadata(const FunctionInfo& func_info, CppContext& context);
    std::unordered_map<std::string, std::string> build_method_metadata(const FunctionInfo& method, CppContext& context);
    
public:
    CppSymbolConverter() = default;
    ~CppSymbolConverter() = default;
    
    // AnalysisResult → SymbolTable 変換
    SymbolTable convert_from_analysis_result(const AnalysisResult& result);
    
    // SymbolTable → AnalysisResult 変換（将来用）
    AnalysisResult convert_to_analysis_result(const SymbolTable& table);
};

} // namespace nekocode