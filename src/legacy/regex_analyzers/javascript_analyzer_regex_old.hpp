#pragma once

//=============================================================================
// 🌟 JavaScript Language Analyzer - JavaScript専用解析エンジン
//
// JavaScriptコードの構造解析・複雑度計算
// ES6+対応、クラス、関数、import/export検出
//=============================================================================

#include "base_analyzer.hpp"
#include <regex>
#include <vector>

namespace nekocode {

//=============================================================================
// 🌟 JavaScriptAnalyzer - JavaScript専用解析クラス
//=============================================================================

class JavaScriptAnalyzer : public BaseAnalyzer {
public:
    JavaScriptAnalyzer();
    ~JavaScriptAnalyzer() override = default;
    
    //=========================================================================
    // 🔍 BaseAnalyzer インターフェース実装
    //=========================================================================
    
    Language get_language() const override;
    std::string get_language_name() const override;
    std::vector<std::string> get_supported_extensions() const override;
    AnalysisResult analyze(const std::string& content, const std::string& filename) override;
    
protected:
    //=========================================================================
    // 🔧 内部実装
    //=========================================================================
    
    /// 正規表現パターン初期化
    virtual void initialize_patterns();
    
    /// クラス抽出（ES6クラス + プロトタイプ）
    void extract_classes(const std::string& content, AnalysisResult& result);
    
    /// 関数抽出（通常関数、アロー関数、async関数）
    void extract_functions(const std::string& content, AnalysisResult& result);
    
    /// import文抽出
    void extract_imports(const std::string& content, AnalysisResult& result);
    
    /// export文抽出
    void extract_exports(const std::string& content, AnalysisResult& result);
    
    /// 関数呼び出し抽出
    void extract_function_calls(const std::string& content, AnalysisResult& result);
    
    /// JavaScript特化複雑度計算
    ComplexityInfo calculate_javascript_complexity(const std::string& content);
    
    //=========================================================================
    // 📊 メンバ変数
    //=========================================================================
    
    // クラスパターン
    std::regex es6_class_pattern_;
    std::regex prototype_pattern_;
    
    // 関数パターン
    std::regex function_pattern_;
    std::regex arrow_function_pattern_;
    std::regex method_pattern_;
    
    // import/exportパターン
    std::vector<std::regex> import_patterns_;
    std::vector<std::regex> export_patterns_;
    
    // その他
    std::regex function_call_pattern_;
};

//=============================================================================
// 🔷 TypeScriptAnalyzer - TypeScript専用解析クラス
//=============================================================================

class TypeScriptAnalyzer : public JavaScriptAnalyzer {
public:
    TypeScriptAnalyzer();
    ~TypeScriptAnalyzer() override = default;
    
    Language get_language() const override;
    std::string get_language_name() const override;
    std::vector<std::string> get_supported_extensions() const override;
    
protected:
    /// TypeScript固有パターン追加
    void initialize_patterns() override;
    
private:
    // TypeScript固有パターン
    std::regex interface_pattern_;
    std::regex type_alias_pattern_;
    std::regex enum_pattern_;
};

} // namespace nekocode