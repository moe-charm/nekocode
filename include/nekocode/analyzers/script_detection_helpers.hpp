#pragma once

//=============================================================================
// 🔍 Script Detection Helpers - JavaScript/TypeScript検出ロジック統合
//
// 重複検出コードを統合してJavaScript/TypeScriptから500行削減
// 責任: export関数検出、メンバ変数検出、重複チェック、パターン統一
//=============================================================================

#include "../types.hpp"
#include <string>
#include <vector>
#include <set>
#include <regex>
#include <sstream>

namespace nekocode {
namespace script_detection {

//=============================================================================
// 🎯 ScriptDetectionHelpers - JS/TS共通検出ロジッククラス
//=============================================================================

class ScriptDetectionHelpers {
public:
    /// export関数検出（JavaScript/TypeScript共通パターン）
    static std::vector<FunctionInfo> detect_export_functions(
        const std::string& content,
        const std::set<std::string>& existing_functions
    ) {
        std::vector<FunctionInfo> export_functions;
        std::istringstream stream(content);
        std::string line;
        size_t line_number = 0;
        
        // export関数パターン（JS/TS共通）
        std::regex export_patterns[] = {
            std::regex(R"(^\s*export\s+function\s+(\w+)(?:<[^>]*>)?\s*\()"),        // export function name()
            std::regex(R"(^\s*export\s+const\s+(\w+)\s*=\s*(?:async\s*)?\([^)]*\)(?:\s*:\s*[^=]+)?\s*=>)"), // export const name = () =>
            std::regex(R"(^\s*export\s+async\s+function\s+(\w+)(?:<[^>]*>)?\s*\()"), // export async function name()
            std::regex(R"(^\s*export\s+\{\s*(\w+)\s*\})"),                          // export { name }
            std::regex(R"(^\s*export\s+default\s+function\s+(\w+)\s*\()")           // export default function name()
        };
        
        while (std::getline(stream, line)) {
            line_number++;
            
            for (const auto& pattern : export_patterns) {
                std::smatch match;
                if (std::regex_search(line, match, pattern)) {
                    std::string function_name = match[1].str();
                    
                    // 重複チェック
                    if (existing_functions.find(function_name) == existing_functions.end() &&
                        !is_control_keyword(function_name)) {
                        
                        FunctionInfo func;
                        func.name = function_name;
                        func.start_line = static_cast<uint32_t>(line_number);
                        func.end_line = 0; // 後で計算
                        func.metadata["is_exported"] = "true";
                        
                        // export async検出
                        if (line.find("async") != std::string::npos) {
                            func.is_async = true;
                        }
                        
                        export_functions.push_back(func);
                        break; // 最初にマッチしたパターンのみ処理
                    }
                }
            }
        }
        
        return export_functions;
    }
    
    /// 関数検出（JavaScript/TypeScript共通パターン）
    static std::vector<FunctionInfo> detect_basic_functions(
        const std::string& content,
        const std::set<std::string>& existing_functions
    ) {
        std::vector<FunctionInfo> basic_functions;
        std::istringstream stream(content);
        std::string line;
        size_t line_number = 0;
        
        // 基本関数パターン（JS/TS共通）
        std::regex function_patterns[] = {
            std::regex(R"(^\s*function\s+(\w+)\s*[<(])"),                           // function name()
            std::regex(R"(^\s*(?:const|let|var)\s+(\w+)\s*=\s*function\s*[<(])"),  // const name = function()
            std::regex(R"(^\s*(?:const|let|var)\s+(\w+)\s*=\s*(?:async\s*)?\([^)]*\)(?:\s*:\s*[^=]+)?\s*=>)"), // const name = () =>
            std::regex(R"(^\s*async\s+function\s+(\w+)\s*[<(])"),                  // async function name()
            std::regex(R"(^\s*(\w+)\s*:\s*function\s*[<(])"),                      // name: function() (オブジェクトメソッド)
            std::regex(R"(^\s*(\w+)\s*:\s*(?:async\s*)?\([^)]*\)(?:\s*:\s*[^=]+)?\s*=>)") // name: () => (オブジェクトメソッド)
        };
        
        while (std::getline(stream, line)) {
            line_number++;
            
            for (const auto& pattern : function_patterns) {
                std::smatch match;
                if (std::regex_search(line, match, pattern)) {
                    std::string function_name = match[1].str();
                    
                    // 重複チェック
                    if (existing_functions.find(function_name) == existing_functions.end() &&
                        !is_control_keyword(function_name)) {
                        
                        FunctionInfo func;
                        func.name = function_name;
                        func.start_line = static_cast<uint32_t>(line_number);
                        func.end_line = 0; // 後で計算
                        func.metadata["is_exported"] = "false";
                        
                        // async検出
                        if (line.find("async") != std::string::npos) {
                            func.is_async = true;
                        }
                        
                        // アロー関数検出
                        if (line.find("=>") != std::string::npos) {
                            func.is_arrow_function = true;
                        }
                        
                        basic_functions.push_back(func);
                        break; // 最初にマッチしたパターンのみ処理
                    }
                }
            }
        }
        
        return basic_functions;
    }
    
