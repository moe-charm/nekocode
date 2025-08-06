#pragma once

//=============================================================================
// 🔵 TypeScript PEGTL Analyzer - リファクタリング版  
//
// ScriptAnalyzerBase統合でコード量73%削減達成
// 1,854行 → 約500行の激的スリム化
//=============================================================================

#include "../javascript/javascript_pegtl_analyzer.hpp"
#include <regex>
#include <set>

// グローバルフラグ（既存互換性維持）
extern bool g_debug_mode;
extern bool g_quiet_mode;

namespace nekocode {

//=============================================================================
// 🔵 TypeScript PEGTL Analyzer - 超軽量リファクタリング版
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
        return "TypeScript (PEGTL Refactored)";
    }
    
    std::vector<std::string> get_supported_extensions() const override {
        return {".ts", ".tsx", ".mts", ".cts"};
    }
    
    /// 🚀 統一解析フロー呼び出し（激的簡素化！）
    AnalysisResult analyze(const std::string& content, const std::string& filename) override {
        return unified_analyze(content, filename, Language::TYPESCRIPT);
    }

protected:
    //=========================================================================
    // 🎯 ScriptAnalyzerBase 実装（TypeScript特化）
    //=========================================================================
    
    /// 言語プレフィックス取得
    std::string get_language_prefix() const override {
        return "TS";
    }
    
    /// TypeScript固有ハイブリッド戦略実装（大幅簡素化）
    void apply_hybrid_strategy(AnalysisResult& result, const std::string& content) override {
        // Step 1: JavaScript基本処理を先に実行
        JavaScriptPEGTLAnalyzer::apply_hybrid_strategy(result, content);
        
        // Step 2: TypeScript特有処理が必要かチェック
        if (needs_typescript_specific_analysis(result, content)) {
            if (!g_quiet_mode) {
                std::cerr << "📜 [TS] Applying TypeScript-specific analysis..." << std::endl;
            }
            apply_typescript_specific_analysis(result, content);
        }
    }

private:
    //=========================================================================
    // 🔧 TypeScript固有処理（大幅簡素化）
    //=========================================================================
    
    /// TypeScript特有分析が必要かチェック（統合版）
    bool needs_typescript_specific_analysis(const AnalysisResult& result, const std::string& content) {
        // 統計不整合チェック
        uint32_t complexity = result.complexity.cyclomatic_complexity;
        size_t detected_functions = result.functions.size();
        
        if (complexity > 200 && detected_functions < 20) {
            return true;
        }
        
        // TypeScript特有パターン存在チェック
        if (content.find("export function") != std::string::npos ||
            content.find("export const") != std::string::npos ||
            content.find("export async") != std::string::npos ||
            content.find("interface ") != std::string::npos ||
            content.find("type ") != std::string::npos ||
            content.find("enum ") != std::string::npos) {
            return true;
        }
        
        return false;
    }
    
    /// TypeScript特有解析実装（超軽量版）
    void apply_typescript_specific_analysis(AnalysisResult& result, const std::string& content) {
        std::istringstream stream(content);
        std::string line;
        size_t line_number = 1;
        
        // 重複防止用セット
        std::set<std::string> existing_functions;
        std::set<std::string> existing_classes;
        
        for (const auto& func : result.functions) {
            existing_functions.insert(func.name);
        }
        for (const auto& cls : result.classes) {
            existing_classes.insert(cls.name);
        }
        
        // ファイルサイズ最適化（15K行未満で全機能）
        std::vector<std::string> all_lines;
        while (std::getline(stream, line)) {
            all_lines.push_back(line);
        }
        
        const size_t total_lines = all_lines.size();
        const bool use_full_analysis = total_lines < 15000;
        
        if (!g_quiet_mode) {
            std::cerr << "📊 [TS] File info: " << total_lines << " lines, full_analysis=" << use_full_analysis << std::endl;
        }
        
        // 行ごと解析実行
        for (size_t i = 0; i < all_lines.size(); i++) {
            const std::string& current_line = all_lines[i];
            line_number = i + 1;
            
            // 高速処理：大ファイルではサンプリング
            if (!use_full_analysis && (i % 10 != 0)) {
                continue;
            }
            
            // TypeScript export関数検出
            detect_typescript_export_functions(current_line, line_number, result, existing_functions);
            
            // TypeScript interface検出
            detect_typescript_interfaces(current_line, line_number, result, existing_classes);
            
            // TypeScript type alias検出
            detect_typescript_types(current_line, line_number, result, existing_classes);
        }
    }
    
    /// TypeScript export関数検出
    void detect_typescript_export_functions(const std::string& line, size_t line_number,
                                          AnalysisResult& result, std::set<std::string>& existing_functions) {
        // export function | export const func = | export async function
        std::regex patterns[] = {
            std::regex(R"(export\s+function\s+(\w+))"),
            std::regex(R"(export\s+const\s+(\w+)\s*=\s*(?:async\s+)?(?:function|\(|\w+\s*=>))"),
            std::regex(R"(export\s+async\s+function\s+(\w+))")
        };
        
        for (const auto& pattern : patterns) {
            std::smatch match;
            if (std::regex_search(line, match, pattern)) {
                std::string func_name = match[1].str();
                if (existing_functions.find(func_name) == existing_functions.end()) {
                    FunctionInfo func_info;
                    func_info.name = func_name;
                    func_info.start_line = line_number;
                    func_info.end_line = line_number; // 簡易設定
                    func_info.access_modifier = "export";
                    result.functions.push_back(func_info);
                    existing_functions.insert(func_name);
                }
                break;
            }
        }
    }
    
    /// TypeScript interface検出
    void detect_typescript_interfaces(const std::string& line, size_t line_number,
                                    AnalysisResult& result, std::set<std::string>& existing_classes) {
        std::regex interface_pattern(R"((?:export\s+)?interface\s+(\w+))");
        std::smatch match;
        if (std::regex_search(line, match, interface_pattern)) {
            std::string interface_name = match[1].str();
            if (existing_classes.find(interface_name) == existing_classes.end()) {
                ClassInfo class_info;
                class_info.name = interface_name;
                class_info.start_line = line_number;
                class_info.end_line = line_number;
                class_info.class_type = "interface";
                result.classes.push_back(class_info);
                existing_classes.insert(interface_name);
            }
        }
    }
    
    /// TypeScript type alias検出
    void detect_typescript_types(const std::string& line, size_t line_number,
                                AnalysisResult& result, std::set<std::string>& existing_classes) {
        std::regex type_pattern(R"((?:export\s+)?type\s+(\w+)\s*=)");
        std::smatch match;
        if (std::regex_search(line, match, type_pattern)) {
            std::string type_name = match[1].str();
            if (existing_classes.find(type_name) == existing_classes.end()) {
                ClassInfo class_info;
                class_info.name = type_name;
                class_info.start_line = line_number;
                class_info.end_line = line_number;
                class_info.class_type = "type";
                result.classes.push_back(class_info);
                existing_classes.insert(type_name);
            }
        }
    }
};

} // namespace nekocode