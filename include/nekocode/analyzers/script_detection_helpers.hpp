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
        
        // クラスパターン（JS/TS共通）
        std::regex class_patterns[] = {
            std::regex(R"(^\s*class\s+(\w+)\s*(?:extends\s+(\w+))?\s*\{)"),         // class Name extends Base {
            std::regex(R"(^\s*export\s+class\s+(\w+)\s*(?:extends\s+(\w+))?\s*\{)"), // export class Name {
            std::regex(R"(^\s*export\s+default\s+class\s+(\w+)\s*(?:extends\s+(\w+))?\s*\{)") // export default class Name {
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