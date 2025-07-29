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
        // 🔥 前処理革命：コメント・文字列除去システム（Gemini先生戦略！）
        std::string preprocessed_content = preprocess_content(content);
        
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
        
        // 第1段階: 行ベース解析（従来通り） 
        while (std::getline(stream, line)) {
            extract_typescript_functions_from_line(line, line_number, result, existing_functions);
            extract_typescript_classes_from_line(line, line_number, result, existing_classes);
            extract_typescript_interfaces_from_line(line, line_number, result, existing_classes);
            
            // 🚀 Gemini先生A案：行レベル二重アタック！
            gemini_line_level_double_attack(line, line_number, result, existing_functions);
            
            line_number++;
        }
        
        // 第2段階: 二重正規表現アタック！クラス全体を捕獲してメソッド検出
        std::cerr << "🎯 二重正規表現アタック開始！" << std::endl;
        double_regex_attack_for_class_methods(content, result, existing_functions);
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
    
    // 🚀 Gemini先生A案：行レベル二重アタック！未検出メソッド攻略
    void gemini_line_level_double_attack(const std::string& line, size_t line_number,
                                        AnalysisResult& result, std::set<std::string>& existing_functions) {
        
        std::cerr << "⚡ Gemini行レベル二重アタック: " << line.substr(0, 50) << "..." << std::endl;
        
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
        // Gemini先生推奨パターン: ^\s*([a-zA-Z0-9_$]+)\s*\([^)]*\)\s*{
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