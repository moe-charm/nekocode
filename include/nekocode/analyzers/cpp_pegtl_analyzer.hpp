#pragma once

//=============================================================================
// 🔥 C++ PEGTL Analyzer - 最終ボス戦・Claude Code支援版
//
// 完全PEGTL移行：std::regex完全撤廃（JavaScript成功パターン適用）
// テンプレート地獄・名前空間地獄・継承地獄に立ち向かう
//=============================================================================

#include "nekocode/analyzers/base_analyzer.hpp"
#include "nekocode/analyzers/cpp_minimal_grammar.hpp"
#include <tao/pegtl.hpp>
#include <vector>
#include <string>

namespace nekocode {

//=============================================================================
// 🎯 C++解析状態（JavaScript成功パターン準拠）
//=============================================================================

struct CppParseState {
    std::vector<ClassInfo> classes;
    std::vector<FunctionInfo> functions;
    std::vector<std::string> namespaces;  // C++特有
    
    // 現在の解析位置情報
    size_t current_line = 1;
    std::string current_content;
    
    void update_line_from_position(size_t pos) {
        current_line = 1;
        for (size_t i = 0; i < pos && i < current_content.size(); ++i) {
            if (current_content[i] == '\n') {
                current_line++;
            }
        }
    }
};

//=============================================================================
// 🎮 PEGTL Action System - C++特化版
//=============================================================================

template<typename Rule>
struct cpp_action : tao::pegtl::nothing<Rule> {};

// 🏛️ namespace検出
template<>
struct cpp_action<cpp::minimal_grammar::simple_namespace> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, CppParseState& state) {
        std::string matched = in.string();
        // PEGTLバージョン互換性問題回避
        // state.update_line_from_position(in.byte());
        
        // namespace name { から名前抽出
        size_t ns_pos = matched.find("namespace");
        if (ns_pos != std::string::npos) {
            size_t name_start = ns_pos + 9; // "namespace"の長さ
            while (name_start < matched.size() && std::isspace(matched[name_start])) {
                name_start++;
            }
            
            size_t name_end = name_start;
            while (name_end < matched.size() && 
                   (std::isalnum(matched[name_end]) || matched[name_end] == '_')) {
                name_end++;
            }
            
            if (name_end > name_start) {
                std::string namespace_name = matched.substr(name_start, name_end - name_start);
                state.namespaces.push_back(namespace_name);
                
                // namespaceをクラスとしても記録（統計表示用）
                ClassInfo ns_info;
                ns_info.name = "namespace:" + namespace_name;
                ns_info.start_line = state.current_line;
                state.classes.push_back(ns_info);
            }
        }
    }
};

// 🏛️ class検出
template<>
struct cpp_action<cpp::minimal_grammar::simple_class> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, CppParseState& state) {
        std::string matched = in.string();
        // PEGTLバージョン互換性問題回避
        // state.update_line_from_position(in.byte());
        
        // class Name { から名前抽出
        size_t class_pos = matched.find("class");
        if (class_pos != std::string::npos) {
            size_t name_start = class_pos + 5; // "class"の長さ
            while (name_start < matched.size() && std::isspace(matched[name_start])) {
                name_start++;
            }
            
            size_t name_end = name_start;
            while (name_end < matched.size() && 
                   (std::isalnum(matched[name_end]) || matched[name_end] == '_')) {
                name_end++;
            }
            
            if (name_end > name_start) {
                ClassInfo class_info;
                class_info.name = matched.substr(name_start, name_end - name_start);
                class_info.start_line = state.current_line;
                state.classes.push_back(class_info);
            }
        }
    }
};

// 🏗️ struct検出（C++特有）
template<>
struct cpp_action<cpp::minimal_grammar::simple_struct> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, CppParseState& state) {
        std::string matched = in.string();
        // PEGTLバージョン互換性問題回避
        // state.update_line_from_position(in.byte());
        
        // struct Name { から名前抽出
        size_t struct_pos = matched.find("struct");
        if (struct_pos != std::string::npos) {
            size_t name_start = struct_pos + 6; // "struct"の長さ
            while (name_start < matched.size() && std::isspace(matched[name_start])) {
                name_start++;
            }
            
            size_t name_end = name_start;
            while (name_end < matched.size() && 
                   (std::isalnum(matched[name_end]) || matched[name_end] == '_')) {
                name_end++;
            }
            
            if (name_end > name_start) {
                ClassInfo class_info;
                class_info.name = "struct:" + matched.substr(name_start, name_end - name_start);
                class_info.start_line = state.current_line;
                // structもclassとして扱う（C++では同等）
                state.classes.push_back(class_info);
            }
        }
    }
};

