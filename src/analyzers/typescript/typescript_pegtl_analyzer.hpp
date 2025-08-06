#pragma once

//=============================================================================
// 🔵 TypeScript PEGTL Analyzer - JavaScript拡張＋型システム対応版
//
// JavaScript PEGTL成功パターンを拡張
// interface, type, enum, namespace, generics等の検出
//=============================================================================

#include "../javascript/javascript_pegtl_analyzer.hpp"
#include "nekocode/analyzers/script_preprocessing.hpp"
#include "nekocode/analyzers/script_postprocessing.hpp"
#include "nekocode/analyzers/script_detection_helpers.hpp"
#include <regex>
#include <sstream>
#include <set>
#include <chrono>
#include <execution>
#include <mutex>
#include <atomic>
#include <iomanip>

// 🔧 グローバルフラグ（ユーザー制御可能）
extern bool g_debug_mode;
extern bool g_quiet_mode;

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
        // 🕐 全体の処理時間測定開始
        auto total_start = std::chrono::high_resolution_clock::now();
        
        // 🐛 デバッグ用: ファイルサイズログ
        if (!g_quiet_mode || g_debug_mode) {
            std::cerr << "🔍 [TS] Analyzing file: " << filename 
                      << " (size: " << content.size() << " bytes)" << std::endl;
        }
        
        // 🚀 統一前処理システム使用（重複削除済み）
        auto preprocess_result = script_preprocessing::ScriptPreprocessor::preprocess_script_content(
            content, "TS", g_debug_mode
        );
        std::string preprocessed_content = preprocess_result.content;
        
        // 基本的にJavaScript PEGTLの解析を使用（ハイブリッド戦略含む）
        auto js_start = std::chrono::high_resolution_clock::now();
        auto result = JavaScriptPEGTLAnalyzer::analyze(preprocessed_content, filename);
        auto js_end = std::chrono::high_resolution_clock::now();
        
        if (!g_quiet_mode || g_debug_mode) {
            auto js_duration = std::chrono::duration_cast<std::chrono::milliseconds>(js_end - js_start).count();
            std::cerr << "⏱️ [TS] JavaScript base analysis took: " << js_duration << "ms" << std::endl;
        }
        
        if (!g_quiet_mode) {
            std::cerr << "📜 TypeScript analyzer: Base JS detected classes=" << result.classes.size() 
                      << ", functions=" << result.functions.size() << std::endl;
        }
        
        // 🔥 コメントアウト行情報を結果に追加（統一前処理から取得）
        result.commented_lines = std::move(preprocess_result.comments);
        // std::cerr << "🔥 Comments added to result: " << result.commented_lines.size() << " items" << std::endl;
        
        // 🚀 TypeScript特有のハイブリッド戦略追加（統一検出システム使用）
        auto ts_start = std::chrono::high_resolution_clock::now();
        apply_typescript_unified_detection(result, preprocessed_content, filename);
        auto ts_end = std::chrono::high_resolution_clock::now();
        
        if (!g_quiet_mode || g_debug_mode) {
            auto ts_duration = std::chrono::duration_cast<std::chrono::milliseconds>(ts_end - ts_start).count();
            std::cerr << "⏱️ [TS] TypeScript unified detection took: " << ts_duration << "ms" << std::endl;
        }
        if (needs_typescript_specific_analysis(result, preprocessed_content)) {
            if (!g_quiet_mode) {
                std::cerr << "📜 TypeScript specific analysis triggered (unified detection)!" << std::endl;
            }
            auto ts_specific_start = std::chrono::high_resolution_clock::now();
            apply_typescript_unified_detection(result, preprocessed_content, filename);
            auto ts_specific_end = std::chrono::high_resolution_clock::now();
            
            if (!g_quiet_mode || g_debug_mode) {
                auto ts_duration = std::chrono::duration_cast<std::chrono::milliseconds>(ts_specific_end - ts_specific_start).count();
                std::cerr << "⏱️ [TS] TypeScript unified detection took: " << ts_duration << "ms" << std::endl;
            }
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
        
        // 🎯 統一後処理実行（メンバ変数検出・統計更新・ログ出力統合）
        script_postprocessing::ScriptPostprocessor::finalize_analysis_result(
            result, content, filename, Language::TYPESCRIPT, "TS"
        );
        
        auto total_end = std::chrono::high_resolution_clock::now();
        auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(total_end - total_start).count();
        if (!g_quiet_mode || g_debug_mode) {
            std::cerr << "⏱️ [TS] Total analysis time: " << total_duration << "ms" << std::endl;
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
    
private:
    /// 統一検出システムを使ったTypeScript解析（Phase 4統合）
    void apply_typescript_unified_detection(AnalysisResult& result, const std::string& content, const std::string& filename) {
        using namespace script_detection;
        
        // 既存の関数/クラス名セットを構築
        auto existing_names = ScriptDetectionHelpers::build_existing_names_set(result.functions, result.classes);
        
        // 統一システムで基本検出を実行
        auto export_functions = ScriptDetectionHelpers::detect_export_functions(content, existing_names);
        auto basic_functions = ScriptDetectionHelpers::detect_basic_functions(content, existing_names);
        auto classes = ScriptDetectionHelpers::detect_classes(content, existing_names);
        
        // TypeScript固有検出
        auto interfaces = ScriptDetectionHelpers::detect_typescript_interfaces(content, existing_names);
        auto type_aliases = ScriptDetectionHelpers::detect_typescript_type_aliases(content, existing_names);
        
        // 結果をマージ
        result.functions.insert(result.functions.end(), export_functions.begin(), export_functions.end());
        result.functions.insert(result.functions.end(), basic_functions.begin(), basic_functions.end());
        result.classes.insert(result.classes.end(), classes.begin(), classes.end());
        result.classes.insert(result.classes.end(), interfaces.begin(), interfaces.end());
        
        if (!g_quiet_mode && !type_aliases.empty()) {
            std::cerr << "🎯 TypeScript type aliases detected: " << type_aliases.size() << std::endl;
        }
    }
};

} // namespace nekocode
