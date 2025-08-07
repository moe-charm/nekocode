//=============================================================================
// 🦀 Rust Symbol Converter
//
// Phase 3: Rust専用のUniversalSymbol変換レイヤー
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
// 🔄 Rust Symbol Converter Class
//=============================================================================

class RustSymbolConverter {
private:
    // ID生成の重複管理
    std::unordered_set<std::string> used_ids_;
    std::unordered_map<std::string, int> id_counters_;
    
    // implブロックの一時管理（変換時に使用）
    struct ImplContext {
        std::string impl_id;
        std::string struct_name;
        std::string trait_name;
        bool is_trait_impl;
    };
    std::vector<ImplContext> impl_contexts_;
    
public:
    // ========== 主要変換メソッド ==========
    
    // AnalysisResult → SymbolTable 変換
    // Phase 1,2の成果（impl分類、metadata）を活用
    SymbolTable convert_from_analysis_result(const AnalysisResult& result);
    
    // SymbolTable → AnalysisResult 逆変換
    // 後方互換性のため（既存コードが動作継続）
    AnalysisResult convert_to_analysis_result(const SymbolTable& symbols);
    
private:
    // ========== 変換ヘルパーメソッド ==========
    
    // struct/class変換（Rustではstruct）
    UniversalSymbolInfo convert_struct(const ClassInfo& cls);
    
    // メソッド変換（implブロック内の関数）
    UniversalSymbolInfo convert_method(
        const FunctionInfo& method, 
        const std::string& parent_struct_id);
    
    // 独立関数変換（トップレベル関数）
    UniversalSymbolInfo convert_function(const FunctionInfo& func);
    
    // メンバ変数変換（structのフィールド）
    UniversalSymbolInfo convert_member_var(
        const MemberVariable& var, 
        const std::string& parent_struct_id);
    
    // ========== 逆変換ヘルパー ==========
    
    // structシンボル → ClassInfo
    ClassInfo symbol_to_class(const UniversalSymbolInfo& symbol, const SymbolTable& table);
    
    // methodシンボル → FunctionInfo
    FunctionInfo symbol_to_function(const UniversalSymbolInfo& symbol);
    
    // member_varシンボル → MemberVariable
    MemberVariable symbol_to_member_var(const UniversalSymbolInfo& symbol);
    
    // ========== ユーティリティ ==========
    
    // ユニークなID生成
    std::string generate_unique_id(const std::string& base);
    
    // 完全修飾名の生成（例: "DatabaseManager::new"）
    std::string build_qualified_name(const std::string& parent, const std::string& name);
    
    // Phase 2のmetadataをそのまま継承
    void inherit_metadata(UniversalSymbolInfo& symbol, const FunctionInfo& func);
    
    // implコンテキストの解決（どのstructに属するか）
    std::string resolve_impl_context(const FunctionInfo& method);
    
    // アクセス修飾子の判定
    std::string determine_access_modifier(const FunctionInfo& func);
    
    // implタイプの判定（inherent or trait）
    std::string determine_impl_type(const FunctionInfo& func);
};

} // namespace nekocode