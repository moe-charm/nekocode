#pragma once

//=============================================================================
// 🔵 TypeScript PEGTL Analyzer - JavaScript拡張＋型システム対応版
//
// JavaScript PEGTL成功パターンを拡張
// interface, type, enum, namespace, generics等の検出
//=============================================================================

#include "javascript_pegtl_analyzer.hpp"
#include <regex>
#include <sstream>
#include <set>

namespace nekocode {

//=============================================================================
// 🔵 TypeScriptPEGTLAnalyzer - TypeScript専用解析クラス
//=============================================================================

class TypeScriptPEGTLAnalyzer : public JavaScriptPEGTLAnalyzer {
public:
    TypeScriptPEGTLAnalyzer() = default;
    ~TypeScriptPEGTLAnalyzer() override = default;
    
    //=========================================================================
    // 🔍 BaseAnalyzer インターフェース実装（オーバーライド）
    //=========================================================================
    
    Language get_language() const override {
        return Language::TYPESCRIPT;
    }
    
    std::string get_language_name() const override {
        return "TypeScript (PEGTL)";
    }
    
    std::vector<std::string> get_supported_extensions() const override {
        return {".ts", ".tsx", ".mts", ".cts"};
    }
    
    AnalysisResult analyze(const std::string& content, const std::string& filename) override {
        // 基本的にJavaScript PEGTLの解析を使用（ハイブリッド戦略含む）
        auto result = JavaScriptPEGTLAnalyzer::analyze(content, filename);
        
        // 🚀 TypeScript特有のハイブリッド戦略追加
        if (needs_typescript_specific_analysis(result, content)) {
            apply_typescript_line_based_analysis(result, content, filename);
        }
        
        // TypeScript専用の追加解析（将来的に実装）
        // - interface検出
        // - type alias検出  
        // - enum検出
        // - namespace検出
        // - ジェネリクス解析
        
        // デバッグ用: TypeScript検出マーカー
        if (!result.classes.empty() && result.classes[0].name == "JS_PEGTL_ANALYZER_CALLED") {
            result.classes[0].name = "TS_PEGTL_ANALYZER_CALLED";
        }
        
        return result;
    }

private:
    // 🚀 TypeScript特有のハイブリッド戦略: 統計整合性チェック
    bool needs_typescript_specific_analysis(const AnalysisResult& result, const std::string& content) {
        uint32_t complexity = result.complexity.cyclomatic_complexity;
        size_t detected_functions = result.functions.size();
        
        // TypeScript大規模ファイルの特別チェック
        if (complexity > 200 && detected_functions < 20) {
            return true;
        }
        
        // TypeScript特有のパターンがある場合
        if (content.find("export function") != std::string::npos ||
            content.find("export const") != std::string::npos ||
            content.find("export async") != std::string::npos) {
            return true;
        }
        
        return false;
    }
    
    // 🚀 TypeScript特有の行ベース解析
    void apply_typescript_line_based_analysis(AnalysisResult& result, const std::string& content, const std::string& filename) {
        std::istringstream stream(content);
        std::string line;
        size_t line_number = 1;
        
        // 既存の関数名を記録（重複検出を防ぐ）
        std::set<std::string> existing_functions;
        for (const auto& func : result.functions) {
            existing_functions.insert(func.name);
        }
        
        // TypeScript特有パターンで行ベース解析
        while (std::getline(stream, line)) {
            extract_typescript_functions_from_line(line, line_number, result, existing_functions);
            line_number++;
        }
    }
    
    // TypeScript関数パターンの抽出
    void extract_typescript_functions_from_line(const std::string& line, size_t line_number,
                                                AnalysisResult& result, std::set<std::string>& existing_functions) {
        
        // パターン1: export function name<T>(
        std::regex export_function_pattern(R"(^\s*export\s+function\s+(\w+)(?:<[^>]*>)?\s*\()");
        std::smatch match;
        
        if (std::regex_search(line, match, export_function_pattern)) {
            std::string func_name = match[1].str();
            if (existing_functions.find(func_name) == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = func_name;
                func_info.start_line = line_number;
                // func_info.is_exported = true;
                // func_info.is_typescript_detected = true;
                result.functions.push_back(func_info);
                existing_functions.insert(func_name);
            }
        }
        
        // パターン2: export const name = (
        std::regex export_const_pattern(R"(^\s*export\s+const\s+(\w+)\s*=\s*(?:async\s*)?\([^)]*\)\s*=>)");
        if (std::regex_search(line, match, export_const_pattern)) {
            std::string func_name = match[1].str();
            if (existing_functions.find(func_name) == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = func_name;
                func_info.start_line = line_number;
                func_info.is_arrow_function = true;
                // func_info.is_exported = true;
                // func_info.is_typescript_detected = true;
                result.functions.push_back(func_info);
                existing_functions.insert(func_name);
            }
        }
        
        // パターン3: export async function name(
        std::regex export_async_pattern(R"(^\s*export\s+async\s+function\s+(\w+)(?:<[^>]*>)?\s*\()");
        if (std::regex_search(line, match, export_async_pattern)) {
            std::string func_name = match[1].str();
            if (existing_functions.find(func_name) == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = func_name;
                func_info.start_line = line_number;
                func_info.is_async = true;
                // func_info.is_exported = true;
                // func_info.is_typescript_detected = true;
                result.functions.push_back(func_info);
                existing_functions.insert(func_name);
            }
        }
    }
};

} // namespace nekocode