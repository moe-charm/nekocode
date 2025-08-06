//=============================================================================
// 🚀 Script Analyzer Base Implementation - 共通処理実装
//=============================================================================

#include "nekocode/analyzers/script_analyzer_base.hpp"
#include <regex>
#include <iostream>

namespace nekocode {

//=============================================================================
// 🧹 前処理システム実装（JavaScript実装から統合移植）
//=============================================================================

std::string ScriptAnalyzerBase::preprocess_content(const std::string& content, std::vector<CommentInfo>* comments) {
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
        
        // 文字列リテラル検出・保護
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
                size_t comment_line_start = line_start;
                
                // コメント終端検索
                while (i < len && data[i] != '\n') {
                    i++;
                }
                
                std::string comment_text = content.substr(comment_start + 2, i - comment_start - 2);
                
                CommentInfo comment_info;
                comment_info.line_number = line_number;
                comment_info.content = comment_text;
                comment_info.type = CommentType::SINGLE_LINE;
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
                
                std::string comment_text = content.substr(comment_start + 2, i - comment_start - 4);
                
                CommentInfo comment_info;
                comment_info.line_number = start_line;
                comment_info.content = comment_text;
                comment_info.type = CommentType::MULTI_LINE;
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

} // namespace nekocode