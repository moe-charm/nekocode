#pragma once

//=============================================================================
// 🔥 C++ Language Analyzer - C++専用解析エンジン
//
// C++コードの構造解析・複雑度計算
// クラス、関数、テンプレート、名前空間、include解析
//=============================================================================

#include "base_analyzer.hpp"
#include "../cpp_analyzer.hpp"  // 既存のCppAnalyzerを活用
#include <memory>

namespace nekocode {

//=============================================================================
// 🔥 CppLanguageAnalyzer - C++専用解析クラス（アダプター）
//=============================================================================

class CppLanguageAnalyzer : public BaseAnalyzer {
public:
    CppLanguageAnalyzer();
    ~CppLanguageAnalyzer() override = default;
    
    //=========================================================================
    // 🔍 BaseAnalyzer インターフェース実装
    //=========================================================================
    
    Language get_language() const override { return Language::CPP; }
    std::string get_language_name() const override { return "C++"; }
    std::vector<std::string> get_supported_extensions() const override {
        return {".cpp", ".cxx", ".cc", ".C", ".hpp", ".hxx", ".hh", ".H", ".h"};
    }
    
    AnalysisResult analyze(const std::string& content, const std::string& filename) override;
    
private:
    //=========================================================================
    // 🔧 内部実装
    //=========================================================================
    
    /// 既存のCppAnalyzerを活用
    std::unique_ptr<CppAnalyzer> cpp_analyzer_;
    
    /// CppAnalysisResultからAnalysisResultへの変換
    AnalysisResult convert_result(const CppAnalysisResult& cpp_result);
};

//=============================================================================
// 🎯 CLanguageAnalyzer - C言語専用解析クラス
//=============================================================================

class CLanguageAnalyzer : public BaseAnalyzer {
public:
    CLanguageAnalyzer();
    ~CLanguageAnalyzer() override = default;
    
    //=========================================================================
    // 🔍 BaseAnalyzer インターフェース実装
    //=========================================================================
    
    Language get_language() const override { return Language::C; }
    std::string get_language_name() const override { return "C"; }
    std::vector<std::string> get_supported_extensions() const override {
        return {".c", ".h"};
    }
    
    AnalysisResult analyze(const std::string& content, const std::string& filename) override;
    
private:
    //=========================================================================
    // 🔧 内部実装（std::regex完全除去版・構造化）
    //=========================================================================
    
    /// C言語 関数検出（文字列ベース・構造化）
    void extract_functions(const std::string& content, AnalysisResult& result);
    
    /// C言語 構造体検出（文字列ベース・構造化）
    void extract_structs(const std::string& content, AnalysisResult& result);
    
    /// C言語 include検出（文字列ベース・構造化）
    void extract_includes(const std::string& content, AnalysisResult& result);
    
    /// C言語 キーワード検出
    bool is_c_function_line(const std::string& line);
    bool is_c_struct_line(const std::string& line);
    bool is_c_include_line(const std::string& line);
    
    //=========================================================================
    // 🎯 構造化されたC言語解析ヘルパー（C++成功パターン参考）
    //=========================================================================
    
    /// 関数宣言解析
    FunctionInfo parse_c_function_declaration(const std::string& line, uint32_t line_number);
    std::string extract_function_name_from_line(const std::string& line, size_t paren_pos);
    std::vector<std::string> extract_c_function_parameters(const std::string& line, size_t paren_start);
    void enhance_c_function_info(FunctionInfo& func_info, const std::string& line);
    std::string extract_parameter_name(const std::string& param);
    
    /// 構造体宣言解析
    ClassInfo parse_c_struct_declaration(const std::string& line, uint32_t line_number);
    std::string extract_struct_name(const std::string& line, size_t name_start, size_t brace_pos);
    std::string generate_anonymous_struct_name(uint32_t line_number);
    void enhance_c_struct_info(ClassInfo& struct_info, const std::string& line);
    
    /// include指示解析
    ImportInfo parse_c_include_directive(const std::string& line, uint32_t line_number);
    std::pair<std::string, bool> extract_header_info(const std::string& line, size_t include_pos);
    void enhance_c_include_info(ImportInfo& include_info, const std::string& line, bool is_system_header);
    
    /// 複雑度計算（C言語特化版）
    ComplexityInfo calculate_c_complexity(const std::string& content);
    uint32_t calculate_c_nesting_depth(const std::string& content);
    void calculate_c_specific_complexity(ComplexityInfo& complexity, const std::string& content);
    
    /// ファイル情報計算
    void calculate_line_info(const std::string& content, FileInfo& file_info);
    
    /// ユーティリティ関数
    bool is_function_already_detected(const std::vector<FunctionInfo>& functions, const std::string& name);
    bool is_struct_already_detected(const std::vector<ClassInfo>& classes, const std::string& name);
    bool is_include_already_detected(const std::vector<ImportInfo>& imports, const std::string& module_path);
    bool is_c_keyword(const std::string& word);
    
    /// 🎯 ハイブリッド戦略: 統計整合性チェック
    bool needs_c_line_based_fallback(const AnalysisResult& result, const std::string& content);
    
    /// 🔧 文字列ベース フォールバック解析
    void apply_c_line_based_analysis(AnalysisResult& result, const std::string& content);
};

} // namespace nekocode