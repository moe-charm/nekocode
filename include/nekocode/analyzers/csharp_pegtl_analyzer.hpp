#pragma once

//=============================================================================
// 🌟 C# PEGTL Analyzer - 革新的PEGベース解析エンジン
//
// std::regexからの完全移行を実現
// 高速・正確・拡張可能な新世代解析
//=============================================================================

#include "base_analyzer.hpp"
#include "csharp_minimal_grammar.hpp"
#include <tao/pegtl.hpp>
#include <stack>
#include <iostream>

namespace nekocode {

//=============================================================================
// 🎯 解析状態管理
//=============================================================================

struct CSharpParseState {
    AnalysisResult result;
    std::vector<ClassInfo> current_classes;
    std::vector<FunctionInfo> current_methods;
    std::vector<ImportInfo> imports;
    
    // 現在の解析コンテキスト
    std::string current_namespace;
    std::stack<ClassInfo*> class_stack;
    std::stack<uint32_t> line_stack;
    
    // 行番号追跡
    uint32_t current_line = 1;
    
    void update_line(const char* from, const char* to) {
        while (from != to) {
            if (*from == '\n') current_line++;
            ++from;
        }
    }
};

//=============================================================================
// 🎯 PEGTLアクション定義
//=============================================================================

namespace csharp_actions {

using namespace tao::pegtl;

// デフォルトアクション（何もしない）
template<typename Rule>
struct action : nothing<Rule> {};

// クラスヘッダーのアクション
template<>
struct action<csharp::minimal_grammar::class_header> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, CSharpParseState& state) {
        ClassInfo class_info;
        std::string decl = in.string();
        std::cout << "DEBUG: Found class header: " << decl << std::endl;
        
        // "class"の後の識別子を抽出
        size_t class_pos = decl.find("class");
        if (class_pos != std::string::npos) {
            size_t name_start = decl.find_first_not_of(" \t", class_pos + 5);
            if (name_start != std::string::npos) {
                std::string class_name = decl.substr(name_start);
                // 空白で終わる場合は削除
                size_t name_end = class_name.find_first_of(" \t\n\r{");
                if (name_end != std::string::npos) {
                    class_name = class_name.substr(0, name_end);
                }
                class_info.name = class_name;
                class_info.start_line = state.current_line;
                state.current_classes.push_back(class_info);
                std::cout << "DEBUG: Extracted class name: " << class_info.name << std::endl;
            }
        }
    }
};

// メソッド宣言のアクション
template<>
struct action<csharp::minimal_grammar::method_decl> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, CSharpParseState& state) {
        FunctionInfo method_info;
        std::string decl = in.string();
        std::cout << "DEBUG: Found method declaration: " << decl << std::endl;
        
        // パラメータリストの前の識別子を探す
        size_t paren_pos = decl.find('(');
        if (paren_pos != std::string::npos) {
            // 右から左に識別子を探す
            size_t name_end = paren_pos;
            while (name_end > 0 && std::isspace(decl[name_end - 1])) {
                name_end--;
            }
            size_t name_start = name_end;
            while (name_start > 0 && 
                   (std::isalnum(decl[name_start - 1]) || decl[name_start - 1] == '_')) {
                name_start--;
            }
            if (name_start < name_end) {
                method_info.name = decl.substr(name_start, name_end - name_start);
                method_info.start_line = state.current_line;
                state.current_methods.push_back(method_info);
                std::cout << "DEBUG: Extracted method name: " << method_info.name << std::endl;
            }
        }
    }
};

} // namespace csharp_actions

//=============================================================================
// 🚀 CSharpPEGTLAnalyzer - PEGTL実装
//=============================================================================

class CSharpPEGTLAnalyzer : public BaseAnalyzer {
public:
    CSharpPEGTLAnalyzer() {
        std::cout << "DEBUG: CSharpPEGTLAnalyzer constructor called" << std::endl;
    }
    virtual ~CSharpPEGTLAnalyzer() = default;
    
    Language get_language() const override {
        return Language::CSHARP;
    }
    
    std::string get_language_name() const override {
        std::cout << "DEBUG: CSharpPEGTLAnalyzer::get_language_name() called" << std::endl;
        return "C# (PEGTL)";
    }
    
    std::vector<std::string> get_supported_extensions() const override {
        return {".cs", ".csx"};
    }
    
    AnalysisResult analyze(const std::string& content, const std::string& filename) override {
        std::cout << "DEBUG: CSharpPEGTLAnalyzer::analyze() called for " << filename << std::endl;
        CSharpParseState state;
        state.result.file_info.name = filename;
        state.result.file_info.size_bytes = content.size();
        state.result.language = Language::CSHARP;
        
        try {
            // PEGTL解析実行
            std::cout << "DEBUG: Starting PEGTL parse for " << filename << std::endl;
            std::cout << "DEBUG: Content length: " << content.length() << " bytes" << std::endl;
            tao::pegtl::string_input input(content, filename);
            bool parse_success = tao::pegtl::parse<csharp::minimal_grammar::csharp_minimal, csharp_actions::action>(input, state);
            std::cout << "DEBUG: Parse result: " << (parse_success ? "SUCCESS" : "FAILED") << std::endl;
            
            // 結果を統合
            state.result.classes = std::move(state.current_classes);
            state.result.functions = std::move(state.current_methods);
            state.result.imports = std::move(state.imports);
            
            // 統計情報更新
            state.result.update_statistics();
            
            // 複雑度計算（基底クラスのデフォルト実装を使用）
            state.result.complexity = calculate_complexity(content);
            
        } catch (const tao::pegtl::parse_error& e) {
            // パースエラー処理（エラーログを出力して空の結果を返す）
            std::cerr << "PEGTL parse error: " << e.what() << std::endl;
            // 部分的な結果でも返す
        }
        
        return state.result;
    }
    
protected:
    // C#固有の複雑度計算（オーバーライド可能）
    ComplexityInfo calculate_complexity(const std::string& content) override {
        ComplexityInfo complexity = BaseAnalyzer::calculate_complexity(content);
        
        // C#固有のキーワード追加
        std::vector<std::string> csharp_keywords = {
            "async", "await", "yield", "lock", "using", "foreach", "?.", "??", "?["
        };
        
        for (const auto& keyword : csharp_keywords) {
            size_t pos = 0;
            while ((pos = content.find(keyword, pos)) != std::string::npos) {
                complexity.cyclomatic_complexity++;
                pos += keyword.length();
            }
        }
        
        complexity.update_rating();
        return complexity;
    }
};

} // namespace nekocode