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
#include <chrono>
#include <execution>
#include <mutex>
#include <atomic>
#include <iomanip>

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
        
        // 🔥 前処理革命：コメント・文字列除去システム（Gemini先生戦略！）
        auto preprocess_start = std::chrono::high_resolution_clock::now();
        std::string preprocessed_content = preprocess_content(content);
        auto preprocess_end = std::chrono::high_resolution_clock::now();
        
        // 安全な削減量計算（アンダーフロー防止）
        long long size_diff = static_cast<long long>(content.length()) - static_cast<long long>(preprocessed_content.length());
        std::cerr << "🧹 前処理完了: " << content.length() << " → " << preprocessed_content.length() 
                  << " bytes (削減: " << size_diff << ")" << std::endl;
        
        // 基本的にJavaScript PEGTLの解析を使用（ハイブリッド戦略含む）
        auto result = JavaScriptPEGTLAnalyzer::analyze(preprocessed_content, filename);
        
        std::cerr << "📜 TypeScript analyzer: Base JS detected classes=" << result.classes.size() 
                  << ", functions=" << result.functions.size() << std::endl;
        
        // 🚀 TypeScript特有のハイブリッド戦略追加
        if (needs_typescript_specific_analysis(result, preprocessed_content)) {
            std::cerr << "📜 TypeScript specific analysis triggered!" << std::endl;
            apply_typescript_line_based_analysis(result, preprocessed_content, filename);
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
    
    // 🚀 TypeScript特有の二重正規表現アタック解析
    void apply_typescript_line_based_analysis(AnalysisResult& result, const std::string& content, const std::string& filename) {
        std::istringstream stream(content);
        std::string line;
        size_t line_number = 1;
        
        // 既存の関数名を記録（重複検出を防ぐ）
        std::set<std::string> existing_functions;
        for (const auto& func : result.functions) {
            existing_functions.insert(func.name);
        }
        
        // 既存のクラス名を記録（重複検出を防ぐ）
        std::set<std::string> existing_classes;
        for (const auto& cls : result.classes) {
            existing_classes.insert(cls.name);
        }
        
        // 🎯 ファイルサイズ検出と戦略決定
        std::vector<std::string> all_lines;
        while (std::getline(stream, line)) {
            all_lines.push_back(line);
        }
        
        const size_t total_lines = all_lines.size();
        const bool use_gemini_first_pass = total_lines < 20000;  // 20000行未満でのみGemini実行
        const bool use_sampling_mode = total_lines >= 20000 && total_lines < 50000;  // サンプリングモード
        
        std::cerr << "📊 ファイル情報: " << total_lines << "行検出" << std::endl;
        
        // 第1段階: 行ベース解析（最適化版）
        size_t gemini_processed_lines = 0;
        auto gemini_start = std::chrono::high_resolution_clock::now();
        
        if (use_gemini_first_pass) {
            std::cerr << "🚀 通常モード: 全機能有効" << std::endl;
            // 通常モード：全行処理
            for (size_t i = 0; i < all_lines.size(); i++) {
                const std::string& current_line = all_lines[i];
                size_t current_line_number = i + 1;
                
                extract_typescript_functions_from_line(current_line, current_line_number, result, existing_functions);
                extract_typescript_classes_from_line(current_line, current_line_number, result, existing_classes);
                extract_typescript_interfaces_from_line(current_line, current_line_number, result, existing_classes);
                
                // 🚀 にゃー先生天才アイデア：行レベル二重アタック！
                gemini_line_level_double_attack(current_line, current_line_number, result, existing_functions);
                
                gemini_processed_lines++;
            }
        } else if (use_sampling_mode) {
            std::cerr << "🎲 サンプリングモード: 10行に1行処理" << std::endl;
            // サンプリングモード：10行に1行だけ処理
            for (size_t i = 0; i < all_lines.size(); i += 10) {
                const std::string& current_line = all_lines[i];
                size_t current_line_number = i + 1;
                
                extract_typescript_functions_from_line(current_line, current_line_number, result, existing_functions);
                extract_typescript_classes_from_line(current_line, current_line_number, result, existing_classes);
                extract_typescript_interfaces_from_line(current_line, current_line_number, result, existing_classes);
                
                // 🚀 にゃー先生天才アイデア：行レベル二重アタック！
                gemini_line_level_double_attack(current_line, current_line_number, result, existing_functions);
                
                gemini_processed_lines++;
            }
        } else {
            std::cerr << "⚡ 高速モード: 基本検出のみ（Geminiスキップ）" << std::endl;
            // 高速モード：基本検出のみ、Geminiスキップ
            for (size_t i = 0; i < all_lines.size(); i++) {
                const std::string& current_line = all_lines[i];
                size_t current_line_number = i + 1;
                
                extract_typescript_functions_from_line(current_line, current_line_number, result, existing_functions);
                extract_typescript_classes_from_line(current_line, current_line_number, result, existing_classes);
                extract_typescript_interfaces_from_line(current_line, current_line_number, result, existing_classes);
                
                gemini_processed_lines++;
            }
        }
        
        auto gemini_end = std::chrono::high_resolution_clock::now();
        auto gemini_duration = std::chrono::duration_cast<std::chrono::milliseconds>(gemini_end - gemini_start).count();
        std::cerr << "✅ 第1段階完了: " << gemini_processed_lines 
                  << "行処理 (" << gemini_duration << "ms)" << std::endl;
        
        // 第2段階: 二重正規表現アタック！クラス全体を捕獲してメソッド検出
        std::cerr << "🎯 二重正規表現アタック開始！" << std::endl;
        double_regex_attack_for_class_methods(content, result, existing_functions);
        
        // 🚀 第3段階: 【ユーザー天才アイデア】無限ネスト掘削アタック！
        infinite_nested_function_attack(content, result, existing_functions);
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
    
    // TypeScriptクラスパターンの抽出
    void extract_typescript_classes_from_line(const std::string& line, size_t line_number,
                                             AnalysisResult& result, std::set<std::string>& existing_classes) {
        
        // パターン1: export class Name
        std::regex export_class_pattern(R"(^\s*export\s+(?:default\s+)?(?:abstract\s+)?class\s+(\w+))");
        std::smatch match;
        
        if (std::regex_search(line, match, export_class_pattern)) {
            std::string class_name = match[1].str();
            if (existing_classes.find(class_name) == existing_classes.end()) {
                ClassInfo class_info;
                class_info.name = class_name;
                class_info.start_line = line_number;
                result.classes.push_back(class_info);
                existing_classes.insert(class_name);
            }
        }
        
        // パターン2: class Name (exportなし)
        std::regex class_pattern(R"(^\s*(?:abstract\s+)?class\s+(\w+))");
        if (std::regex_search(line, match, class_pattern)) {
            std::string class_name = match[1].str();
            if (existing_classes.find(class_name) == existing_classes.end()) {
                ClassInfo class_info;
                class_info.name = class_name;
                class_info.start_line = line_number;
                result.classes.push_back(class_info);
                existing_classes.insert(class_name);
            }
        }
    }
    
    // TypeScriptインターフェースパターンの抽出
    void extract_typescript_interfaces_from_line(const std::string& line, size_t line_number,
                                                AnalysisResult& result, std::set<std::string>& existing_classes) {
        
        // interface Name をクラスとして扱う（統計用）
        std::regex interface_pattern(R"(^\s*(?:export\s+)?interface\s+(\w+))");
        std::smatch match;
        
        if (std::regex_search(line, match, interface_pattern)) {
            std::string interface_name = match[1].str();
            if (existing_classes.find(interface_name) == existing_classes.end()) {
                ClassInfo interface_info;
                interface_info.name = "interface:" + interface_name;
                interface_info.start_line = line_number;
                result.classes.push_back(interface_info);
                existing_classes.insert(interface_name);
            }
        }
        
        // type alias もクラスとして扱う
        std::regex type_pattern(R"(^\s*(?:export\s+)?type\s+(\w+))");
        if (std::regex_search(line, match, type_pattern)) {
            std::string type_name = match[1].str();
            if (existing_classes.find(type_name) == existing_classes.end()) {
                ClassInfo type_info;
                type_info.name = "type:" + type_name;
                type_info.start_line = line_number;
                result.classes.push_back(type_info);
                existing_classes.insert(type_name);
            }
        }
    }
    
    // TypeScriptメソッドパターンの抽出
    void extract_typescript_methods_from_line(const std::string& line, size_t line_number,
                                             AnalysisResult& result, std::set<std::string>& existing_functions) {
        
        // パターン1: async methodName(
        std::regex async_method_pattern(R"(^\s*(?:private\s+|public\s+|protected\s+)?async\s+(\w+)\s*\()");
        std::smatch match;
        
        if (std::regex_search(line, match, async_method_pattern)) {
            std::string method_name = match[1].str();
            if (existing_functions.find(method_name) == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = method_name;
                func_info.start_line = line_number;
                func_info.is_async = true;
                result.functions.push_back(func_info);
                existing_functions.insert(method_name);
            }
        }
        
        // パターン2: methodName(params): ReturnType
        std::regex method_pattern(R"(^\s*(?:private\s+|public\s+|protected\s+)?(\w+)\s*\([^)]*\)\s*:\s*\w)");
        if (std::regex_search(line, match, method_pattern)) {
            std::string method_name = match[1].str();
            if (existing_functions.find(method_name) == existing_functions.end() && 
                method_name != "constructor") {
                FunctionInfo func_info;
                func_info.name = method_name;
                func_info.start_line = line_number;
                result.functions.push_back(func_info);
                existing_functions.insert(method_name);
            }
        }
        
        // パターン3: interface内のメソッド定義 methodName(params): ReturnType;
        std::regex interface_method_pattern(R"(^\s*(\w+)\s*\([^)]*\)\s*:\s*[^;]+;)");
        if (std::regex_search(line, match, interface_method_pattern)) {
            std::string method_name = match[1].str();
            if (existing_functions.find(method_name) == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = method_name;
                func_info.start_line = line_number;
                result.functions.push_back(func_info);
                existing_functions.insert(method_name);
            }
        }
        
        // パターン4: getter/setter
        std::regex getter_setter_pattern(R"(^\s*(?:get|set)\s+(\w+)\s*\()");
        if (std::regex_search(line, match, getter_setter_pattern)) {
            std::string prop_name = match[1].str();
            std::string accessor_name = "get_" + prop_name;  // or set_
            if (existing_functions.find(accessor_name) == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = accessor_name;
                func_info.start_line = line_number;
                result.functions.push_back(func_info);
                existing_functions.insert(accessor_name);
            }
        }
        
        // パターン5: constructor
        std::regex constructor_pattern(R"(^\s*constructor\s*\()");
        if (std::regex_search(line, match, constructor_pattern)) {
            if (existing_functions.find("constructor") == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = "constructor";
                func_info.start_line = line_number;
                result.functions.push_back(func_info);
                existing_functions.insert("constructor");
            }
        }
        
        // パターン6: async *[Symbol.asyncIterator]()
        std::regex async_iterator_pattern(R"(^\s*async\s*\*\s*\[Symbol\.asyncIterator\]\s*\(\))");
        if (std::regex_search(line, match, async_iterator_pattern)) {
            if (existing_functions.find("[Symbol.asyncIterator]") == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = "[Symbol.asyncIterator]";
                func_info.start_line = line_number;
                func_info.is_async = true;
                result.functions.push_back(func_info);
                existing_functions.insert("[Symbol.asyncIterator]");
            }
        }
    }
    
    // 🎯 二重正規表現アタック！クラス内メソッド検出の真の力
    void double_regex_attack_for_class_methods(const std::string& content, AnalysisResult& result, std::set<std::string>& existing_functions) {
        
        // 第1段階：クラス全体を探知・捕獲
        std::regex class_declaration_pattern(R"((?:export\s+)?class\s+(\w+)[^{]*\{)");
        std::sregex_iterator class_iter(content.begin(), content.end(), class_declaration_pattern);
        std::sregex_iterator class_end;
        
        for (; class_iter != class_end; ++class_iter) {
            const std::smatch& class_match = *class_iter;
            std::string class_name = class_match[1].str();
            size_t class_start_pos = class_match.position() + class_match.length() - 1; // '{' の位置
            
            std::cerr << "🎯 クラス捕獲: " << class_name << " at position " << class_start_pos << std::endl;
            
            // クラス内容を中括弧バランスで抽出
            std::string class_content = extract_balanced_braces_content(content, class_start_pos);
            
            if (!class_content.empty()) {
                std::cerr << "📦 クラス内容長: " << class_content.length() << " bytes" << std::endl;
                
                // 第2段階：クラス内容に集中メソッド検出アタック！
                second_stage_method_attack(class_content, result, existing_functions, class_start_pos);
            }
        }
    }
    
    // 中括弧バランス取りながらクラス内容抽出
    std::string extract_balanced_braces_content(const std::string& content, size_t start_pos) {
        if (start_pos >= content.length() || content[start_pos] != '{') {
            return "";
        }
        
        int brace_count = 1;
        size_t pos = start_pos + 1;
        
        while (pos < content.length() && brace_count > 0) {
            if (content[pos] == '{') {
                brace_count++;
            } else if (content[pos] == '}') {
                brace_count--;
            }
            pos++;
        }
        
        if (brace_count == 0) {
            return content.substr(start_pos + 1, pos - start_pos - 2); // 中括弧を除外
        }
        return "";
    }
    
    // 🎯 三重正規表現アタック：第2・第3段階
    void second_stage_method_attack(const std::string& class_content, AnalysisResult& result, 
                                   std::set<std::string>& existing_functions, size_t class_start_pos) {
        
        std::cerr << "🔥 三重正規表現アタック第2段階開始！" << std::endl;
        
        // 第2段階：基本形抽出（にゃーのアイデア）- シンプルな正規表現
        std::regex basic_method_pattern(R"((\w+)\s*\([^)]*\)\s*(?::\s*[^{]+)?\s*\{)");
        std::sregex_iterator method_iter(class_content.begin(), class_content.end(), basic_method_pattern);
        std::sregex_iterator method_end;
        
        int method_count = 0;
        for (; method_iter != method_end; ++method_iter) {
            const std::smatch& method_match = *method_iter;
            std::string method_name = method_match[1].str();
            
            // 制御フロー文を除外
            if (method_name == "if" || method_name == "for" || method_name == "while" || 
                method_name == "switch" || method_name == "try" || method_name == "catch" ||
                method_name == "else" || method_name == "return") {
                std::cerr << "🚫 制御フロー文除外: " << method_name << std::endl;
                continue;
            }
            
            if (existing_functions.find(method_name) == existing_functions.end()) {
                std::cerr << "🎯 第2段階基本形抽出成功: " << method_name << std::endl;
                
                // 第3段階：詳細情報狙い撃ち（にゃーのアイデア）
                FunctionInfo func_info = triple_regex_attack_for_details(class_content, method_match, method_name);
                
                result.functions.push_back(func_info);
                existing_functions.insert(method_name);
                method_count++;
            }
        }
        
        std::cerr << "🎯 三重攻撃完了: " << method_count << "個のメソッドを高精度検出" << std::endl;
    }
    
    // 🎯 第3段階：詳細情報狙い撃ち（にゃーのアイデア）
    FunctionInfo triple_regex_attack_for_details(const std::string& class_content, 
                                                const std::smatch& method_match,
                                                const std::string& method_name) {
        FunctionInfo func_info;
        func_info.name = method_name;
        func_info.start_line = calculate_line_number(class_content, method_match.position());
        
        std::cerr << "💥 第3段階開始: " << method_name << " の詳細情報を狙い撃ち" << std::endl;
        
        // マッチした位置の行を抽出
        std::string matched_line = extract_line_from_position(class_content, method_match.position());
        std::cerr << "🎯 攻撃対象行: " << matched_line.substr(0, 100) << "..." << std::endl;
        
        // 第3段階：同一行に対して複数の正規表現攻撃！
        
        // ⚡ async攻撃
        if (std::regex_search(matched_line, std::regex(R"(async\s+)"))) {
            func_info.is_async = true;
            std::cerr << "⚡ async検出成功！" << std::endl;
        }
        
        // 🔒 private攻撃
        bool is_private = std::regex_search(matched_line, std::regex(R"(private\s+)"));
        if (is_private) {
            std::cerr << "🔒 private検出成功！" << std::endl;
        }
        
        // 🌍 public攻撃
        bool is_public = std::regex_search(matched_line, std::regex(R"(public\s+)"));
        if (is_public) {
            std::cerr << "🌍 public検出成功！" << std::endl;
        }
        
        // 🛡️ protected攻撃
        bool is_protected = std::regex_search(matched_line, std::regex(R"(protected\s+)"));
        if (is_protected) {
            std::cerr << "🛡️ protected検出成功！" << std::endl;
        }
            
        // 🏗️ static攻撃
        bool is_static = std::regex_search(matched_line, std::regex(R"(static\s+)"));
        if (is_static) {
            std::cerr << "🏗️ static検出成功！" << std::endl;
        }
        
        // 📖 readonly攻撃
        bool is_readonly = std::regex_search(matched_line, std::regex(R"(readonly\s+)"));
        if (is_readonly) {
            std::cerr << "📖 readonly検出成功！" << std::endl;
        }
        
        // 🎯 戻り値型攻撃
        std::regex return_type_pattern(R"(:\s*([^{]+)\s*\{)");
        std::smatch return_match;
        if (std::regex_search(matched_line, return_match, return_type_pattern)) {
            std::string return_type = trim_whitespace(return_match[1].str());
            std::cerr << "🎯 戻り値型検出: " << return_type << std::endl;
        }
        
        // 🧬 ジェネリクス攻撃
        std::regex generic_pattern(method_name + R"(<([^>]+)>)");
        std::smatch generic_match;
        if (std::regex_search(matched_line, generic_match, generic_pattern)) {
            std::string generic_params = generic_match[1].str();
            std::cerr << "🧬 ジェネリクス検出: <" << generic_params << ">" << std::endl;
        }
        
        std::cerr << "💥 第3段階完了: " << method_name << " の詳細分析成功" << std::endl;
        return func_info;
    }
    
    // マッチした位置から実際の行を抽出
    std::string extract_line_from_position(const std::string& content, size_t pos) {
        // 行の開始位置を探す
        size_t line_start = content.rfind('\n', pos);
        if (line_start == std::string::npos) {
            line_start = 0;
        } else {
            line_start += 1; // \nの次から
        }
        
        // 行の終了位置を探す
        size_t line_end = content.find('\n', pos);
        if (line_end == std::string::npos) {
            line_end = content.length();
        }
        
        return content.substr(line_start, line_end - line_start);
    }
    
    // 文字列の前後の空白を除去
    std::string trim_whitespace(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        
        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }
    
    // 文字列内の位置から行番号を計算
    size_t calculate_line_number(const std::string& content, size_t pos) {
        return std::count(content.begin(), content.begin() + pos, '\n') + 1;
    }
    
    // 🚀 にゃー先生天才アイデア：行レベル二重アタック！未検出メソッド攻略
    void gemini_line_level_double_attack(const std::string& line, size_t line_number,
                                        AnalysisResult& result, std::set<std::string>& existing_functions) {
        
        // 🔇 行レベルデバッグログ無効化（巨大ファイル対策）
        // std::cerr << "⚡ にゃー先生行レベル二重アタック: " << line.substr(0, 50) << "..." << std::endl;
        
        // 🎯 アタックパターン1: オブジェクトメソッド (method() {})
        gemini_attack_object_methods(line, line_number, result, existing_functions);
        
        // 🎯 アタックパターン2: プロパティ構文 (prop: function() {})
        gemini_attack_property_functions(line, line_number, result, existing_functions);
        
        // 🎯 アタックパターン3: アロー関数プロパティ (prop: () => {})
        gemini_attack_arrow_properties(line, line_number, result, existing_functions);
        
        // 🎯 アタックパターン4: インターフェースメソッド (method(): type;)
        gemini_attack_interface_methods(line, line_number, result, existing_functions);
    }
    
    // 🎯 オブジェクトメソッド攻撃 (method() {})
    void gemini_attack_object_methods(const std::string& line, size_t line_number,
                                     AnalysisResult& result, std::set<std::string>& existing_functions) {
        // にゃー先生推奨パターン: ^\s*([a-zA-Z0-9_$]+)\s*\([^)]*\)\s*{
        std::regex object_method_pattern(R"(^\s*([a-zA-Z0-9_$]+)\s*\([^)]*\)\s*\{)");
        std::smatch match;
        
        if (std::regex_search(line, match, object_method_pattern)) {
            std::string method_name = match[1].str();
            
            // 制御フロー文除外
            if (method_name == "if" || method_name == "for" || method_name == "while" || 
                method_name == "switch" || method_name == "try" || method_name == "catch" ||
                method_name == "else" || method_name == "return") {
                return;
            }
            
            if (existing_functions.find(method_name) == existing_functions.end()) {
                std::cerr << "🎯 Geminiオブジェクトメソッド発見: " << method_name << std::endl;
                
                FunctionInfo func_info;
                func_info.name = method_name;
                func_info.start_line = line_number;
                
                // 詳細検出
                if (line.find("async") != std::string::npos) {
                    func_info.is_async = true;
                }
                
                result.functions.push_back(func_info);
                existing_functions.insert(method_name);
            }
        }
    }
    
    // 🎯 プロパティ構文攻撃 (prop: function() {})
    void gemini_attack_property_functions(const std::string& line, size_t line_number,
                                         AnalysisResult& result, std::set<std::string>& existing_functions) {
        // Gemini先生推奨パターン: ^\s*([a-zA-Z0-9_$]+)\s*:\s*(?:async\s+)?function
        std::regex property_function_pattern(R"(^\s*([a-zA-Z0-9_$]+)\s*:\s*(?:async\s+)?function)");
        std::smatch match;
        
        if (std::regex_search(line, match, property_function_pattern)) {
            std::string prop_name = match[1].str();
            
            if (existing_functions.find(prop_name) == existing_functions.end()) {
                std::cerr << "🎯 Geminiプロパティ関数発見: " << prop_name << std::endl;
                
                FunctionInfo func_info;
                func_info.name = prop_name;
                func_info.start_line = line_number;
                
                if (line.find("async") != std::string::npos) {
                    func_info.is_async = true;
                }
                
                result.functions.push_back(func_info);
                existing_functions.insert(prop_name);
            }
        }
    }
    
    // 🎯 アロー関数プロパティ攻撃 (prop: () => {})
    void gemini_attack_arrow_properties(const std::string& line, size_t line_number,
                                       AnalysisResult& result, std::set<std::string>& existing_functions) {
        // Gemini先生推奨パターン: ^\s*([a-zA-Z0-9_$]+)\s*:\s*\(.*\)\s*=>
        std::regex arrow_property_pattern(R"(^\s*([a-zA-Z0-9_$]+)\s*:\s*\(.*\)\s*=>)");
        std::smatch match;
        
        if (std::regex_search(line, match, arrow_property_pattern)) {
            std::string prop_name = match[1].str();
            
            if (existing_functions.find(prop_name) == existing_functions.end()) {
                std::cerr << "🎯 Geminiアロー関数プロパティ発見: " << prop_name << std::endl;
                
                FunctionInfo func_info;
                func_info.name = prop_name;
                func_info.start_line = line_number;
                func_info.is_arrow_function = true;
                
                result.functions.push_back(func_info);
                existing_functions.insert(prop_name);
            }
        }
    }
    
    // 🎯 インターフェースメソッド攻撃 (method(): type;)
    void gemini_attack_interface_methods(const std::string& line, size_t line_number,
                                        AnalysisResult& result, std::set<std::string>& existing_functions) {
        // Gemini先生推奨パターン: ^\s*([a-zA-Z0-9_$]+)\s*\(([^;]*)\)\s*:\s*[^;]+;
        std::regex interface_method_pattern(R"(^\s*([a-zA-Z0-9_$]+)\s*\([^)]*\)\s*:\s*[^;]+;)");
        std::smatch match;
        
        if (std::regex_search(line, match, interface_method_pattern)) {
            std::string method_name = match[1].str();
            
            if (existing_functions.find(method_name) == existing_functions.end()) {
                std::cerr << "🎯 Geminiインターフェースメソッド発見: " << method_name << std::endl;
                
                FunctionInfo func_info;
                func_info.name = method_name;
                func_info.start_line = line_number;
                
                result.functions.push_back(func_info);
                existing_functions.insert(method_name);
            }
        }
    }

    // 🎯 関数範囲を表す構造体
    struct FunctionRange {
        size_t start_line;
        size_t end_line;
        size_t indent_level;  // インデントレベルで深さを管理
    };
    
    // 🔍 関数の終了行を見つける（ブレースバランス方式）
    size_t find_function_end_line(const std::vector<std::string>& lines, size_t start_line) {
        int brace_count = 0;
        bool in_function = false;
        
        for (size_t i = start_line; i < lines.size(); i++) {
            const std::string& line = lines[i];
            
            // ブレースをカウント
            for (char ch : line) {
                if (ch == '{') {
                    brace_count++;
                    in_function = true;
                } else if (ch == '}') {
                    brace_count--;
                    if (in_function && brace_count == 0) {
                        return i;  // 関数の終了行
                    }
                }
            }
            
            // アロー関数の単一式の場合（セミコロンで終了）
            if (!in_function && line.find("=>") != std::string::npos) {
                if (line.find(';') != std::string::npos) {
                    return i;
                }
                // 次の行がインデントされていない場合も終了
                if (i + 1 < lines.size()) {
                    size_t next_indent = lines[i + 1].find_first_not_of(" \t");
                    size_t curr_indent = line.find_first_not_of(" \t");
                    if (next_indent != std::string::npos && curr_indent != std::string::npos &&
                        next_indent <= curr_indent) {
                        return i;
                    }
                }
            }
        }
        
        return lines.size() - 1;  // ファイル終端まで
    }

    // 🎯 【ユーザー天才アイデア】無限ネスト掘削アタック！（並列化版）
    void infinite_nested_function_attack(const std::string& content, AnalysisResult& result, 
                                       std::set<std::string>& existing_functions) {
        std::cerr << "🚀 【ユーザー天才アイデア】無限ネスト掘削アタック開始！（並列化版）" << std::endl;
        
        // 🕐 性能測定追加
        auto total_start = std::chrono::high_resolution_clock::now();
        std::atomic<size_t> total_lines_scanned{0};
        
        // 全内容を行単位で分割（一度だけ！）
        std::vector<std::string> lines;
        std::istringstream iss(content);
        std::string line;
        while (std::getline(iss, line)) {
            lines.push_back(line);
        }
        
        int attack_round = 1;
        size_t previous_count = existing_functions.size();
        
        // 次回検索する関数範囲のリスト
        std::vector<FunctionRange> search_ranges;
        
        // 第1回は全体を検索対象に
        search_ranges.push_back({0, lines.size() - 1, 0});
        
        // 🕐 各層の処理時間を記録
        std::vector<std::chrono::milliseconds> layer_times;
        std::vector<size_t> layer_ranges;
        std::vector<size_t> layer_detections;
        std::vector<size_t> layer_lines;
        
        // 0個になるまで繰り返し攻撃！
        while (!search_ranges.empty()) {
            auto round_start = std::chrono::high_resolution_clock::now();
            std::cerr << "🎯 第" << attack_round << "回ネスト掘削攻撃開始！（検索範囲: " 
                      << search_ranges.size() << "個）" << std::endl;
            
            std::vector<FunctionRange> next_search_ranges;  // 次回の検索範囲
            std::atomic<size_t> round_detections{0};
            std::atomic<size_t> round_lines_scanned{0};
            
            // 🔒 並列処理用のミューテックス
            std::mutex functions_mutex;
            std::mutex ranges_mutex;
            std::mutex output_mutex;
            
            // 🚀 各範囲を並列検索！
            std::for_each(std::execution::par_unseq,
                          search_ranges.begin(),
                          search_ranges.end(),
                          [&](const FunctionRange& range) {
                for (size_t line_idx = range.start_line; line_idx <= range.end_line && line_idx < lines.size(); line_idx++) {
                    const std::string& line = lines[line_idx];
                    round_lines_scanned.fetch_add(1);
                
                    // 🎯 完全掘削モード：同一行に複数関数も検出！
                    size_t line_number = line_idx + 1;  // 1ベースの行番号
                    
                    // パターン1: インデント付きfunction検出（ネスト関数）
                    std::regex nested_function_pattern(R"(^[ \t]+function\s+([a-zA-Z_]\w*)\s*[<(])");
                    std::smatch match;
                    
                    if (std::regex_search(line, match, nested_function_pattern)) {
                        std::string func_name = match[1].str();
                        
                        bool should_add = false;
                        {
                            std::lock_guard<std::mutex> lock(functions_mutex);
                            if (existing_functions.find(func_name) == existing_functions.end()) {
                                existing_functions.insert(func_name);
                                should_add = true;
                            }
                        }
                        
                        if (should_add) {
                            {
                                std::lock_guard<std::mutex> lock(output_mutex);
                                std::cerr << "🎯 第" << attack_round << "回でネスト関数発見: " << func_name 
                                          << " (行:" << line_number << ")" << std::endl;
                            }
                            
                            FunctionInfo func_info;
                            func_info.name = func_name;
                            func_info.start_line = line_number;
                            
                            // async判定
                            if (line.find("async") != std::string::npos) {
                                func_info.is_async = true;
                            }
                            
                            {
                                std::lock_guard<std::mutex> lock(functions_mutex);
                                result.functions.push_back(func_info);
                            }
                            round_detections.fetch_add(1);
                            
                            // 🚀 関数の範囲を特定して次回検索対象に追加！
                            size_t func_end = find_function_end_line(lines, line_idx);
                            if (func_end > line_idx) {
                                FunctionRange new_range;
                                new_range.start_line = line_idx + 1;  // 関数内部から開始
                                new_range.end_line = func_end - 1;    // 閉じブレースの前まで
                                new_range.indent_level = range.indent_level + 1;
                                
                                // 🛡️ 深さ制限（5レベルまで）
                                if (new_range.indent_level <= 5) {
                                    std::lock_guard<std::mutex> lock(ranges_mutex);
                                    next_search_ranges.push_back(new_range);
                                    {
                                        std::lock_guard<std::mutex> out_lock(output_mutex);
                                        std::cerr << "  → 次回検索範囲追加: 行" << new_range.start_line + 1 
                                                  << "-" << new_range.end_line + 1 
                                                  << " (深さ:" << new_range.indent_level << ")" << std::endl;
                                    }
                                }
                            }
                        }
                    }
                
                // パターン2: インデント付きアロー関数（const/let/var）
                std::regex nested_arrow_pattern(R"(^[ \t]+(const|let|var)\s+([a-zA-Z_]\w*)\s*=\s*.*=>)");
                std::smatch arrow_match;
                
                if (std::regex_search(line, arrow_match, nested_arrow_pattern)) {
                    std::string arrow_name = arrow_match[2].str();
                    
                    bool should_add_arrow = false;
                    {
                        std::lock_guard<std::mutex> lock(functions_mutex);
                        if (existing_functions.find(arrow_name) == existing_functions.end()) {
                            existing_functions.insert(arrow_name);
                            should_add_arrow = true;
                        }
                    }
                    
                    if (should_add_arrow) {
                        {
                            std::lock_guard<std::mutex> lock(output_mutex);
                            std::cerr << "🎯 第" << attack_round << "回でネストアロー関数発見: " << arrow_name 
                                      << " (行:" << line_number << ")" << std::endl;
                        }
                        
                        FunctionInfo func_info;
                        func_info.name = arrow_name;
                        func_info.start_line = line_number;
                        func_info.is_arrow_function = true;
                        
                        if (line.find("async") != std::string::npos) {
                            func_info.is_async = true;
                        }
                        
                        {
                            std::lock_guard<std::mutex> lock(functions_mutex);
                            result.functions.push_back(func_info);
                        }
                        round_detections.fetch_add(1);
                        
                        // 🚀 アロー関数の範囲も次回検索対象に！
                        size_t func_end = find_function_end_line(lines, line_idx);
                        if (func_end > line_idx) {
                            FunctionRange new_range;
                            new_range.start_line = line_idx + 1;
                            new_range.end_line = func_end - 1;
                            new_range.indent_level = range.indent_level + 1;
                            
                            // 🛡️ 深さ制限（5レベルまで）
                            if (new_range.indent_level <= 5) {
                                std::lock_guard<std::mutex> lock(ranges_mutex);
                                next_search_ranges.push_back(new_range);
                            }
                        }
                    }
                }
                
                // パターン3: インデント付き関数式代入
                std::regex nested_func_assign_pattern(R"(^[ \t]+(const|let|var)\s+([a-zA-Z_]\w*)\s*=\s*function)");
                std::smatch assign_match;
                
                if (std::regex_search(line, assign_match, nested_func_assign_pattern)) {
                    std::string assign_name = assign_match[2].str();
                    
                    bool should_add_assign = false;
                    {
                        std::lock_guard<std::mutex> lock(functions_mutex);
                        if (existing_functions.find(assign_name) == existing_functions.end()) {
                            existing_functions.insert(assign_name);
                            should_add_assign = true;
                        }
                    }
                    
                    if (should_add_assign) {
                        {
                            std::lock_guard<std::mutex> lock(output_mutex);
                            std::cerr << "🎯 第" << attack_round << "回でネスト関数式発見: " << assign_name 
                                      << " (行:" << line_number << ")" << std::endl;
                        }
                        
                        FunctionInfo func_info;
                        func_info.name = assign_name;
                        func_info.start_line = line_number;
                        
                        {
                            std::lock_guard<std::mutex> lock(functions_mutex);
                            result.functions.push_back(func_info);
                        }
                        round_detections.fetch_add(1);
                        
                        // 🚀 関数式の範囲も次回検索対象に！
                        size_t func_end = find_function_end_line(lines, line_idx);
                        if (func_end > line_idx) {
                            FunctionRange new_range;
                            new_range.start_line = line_idx + 1;
                            new_range.end_line = func_end - 1;
                            new_range.indent_level = range.indent_level + 1;
                            
                            // 🛡️ 深さ制限（5レベルまで）
                            if (new_range.indent_level <= 5) {
                                std::lock_guard<std::mutex> lock(ranges_mutex);
                                next_search_ranges.push_back(new_range);
                            }
                        }
                    }
                }
                }  // 行ループ終了
            });  // 並列処理終了
            
            // 性能測定
            auto round_end = std::chrono::high_resolution_clock::now();
            auto round_duration = std::chrono::duration_cast<std::chrono::milliseconds>(round_end - round_start);
            total_lines_scanned.fetch_add(round_lines_scanned.load());
            
            // 🕐 層ごとの統計を記録
            layer_times.push_back(round_duration);
            layer_ranges.push_back(search_ranges.size());
            layer_detections.push_back(round_detections.load());
            layer_lines.push_back(round_lines_scanned.load());
            
            std::cerr << "🎯 第" << attack_round << "回攻撃完了！新規検出: " << round_detections.load() << "個"
                      << " (処理時間: " << round_duration.count() << "ms, 処理行数: " << round_lines_scanned.load() << "行)" << std::endl;
            
            // 次回の検索範囲を更新
            search_ranges = std::move(next_search_ranges);
            
            // 検索範囲が空になったら終了！
            if (search_ranges.empty()) {
                std::cerr << "🎉 無限ネスト掘削アタック完了！検索範囲が空になりました" << std::endl;
                break;
            }
            
            attack_round++;
            
            // 無限ループ防止（最大20回まで - 深いネストに対応）
            if (attack_round > 20) {
                std::cerr << "⚠️ 安全装置発動：20回で強制終了" << std::endl;
                break;
            }
        }
        
        size_t total_nested = existing_functions.size() - previous_count;
        // 総計性能測定
        auto total_end = std::chrono::high_resolution_clock::now();
        auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(total_end - total_start).count();
        
        std::cerr << "🏆 無限ネスト掘削アタック最終結果：" << total_nested << "個のネスト関数を発見！" << std::endl;
        std::cerr << "⏱️  総処理時間: " << total_duration << "ms, 総スキャン行数: " << total_lines_scanned.load() 
                  << "行 (ラウンド数: " << (attack_round - 1) << "回)" << std::endl;
        
        // 🕐 層ごとの詳細プロファイリング結果
        std::cerr << "\n📊 === 層ごとの詳細プロファイリング ===" << std::endl;
        for (size_t i = 0; i < layer_times.size(); i++) {
            std::cerr << "📈 第" << (i + 1) << "層: "
                      << layer_times[i].count() << "ms, "
                      << layer_ranges[i] << "範囲, "
                      << layer_detections[i] << "個検出, "
                      << layer_lines[i] << "行処理";
            
            // 1行あたりの処理時間を計算
            if (layer_lines[i] > 0) {
                double ms_per_line = static_cast<double>(layer_times[i].count()) / layer_lines[i];
                std::cerr << " (1行あたり: " << std::fixed << std::setprecision(3) << ms_per_line << "ms)";
            }
            std::cerr << std::endl;
        }
        
        // 🕐 累積時間も表示
        std::cerr << "\n📊 === 累積処理時間 ===" << std::endl;
        std::chrono::milliseconds cumulative_time{0};
        for (size_t i = 0; i < layer_times.size(); i++) {
            cumulative_time += layer_times[i];
            std::cerr << "🏃 第1層〜第" << (i + 1) << "層までの累積: " << cumulative_time.count() << "ms" << std::endl;
        }
        std::cerr << "===================================\n" << std::endl;
    }
    
    // 🔥 前処理革命：コメント・文字列除去システム（Gemini先生の知恵！）
    std::string preprocess_content(const std::string& content) {
        std::string result = content;
        std::cerr << "🧹 前処理開始: コメント・文字列除去システム起動" << std::endl;
        
        // 1. 複数行コメント /* ... */ を除去
        result = remove_multiline_comments(result);
        
        // 2. 単行コメント // ... を除去  
        result = remove_single_line_comments(result);
        
        // 3. 文字列リテラル "...", '...', `...` を除去
        result = remove_string_literals(result);
        
        std::cerr << "🧹 前処理完了: 全てのコメント・文字列を無害化" << std::endl;
        return result;
    }
    
    // 複数行コメント /* ... */ 除去
    std::string remove_multiline_comments(const std::string& content) {
        std::string result;
        size_t pos = 0;
        
        while (pos < content.length()) {
            size_t comment_start = content.find("/*", pos);
            if (comment_start == std::string::npos) {
                result += content.substr(pos);
                break;
            }
            
            // コメント開始前までをコピー
            result += content.substr(pos, comment_start - pos);
            
            // コメント終了を検索
            size_t comment_end = content.find("*/", comment_start + 2);
            if (comment_end == std::string::npos) {
                // コメントが閉じられていない場合、残り全部をスペースに
                result += std::string(content.length() - comment_start, ' ');
                break;
            }
            
            // コメント部分をスペースで置換（行数維持のため）
            std::string comment_text = content.substr(comment_start, comment_end - comment_start + 2);
            for (char c : comment_text) {
                result += (c == '\n') ? '\n' : ' ';
            }
            
            pos = comment_end + 2;
        }
        
        return result;
    }
    
    // 単行コメント // ... 除去
    std::string remove_single_line_comments(const std::string& content) {
        std::istringstream stream(content);
        std::string result;
        std::string line;
        
        while (std::getline(stream, line)) {
            size_t comment_pos = line.find("//");
            if (comment_pos != std::string::npos) {
                // コメント部分をスペースで置換
                std::string clean_line = line.substr(0, comment_pos);
                clean_line += std::string(line.length() - comment_pos, ' ');
                result += clean_line + "\n";
            } else {
                result += line + "\n";
            }
        }
        
        return result;
    }
    
    // 文字列リテラル "...", '...', `...` 除去
    std::string remove_string_literals(const std::string& content) {
        std::string result;
        size_t pos = 0;
        
        while (pos < content.length()) {
            char c = content[pos];
            
            // 文字列開始文字を検出
            if (c == '"' || c == '\'' || c == '`') {
                char quote = c;
                result += ' '; // クォート自体もスペースに
                pos++;
                
                // 文字列終了まで検索
                while (pos < content.length()) {
                    char current = content[pos];
                    
                    if (current == quote) {
                        result += ' '; // 終了クォートもスペースに
                        pos++;
                        break;
                    } else if (current == '\\' && pos + 1 < content.length()) {
                        // エスケープシーケンス処理
                        result += (current == '\n') ? '\n' : ' ';
                        pos++;
                        if (pos < content.length()) {
                            result += (content[pos] == '\n') ? '\n' : ' ';
                            pos++;
                        }
                    } else {
                        result += (current == '\n') ? '\n' : ' ';
                        pos++;
                    }
                }
            } else {
                result += c;
                pos++;
            }
        }
        
        return result;
    }
};

} // namespace nekocode