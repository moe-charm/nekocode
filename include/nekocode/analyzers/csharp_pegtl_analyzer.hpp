#pragma once

//=============================================================================
// 🌟 C# PEGTL Analyzer - 革新的PEGベース解析エンジン
//
// std::regexからの完全移行を実現
// 高速・正確・拡張可能な新世代解析
//=============================================================================

#include "base_analyzer.hpp"
#include "csharp_minimal_grammar.hpp"
// 🚀 Phase 5: Universal Symbol直接生成
#include "nekocode/universal_symbol.hpp"
#include "nekocode/symbol_table.hpp"
#include <tao/pegtl.hpp>
#include <stack>
#include <iostream>
#include <regex>
#include <set>
#include <sstream>
#include <fstream>

// 🔧 グローバルデバッグフラグ（analyzer_factory.cppで定義済み）
extern bool g_debug_mode;

namespace nekocode {

//=============================================================================
// 🎯 解析状態管理
//=============================================================================

struct CSharpParseState {
    AnalysisResult result;
    std::vector<ClassInfo> current_classes;
    std::vector<FunctionInfo> current_methods;
    std::vector<ImportInfo> imports;
    
    // 現在の解析コンテキスト
    std::string current_namespace;
    std::stack<ClassInfo*> class_stack;
    std::stack<uint32_t> line_stack;
    
    // 行番号追跡
    uint32_t current_line = 1;
    
    // 🚀 Phase 5: Universal Symbol直接生成
    std::shared_ptr<SymbolTable> symbol_table;      // Universal Symbolテーブル
    std::unordered_map<std::string, int> id_counters; // ID生成用カウンター
    
    // コンストラクタ
    CSharpParseState();
    
    // 🚀 Phase 5: Universal Symbol生成メソッド  
    std::string generate_unique_id(const std::string& base);
    void add_test_class_symbol(const std::string& class_name, std::uint32_t start_line);
    void add_test_method_symbol(const std::string& method_name, std::uint32_t start_line);
    void update_line(const char* from, const char* to);
};

//=============================================================================
// 🚀 CSharpPEGTLAnalyzer - PEGTL実装
//=============================================================================

class CSharpPEGTLAnalyzer : public BaseAnalyzer {
public:
    CSharpPEGTLAnalyzer();
    virtual ~CSharpPEGTLAnalyzer() = default;
    
    Language get_language() const override;
    std::string get_language_name() const override;
    std::vector<std::string> get_supported_extensions() const override;
    AnalysisResult analyze(const std::string& content, const std::string& filename) override;

protected:
    // C#固有の複雑度計算（オーバーライド可能）
    ComplexityInfo calculate_complexity(const std::string& content) override;

private:
    // 🚀 C#ハイブリッド戦略: 統計整合性チェック
    bool needs_csharp_line_based_fallback(const AnalysisResult& result, const std::string& content);
    
    // 🚀 C#ハイブリッド戦略: 行ベース補完解析
    void apply_csharp_line_based_analysis(AnalysisResult& result, const std::string& content, const std::string& filename);
    
    // 🔥 簡易class検出（partial class対応）
    std::vector<ClassInfo> analyze_csharp_classes_simple(const std::string& content);
    
    // 行ごとの要素抽出
    void extract_csharp_elements_from_line(const std::string& line, size_t line_number,
                                           AnalysisResult& result, 
                                           std::set<std::string>& existing_classes,
                                           std::set<std::string>& existing_functions,
                                           const std::vector<std::string>& all_lines);
    
    // C# 関数の終了行を検出
    uint32_t find_function_end_line(const std::vector<std::string>& lines, size_t start_line);
};

} // namespace nekocode