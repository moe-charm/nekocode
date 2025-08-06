#pragma once

//=============================================================================
// 🎯 Script Postprocessing - JavaScript/TypeScript後処理統合
//
// 重複後処理コードを統合してJavaScript/TypeScriptから300行削減
// 責任: 統計更新、関数終了行計算、メンバ変数検出、結果ログ出力
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
namespace script_postprocessing {

//=============================================================================
// 🎯 ScriptPostprocessor - JS/TS共通後処理クラス
//=============================================================================

class ScriptPostprocessor {
public:
    /// 統合後処理実行
    static void finalize_analysis_result(
        AnalysisResult& result,
        const std::string& content,
        const std::string& filename,
        Language target_language,
        const std::string& language_prefix  // "JS" or "TS"
    ) {
        auto postprocess_start = std::chrono::high_resolution_clock::now();
        
        if (!g_quiet_mode || g_debug_mode) {
            std::cerr << "🔧 [" << language_prefix << "] Starting post-processing..." << std::endl;
        }
        
        // Step 1: ファイル情報設定
        result.file_info.name = filename;
        result.file_info.size_bytes = content.size();
        result.language = target_language;
        
        // Step 2: 関数終了行計算
        calculate_function_end_lines(result, content);
        
        // Step 3: メンバ変数検出（共通パターン）
        detect_member_variables(result, content);
        
        // Step 4: 統計更新
        result.update_statistics();
        
        auto postprocess_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            postprocess_end - postprocess_start).count();
        
        // 統一ログ出力
        if (!g_quiet_mode || g_debug_mode) {
            std::cerr << "✅ [" << language_prefix << "] Post-processing completed: " 
                      << duration << "ms" << std::endl;
            
            // メンバ変数総数計算
            size_t total_member_variables = 0;
            for (const auto& cls : result.classes) {
                total_member_variables += cls.member_variables.size();
            }
            
            std::cerr << "📊 [" << language_prefix << "] Final stats - Functions: " 
                      << result.functions.size() << ", Classes: " << result.classes.size()
                      << ", Member Variables: " << total_member_variables << std::endl;
        }
    }
    
    /// 関数終了行計算（ブレース追跡版）
    static void calculate_function_end_lines(
        AnalysisResult& result,
        const std::string& content
    ) {
        if (result.functions.empty()) {
            return;
        }
        
        // コンテンツを行分割
        std::vector<std::string> content_lines;
        std::istringstream stream(content);
        std::string line;
        while (std::getline(stream, line)) {
            content_lines.push_back(line);
        }
        
        // 各関数の終了行を計算
        for (auto& func : result.functions) {
            if (func.start_line > 0 && func.start_line <= content_lines.size()) {
                func.end_line = find_function_end_line(content_lines, func.start_line - 1);
            }
        }
    }
    
private:
    /// ブレースバランス追跡による関数終了行検出
    static uint32_t find_function_end_line(
        const std::vector<std::string>& lines, 
        size_t start_line
    ) {
        int brace_count = 0;
        bool found_opening_brace = false;
        
        for (size_t i = start_line; i < lines.size(); ++i) {
            const auto& line = lines[i];
            
            for (char c : line) {
                if (c == '{') {
                    brace_count++;
                    found_opening_brace = true;
                } else if (c == '}') {
                    if (brace_count > 0) {
                        brace_count--;
                        if (brace_count == 0 && found_opening_brace) {
                            return static_cast<uint32_t>(i + 1); // 1-based line numbers
                        }
                    }
                }
            }
        }
        
        // 終了ブレースが見つからない場合、ファイル末尾を返す
        return static_cast<uint32_t>(lines.size());
    }
    
    /// JavaScript/TypeScript共通メンバ変数検出
    static void detect_member_variables(AnalysisResult& result, const std::string& content) {
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
    
    /// クラス開始検出
    static void detect_class_start(
        const std::string& line, 
        std::string& current_class,
        size_t& class_brace_depth, 
        size_t current_brace_depth
    ) {
        // export class Pattern | class Pattern 検出（PEGTL風、std::regex不使用）
        size_t pos = 0;
        
        // 先頭の空白をスキップ
        while (pos < line.length() && std::isspace(line[pos])) {
            pos++;
        }
        
        // export をスキップ（オプション）
        if (line.substr(pos, 6) == "export") {
            pos += 6;
            while (pos < line.length() && std::isspace(line[pos])) {
                pos++;
            }
        }
        
        // class キーワードチェック
        if (line.substr(pos, 5) == "class" && 
            (pos + 5 >= line.length() || std::isspace(line[pos + 5]))) {
            pos += 5;
            
            // 空白をスキップ
            while (pos < line.length() && std::isspace(line[pos])) {
                pos++;
            }
            
            // クラス名抽出
            size_t name_start = pos;
            while (pos < line.length() && 
                   (std::isalnum(line[pos]) || line[pos] == '_')) {
                pos++;
            }
            
            if (pos > name_start) {
                current_class = line.substr(name_start, pos - name_start);
                class_brace_depth = current_brace_depth;
            }
        }
    }
    
    /// クラス内メンバ変数検出
    static void detect_member_variable_in_class(
        AnalysisResult& result, 
        const std::string& line,
        const std::string& current_class, 
        size_t line_number
    ) {
        // this.property = value パターン（PEGTL風、std::regex不使用）
        size_t pos = 0;
        
        // 先頭の空白をスキップ
        while (pos < line.length() && std::isspace(line[pos])) {
            pos++;
        }
        
        // "this." チェック
        if (line.substr(pos, 5) == "this." && pos + 5 < line.length()) {
            pos += 5;
            
            // プロパティ名抽出
            size_t name_start = pos;
            while (pos < line.length() && 
                   (std::isalnum(line[pos]) || line[pos] == '_')) {
                pos++;
            }
            
            if (pos > name_start) {
                std::string property_name = line.substr(name_start, pos - name_start);
                
                // = チェック（空白スキップ付き）
                while (pos < line.length() && std::isspace(line[pos])) {
                    pos++;
                }
                
                if (pos < line.length() && line[pos] == '=') {
                    // 該当するクラスを検索してメンバ変数を追加
                    for (auto& cls : result.classes) {
                        if (cls.name == current_class) {
                            // 重複チェック
                            bool already_exists = false;
                            for (const auto& member : cls.member_variables) {
                                if (member.name == property_name) {
                                    already_exists = true;
                                    break;
                                }
                            }
                            
                            if (!already_exists) {
                                MemberVariable member_var;
                                member_var.name = property_name;
                                member_var.type = "any"; // JavaScript/TypeScript動的型付け
                                member_var.declaration_line = static_cast<uint32_t>(line_number);
                                member_var.access_modifier = "public";
                                cls.member_variables.push_back(member_var);
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
};

} // namespace script_postprocessing
} // namespace nekocode