    /// クラス検出（JavaScript/TypeScript共通パターン）
    static std::vector<ClassInfo> detect_classes(
        const std::string& content,
        const std::set<std::string>& existing_classes
    ) {
        std::vector<ClassInfo> classes;
        std::istringstream stream(content);
        std::string line;
        size_t line_number = 0;
        
        // クラスパターン（JS/TS共通）- React.Component対応版
        std::regex class_patterns[] = {
            std::regex(R"(^\s*class\s+(\w+)\s*(?:extends\s+([\w\.]+))?\s*\{)"),         // class Name extends React.Component {
            std::regex(R"(^\s*export\s+class\s+(\w+)\s*(?:extends\s+([\w\.]+))?\s*\{)"), // export class Name extends React.Component {
            std::regex(R"(^\s*export\s+default\s+class\s+(\w+)\s*(?:extends\s+([\w\.]+))?\s*\{)") // export default class Name extends React.Component {
        };
        
        while (std::getline(stream, line)) {
            line_number++;
            
            for (const auto& pattern : class_patterns) {
                std::smatch match;
                if (std::regex_search(line, match, pattern)) {
                    std::string class_name = match[1].str();
                    std::string parent_class = match.size() > 2 ? match[2].str() : "";
                    
                    // 重複チェック
                    if (existing_classes.find(class_name) == existing_classes.end()) {
                        ClassInfo cls;
                        cls.name = class_name;
                        cls.parent_class = parent_class;
                        cls.start_line = static_cast<uint32_t>(line_number);
                        cls.end_line = 0; // 後で計算
                        
                        classes.push_back(cls);
                        break; // 最初にマッチしたパターンのみ処理
                    }
                }
            }
        }
        
        return classes;
    }
    
