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
#include <regex>
#include <set>
#include <sstream>
#include <fstream>

// 🔧 グローバルデバッグフラグ（analyzer_factory.cppで定義済み）
extern bool g_debug_mode;

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
        std::cerr << "DEBUG: Found class header: " << decl << std::endl;
        
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
                std::cerr << "DEBUG: Extracted class name: " << class_info.name << std::endl;
            }
        }
    }
};

// 🚀 新文法対応: 通常メソッド検出
template<>
struct action<csharp::minimal_grammar::normal_method> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, CSharpParseState& state) {
        FunctionInfo method_info;
        std::string decl = in.string();
        std::cerr << "DEBUG: Found normal method: " << decl << std::endl;
        
        // パラメータリストの前の識別子を探す（改良版）
        size_t paren_pos = decl.find('(');
        if (paren_pos != std::string::npos) {
            // 型名の後の識別子を探す（より精密）
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
                std::cerr << "DEBUG: Extracted normal method name: " << method_info.name << std::endl;
            }
        }
    }
};

// 🚀 新文法対応: コンストラクタ検出
template<>
struct action<csharp::minimal_grammar::constructor> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, CSharpParseState& state) {
        FunctionInfo constructor_info;
        std::string decl = in.string();
        std::cerr << "DEBUG: Found constructor: " << decl << std::endl;
        
        // コンストラクタ名を抽出（修飾子の後の最初の識別子）
        size_t paren_pos = decl.find('(');
        if (paren_pos != std::string::npos) {
            // 修飾子をスキップして識別子を探す
            std::string temp = decl.substr(0, paren_pos);
            
            // 右から左に最後の識別子を探す
            size_t name_end = temp.length();
            while (name_end > 0 && std::isspace(temp[name_end - 1])) {
                name_end--;
            }
            size_t name_start = name_end;
            while (name_start > 0 && 
                   (std::isalnum(temp[name_start - 1]) || temp[name_start - 1] == '_')) {
                name_start--;
            }
            if (name_start < name_end) {
                constructor_info.name = temp.substr(name_start, name_end - name_start) + "()"; // コンストラクタ明示
                constructor_info.start_line = state.current_line;
                state.current_methods.push_back(constructor_info);
                std::cerr << "DEBUG: Extracted constructor name: " << constructor_info.name << std::endl;
            }
        }
    }
};

// 🚀 新文法対応: プロパティ（=>記法）検出
template<>
struct action<csharp::minimal_grammar::property_arrow> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, CSharpParseState& state) {
        FunctionInfo property_info;
        std::string decl = in.string();
        std::cerr << "DEBUG: Found property (arrow): " << decl << std::endl;
        
        // =>の前の識別子を探す
        size_t arrow_pos = decl.find("=>");
        if (arrow_pos != std::string::npos) {
            std::string before_arrow = decl.substr(0, arrow_pos);
            
            // 型名の後の識別子を探す
            size_t name_end = before_arrow.length();
            while (name_end > 0 && std::isspace(before_arrow[name_end - 1])) {
                name_end--;
            }
            size_t name_start = name_end;
            while (name_start > 0 && 
                   (std::isalnum(before_arrow[name_start - 1]) || before_arrow[name_start - 1] == '_')) {
                name_start--;
            }
            if (name_start < name_end) {
                property_info.name = "property:" + before_arrow.substr(name_start, name_end - name_start);
                property_info.start_line = state.current_line;
                state.current_methods.push_back(property_info);
                std::cerr << "DEBUG: Extracted property (arrow) name: " << property_info.name << std::endl;
            }
        }
    }
};

// 🚀 新文法対応: プロパティ（get/set記法）検出
template<>
struct action<csharp::minimal_grammar::property_getset> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, CSharpParseState& state) {
        FunctionInfo property_info;
        std::string decl = in.string();
        std::cerr << "DEBUG: Found property (get/set): " << decl << std::endl;
        
