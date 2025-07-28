#pragma once

//=============================================================================
// 🐍 Python Language Analyzer - Python専用解析エンジン
//
// Pythonコードの構造解析・複雑度計算
// クラス、関数、デコレータ、import文の検出
//=============================================================================

#include "base_analyzer.hpp"
#include <vector>
#include <sstream>

// 🚫 std::regex は使用禁止！代わりに文字列ベース検索を使用

namespace nekocode {

//=============================================================================
// 🐍 PythonAnalyzer - Python専用解析クラス
//=============================================================================

class PythonAnalyzer : public BaseAnalyzer {
public:
    PythonAnalyzer();
    ~PythonAnalyzer() override = default;
    
    //=========================================================================
    // 🔍 BaseAnalyzer インターフェース実装
    //=========================================================================
    
    Language get_language() const override;
    std::string get_language_name() const override;
    std::vector<std::string> get_supported_extensions() const override;
    AnalysisResult analyze(const std::string& content, const std::string& filename) override;
    
private:
    //=========================================================================
    // 🔧 内部実装（std::regex完全除去版）
    //=========================================================================
    
    /// Python クラス検出（文字列ベース）
    void extract_classes(const std::string& content, AnalysisResult& result);
    
    /// Python 関数検出（文字列ベース）
    void extract_functions(const std::string& content, AnalysisResult& result);
    
    /// Python import文検出（文字列ベース）
    void extract_imports(const std::string& content, AnalysisResult& result);
    
    /// パラメータ抽出（文字列ベース）
    std::vector<std::string> extract_parameters(const std::string& params_str);
    
    /// Python特化複雑度計算
    ComplexityInfo calculate_python_complexity(const std::string& content);
    
    /// インデント深度計算
    int calculate_indentation_depth(const std::string& line);
    
    /// Python キーワード検出
    bool is_python_function_line(const std::string& line);
    bool is_python_class_line(const std::string& line);
    bool is_python_import_line(const std::string& line);
    
    /// 🎯 ハイブリッド戦略: 統計整合性チェック
    bool needs_python_line_based_fallback(const AnalysisResult& result, const std::string& content);
    
    /// 🔧 文字列ベース フォールバック解析
    void apply_python_line_based_analysis(AnalysisResult& result, const std::string& content);
};

} // namespace nekocode