    /// 🚀 JavaScriptクラスメソッド検出（Phase 5緊急対応）
    static void detect_class_methods(
        std::vector<ClassInfo>& classes,
        const std::string& content
    ) {
#ifdef NEKOCODE_DEBUG_SYMBOLS
        std::cerr << "[DEBUG detect_class_methods] Starting. Classes count: " << classes.size() << std::endl;
#endif
        
        std::istringstream stream(content);
        std::string line;
        size_t line_number = 0;
        
        // 現在解析中のクラスを追跡
        ClassInfo* current_class = nullptr;
        int brace_depth = 0;
        int class_brace_depth = -1;
        
        // JavaScriptクラスメソッドパターン
        std::regex method_patterns[] = {
            std::regex(R"(^\s+(\w+)\s*\([^)]*\)\s*\{)"),                    // 　　methodName() {
            std::regex(R"(^\s+async\s+(\w+)\s*\([^)]*\)\s*\{)"),             // 　　async methodName() {
            std::regex(R"(^\s+static\s+(\w+)\s*\([^)]*\)\s*\{)"),           // 　　static methodName() {
            std::regex(R"(^\s+static\s+async\s+(\w+)\s*\([^)]*\)\s*\{)"),   // 　　static async methodName() {
            std::regex(R"(^\s+async\s+static\s+(\w+)\s*\([^)]*\)\s*\{)"),   // 　　async static methodName() {
            std::regex(R"(^\s*(\w+)\s*\([^)]*\)\s*\{)")                     // methodName() { （フォールバック）
        };
        
        while (std::getline(stream, line)) {
            line_number++;
            
#ifdef NEKOCODE_DEBUG_SYMBOLS
            std::cerr << "[DEBUG] Line " << line_number << ": [" << line << "]" 
                      << " brace_depth=" << brace_depth 
                      << " class_brace_depth=" << class_brace_depth
                      << " current_class=" << (current_class ? current_class->name : "null") << std::endl;
#endif
            
            // ブレース深度更新（先に処理）
            int old_brace_depth = brace_depth;
            for (char c : line) {
                if (c == '{') {
                    brace_depth++;
                } else if (c == '}') {
                    brace_depth--;
                }
            }
            
            // クラス開始時の深度設定（classキーワードがある行の後の深度を記録）
            if (current_class && class_brace_depth == -1 && line.find('{') != std::string::npos) {
                class_brace_depth = old_brace_depth;  // クラスの中身がある深度を記録（深度1）
#ifdef NEKOCODE_DEBUG_SYMBOLS
                std::cerr << "[DEBUG] Entered class " << current_class->name 
                          << " at content depth=" << class_brace_depth << std::endl;
#endif
            }
            
            // クラス終了チェック（深度が0になったらクラス終了）
            if (current_class && brace_depth == 0 && old_brace_depth > 0) {
#ifdef NEKOCODE_DEBUG_SYMBOLS
                std::cerr << "[DEBUG] Exited class " << current_class->name << std::endl;
#endif
                current_class = nullptr;
                class_brace_depth = -1;
            }
            
            // クラス開始チェック
            std::regex class_start_pattern(R"(^\s*(?:export\s+)?class\s+(\w+))");
            std::smatch class_match;
            if (std::regex_search(line, class_match, class_start_pattern)) {
                std::string class_name = class_match[1].str();
                // 対応するクラスを検索
                for (auto& cls : classes) {
                    if (cls.name == class_name) {
                        current_class = &cls;
                        break;
                    }
                }
                continue;
            }
            
            // クラス内でのみメソッド検出（修正版：クラス内かつ深度1の行）
            if (current_class && old_brace_depth == 1 && class_brace_depth >= 0) {
                for (size_t i = 0; i < sizeof(method_patterns) / sizeof(method_patterns[0]); i++) {
                    std::smatch match;
                    if (std::regex_search(line, match, method_patterns[i])) {
                        std::string method_name;
                        
                        // パターンに応じてメソッド名抽出
                        // 全てのパターンでmatch[1]がメソッド名
                        method_name = match[1].str();
                        
#ifdef NEKOCODE_DEBUG_SYMBOLS
                        std::cerr << "[DEBUG detect_class_methods] Found method: " << method_name 
                                  << " in class " << current_class->name
                                  << " at line " << line_number << std::endl;
#endif
                        
                        // コンストラクタ名をクラス名と同じにしない
                        if (method_name != current_class->name && method_name != "constructor") {
                            FunctionInfo method;
                            method.name = method_name;
                            method.start_line = static_cast<uint32_t>(line_number);
                            method.metadata["is_class_method"] = "true";
                            
                            if (line.find("async") != std::string::npos) {
                                method.is_async = true;
                            }
                            if (line.find("static") != std::string::npos) {
                                method.metadata["is_static"] = "true";
                            }
                            
                            current_class->methods.push_back(method);
#ifdef NEKOCODE_DEBUG_SYMBOLS
                            std::cerr << "[DEBUG detect_class_methods] Added method " << method_name 
                                      << " to class " << current_class->name
                                      << ". Total methods: " << current_class->methods.size() << std::endl;
#endif
                        } else if (method_name == "constructor") {
                            // コンストラクターも追加します
                            FunctionInfo constructor;
                            constructor.name = "constructor";
                            constructor.start_line = static_cast<uint32_t>(line_number);
                            constructor.metadata["is_class_method"] = "true";
                            constructor.metadata["is_constructor"] = "true";
                            
                            current_class->methods.push_back(constructor);
#ifdef NEKOCODE_DEBUG_SYMBOLS
                            std::cerr << "[DEBUG detect_class_methods] Added constructor to class " 
                                      << current_class->name
                                      << ". Total methods: " << current_class->methods.size() << std::endl;
#endif
                        }
                        break;
                    }
                }
            }
        }
    }
    
