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
        
        std::cerr << "📜 TypeScript analyzer: Base JS detected classes=" << result.classes.size() 
                  << ", functions=" << result.functions.size() << std::endl;
        
        // 🚀 TypeScript特有のハイブリッド戦略追加
        if (needs_typescript_specific_analysis(result, content)) {
            std::cerr << "📜 TypeScript specific analysis triggered!" << std::endl;
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
};

} // namespace nekocode