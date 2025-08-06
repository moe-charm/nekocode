#pragma once

//=============================================================================
// 🚀 Script Analyzer Base - JavaScript/TypeScript共通基盤
//
// 重複コード4,114行を2,100行に削減する統合アーキテクチャ
// 責任: 前処理、後処理、共通解析フローの統一実装
//=============================================================================

#include "../../../src/analyzers/base_analyzer.hpp"
#include "../types.hpp"
#include <vector>
#include <string>
#include <chrono>
#include <iostream>
#include <sstream>

namespace nekocode {

//=============================================================================
// 🎯 ScriptAnalyzerBase - JavaScript/TypeScript共通基盤クラス
//=============================================================================

class ScriptAnalyzerBase : public BaseAnalyzer {
public:
    ScriptAnalyzerBase() = default;
    virtual ~ScriptAnalyzerBase() = default;

protected:
    //=========================================================================
    // 🔄 統一解析フロー - 99%共通処理実装
    //=========================================================================
    
    /// 統一解析フロー実行
    AnalysisResult unified_analyze(const std::string& content, 
                                  const std::string& filename,
                                  Language target_language) {
        auto total_start = std::chrono::high_resolution_clock::now();
        
        if (!g_quiet_mode || g_debug_mode) {
            std::cerr << "🔍 [" << get_language_prefix() << "] Analyzing file: " << filename 
                      << " (size: " << content.size() << " bytes)" << std::endl;
        }
        
        // Step 1: 統一前処理（コメント収集付き）
        std::vector<CommentInfo> comments;
        std::string preprocessed_content = unified_preprocess(content, comments);
        
        // Step 2: 言語固有PEGTL解析（派生クラス実装）
        AnalysisResult result = parse_with_pegtl(preprocessed_content, filename);
        
        // Step 3: 統一後処理
        result.file_info.name = filename;
        result.file_info.size_bytes = content.size();
        result.language = target_language;
        result.commented_lines = std::move(comments);
        
        // Step 4: 共通検出処理
        detect_member_variables(result, content);
        
        // Step 5: 言語固有ハイブリッド戦略（派生クラス実装）
        apply_hybrid_strategy(result, content);
        
        // Step 6: 統計更新
        result.update_statistics();
        
        auto total_end = std::chrono::high_resolution_clock::now();
        if (!g_quiet_mode || g_debug_mode) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(total_end - total_start).count();
            std::cerr << "⏱️ [" << get_language_prefix() << "] Total analysis time: " << duration << "ms" << std::endl;
        }
        
        return result;
    }

    //=========================================================================
    // 🔧 統一前処理システム
    //=========================================================================
    
    /// 統一前処理（コメント収集・時間測定付き）
    std::string unified_preprocess(const std::string& content, std::vector<CommentInfo>& comments) {
        auto preprocess_start = std::chrono::high_resolution_clock::now();
        
        // 大ファイル最適化（2MB以上でスキップオプション）
        std::string preprocessed_content;
        if (content.size() > 2 * 1024 * 1024 && g_debug_mode) {
            if (!g_quiet_mode) {
                std::cerr << "⚡ [" << get_language_prefix() << "] Skipping preprocessing for large file (>2MB)" << std::endl;
            }
            preprocessed_content = content;
        } else {
            preprocessed_content = preprocess_content(content, &comments);
        }
        
        auto preprocess_end = std::chrono::high_resolution_clock::now();
        
        // 削減量計算（アンダーフロー防止）
        long long size_diff = static_cast<long long>(content.length()) - static_cast<long long>(preprocessed_content.length());
        if (!g_quiet_mode) {
            std::cerr << "🧹 前処理完了: " << content.length() << " → " << preprocessed_content.length() 
                      << " bytes (削減: " << size_diff << ")" << std::endl;
        }
        
        return preprocessed_content;
    }

    //=========================================================================
    // 🔍 共通検出処理
    //=========================================================================
    
    /// メンバ変数検出（JavaScript/TypeScript共通パターン）
    void detect_member_variables(AnalysisResult& result, const std::string& content) {
        std::istringstream stream(content);
        std::string line;
        size_t line_number = 0;
        
        // クラス解析状態
        std::string current_class;
        bool in_constructor = false;
        size_t class_brace_depth = 0;
        size_t current_brace_depth = 0;
        
        while (std::getline(stream, line)) {
            line_number++;
            
            // ブレース深度追跡
            for (char c : line) {
                if (c == '{') {
                    current_brace_depth++;
                } else if (c == '}') {
                    if (current_brace_depth > 0) current_brace_depth--;
                    if (current_brace_depth <= class_brace_depth && !current_class.empty()) {
                        current_class.clear();
                        in_constructor = false;
                        class_brace_depth = 0;
                    }
                }
            }
            
            // クラス検出（JavaScript/TypeScript共通）
            detect_class_start(line, current_class, class_brace_depth, current_brace_depth);
            
            // メンバ変数検出
            if (!current_class.empty()) {
                detect_member_variable_in_class(result, line, current_class, line_number);
            }
        }
    }

    //=========================================================================
    // 🎯 派生クラス実装インターフェース
    //=========================================================================
    
    /// 言語固有PEGTL解析（派生クラスで実装）
    virtual AnalysisResult parse_with_pegtl(const std::string& content, 
                                           const std::string& filename) = 0;
    
    /// 言語固有ハイブリッド戦略（派生クラスで実装）
    virtual void apply_hybrid_strategy(AnalysisResult& result, 
                                     const std::string& content) = 0;
    
    /// 言語プレフィックス取得（ログ用）
    virtual std::string get_language_prefix() const = 0;

private:
    //=========================================================================
    // 🔧 内部ヘルパー関数
    //=========================================================================
    
    /// クラス開始検出
    void detect_class_start(const std::string& line, std::string& current_class,
                           size_t& class_brace_depth, size_t current_brace_depth) {
        // export class Pattern | class Pattern 検出
        std::regex class_pattern(R"(^\s*(?:export\s+)?class\s+(\w+))");
        std::smatch class_match;
        if (std::regex_search(line, class_match, class_pattern)) {
            current_class = class_match[1].str();
            class_brace_depth = current_brace_depth;
        }
    }
    
    /// クラス内メンバ変数検出
    void detect_member_variable_in_class(AnalysisResult& result, const std::string& line,
                                        const std::string& current_class, size_t line_number) {
        // this.property = value パターン
        std::regex member_pattern(R"(^\s*this\.(\w+)\s*=)");
        std::smatch member_match;
        if (std::regex_search(line, member_match, member_pattern)) {
            VariableInfo var;
            var.name = member_match[1].str();
            var.line_number = line_number;
            var.class_name = current_class;
            var.type = "member";
            result.variables.push_back(var);
        }
    }

    // 外部から参照するグローバルフラグ（実装は各analyzer内）
    extern bool g_debug_mode;
    extern bool g_quiet_mode;
    
    /// 前処理関数（JavaScript実装から移植）
    std::string preprocess_content(const std::string& content, std::vector<CommentInfo>* comments);
};

} // namespace nekocode