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
    // 🔧 内部実装
    //=========================================================================
    
    void initialize_patterns();
    void extract_functions(const std::string& content, AnalysisResult& result);
    void extract_structs(const std::string& content, AnalysisResult& result);
    void extract_includes(const std::string& content, AnalysisResult& result);
    
    // C言語用パターン
    std::regex function_pattern_;
    std::regex struct_pattern_;
    std::regex include_pattern_;
    std::regex typedef_pattern_;
};

} // namespace nekocode