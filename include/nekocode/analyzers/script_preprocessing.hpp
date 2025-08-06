#pragma once

//=============================================================================
// 🔧 Script Preprocessing - JavaScript/TypeScript前処理統合
//
// 重複前処理コードを統合してTypeScriptから200行削減
// 責任: コメント除去、文字列リテラル保護、統計測定
//=============================================================================

#include "../types.hpp"
#include <string>
#include <vector>
#include <chrono>
#include <iostream>

// グローバルフラグ（外部定義）
extern bool g_debug_mode;
extern bool g_quiet_mode;

namespace nekocode {
namespace script_preprocessing {

//=============================================================================
// 🎯 ScriptPreprocessor - JS/TS共通前処理クラス
//=============================================================================

class ScriptPreprocessor {
public:
    /// 前処理結果構造体
    struct PreprocessResult {
        std::string content;                 // 処理済みコンテンツ
        std::vector<CommentInfo> comments;   // 抽出されたコメント情報
        size_t bytes_reduced;                // 削減されたバイト数
        long long processing_time_ms;       // 処理時間(ミリ秒)
    };
    
    /// JavaScript/TypeScript共通前処理実行
    static PreprocessResult preprocess_script_content(
        const std::string& original_content,
        const std::string& language_prefix,  // "JS" or "TS"
        bool enable_debug_timing = false,
        bool large_file_skip_threshold = true  // 2MB以上でスキップ
    ) {
        auto processing_start = std::chrono::high_resolution_clock::now();
        
        PreprocessResult result;
        result.processing_time_ms = 0;
        result.bytes_reduced = 0;
        
        // ファイルサイズログ出力
        if (!g_quiet_mode || g_debug_mode) {
            std::cerr << "🔍 [" << language_prefix << "] Starting preprocessing: " 
                      << original_content.size() << " bytes" << std::endl;
        }
        
        // 大ファイル最適化（2MB以上でスキップオプション）
        const size_t large_file_threshold = 2 * 1024 * 1024; // 2MB
        if (large_file_skip_threshold && 
            original_content.size() > large_file_threshold && g_debug_mode) {
            
            if (!g_quiet_mode) {
                std::cerr << "⚡ [" << language_prefix 
                          << "] Skipping preprocessing for large file (>2MB)" << std::endl;
            }
            
            // 前処理をスキップ
            result.content = original_content;
            result.bytes_reduced = 0;
            // コメント配列は空のまま
        } else {
            // 通常の前処理実行
            result.content = preprocess_content(original_content, &result.comments);
            result.bytes_reduced = original_content.length() - result.content.length();
        }
        
        auto processing_end = std::chrono::high_resolution_clock::now();
        result.processing_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            processing_end - processing_start).count();
        
        // 統一ログ出力
        if (!g_quiet_mode) {
            // 安全な削減量計算（アンダーフロー防止）
            long long safe_reduction = static_cast<long long>(original_content.length()) - 
                                      static_cast<long long>(result.content.length());
            
            std::cerr << "🧹 [" << language_prefix << "] 前処理完了: " 
                      << original_content.length() << " → " << result.content.length() 
                      << " bytes (削減: " << safe_reduction << ")" << std::endl;
            
            if (enable_debug_timing || g_debug_mode) {
                std::cerr << "⏱️ [" << language_prefix << "] 前処理時間: " 
                          << result.processing_time_ms << "ms" << std::endl;
            }
        }
        
        return result;
    }

private:
    /// コメント・文字列リテラル除去処理（既存実装から移植）
    static std::string preprocess_content(const std::string& content, 
                                         std::vector<CommentInfo>* comments) {
        std::string result;
        result.reserve(content.size());
        
        const char* data = content.data();
        size_t len = content.length();
        size_t i = 0;
        size_t line_number = 1;
        size_t line_start = 0;
        
        while (i < len) {
            char c = data[i];
            
            // 改行文字処理
            if (c == '\n') {
                result += c;
                line_number++;
                line_start = i + 1;
                i++;
                continue;
            }
            
            // 文字列リテラル検出・保護（`, ", ' 対応）
            if (c == '"' || c == '\'' || c == '`') {
                char quote = c;
                result += c;
                i++;
                
                // 文字列終端まで保護
                while (i < len) {
                    char str_c = data[i];
                    result += str_c;
                    
                    if (str_c == quote) {
                        i++;
                        break;
                    } else if (str_c == '\\' && i + 1 < len) {
                        // エスケープ文字処理
                        i++;
                        if (i < len) {
                            result += data[i];
                            i++;
                        }
                    } else if (str_c == '\n') {
                        line_number++;
                        line_start = i + 1;
                        i++;
                    } else {
                        i++;
                    }
                }
                continue;
            }
            
            // 単行コメント検出
            if (c == '/' && i + 1 < len && data[i + 1] == '/') {
                if (comments) {
                    // コメント情報収集
                    size_t comment_start = i;
                    
                    // コメント終端検索
                    while (i < len && data[i] != '\n') {
                        i++;
                    }
                    
                    std::string comment_text = content.substr(comment_start + 2, 
                                                            i - comment_start - 2);
                    
                    CommentInfo comment_info;
                    comment_info.line_start = line_number;
                    comment_info.line_end = line_number;
                    comment_info.content = comment_text;
                    comment_info.type = "single_line";
                    comments->push_back(comment_info);
                } else {
                    // コメントをスペースに置換
                    while (i < len && data[i] != '\n') {
                        result += ' ';
                        i++;
                    }
                }
                continue;
            }
            
            // 複数行コメント検出
            if (c == '/' && i + 1 < len && data[i + 1] == '*') {
                if (comments) {
                    // コメント情報収集
                    size_t comment_start = i;
                    size_t start_line = line_number;
                    
                    i += 2; // /* をスキップ
                    
                    // */ 検索
                    while (i + 1 < len) {
                        if (data[i] == '*' && data[i + 1] == '/') {
                            i += 2; // */ をスキップ
                            break;
                        }
                        if (data[i] == '\n') {
                            line_number++;
                            line_start = i + 1;
                        }
                        i++;
                    }
                    
                    std::string comment_text = content.substr(comment_start + 2, 
                                                            i - comment_start - 4);
                    
                    CommentInfo comment_info;
                    comment_info.line_start = start_line;
                    comment_info.line_end = line_number;
                    comment_info.content = comment_text;
                    comment_info.type = "multi_line";
                    comments->push_back(comment_info);
                } else {
                    // コメントをスペースに置換
                    i += 2; // /* をスキップ
                    result += "  ";
                    
                    while (i + 1 < len) {
                        if (data[i] == '*' && data[i + 1] == '/') {
                            result += "  ";
                            i += 2;
                            break;
                        }
                        if (data[i] == '\n') {
                            result += '\n';
                            line_number++;
                            line_start = i + 1;
                        } else {
                            result += ' ';
                        }
                        i++;
                    }
                }
                continue;
            }
            
            // 通常文字
            result += c;
            i++;
        }
        
        return result;
    }
};

} // namespace script_preprocessing
} // namespace nekocode