// 🎯 function検出
template<>
struct cpp_action<cpp::minimal_grammar::simple_function> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, CppParseState& state) {
        std::string matched = in.string();
        // PEGTLバージョン互換性問題回避
        // state.update_line_from_position(in.byte());
        
        // type name() { から関数名抽出
        // 戻り値型の後の最初の識別子を関数名とする
        size_t paren_pos = matched.find('(');
        if (paren_pos != std::string::npos) {
            // '(' より前の最後の識別子を探す
            size_t name_end = paren_pos;
            while (name_end > 0 && std::isspace(matched[name_end - 1])) {
                name_end--;
            }
            
            size_t name_start = name_end;
            while (name_start > 0 && 
                   (std::isalnum(matched[name_start - 1]) || matched[name_start - 1] == '_')) {
                name_start--;
            }
            
            if (name_end > name_start) {
                FunctionInfo func_info;
                func_info.name = matched.substr(name_start, name_end - name_start);
                func_info.start_line = state.current_line;
                state.functions.push_back(func_info);
            }
        }
    }
};

//=============================================================================
// 🔥 C++ PEGTL Analyzer 本体
//=============================================================================

class CppPEGTLAnalyzer : public BaseAnalyzer {
public:
    CppPEGTLAnalyzer() = default;
    ~CppPEGTLAnalyzer() = default;
    
    Language get_language() const override {
        return Language::CPP;
    }
    
    std::string get_language_name() const override {
        return "C++ (PEGTL)";
    }
    
    std::vector<std::string> get_supported_extensions() const override {
        return {".cpp", ".cxx", ".cc", ".hpp", ".hxx", ".hh", ".h++"};
    }
    
    AnalysisResult analyze(const std::string& content, const std::string& filename) override {
        AnalysisResult result;
        
        // ファイル情報設定
        result.file_info.name = filename;
        result.file_info.size_bytes = content.size();
        result.language = Language::CPP;
        
        // 行数カウント
        result.file_info.total_lines = 1 + std::count(content.begin(), content.end(), '\n');
        size_t code_lines = 0;
        bool in_comment = false;
        for (size_t i = 0; i < content.size(); ++i) {
            if (!in_comment && i + 1 < content.size() && content[i] == '/' && content[i+1] == '*') {
                in_comment = true;
                i++;
            } else if (in_comment && i + 1 < content.size() && content[i] == '*' && content[i+1] == '/') {
                in_comment = false;
                i++;
            } else if (!in_comment && content[i] == '\n') {
                // TODO: 空行とコメント行を除外するロジック追加
                code_lines++;
            }
        }
        result.file_info.code_lines = code_lines > 0 ? code_lines : result.file_info.total_lines;
        
        // 強制デバッグ: C++ PEGTL analyzer が呼ばれたことを確認
        ClassInfo debug_class;
        debug_class.name = "CPP_PEGTL_ANALYZER_CALLED";
        debug_class.start_line = 1;
        result.classes.push_back(debug_class);
        
        // PEGTL解析実行
        bool pegtl_success = false;
        try {
            CppParseState state;
            state.current_content = content;
            
            tao::pegtl::string_input input(content, filename);
            bool success = tao::pegtl::parse<cpp::minimal_grammar::cpp_minimal, 
                                          cpp_action>(input, state);
            
            if (success && (!state.classes.empty() || !state.functions.empty())) {
                // 解析結果をAnalysisResultに移動
                result.classes = std::move(state.classes);
                result.functions = std::move(state.functions);
                
                // namespaceは将来的に専用フィールド追加予定
                pegtl_success = true;
            }
            
        } catch (const tao::pegtl::parse_error& e) {
            // パースエラーは警告として記録（完全失敗ではない）
            pegtl_success = false;
        }
        
        // 🚨 PEGTL失敗時のフォールバック戦略
        if (!pegtl_success) {
            // 簡易パターンマッチング（std::regex代替）
            auto fallback_classes = extract_classes_fallback(content);
            auto fallback_functions = extract_functions_fallback(content);
            
            // デバッグクラスを保持しつつフォールバック結果を追加
            result.classes.insert(result.classes.end(), fallback_classes.begin(), fallback_classes.end());
            result.functions.insert(result.functions.end(), fallback_functions.begin(), fallback_functions.end());
        }
        
        // 複雑度計算（既存ロジック流用）
        result.complexity = calculate_cpp_complexity(content);
        
        // 統計更新
        result.update_statistics();
        
        return result;
    }

private:
    // 複雑度計算（C++特化版）
    ComplexityInfo calculate_cpp_complexity(const std::string& content) {
        ComplexityInfo complexity;
        complexity.cyclomatic_complexity = 1;
        
        // C++固有の複雑度キーワード
        std::vector<std::string> complexity_keywords = {
            "if ", "else if", "else ", "for ", "while ", "do ",
            "switch ", "case ", "catch ", "&&", "||", "? ",
            "template<", "try ", "throw ", "::"
        };
        
        for (const auto& keyword : complexity_keywords) {
            size_t pos = 0;
            while ((pos = content.find(keyword, pos)) != std::string::npos) {
                complexity.cyclomatic_complexity++;
                pos += keyword.length();
            }
        }
        
        // ネスト深度計算
        complexity.max_nesting_depth = 0;
        uint32_t current_depth = 0;
        
        for (char c : content) {
            if (c == '{') {
                current_depth++;
                if (current_depth > complexity.max_nesting_depth) {
                    complexity.max_nesting_depth = current_depth;
                }
            } else if (c == '}' && current_depth > 0) {
                current_depth--;
            }
        }
        
        complexity.update_rating();
        return complexity;
    }
    