        // {の前の識別子を探す
        size_t brace_pos = decl.find('{');
        if (brace_pos != std::string::npos) {
            std::string before_brace = decl.substr(0, brace_pos);
            
            // 型名の後の識別子を探す
            size_t name_end = before_brace.length();
            while (name_end > 0 && std::isspace(before_brace[name_end - 1])) {
                name_end--;
            }
            size_t name_start = name_end;
            while (name_start > 0 && 
                   (std::isalnum(before_brace[name_start - 1]) || before_brace[name_start - 1] == '_')) {
                name_start--;
            }
            if (name_start < name_end) {
                property_info.name = "property:" + before_brace.substr(name_start, name_end - name_start);
                property_info.start_line = state.current_line;
                state.current_methods.push_back(property_info);
                std::cerr << "DEBUG: Extracted property (get/set) name: " << property_info.name << std::endl;
            }
        }
    }
};

// 🔄 レガシー互換: 既存method_declも維持（後方互換性）
template<>
struct action<csharp::minimal_grammar::method_decl> {
    template<typename ParseInput>
    static void apply(const ParseInput& /*in*/, CSharpParseState& /*state*/) {
        // 新文法では個別のアクションが処理するため、ここは空でOK
        if (g_debug_mode) {
            std::cerr << "DEBUG: method_decl triggered (handled by specific actions)" << std::endl;
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
        std::cerr << "DEBUG: CSharpPEGTLAnalyzer constructor called" << std::endl;
    }
    virtual ~CSharpPEGTLAnalyzer() = default;
    
    Language get_language() const override {
        return Language::CSHARP;
    }
    
    std::string get_language_name() const override {
        std::cerr << "DEBUG: CSharpPEGTLAnalyzer::get_language_name() called" << std::endl;
        return "C# (PEGTL)";
    }
    
    std::vector<std::string> get_supported_extensions() const override {
        return {".cs", ".csx"};
    }
    
    AnalysisResult analyze(const std::string& content, const std::string& filename) override {
        std::cerr << "DEBUG: CSharpPEGTLAnalyzer::analyze() called for " << filename << std::endl;
        
        // 🚀 デバッグファイル初期化（新しい解析開始）
        {
            std::ofstream debug_file("/tmp/csharp_regex_debug.txt", std::ios::trunc);  // trunc=上書き
            debug_file << "🚀 C# REGEX DEBUG SESSION STARTED 🚀\n";
            debug_file << "Analyzing file: " << filename << "\n";
            debug_file << "Content length: " << content.length() << " bytes\n";
        }
        
        CSharpParseState state;
        state.result.file_info.name = filename;
        state.result.file_info.size_bytes = content.size();
        state.result.language = Language::CSHARP;
        
        try {
            // PEGTL解析実行
            std::cerr << "DEBUG: Starting PEGTL parse for " << filename << std::endl;
            std::cerr << "DEBUG: Content length: " << content.length() << " bytes" << std::endl;
            tao::pegtl::string_input input(content, filename);
            bool parse_success = tao::pegtl::parse<csharp::minimal_grammar::csharp_minimal, csharp_actions::action>(input, state);
            std::cerr << "DEBUG: Parse result: " << (parse_success ? "SUCCESS" : "FAILED") << std::endl;
            
            // 結果を統合
            state.result.classes = std::move(state.current_classes);
            state.result.functions = std::move(state.current_methods);
            state.result.imports = std::move(state.imports);
            
        } catch (const tao::pegtl::parse_error& e) {
            // パースエラー処理（エラーログを出力して空の結果を返す）
            std::cerr << "PEGTL parse error: " << e.what() << std::endl;
            // 部分的な結果でも返す
        }
        
        // 複雑度計算（ハイブリッド戦略の前に実行）
        state.result.complexity = calculate_complexity(content);
        
        // 🚀 C#ハイブリッド戦略: JavaScript/TypeScript/C++成功パターン移植
        if (needs_csharp_line_based_fallback(state.result, content)) {
            std::cerr << "🔥 C# Hybrid Strategy TRIGGERED!" << std::endl;
            apply_csharp_line_based_analysis(state.result, content, filename);
            std::cerr << "✅ C# Line-based analysis completed. Classes: " << state.result.classes.size() 
                      << ", Functions: " << state.result.functions.size() << std::endl;
        } else {
            std::cerr << "⚠️  C# Hybrid Strategy NOT triggered" << std::endl;
        }
        
        // 🎯 メンバ変数検出（C++/Python/JS/TSと同じパターン）
        detect_member_variables(state.result, content);
        
        // 統計情報更新
        state.result.update_statistics();
        
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

private:
    // 🚀 C#ハイブリッド戦略: 統計整合性チェック（JavaScript成功パターン移植）
    bool needs_csharp_line_based_fallback(const AnalysisResult& result, const std::string& content) {
        // JavaScript戦略と同様: 複雑度 vs 検出数の妥当性検証
        uint32_t complexity = result.complexity.cyclomatic_complexity;
        size_t detected_classes = result.classes.size();
        size_t detected_functions = result.functions.size();
        
        // デバッグ出力
        std::cerr << "🔍 Debug: complexity=" << complexity 
                  << ", detected_classes=" << detected_classes
                  << ", detected_functions=" << detected_functions << std::endl;
        bool has_class = content.find("class ") != std::string::npos;
        bool has_namespace = content.find("namespace ") != std::string::npos;
        bool has_interface = content.find("interface ") != std::string::npos;
        std::cerr << "🔍 Debug: has_class=" << has_class 
                  << ", has_namespace=" << has_namespace 
                  << ", has_interface=" << has_interface << std::endl;
        
        // C#特化閾値: C#は規則正しいので、C++より厳しい閾値
        if (complexity > 30 && detected_classes == 0 && detected_functions < 3) {
            std::cerr << "📊 Trigger reason: High complexity with no detection (C# specific)" << std::endl;
            return true;
        }
        
        // 複雑度100以上で関数検出0は絶対におかしい
        if (complexity > 100 && detected_functions == 0) {
            std::cerr << "📊 Trigger reason: Very high complexity with no functions" << std::endl;
            return true;
        }
        
        // C#特有パターンがあるのに検出できていない場合
        if ((has_class || has_namespace || has_interface) && detected_classes == 0) {
            std::cerr << "📊 Trigger reason: C# patterns found but no classes detected" << std::endl;
            return true;
        }
        
        std::cerr << "❌ No trigger conditions met" << std::endl;
        return false;
    }
    
    // 🚀 C#ハイブリッド戦略: 行ベース補完解析（JavaScript成功パターン移植）
    void apply_csharp_line_based_analysis(AnalysisResult& result, const std::string& content, const std::string& filename) {
        std::istringstream stream(content);
        std::string line;
        size_t line_number = 1;
        
        // 既存の要素名を記録（重複検出を防ぐ - JavaScript成功パターン）
        std::set<std::string> existing_classes;
        std::set<std::string> existing_functions;
        
        for (const auto& cls : result.classes) {
            existing_classes.insert(cls.name);
        }
        for (const auto& func : result.functions) {
            existing_functions.insert(func.name);
        }
        
        // C#特化の行ベース解析
        while (std::getline(stream, line)) {
            extract_csharp_elements_from_line(line, line_number, result, existing_classes, existing_functions);
            line_number++;
        }
    }
    
    // C#要素の行ベース抽出（JavaScript正規表現パターン移植＋C#特化）
    void extract_csharp_elements_from_line(const std::string& line, size_t line_number,
                                           AnalysisResult& result, 
                                           std::set<std::string>& existing_classes,
                                           std::set<std::string>& existing_functions) {
        
        // 🚀 デバッグファイル出力（詳細マッチング調査用）
        static std::ofstream debug_file("/tmp/csharp_regex_debug.txt", std::ios::app);
        debug_file << "\n=== LINE " << line_number << " ===\n";
        debug_file << "Content: [" << line << "]\n";
        
        // パターン1: public class ClassName
        std::regex class_pattern(R"(^\s*(?:public|internal|private|protected)?\s*(?:static|sealed|abstract)?\s*class\s+(\w+))");
        std::smatch match;
        
        debug_file << "Testing class_pattern... ";
        if (std::regex_search(line, match, class_pattern)) {
            std::string class_name = match[1].str();
            debug_file << "MATCHED! class_name=[" << class_name << "]\n";
            if (existing_classes.find(class_name) == existing_classes.end()) {
                ClassInfo class_info;
                class_info.name = class_name;
                class_info.start_line = line_number;
                result.classes.push_back(class_info);
                existing_classes.insert(class_name);
                debug_file << "Added new class: " << class_name << "\n";
            } else {
                debug_file << "Class already exists, skipped\n";
            }
        } else {
            debug_file << "NO MATCH\n";
        }
        
        // パターン2: namespace CompanyName.ProductName
        std::regex namespace_pattern(R"(^\s*namespace\s+([\w\.]+))");
        if (std::regex_search(line, match, namespace_pattern)) {
            std::string ns_name = match[1].str();
            if (existing_classes.find("namespace:" + ns_name) == existing_classes.end()) {
                ClassInfo ns_info;
                ns_info.name = "namespace:" + ns_name;
                ns_info.start_line = line_number;
                result.classes.push_back(ns_info);
                existing_classes.insert("namespace:" + ns_name);
            }
        }
        
        // パターン3: public interface IInterfaceName
        std::regex interface_pattern(R"(^\s*(?:public|internal)?\s*interface\s+(\w+))");
        if (std::regex_search(line, match, interface_pattern)) {
            std::string interface_name = match[1].str();
            if (existing_classes.find("interface:" + interface_name) == existing_classes.end()) {
                ClassInfo interface_info;
                interface_info.name = "interface:" + interface_name;
                interface_info.start_line = line_number;
                result.classes.push_back(interface_info);
                existing_classes.insert("interface:" + interface_name);
            }
        }
        
        // パターン3.5: public struct StructName（structもクラスとして扱う）
        std::regex struct_pattern(R"(^\s*(?:public|internal|private)?\s*struct\s+(\w+))");
        if (std::regex_search(line, match, struct_pattern)) {
            std::string struct_name = match[1].str();
            if (existing_classes.find(struct_name) == existing_classes.end()) {
                ClassInfo struct_info;
                struct_info.name = struct_name;
                struct_info.start_line = line_number;
                result.classes.push_back(struct_info);
                existing_classes.insert(struct_name);
            }
        }
        
        // パターン4: public enum EnumName
        std::regex enum_pattern(R"(^\s*(?:public|internal)?\s*enum\s+(\w+))");
        if (std::regex_search(line, match, enum_pattern)) {
            std::string enum_name = match[1].str();
            if (existing_classes.find("enum:" + enum_name) == existing_classes.end()) {
                ClassInfo enum_info;
                enum_info.name = "enum:" + enum_name;
                enum_info.start_line = line_number;
                result.classes.push_back(enum_info);
                existing_classes.insert("enum:" + enum_name);
            }
        }
        
        // 🚀 パターン5A: コンストラクタ（戻り値型なし） - より柔軟に
        std::regex constructor_pattern(R"(\s+(?:public|private|protected|internal)?\s*(?:static\s+)?(\w+)\s*\([^)]*\)\s*(?:\{|$))");
        debug_file << "Testing constructor_pattern... ";
        if (std::regex_search(line, match, constructor_pattern)) {
            std::string constructor_name = match[1].str();
            debug_file << "MATCHED! constructor_name=[" << constructor_name << "]\n";
            
            // コンストラクタ判定: キーワードでないこと（拡張版）
            if (constructor_name != "if" && constructor_name != "for" && constructor_name != "while" && 
                constructor_name != "switch" && constructor_name != "return" && constructor_name != "using" &&
                constructor_name != "namespace" && constructor_name != "class" && constructor_name != "interface" &&
                constructor_name != "enum" && constructor_name != "struct" && constructor_name != "get" && constructor_name != "set" &&
                constructor_name != "public" && constructor_name != "private" && constructor_name != "protected" && constructor_name != "internal" &&
                constructor_name != "catch" && constructor_name != "try" && constructor_name != "finally" && constructor_name != "throw" &&
                constructor_name != "void" && constructor_name != "int" && constructor_name != "string" && constructor_name != "bool") {
                
                debug_file << "Constructor name validated (not a keyword)\n";
                if (existing_functions.find(constructor_name + "()") == existing_functions.end()) {
                    FunctionInfo constructor_info;
                    constructor_info.name = constructor_name + "()";  // コンストラクタ明示
                    constructor_info.start_line = line_number;
                    result.functions.push_back(constructor_info);
                    existing_functions.insert(constructor_name + "()");
                    debug_file << "Added new constructor: " << constructor_name << "()\n";
                } else {
                    debug_file << "Constructor already exists, skipped\n";
                }
            } else {
                debug_file << "Constructor name is a keyword, rejected: " << constructor_name << "\n";
            }
        } else {
            debug_file << "NO MATCH\n";
        }
        
        // 🚀 パターン5B: 通常メソッド（戻り値型あり・順序柔軟） - より柔軟に
        std::regex method_pattern(R"(\s+(?:public|private|protected|internal)?\s*(?:static\s+)?(?:async\s+)?(?:virtual\s+)?(?:override\s+)?(?:sealed\s+)?([^\s\(]+(?:<[^>]*>)?)\s+(\w+)(?:<[^>]*>)?\s*\([^)]*\)\s*(?:\{|$))");
        debug_file << "Testing method_pattern... ";
        if (std::regex_search(line, match, method_pattern)) {
            std::string method_name = match[2].str();  // 戻り値型追加でindex変更
            debug_file << "MATCHED! method_name=[" << method_name << "] return_type=[" << match[1].str() << "]\n";
            
            // C#キーワード除外
            if (method_name != "if" && method_name != "for" && method_name != "while" && 
                method_name != "switch" && method_name != "return" && method_name != "using" &&
                method_name != "namespace" && method_name != "class" && method_name != "interface" &&
                method_name != "enum" && method_name != "struct" && method_name != "get" && method_name != "set" &&
                method_name != "public" && method_name != "private" && method_name != "protected" && method_name != "internal" &&
                method_name != "static" && method_name != "async" && method_name != "virtual" && method_name != "override") {
                
                debug_file << "Method name validated (not a keyword)\n";
                if (existing_functions.find(method_name) == existing_functions.end()) {
                    FunctionInfo method_info;
                    method_info.name = method_name;
                    method_info.start_line = line_number;
                    result.functions.push_back(method_info);
                    existing_functions.insert(method_name);
                    debug_file << "Added new method: " << method_name << "\n";
                } else {
                    debug_file << "Method already exists, skipped\n";
                }
            } else {
                debug_file << "Method name is a keyword, rejected: " << method_name << "\n";
            }
        } else {
            debug_file << "NO MATCH\n";
        }
        
        // パターン6A: プロパティ定義 ({get/set形式)
        std::regex property_pattern(R"(^\s*(?:public|private|protected|internal)?\s*(?:static|virtual|override)?\s*\w+\s+(\w+)\s*\{\s*(?:get|set))");
        
        // パターン6B: プロパティ定義 (=>形式)
        std::regex property_arrow_pattern(R"(^\s*(?:public|private|protected|internal)?\s*(?:static|virtual|override)?\s*\w+\s+(\w+)\s*=>\s*)");
        debug_file << "Testing property_pattern (get/set)... ";
        if (std::regex_search(line, match, property_pattern)) {
            std::string property_name = match[1].str();
            debug_file << "MATCHED! property_name=[" << property_name << "]\n";
            if (existing_functions.find("property:" + property_name) == existing_functions.end()) {
                FunctionInfo property_info;
                property_info.name = "property:" + property_name;
                property_info.start_line = line_number;
                result.functions.push_back(property_info);
                existing_functions.insert("property:" + property_name);
                debug_file << "Added new property: " << property_name << "\n";
            } else {
                debug_file << "Property already exists, skipped\n";
            }
        } else {
            debug_file << "NO MATCH\n";
        }
        
        // パターン6B追加: =>形式プロパティテスト
        debug_file << "Testing property_arrow_pattern (=>) ... ";
        if (std::regex_search(line, match, property_arrow_pattern)) {
            std::string property_name = match[1].str();
            debug_file << "MATCHED! arrow_property_name=[" << property_name << "]\n";
            if (existing_functions.find("property:" + property_name) == existing_functions.end()) {
                FunctionInfo property_info;
                property_info.name = "property:" + property_name;
                property_info.start_line = line_number;
                result.functions.push_back(property_info);
                existing_functions.insert("property:" + property_name);
                debug_file << "Added new arrow property: " << property_name << "\n";
            } else {
                debug_file << "Arrow property already exists, skipped\n";
            }
        } else {
            debug_file << "NO MATCH\n";
        }
        
        // デバッグファイルをflush（即座に書き込み）
        debug_file.flush();
    }
    
    // 🎯 C#メンバ変数検出（C++/Python/JS/TSと同じパターン）
    void detect_member_variables(AnalysisResult& result, const std::string& content) {
        if (g_debug_mode) {
            std::cerr << "🔥 C# detect_member_variables called with " << result.classes.size() << " classes" << std::endl;
        }
        
        std::istringstream stream(content);
        std::string line;
        size_t line_number = 0;
        
        // 各クラスに対してメンバ変数を検出
        for (auto& cls : result.classes) {
            // interface, namespace, enumはスキップ
            if (cls.name.find("interface:") == 0 || 
                cls.name.find("namespace:") == 0 || 
                cls.name.find("enum:") == 0) continue;
            
            if (g_debug_mode) {
                std::cerr << "🔍 Detecting member variables for class: " << cls.name << std::endl;
            }
            
            // クラス内のメンバ変数を検出
            stream.clear();
            stream.seekg(0);
            line_number = 0;
            bool in_class = false;
            int brace_depth = 0;
            std::string access_modifier = "private"; // C#のデフォルトはprivate
            
            while (std::getline(stream, line)) {
                line_number++;
                
                // クラス定義の開始を検出
                if (line_number == cls.start_line) {
                    in_class = true;
                    if (line.find("{") != std::string::npos) {
                        brace_depth = 1;
                    }
                    continue;
                }
                
                // デバッグ: GenericClassの53行目を特別に監視
                if (g_debug_mode && cls.name == "GenericClass" && line_number == 53) {
                    std::cerr << "🔎 Line 53 in GenericClass: '" << line << "'" << std::endl;
                    std::cerr << "🔎 in_class: " << in_class << ", brace_depth: " << brace_depth << std::endl;
                    // この行の処理を詳しく追跡
                    std::cerr << "🔎 Processing line 53..." << std::endl;
                }
                
                if (!in_class) continue;
                
                // クラスの終了を検出（簡易版：ブレース数で判定）
                for (char c : line) {
                    if (c == '{') brace_depth++;
                    else if (c == '}') {
                        brace_depth--;
                        if (brace_depth == 0) {
                            in_class = false;
                            break;
                        }
                    }
                }
                
                if (!in_class) break;
                
                // コメント行をスキップ
                std::string trimmed_line = line;
                trimmed_line.erase(0, trimmed_line.find_first_not_of(" \t"));
                if (trimmed_line.empty() || 
                    trimmed_line.find("//") == 0 || 
                    trimmed_line.find("/*") == 0) {
                    if (g_debug_mode && cls.name == "GenericClass" && line_number == 53) {
                        std::cerr << "🔎 Line 53: Skipped as comment" << std::endl;
                    }
                    continue;
                }
                
                // メソッド定義をスキップ（括弧がある行）- ただしメンバ変数の初期化は除外
                if (line.find("(") != std::string::npos) {
                    // セミコロンで終わる行は初期化付きメンバ変数の可能性があるのでスキップしない
                    if (line.find(";") == std::string::npos) {
                        if (g_debug_mode && cls.name == "GenericClass" && line_number == 53) {
                            std::cerr << "🔎 Line 53: Skipped as method (contains parentheses, no semicolon)" << std::endl;
                        }
                        continue;
                    } else {
                        if (g_debug_mode && cls.name == "GenericClass" && line_number == 53) {
                            std::cerr << "🔎 Line 53: Parentheses found but has semicolon, continuing as potential member variable" << std::endl;
                        }
                    }
                }
                
                // プロパティ定義をスキップ（{ get; set; }形式）
                if (line.find("get") != std::string::npos && 
                    (line.find("set") != std::string::npos || line.find("}") != std::string::npos)) {
                    if (g_debug_mode && cls.name == "GenericClass" && line_number == 53) {
                        std::cerr << "🔎 Line 53: Skipped as property (get/set)" << std::endl;
                    }
                    continue;
                }
                
                // =>形式のプロパティをスキップ
                if (line.find("=>") != std::string::npos) {
                    if (g_debug_mode && cls.name == "GenericClass" && line_number == 53) {
                        std::cerr << "🔎 Line 53: Skipped as arrow property" << std::endl;
                    }
                    continue;
                }
                
                // returnステートメントをスキップ
                if (line.find("return") != std::string::npos) {
                    if (g_debug_mode && cls.name == "GenericClass" && line_number == 53) {
                        std::cerr << "🔎 Line 53: Skipped as return statement" << std::endl;
                    }
                    continue;
                }
                
                // C#のメンバ変数パターン - より厳密に
                // 例: private string name;  public static int count = 0;  readonly DateTime date;
                // 例: private List<T> items = new List<T>();
                
                // まず代入文（name = value;）を除外チェック
                std::regex assignment_pattern(R"(^\s*\w+\s*=\s*)");
                if (std::regex_search(line, assignment_pattern) && 
                    line.find("static") == std::string::npos &&
                    line.find("private") == std::string::npos &&
                    line.find("public") == std::string::npos &&
                    line.find("protected") == std::string::npos &&
                    line.find("internal") == std::string::npos) {
                    continue; // 単純な代入文はスキップ
                }
                
                std::regex member_var_pattern(
                    R"(^\s*(?:(public|private|protected|internal)\s+)?)"       // アクセス修飾子
                    R"((?:static\s+)?(?:readonly\s+)?(?:const\s+)?)"          // 修飾子
                    R"((?:[\w\.\<\>,\s]+(?:\s*\[\s*\])?)\s+)"                // 型（ジェネリック・配列・複雑な型対応）
                    R"((\w+))"                                                 // 変数名
                    R"(\s*(?:=\s*[^;]+)?\s*;)"                               // 初期化子
                );
                
                // デバッグ: List<T>型の行を特別にチェック
                if (g_debug_mode && line.find("List<") != std::string::npos) {
                    std::cerr << "🔎 Checking List<T> line: '" << line << "'" << std::endl;
                }
                
                std::smatch var_match;
                if (std::regex_search(line, var_match, member_var_pattern)) {
                    std::string var_name = var_match[2].str();
                    std::string var_access = var_match[1].str();
                    if (!var_access.empty()) {
                        access_modifier = var_access;
                    }
                    
                    if (g_debug_mode) {
                        std::cerr << "🎯 Found member variable: " << var_name 
                                  << " in class " << cls.name 
                                  << " at line " << line_number << std::endl;
                        std::cerr << "    📝 Line content: '" << line << "'" << std::endl;
                    }
                    
                    // メンバ変数情報を作成
                    MemberVariable member_var;
                    member_var.name = var_name;
                    member_var.declaration_line = line_number;
                    member_var.access_modifier = access_modifier;
                    
                    // 型を推定
                    size_t type_end = line.find(var_name);
                    if (type_end != std::string::npos) {
                        std::string type_part = line.substr(0, type_end);
                        // アクセス修飾子と変数修飾子を除去
                        type_part = std::regex_replace(type_part, std::regex(R"(^\s*(public|private|protected|internal)\s+)"), "");
                        type_part = std::regex_replace(type_part, std::regex(R"(^\s*static\s+)"), "");
                        type_part = std::regex_replace(type_part, std::regex(R"(^\s*readonly\s+)"), "");
                        type_part = std::regex_replace(type_part, std::regex(R"(^\s*const\s+)"), "");
                        // 前後の空白を除去
                        type_part = std::regex_replace(type_part, std::regex(R"(^\s+|\s+$)"), "");
                        member_var.type = type_part;
                    }
                    
                    // static/const/readonlyフラグを設定
                    member_var.is_static = (line.find("static") != std::string::npos);
                    member_var.is_const = (line.find("const") != std::string::npos || 
                                          line.find("readonly") != std::string::npos);
                    
                    cls.member_variables.push_back(member_var);
                }
            }
        }
    }
};

} // namespace nekocode