    /// 重複チェックヘルパー（既存名前セット構築）
    static std::set<std::string> build_existing_names_set(
        const std::vector<FunctionInfo>& functions,
        const std::vector<ClassInfo>& classes
    ) {
        std::set<std::string> existing_names;
        
        // 関数名を追加
        for (const auto& func : functions) {
            existing_names.insert(func.name);
        }
        
        // クラス名を追加
        for (const auto& cls : classes) {
            existing_names.insert(cls.name);
        }
        
        return existing_names;
    }
    
    /// 制御構造キーワードチェック
    static bool is_control_keyword(const std::string& name) {
        static const std::set<std::string> control_keywords = {
            "if", "else", "for", "while", "do", "switch", "case", "catch", 
            "try", "finally", "return", "break", "continue", "throw", 
            "typeof", "instanceof", "new", "delete", "var", "let", "const",
            "true", "false", "null", "undefined", "this", "super"
        };
        
        return control_keywords.find(name) != control_keywords.end();
    }
    
    /// TypeScript固有interface検出
    static std::vector<ClassInfo> detect_typescript_interfaces(
        const std::string& content,
        const std::set<std::string>& existing_names
    ) {
        std::vector<ClassInfo> interfaces;
        std::istringstream stream(content);
        std::string line;
        size_t line_number = 0;
        
        std::regex interface_patterns[] = {
            std::regex(R"(^\s*interface\s+(\w+)\s*(?:extends\s+[\w,\s]+)?\s*\{)"),        // interface Name extends Base {
            std::regex(R"(^\s*export\s+interface\s+(\w+)\s*(?:extends\s+[\w,\s]+)?\s*\{)") // export interface Name {
        };
        
        while (std::getline(stream, line)) {
            line_number++;
            
            for (const auto& pattern : interface_patterns) {
                std::smatch match;
                if (std::regex_search(line, match, pattern)) {
                    std::string interface_name = match[1].str();
                    
                    // 重複チェック
                    if (existing_names.find(interface_name) == existing_names.end()) {
                        ClassInfo interface_info;
                        interface_info.name = interface_name;
                        interface_info.start_line = static_cast<uint32_t>(line_number);
                        interface_info.end_line = 0; // 後で計算
                        
                        // TypeScript interface専用メタデータ
                        interface_info.metadata["type"] = "interface";
                        interface_info.metadata["is_exported"] = (line.find("export") != std::string::npos) ? "true" : "false";
                        
                        interfaces.push_back(interface_info);
                        break;
                    }
                }
            }
        }
        
        return interfaces;
    }
    
    /// TypeScript固有type alias検出
    static std::vector<std::string> detect_typescript_type_aliases(
        const std::string& content,
        const std::set<std::string>& existing_names
    ) {
        std::vector<std::string> type_aliases;
        std::istringstream stream(content);
        std::string line;
        
        std::regex type_patterns[] = {
            std::regex(R"(^\s*type\s+(\w+)\s*=)"),         // type Name =
            std::regex(R"(^\s*export\s+type\s+(\w+)\s*=)")  // export type Name =
        };
        
        while (std::getline(stream, line)) {
            for (const auto& pattern : type_patterns) {
                std::smatch match;
                if (std::regex_search(line, match, pattern)) {
                    std::string type_name = match[1].str();
                    
                    if (existing_names.find(type_name) == existing_names.end()) {
                        type_aliases.push_back(type_name);
                        break;
                    }
                }
            }
        }
        
        return type_aliases;
    }
};

} // namespace script_detection
} // namespace nekocode