    // 🚨 フォールバック戦略（std::regex不使用版）
    std::vector<ClassInfo> extract_classes_fallback(const std::string& content) {
        std::vector<ClassInfo> classes;
        
        // namespaceパターン検索
        size_t pos = 0;
        while ((pos = content.find("namespace ", pos)) != std::string::npos) {
            size_t name_start = pos + 10; // "namespace "の長さ
            while (name_start < content.size() && std::isspace(content[name_start])) {
                name_start++;
            }
            
            size_t name_end = name_start;
            while (name_end < content.size() && 
                   (std::isalnum(content[name_end]) || content[name_end] == '_')) {
                name_end++;
            }
            
            if (name_end > name_start) {
                ClassInfo ns_info;
                ns_info.name = "namespace:" + content.substr(name_start, name_end - name_start);
                ns_info.start_line = 1; // 簡易版
                classes.push_back(ns_info);
            }
            
            pos = name_end;
        }
        
        // classパターン検索
        pos = 0;
        while ((pos = content.find("class ", pos)) != std::string::npos) {
            size_t name_start = pos + 6; // "class "の長さ
            while (name_start < content.size() && std::isspace(content[name_start])) {
                name_start++;
            }
            
            size_t name_end = name_start;
            while (name_end < content.size() && 
                   (std::isalnum(content[name_end]) || content[name_end] == '_')) {
                name_end++;
            }
            
            if (name_end > name_start) {
                ClassInfo class_info;
                class_info.name = content.substr(name_start, name_end - name_start);
                class_info.start_line = 1; // 簡易版
                classes.push_back(class_info);
            }
            
            pos = name_end;
        }
        
        return classes;
    }
    
    std::vector<FunctionInfo> extract_functions_fallback(const std::string& content) {
        std::vector<FunctionInfo> functions;
        
        // 関数パターン検索 (identifier() { の簡易版)
        size_t pos = 0;
        while ((pos = content.find("(", pos)) != std::string::npos) {
            // '(' より前の識別子を探す
            if (pos == 0) {
                pos++;
                continue;
            }
            
            size_t name_end = pos;
            while (name_end > 0 && std::isspace(content[name_end - 1])) {
                name_end--;
            }
            
            size_t name_start = name_end;
            while (name_start > 0 && 
                   (std::isalnum(content[name_start - 1]) || content[name_start - 1] == '_')) {
                name_start--;
            }
            
            if (name_end > name_start) {
                std::string func_name = content.substr(name_start, name_end - name_start);
                
                // キーワードを除外
                if (func_name != "if" && func_name != "for" && func_name != "while" && 
                    func_name != "switch" && func_name != "return" && func_name != "sizeof") {
                    
                    FunctionInfo func_info;
                    func_info.name = func_name;
                    func_info.start_line = 1; // 簡易版
                    functions.push_back(func_info);
                }
            }
            
            pos++;
        }
        
        return functions;
    }
};

} // namespace nekocode