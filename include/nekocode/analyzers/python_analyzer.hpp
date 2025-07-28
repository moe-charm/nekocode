#pragma once

//=============================================================================
// 🐍 Python Language Analyzer - Python専用解析エンジン
//
// Pythonコードの構造解析・複雑度計算
// クラス、関数、デコレータ、import文の検出
//=============================================================================

#include "base_analyzer.hpp"
#include <regex>
#include <vector>

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
    // 🔧 内部実装
    //=========================================================================
    
    /// 正規表現パターン初期化
    void initialize_patterns();
    
    /// クラス抽出
    void extract_classes(const std::string& content, AnalysisResult& result);
    
    /// 関数抽出（トップレベル）
    void extract_functions(const std::string& content, AnalysisResult& result);
    
    /// メソッド抽出（クラス内）
    void extract_methods(const std::string& class_content, ClassInfo& class_info, uint32_t base_line);
    
    /// import文抽出
    void extract_imports(const std::string& content, AnalysisResult& result);
    
    /// パラメータ抽出
    void extract_parameters(const std::string& params_str, std::vector<std::string>& parameters);
    
    /// クラス終端位置検出
    size_t find_class_end(const std::string& content, size_t class_start);
    
    /// Python特化複雑度計算
    ComplexityInfo calculate_python_complexity(const std::string& content);
    
    /// ネスト深度計算（インデントベース）
    void calculate_nesting_depth(const std::string& content, ComplexityInfo& complexity);
    
    //=========================================================================
    // 📊 メンバ変数
    //=========================================================================
    
    std::regex class_pattern_;
    std::regex function_pattern_;
    std::regex method_pattern_;
    std::vector<std::regex> import_patterns_;
    std::regex decorator_pattern_;
};

} // namespace nekocode