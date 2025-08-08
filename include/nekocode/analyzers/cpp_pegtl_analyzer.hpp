#pragma once

//=============================================================================
// 🔥 C++ PEGTL Analyzer - 最終ボス戦・Claude Code支援版
//
// 完全PEGTL移行：std::regex完全撤廃（JavaScript成功パターン適用）
// テンプレート地獄・名前空間地獄・継承地獄に立ち向かう

// 🚨 一時的にregex有効化（メンバ変数検出のため）
#define NEKOCODE_FOUNDATION_CORE_CPP
//=============================================================================

#include "base_analyzer.hpp"
#include "nekocode/analyzers/cpp_minimal_grammar.hpp"
#include "nekocode/debug_logger.hpp"
// 🚀 Phase 5: Universal Symbol直接生成
#include "nekocode/universal_symbol.hpp"
#include "nekocode/symbol_table.hpp"
#include <tao/pegtl.hpp>
#include <vector>
#include <string>
#include <regex>
#include <set>
#include <sstream>
#include <iostream>
#include <chrono>
#include <execution>  // 並列処理用
#include <algorithm>  // std::for_each
#include <mutex>      // スレッドセーフ用
#include <atomic>     // 原子操作用

// 🔧 グローバルデバッグフラグ（analyzer_factory.cppで定義済み）
extern bool g_debug_mode;
extern bool g_quiet_mode;

// 🐛 デバッグ出力マクロ（--debugフラグがある時のみ出力）
#define DEBUG_LOG(msg) do { if (g_debug_mode) { std::cerr << msg << std::endl; } } while(0)

// 🔇 サイレント出力マクロ（Claude Code用：--quietフラグでstderr出力抑制）
#define STDERR_LOG(msg) do { if (!g_quiet_mode) { std::cerr << msg << std::endl; } } while(0)

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
    
    // 🚀 Phase 5: Universal Symbol直接生成
    std::shared_ptr<SymbolTable> symbol_table;      // Universal Symbolテーブル
    std::unordered_map<std::string, int> id_counters; // ID生成用カウンター
    
    // コンストラクタ
    CppParseState() {
        // 🚀 Phase 5: Universal Symbol初期化
        symbol_table = std::make_shared<SymbolTable>();
    }
    
    // 🚀 Phase 5: Universal Symbol生成メソッド
    std::string generate_unique_id(const std::string& base) {
        id_counters[base]++;
        return base + "_" + std::to_string(id_counters[base] - 1);
    }
    
    void add_test_class_symbol(const std::string& class_name, std::uint32_t start_line) {
        UniversalSymbolInfo symbol;
        symbol.symbol_id = generate_unique_id("class_" + class_name);
        symbol.symbol_type = SymbolType::CLASS;
        symbol.name = class_name;
        symbol.start_line = start_line;
        symbol.metadata["language"] = "cpp";
        
#ifdef NEKOCODE_DEBUG_SYMBOLS
        std::cerr << "[Phase 5 Test] C++ adding class symbol: " << class_name 
                  << " with ID: " << symbol.symbol_id << std::endl;
#endif
        
        symbol_table->add_symbol(std::move(symbol));
    }
    
    void add_test_function_symbol(const std::string& function_name, std::uint32_t start_line) {
        UniversalSymbolInfo symbol;
        symbol.symbol_id = generate_unique_id("function_" + function_name);
        symbol.symbol_type = SymbolType::FUNCTION;
        symbol.name = function_name;
        symbol.start_line = start_line;
        symbol.metadata["language"] = "cpp";
        
#ifdef NEKOCODE_DEBUG_SYMBOLS
        std::cerr << "[Phase 5 Test] C++ adding function symbol: " << function_name 
                  << " with ID: " << symbol.symbol_id << std::endl;
#endif
        
        symbol_table->add_symbol(std::move(symbol));
    }
    
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
                
                // 🚀 Phase 5: Universal Symbol直接生成
                state.add_test_class_symbol(class_info.name, class_info.start_line);
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
                
                // 🚀 Phase 5: Universal Symbol直接生成
                state.add_test_function_symbol(func_info.name, func_info.start_line);
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
        using namespace nekocode::debug;
        NEKOCODE_PERF_TIMER("CppPEGTLAnalyzer::analyze " + filename);
        
        NEKOCODE_LOG_INFO("CppAnalyzer", "Starting C++ PEGTL analysis of " + filename + " (" + std::to_string(content.size()) + " bytes)");
        
        AnalysisResult result;
        
        // 🔥 前処理革命：コメント・文字列除去システム（コメント収集付き）
        std::vector<CommentInfo> comments;
        std::string preprocessed_content = preprocess_content(content, &comments);
        
        // ファイル情報設定
        result.file_info.name = filename;
        result.file_info.size_bytes = content.size();
        result.language = Language::CPP;
        
        // 🆕 コメントアウト行情報を結果に追加
        result.commented_lines = std::move(comments);
        // std::cerr << "🔥 After move: result.commented_lines.size()=" << result.commented_lines.size() << std::endl;
        
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
            state.current_content = preprocessed_content;
            
            tao::pegtl::string_input input(preprocessed_content, filename);
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
        
        // 複雑度計算（ハイブリッド戦略の前に実行）
        NEKOCODE_PERF_CHECKPOINT("complexity");
        result.complexity = calculate_cpp_complexity(content);
        NEKOCODE_LOG_DEBUG("CppAnalyzer", "Complexity calculated: " + std::to_string(result.complexity.cyclomatic_complexity));
        
        // PEGTL解析結果のデバッグ
        NEKOCODE_LOG_DEBUG("CppAnalyzer", "PEGTL analysis result: classes=" + std::to_string(result.classes.size()) +
                          ", functions=" + std::to_string(result.functions.size()) + 
                          ", pegtl_success=" + (pegtl_success ? "true" : "false"));
        
        // 🚀 C++ハイブリッド戦略: JavaScript/TypeScript成功パターン移植
        NEKOCODE_PERF_CHECKPOINT("hybrid_strategy");
        if (needs_cpp_line_based_fallback(result, content)) {
            // std::cerr << "🔥 C++ Hybrid Strategy TRIGGERED!" << std::endl;
            NEKOCODE_LOG_INFO("CppAnalyzer", "Hybrid strategy triggered - applying line-based fallback");
            
            size_t classes_before = result.classes.size();
            size_t functions_before = result.functions.size();
            
            apply_cpp_line_based_analysis(result, content, filename);
            
            // std::cerr << "✅ C++ Line-based analysis completed. Classes: " << result.classes.size() 
            //           << ", Functions: " << result.functions.size() << std::endl;
            // std::cerr << "🔍 Debug: Classes before=" << classes_before << ", after=" << result.classes.size() 
            //           << ", Functions before=" << functions_before << ", after=" << result.functions.size() << std::endl;
            NEKOCODE_LOG_DEBUG("CppAnalyzer", "Hybrid strategy completed: classes " + 
                              std::to_string(classes_before) + "->" + std::to_string(result.classes.size()) +
                              ", functions " + std::to_string(functions_before) + "->" + std::to_string(result.functions.size()));
        } else {
            // std::cerr << "⚠️  C++ Hybrid Strategy NOT triggered" << std::endl;
            NEKOCODE_LOG_DEBUG("CppAnalyzer", "Hybrid strategy not needed");
        }
        
        // メンバ変数検出（analyze機能用）
        NEKOCODE_PERF_CHECKPOINT("member_variables");
        detect_member_variables(result, content);
        NEKOCODE_LOG_DEBUG("CppAnalyzer", "Member variables detected");
        
        // メソッド検出（analyze機能用）
        NEKOCODE_PERF_CHECKPOINT("method_detection");
        detect_class_methods(result, content);
        NEKOCODE_LOG_DEBUG("CppAnalyzer", "Class methods detected");
        
        // 統計更新
        NEKOCODE_PERF_CHECKPOINT("statistics");
        // std::cerr << "🔍 Before update_statistics: classes=" << result.classes.size() 
        //           << ", functions=" << result.functions.size() 
        //           << ", commented_lines=" << result.commented_lines.size() << std::endl;
        
        result.update_statistics();
        
        // std::cerr << "🔍 After update_statistics: stats.class_count=" << result.stats.class_count 
        //           << ", stats.function_count=" << result.stats.function_count 
        //           << ", stats.commented_lines_count=" << result.stats.commented_lines_count
        //           << ", commented_lines.size()=" << result.commented_lines.size() << std::endl;
        
        NEKOCODE_LOG_DEBUG("CppAnalyzer", "Final statistics: total_classes=" + std::to_string(result.stats.class_count) +
                          ", total_functions=" + std::to_string(result.stats.function_count));
        
        NEKOCODE_LOG_INFO("CppAnalyzer", "C++ PEGTL analysis completed successfully for " + filename);
        
        // 🔥 デバッグ：最終リターン直前の統計確認
        // std::cerr << "🔥 Final return: result.stats.class_count=" << result.stats.class_count 
        //           << ", result.stats.function_count=" << result.stats.function_count 
        //           << ", result.commented_lines.size()=" << result.commented_lines.size() << std::endl;
        
        // 🚀 Phase 5: Universal Symbol直接生成（CppParseStateから取得）
        try {
            CppParseState state;  // 上で作成されたstateを再利用したいが、スコープ外なので一時的に新しいstateを作成
            state.current_content = content;
            tao::pegtl::string_input input(content, filename);
            tao::pegtl::parse<cpp::minimal_grammar::cpp_minimal, cpp_action>(input, state);
            
            if (state.symbol_table && state.symbol_table->get_all_symbols().size() > 0) {
                result.universal_symbols = state.symbol_table;
                std::cerr << "[Phase 5] C++ analyzer generated " 
                          << state.symbol_table->get_all_symbols().size() 
                          << " Universal Symbols" << std::endl;
            }
        } catch (...) {
            // Phase 5のエラーは無視（メイン解析に影響しない）
        }
        
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
    
    // 🔧 元のコンテンツから正しいクラス開始行を検索
    size_t find_correct_class_start_line(const std::string& content, const std::string& class_name, bool is_struct) {
        std::istringstream stream(content);
        std::string line;
        size_t line_number = 0;
        
        std::string search_pattern = is_struct ? ("struct " + class_name) : ("class " + class_name);
        
        while (std::getline(stream, line)) {
            line_number++;
            if (line.find(search_pattern) != std::string::npos) {
                // std::cerr << "🎯 Found correct " << (is_struct ? "struct" : "class") << " '" << class_name << "' at line " << line_number << std::endl;
                return line_number;
            }
        }
        
        // std::cerr << "❌ Could not find " << (is_struct ? "struct" : "class") << " '" << class_name << "' in original content" << std::endl;
        return 0;
    }
    
    // 🔍 メンバ変数検出（analyze機能用）
    void detect_member_variables(AnalysisResult& result, const std::string& content) {
        DEBUG_LOG("🔥 C++ detect_member_variables called with " + std::to_string(result.classes.size()) + " classes");
        
        // クラス一覧をデバッグ出力
        if (g_debug_mode) {
            for (size_t i = 0; i < result.classes.size(); ++i) {
                // std::cerr << "🏷️  Class[" << i << "]: '" << result.classes[i].name << "' (lines " 
                //           << result.classes[i].start_line << "-" << result.classes[i].end_line << ")" << std::endl;
            }
        }
        
        std::istringstream stream(content);
        std::string line;
        size_t line_number = 0;
        
        // 各クラスに対してメンバ変数を検出
        for (auto& cls : result.classes) {
            // std::cerr << "🔍 Processing class: '" << cls.name << "'" << std::endl;
            
            // namespace:やstruct:プレフィックスを除去
            std::string clean_class_name = cls.name;
            if (clean_class_name.find("namespace:") == 0) {
                // std::cerr << "⏭️  Skipping namespace: " << clean_class_name << std::endl;
                continue; // namespaceはスキップ
            }
            if (clean_class_name.find("struct:") == 0) {
                clean_class_name = clean_class_name.substr(7);
                // std::cerr << "📦 Struct detected, clean name: '" << clean_class_name << "'" << std::endl;
            }
            
            // 🔧 元のコンテンツから正しいクラス開始行を再検索
            size_t correct_start_line = find_correct_class_start_line(content, clean_class_name, cls.name.find("struct:") == 0);
            if (correct_start_line > 0) {
                cls.start_line = correct_start_line;
                // std::cerr << "✅ Corrected start_line for '" << cls.name << "': " << correct_start_line << std::endl;
            }
            
            // クラス/構造体の終了行を推定（次のクラスの開始行または最終行）
            size_t end_line = result.file_info.total_lines;
            for (const auto& other_cls : result.classes) {
                if (other_cls.start_line > cls.start_line && other_cls.start_line < end_line) {
                    end_line = other_cls.start_line - 1;
                }
            }
            cls.end_line = end_line;
            
            // クラス内のメンバ変数を検出
            stream.clear();
            stream.seekg(0);
            line_number = 0;
            bool in_class = false;
            int brace_depth = 0;
            std::string access_modifier = "private"; // デフォルトはprivate（classの場合）
            if (cls.name.find("struct:") == 0) {
                access_modifier = "public"; // structのデフォルトはpublic
            }
            
            // std::cerr << "🔍 Scanning lines " << cls.start_line << "-" << end_line << " for class '" << clean_class_name << "'" << std::endl;
            
            while (std::getline(stream, line)) {
                line_number++;
                
                // クラス定義の開始を検出
                if (line_number == cls.start_line) {
                    in_class = true;
                    if (line.find("{") != std::string::npos) {
                        brace_depth = 1;
                    }
                    // std::cerr << "🎯 Class start detected at line " << line_number << ": " << line << std::endl;
                    
                    // 🚀 単行クラス定義対応：同じ行にメンバ変数がある場合を検出
                    if (line.find("{") != std::string::npos && line.find("}") != std::string::npos) {
                        // std::cerr << "🎯 Single-line class detected, processing members inline" << std::endl;
                        
                        // { と } の間のコンテンツを抽出
                        size_t start_brace = line.find("{");
                        size_t end_brace = line.find("}", start_brace);
                        if (start_brace != std::string::npos && end_brace != std::string::npos) {
                            std::string class_body = line.substr(start_brace + 1, end_brace - start_brace - 1);
                            // std::cerr << "📝 Class body: '" << class_body << "'" << std::endl;
                            
                            // デバッグ：分割前の内容を出力
                            // std::cerr << "🔍 Processing segments from body: '" << class_body << "'" << std::endl;
                            
                            // 複数のセグメントを処理
                            std::string current_access = "private"; // classのデフォルト
                            std::istringstream body_stream(class_body);
                            std::string token;
                            std::string accumulator;
                            
                            // セミコロンまたはアクセス修飾子で分割
                            size_t pos = 0;
                            while (pos < class_body.length()) {
                                size_t next_semi = class_body.find(';', pos);
                                size_t next_access = std::min({
                                    class_body.find("public:", pos),
                                    class_body.find("private:", pos),
                                    class_body.find("protected:", pos)
                                });
                                
                                size_t next_break = std::min(next_semi, next_access);
                                if (next_break == std::string::npos) next_break = class_body.length();
                                
                                std::string segment = class_body.substr(pos, next_break - pos);
                                // std::cerr << "📋 Segment[" << pos << "-" << next_break << "]: '" << segment << "'" << std::endl;
                                // trim whitespace
                                segment.erase(0, segment.find_first_not_of(" \t"));
                                segment.erase(segment.find_last_not_of(" \t") + 1);
                                
                                if (!segment.empty()) {
                                    // アクセス修飾子チェック
                                    if (segment.find("public:") != std::string::npos) {
                                        current_access = "public";
                                        // std::cerr << "🔑 Access changed to: " << current_access << std::endl;
                                    } else if (segment.find("private:") != std::string::npos) {
                                        current_access = "private";
                                        // std::cerr << "🔑 Access changed to: " << current_access << std::endl;
                                    } else if (segment.find("protected:") != std::string::npos) {
                                        current_access = "protected";
                                        // std::cerr << "🔑 Access changed to: " << current_access << std::endl;
                                    } else {
                                        // メンバ変数パターンをチェック（改良版）
                                        std::regex member_pattern(R"(^\s*(?:static\s+)?(?:const\s+)?(\w+)\s+(\w+)\s*$)");
                                        std::smatch var_match;
                                        if (std::regex_search(segment, var_match, member_pattern)) {
                                            std::string var_name = var_match[2].str();
                                            
                                            // 関数宣言を除外
                                            if (segment.find("(") == std::string::npos) {
                                                // std::cerr << "🎯 Found member variable: " << var_name << " in class " << clean_class_name << " (single-line)" << std::endl;
                                                // std::cerr << "    📝 Segment content: '" << segment << "'" << std::endl;
                                                // std::cerr << "    🔑 Access: " << current_access << std::endl;
                                                
                                                MemberVariable member;
                                                member.name = var_name;
                                                member.type = "auto";
                                                member.access_modifier = current_access;
                                                cls.member_variables.push_back(member);
                                            }
                                        }
                                    }
                                }
                                
                                // 次の位置に移動
                                pos = next_break;
                                if (pos < class_body.length()) {
                                    if (class_body[pos] == ';') pos++;
                                    else if (next_break == next_access) {
                                        // アクセス修飾子をスキップ
                                        size_t colon_pos = class_body.find(':', pos);
                                        if (colon_pos != std::string::npos) pos = colon_pos + 1;
                                    }
                                }
                            }
                        }
                        
                        in_class = false; // 単行クラスは処理完了
                        continue;
                    } else {
                        continue; // 通常の複数行クラス処理へ
                    }
                }
                
                if (!in_class) continue;
                if (line_number > end_line) {
                    // std::cerr << "📍 Reached end_line " << end_line << " for class " << clean_class_name << std::endl;
                    break;
                }
                
                // std::cerr << "📄 Line " << line_number << " (in_class=" << in_class << ", brace_depth=" << brace_depth << "): " << line << std::endl;
                
                // ブレース深度を追跡
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
                
                if (!in_class || brace_depth <= 0) break;
                
                // アクセス修飾子を検出
                std::regex access_pattern(R"(^\s*(public|private|protected)\s*:)");
                std::smatch access_match;
                if (std::regex_search(line, access_match, access_pattern)) {
                    access_modifier = access_match[1].str();
                    continue;
                }
                
                // メンバ変数パターン
                // 例: int m_count;  std::string name;  static const bool flag = true;
                std::regex member_var_pattern(
                    R"(^\s*(?:static\s+)?(?:const\s+)?(?:mutable\s+)?)"  // 修飾子
                    R"((?:[\w:]+(?:\s*<[^>]+>)?(?:\s*::\s*\w+)*\s*[&*]*)\s+)"  // 型
                    R"((\w+))"  // 変数名
                    R"(\s*(?:\[[^\]]*\])?\s*(?:=\s*[^;]+)?\s*;)"  // 配列・初期化子
                );
                
                std::smatch var_match;
                if (std::regex_search(line, var_match, member_var_pattern)) {
                    std::string var_name = var_match[1].str();
                    // std::cerr << "🎯 Found member variable: " << var_name << " in class " << clean_class_name << " at line " << line_number << std::endl;
                    // std::cerr << "    📝 Line content: '" << line << "'" << std::endl;
                    
                    // 関数宣言を除外（括弧がある場合）
                    if (line.find("(") != std::string::npos && line.find(")") != std::string::npos) {
                        continue;
                    }
                    
                    // typedef/usingを除外
                    if (line.find("typedef") != std::string::npos || line.find("using") != std::string::npos) {
                        continue;
                    }
                    
                    // メンバ変数情報を作成
                    MemberVariable member_var;
                    member_var.name = var_name;
                    member_var.declaration_line = line_number;
                    member_var.access_modifier = access_modifier;
                    
                    // 型を推定（簡易版）
                    size_t type_end = line.find(var_name);
                    if (type_end != std::string::npos) {
                        std::string type_part = line.substr(0, type_end);
                        // 修飾子を除去
                        type_part = std::regex_replace(type_part, std::regex(R"(^\s*static\s+)"), "");
                        type_part = std::regex_replace(type_part, std::regex(R"(^\s*const\s+)"), "");
                        type_part = std::regex_replace(type_part, std::regex(R"(^\s*mutable\s+)"), "");
                        // 前後の空白を除去
                        type_part = std::regex_replace(type_part, std::regex(R"(^\s+|\s+$)"), "");
                        member_var.type = type_part;
                    }
                    
                    // static/constフラグを設定
                    member_var.is_static = (line.find("static") != std::string::npos);
                    member_var.is_const = (line.find("const") != std::string::npos);
                    
                    cls.member_variables.push_back(member_var);
                }
            }
        }
    }
    
    // 🔍 クラスメソッド検出（analyze機能用）
    void detect_class_methods(AnalysisResult& result, const std::string& content) {
        std::istringstream stream(content);
        std::string line;
        size_t line_number = 0;
        
        // 各クラスに対してメソッドを検出
        for (auto& cls : result.classes) {
            // namespace:やデバッグクラスはスキップ
            if (cls.name.find("namespace:") == 0 || 
                cls.name == "CPP_PEGTL_ANALYZER_CALLED") continue;
            
            // クラス内のメソッドを検出
            stream.clear();
            stream.seekg(0);
            line_number = 0;
            bool in_class = false;
            int brace_depth = 0;
            
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
                
                if (!in_class) continue;
                if (line_number > cls.end_line) break;
                
                // ブレース深度を追跡
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
                
                if (!in_class || brace_depth <= 0) break;
                
                // メソッド宣言パターン（ヘッダーファイル用）
                // 条件: '(' と ')' があり、';' で終わり、'{' がない
                if (line.find('(') != std::string::npos && 
                    line.find(')') != std::string::npos &&
                    line.find(';') != std::string::npos &&
                    line.find('{') == std::string::npos) {
                    
                    // コメントや文字列リテラルを除外
                    size_t comment_pos = line.find("//");
                    if (comment_pos != std::string::npos) {
                        line = line.substr(0, comment_pos);
                    }
                    
                    // メソッド名を抽出
                    size_t paren_pos = line.find('(');
                    if (paren_pos != std::string::npos && paren_pos > 0) {
                        // '(' の前の識別子を探す
                        size_t name_end = paren_pos;
                        while (name_end > 0 && std::isspace(line[name_end - 1])) {
                            name_end--;
                        }
                        
                        size_t name_start = name_end;
                        while (name_start > 0 && 
                               (std::isalnum(line[name_start - 1]) || 
                                line[name_start - 1] == '_' ||
                                line[name_start - 1] == '~')) { // デストラクタ用
                            name_start--;
                        }
                        
                        if (name_end > name_start) {
                            std::string method_name = line.substr(name_start, name_end - name_start);
                            
                            // 予約語や型名を除外
                            static const std::set<std::string> cpp_keywords = {
                                "if", "else", "for", "while", "return", "switch",
                                "case", "break", "continue", "typedef", "using",
                                "sizeof", "static_cast", "dynamic_cast", "const_cast",
                                "reinterpret_cast", "new", "delete", "throw"
                            };
                            
                            if (cpp_keywords.find(method_name) == cpp_keywords.end() &&
                                !method_name.empty()) {
                                
                                // クラス名と同じ場合はコンストラクタ
                                std::string clean_class_name = cls.name;
                                if (clean_class_name.find("struct:") == 0) {
                                    clean_class_name = clean_class_name.substr(7);
                                }
                                
                                // パラメータを抽出（簡易版）
                                std::vector<std::string> parameters;
                                size_t param_start = paren_pos + 1;
                                size_t param_end = line.find(')', param_start);
                                if (param_end != std::string::npos && param_end > param_start) {
                                    std::string params = line.substr(param_start, param_end - param_start);
                                    // 簡易的にカンマで分割（ネストした括弧は考慮しない）
                                    if (!params.empty() && params != "void") {
                                        parameters.push_back(params); // 簡易実装
                                    }
                                }
                                
                                FunctionInfo method;
                                method.name = method_name;
                                method.start_line = line_number;
                                method.end_line = line_number;
                                method.parameters = parameters;
                                
                                // 仮想関数チェック
                                if (line.find("virtual") != std::string::npos) {
                                    method.metadata["virtual"] = "true";
                                }
                                if (line.find("= 0") != std::string::npos) {
                                    method.metadata["pure_virtual"] = "true";
                                }
                                if (line.find("override") != std::string::npos) {
                                    method.metadata["override"] = "true";
                                }
                                
                                cls.methods.push_back(method);
                            }
                        }
                    }
                }
            }
        }
        
        // クラス外のメソッド実装も検出（ClassName::methodName パターン）
        stream.clear();
        stream.seekg(0);
        line_number = 0;
        
        std::regex class_method_pattern(R"(^\s*(?:[\w:]+(?:\s*<[^>]+>)?(?:\s*[&*]+)?\s+)?(\w+)::(\w+)\s*\([^)]*\)\s*(?:const\s*)?\s*\{)");
        
        while (std::getline(stream, line)) {
            line_number++;
            std::smatch match;
            
            if (std::regex_search(line, match, class_method_pattern)) {
                std::string class_name = match[1].str();
                std::string method_name = match[2].str();
                
                // 対応するクラスを探す
                for (auto& cls : result.classes) {
                    std::string clean_class_name = cls.name;
                    if (clean_class_name.find("struct:") == 0) {
                        clean_class_name = clean_class_name.substr(7);
                    }
                    
                    if (clean_class_name == class_name) {
                        // 既に検出されているか確認
                        bool already_exists = false;
                        for (const auto& existing_method : cls.methods) {
                            if (existing_method.name == method_name) {
                                already_exists = true;
                                break;
                            }
                        }
                        
                        if (!already_exists) {
                            FunctionInfo method;
                            method.name = method_name;
                            method.start_line = line_number;
                            method.metadata["implementation"] = "true";
                            cls.methods.push_back(method);
                        }
                        break;
                    }
                }
            }
        }
    }
    
    // 🚀 C++ハイブリッド戦略: 統計整合性チェック（JavaScript成功パターン移植）
    bool needs_cpp_line_based_fallback(const AnalysisResult& result, const std::string& content) {
        // JavaScript戦略と同様: 複雑度 vs 検出数の妥当性検証
        uint32_t complexity = result.complexity.cyclomatic_complexity;
        size_t detected_classes = result.classes.size();
        size_t detected_functions = result.functions.size();
        
        // デバッグクラスを除外して実際の検出数を計算
        size_t actual_classes = 0;
        for (const auto& cls : result.classes) {
            // std::cerr << "🔍 Detected class: '" << cls.name << "'" << std::endl;
            if (cls.name != "CPP_PEGTL_ANALYZER_CALLED") {
                actual_classes++;
            }
        }
        
        // デバッグ出力
        // std::cerr << "🔍 Debug: complexity=" << complexity 
        //           << ", detected_classes=" << detected_classes
        //           << ", actual_classes=" << actual_classes
        //           << ", detected_functions=" << detected_functions << std::endl;
        bool has_class = content.find("class ") != std::string::npos;
        bool has_struct = content.find("struct ") != std::string::npos;
        bool has_namespace = content.find("namespace ") != std::string::npos;
        // std::cerr << "🔍 Debug: has_class=" << has_class 
        //           << ", has_struct=" << has_struct 
        //           << ", has_namespace=" << has_namespace << std::endl;
        
        // C++特化閾値: 複雑度が高いのに検出数が少ない場合は明らかにおかしい
        if (complexity > 50 && actual_classes == 0 && detected_functions < 5) {
            // std::cerr << "📊 Trigger reason: High complexity with low detection" << std::endl;
            return true;
        }
        
        // 複雑度200以上で関数検出0は絶対におかしい
        if (complexity > 200 && detected_functions == 0) {
            // std::cerr << "📊 Trigger reason: Very high complexity with no functions" << std::endl;
            return true;
        }
        
        // C++特有パターンがあるのに検出できていない場合
        if ((has_class || has_struct || has_namespace) && actual_classes == 0) {
            // std::cerr << "📊 Trigger reason: C++ patterns found but no classes detected" << std::endl;
            return true;
        }
        
        // std::cerr << "❌ No trigger conditions met" << std::endl;
        return false;
    }
    
    // 🚀 C++最強戦略: 自動最適化ハイブリッド解析（JavaScript/TypeScript成功パターン完全移植）
    void apply_cpp_line_based_analysis(AnalysisResult& result, const std::string& content, const std::string& /* filename */) {
        // プリプロセッサ除去（C++特化）
        std::string preprocessed = preprocess_cpp_content(content);
        
        // 🎯 ファイルサイズ検出と戦略決定（JavaScript戦略移植）
        std::vector<std::string> all_lines;
        std::istringstream stream(preprocessed);
        std::string line;
        while (std::getline(stream, line)) {
            all_lines.push_back(line);
        }
        
        const size_t total_lines = all_lines.size();
        // 🚀 並列処理実験モード！
        const bool use_parallel_mode = false;  // 並列処理を無効にして比較
        const bool use_full_analysis = !use_parallel_mode;  // total_lines < 15000;     // JavaScript戦略: 15K行未満で全機能
        const bool use_sampling_mode = false; // total_lines >= 15000 && total_lines < 40000;  // サンプリングモード
        const bool use_high_speed_mode = false; // total_lines >= 40000;  // 高速モード（基本検出のみ）
        
        // std::cerr << "📊 C++解析開始: " << total_lines << "行検出" << std::endl;
        
        // 🔧 デバッグモードでのみ詳細情報表示
        if (g_debug_mode) {
            std::cerr << "🔧 デバッグ: total_lines=" << total_lines << std::endl;
            std::cerr << "🔧 デバッグ: use_full_analysis=" << use_full_analysis << std::endl;
            std::cerr << "🔧 デバッグ: use_sampling_mode=" << use_sampling_mode << std::endl;
            std::cerr << "🔧 デバッグ: use_high_speed_mode=" << use_high_speed_mode << std::endl;
        }
        
        // 既存の要素名を記録（重複検出を防ぐ - JavaScript成功パターン）
        std::set<std::string> existing_classes;
        std::set<std::string> existing_functions;
        
        for (const auto& cls : result.classes) {
            if (cls.name != "CPP_PEGTL_ANALYZER_CALLED") {
                existing_classes.insert(cls.name);
            }
        }
        for (const auto& func : result.functions) {
            existing_functions.insert(func.name);
        }
        
        // 🕐 処理時間測定開始
        auto analysis_start = std::chrono::high_resolution_clock::now();
        size_t processed_lines = 0;
        
        if (use_parallel_mode) {
            // std::cerr << "⚡ 並列処理モード: std::execution::par_unseq で高速化！" << std::endl;
            
            // 並列処理用のmutex
            std::mutex result_mutex;
            std::atomic<size_t> processed_count{0};
            
            // インデックス付きベクトルを作成（行番号を保持するため）
            std::vector<std::pair<size_t, std::string>> indexed_lines;
            indexed_lines.reserve(all_lines.size());
            for (size_t i = 0; i < all_lines.size(); ++i) {
                indexed_lines.emplace_back(i, all_lines[i]);
            }
            
            // 並列処理でC++要素を抽出
            std::for_each(std::execution::par_unseq,
                         indexed_lines.begin(),
                         indexed_lines.end(),
                         [&](const std::pair<size_t, std::string>& indexed_line) {
                // ローカル結果を保存
                std::vector<ClassInfo> local_classes;
                std::vector<FunctionInfo> local_functions;
                
                const size_t line_number = indexed_line.first + 1;
                const std::string& line = indexed_line.second;
                
                // C++要素を検出（ローカル処理）
                extract_cpp_elements_parallel(line, line_number, 
                                            local_classes, local_functions);
                
                // 結果がある場合のみロックして追加
                if (!local_classes.empty() || !local_functions.empty()) {
                    std::lock_guard<std::mutex> lock(result_mutex);
                    
                    // 重複チェックして追加
                    for (const auto& cls : local_classes) {
                        if (existing_classes.find(cls.name) == existing_classes.end()) {
                            result.classes.push_back(cls);
                            existing_classes.insert(cls.name);
                        }
                    }
                    
                    for (const auto& func : local_functions) {
                        if (existing_functions.find(func.name) == existing_functions.end()) {
                            result.functions.push_back(func);
                            existing_functions.insert(func.name);
                        }
                    }
                }
                
                processed_count.fetch_add(1, std::memory_order_relaxed);
            });
            
            processed_lines = processed_count.load();
            
        } else if (use_full_analysis) {
            // std::cerr << "🚀 通常モード: 全機能有効（C++最高精度）" << std::endl;
            // 通常モード：全行処理
            for (size_t i = 0; i < all_lines.size(); i++) {
                const std::string& current_line = all_lines[i];
                size_t current_line_number = i + 1;
                
                extract_cpp_elements_from_line(current_line, current_line_number, result, existing_classes, existing_functions);
                processed_lines++;
            }
        } else if (use_sampling_mode) {
            // std::cerr << "🎲 サンプリングモード: 10行に1行処理（効率重視）" << std::endl;
            // サンプリングモード：10行に1行だけ処理
            for (size_t i = 0; i < all_lines.size(); i += 10) {
                const std::string& current_line = all_lines[i];
                size_t current_line_number = i + 1;
                
                extract_cpp_elements_from_line(current_line, current_line_number, result, existing_classes, existing_functions);
                processed_lines++;
            }
        } else {
            // std::cerr << "⚡ 高速モード: 基本検出のみ（大規模C++対応）" << std::endl;
            // 高速モード：基本検出のみ
            for (size_t i = 0; i < all_lines.size(); i++) {
                const std::string& current_line = all_lines[i];
                size_t current_line_number = i + 1;
                
                // 基本的なC++パターンのみ検出
                extract_basic_cpp_elements_from_line(current_line, current_line_number, result, existing_classes, existing_functions);
                processed_lines++;
            }
        }
        
        auto analysis_end = std::chrono::high_resolution_clock::now();
        auto analysis_time = std::chrono::duration_cast<std::chrono::milliseconds>(analysis_end - analysis_start);
        
        // std::cerr << "✅ C++ハイブリッド戦略完了: " << processed_lines << "行処理 (" 
        //           << analysis_time.count() << "ms)" << std::endl;
        
        // 🏁 処理戦略のサマリー
        if (use_high_speed_mode) {
            // std::cerr << "\n📊 処理戦略: 大規模C++ファイルモード（基本検出のみ）" << std::endl;
        } else if (use_sampling_mode) {
            // std::cerr << "\n📊 処理戦略: サンプリングモード（10%処理）" << std::endl;
        } else {
            // std::cerr << "\n📊 処理戦略: 通常モード（全機能有効）" << std::endl;
        }
    }
    
    // C++プリプロセッサ除去（戦略文書通り）
    std::string preprocess_cpp_content(const std::string& content) {
        std::istringstream stream(content);
        std::string line;
        std::ostringstream result;
        
        while (std::getline(stream, line)) {
            // プリプロセッサ指令を除去
            std::string trimmed = line;
            size_t first_non_space = trimmed.find_first_not_of(" \t");
            if (first_non_space == std::string::npos || trimmed[first_non_space] != '#') {
                result << line << "\n";
            }
        }
        
        return result.str();
    }
    
    // C++要素の行ベース抽出（JavaScript正規表現パターン移植＋C++特化）
    void extract_cpp_elements_from_line(const std::string& line, size_t line_number,
                                        AnalysisResult& result, 
                                        std::set<std::string>& existing_classes,
                                        std::set<std::string>& existing_functions) {
        
        // パターン1: class ClassName
        std::regex class_pattern(R"(^\s*class\s+(\w+)(?:\s*:\s*(?:public|private|protected)\s+\w+)?\s*\{?)");
        std::smatch match;
        
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
        
        // パターン2: struct StructName
        std::regex struct_pattern(R"(^\s*struct\s+(\w+)(?:\s*:\s*(?:public|private|protected)\s+\w+)?\s*\{?)");
        if (std::regex_search(line, match, struct_pattern)) {
            std::string struct_name = match[1].str();
            if (existing_classes.find("struct:" + struct_name) == existing_classes.end()) {
                ClassInfo struct_info;
                struct_info.name = "struct:" + struct_name;
                struct_info.start_line = line_number;
                result.classes.push_back(struct_info);
                existing_classes.insert("struct:" + struct_name);
            }
        }
        
        // パターン3: namespace NamespaceName
        std::regex namespace_pattern(R"(^\s*namespace\s+(\w+)\s*\{?)");
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
        
        // パターン4: 関数定義（戻り値型付き）
        std::regex function_pattern(R"(^\s*(?:inline\s+|static\s+|virtual\s+|explicit\s+)*(?:\w+(?:\s*::\s*\w+)*\s*[&*]*)\s+(\w+)\s*\([^)]*\)\s*(?:const\s*)?(?:noexcept\s*)?(?:override\s*)?\s*\{?)");
        if (std::regex_search(line, match, function_pattern)) {
            std::string func_name = match[1].str();
            
            // C++キーワードを除外
            if (func_name != "if" && func_name != "for" && func_name != "while" && 
                func_name != "switch" && func_name != "return" && func_name != "sizeof" &&
                func_name != "template" && func_name != "typename" && func_name != "class" &&
                func_name != "struct" && func_name != "namespace" && func_name != "using") {
                
                if (existing_functions.find(func_name) == existing_functions.end()) {
                    FunctionInfo func_info;
                    func_info.name = func_name;
                    func_info.start_line = line_number;
                    result.functions.push_back(func_info);
                    existing_functions.insert(func_name);
                }
            }
        }
    }
    
    // 🚀 高速モード専用：基本的なC++パターンのみ検出（大規模ファイル対応）
    void extract_basic_cpp_elements_from_line(const std::string& line, size_t line_number,
                                             AnalysisResult& result, 
                                             std::set<std::string>& existing_classes,
                                             std::set<std::string>& existing_functions) {
        
        // C++キーワードフィルタリング
        static const std::set<std::string> cpp_keywords = {
            "if", "else", "for", "while", "do", "switch", "case", "catch", 
            "try", "finally", "return", "break", "continue", "throw", 
            "typeof", "sizeof", "new", "delete", "const", "static", "virtual",
            "override", "final", "explicit", "inline", "template", "typename"
        };
        
        auto is_cpp_keyword = [&](const std::string& name) {
            return cpp_keywords.find(name) != cpp_keywords.end();
        };
        
        // 🎯 高速モード：最も一般的なパターンのみ検出
        std::smatch match;
        
        // パターン1: class Name - 最も基本的
        std::regex basic_class_pattern(R"(^\s*class\s+(\w+))");
        if (std::regex_search(line, match, basic_class_pattern)) {
            std::string class_name = match[1].str();
            if (!is_cpp_keyword(class_name) && existing_classes.find(class_name) == existing_classes.end()) {
                ClassInfo class_info;
                class_info.name = class_name;
                class_info.start_line = line_number;
                class_info.metadata["detection_mode"] = "basic";
                result.classes.push_back(class_info);
                existing_classes.insert(class_name);
            }
        }
        
        // パターン2: ReturnType functionName( - C++関数の基本形
        std::regex basic_function_pattern(R"(^\s*(?:[\w:]+\s+)*(\w+)\s*\()");
        if (std::regex_search(line, match, basic_function_pattern)) {
            std::string func_name = match[1].str();
            if (!is_cpp_keyword(func_name) && existing_functions.find(func_name) == existing_functions.end()) {
                // コンストラクタ/デストラクタチェック（簡易版）
                bool is_constructor_destructor = false;
                for (const auto& cls : existing_classes) {
                    std::string cls_name = cls;
                    if (cls_name.find("struct:") == 0) {
                        cls_name = cls_name.substr(7);
                    }
                    if (func_name == cls_name || func_name == "~" + cls_name) {
                        is_constructor_destructor = true;
                        break;
                    }
                }
                
                if (!is_constructor_destructor) {
                    FunctionInfo func_info;
                    func_info.name = func_name;
                    func_info.start_line = line_number;
                    func_info.metadata["detection_mode"] = "basic";
                    result.functions.push_back(func_info);
                    existing_functions.insert(func_name);
                }
            }
        }
    }
    
    // 🚀 並列処理専用：C++要素抽出（スレッドセーフ版）
    void extract_cpp_elements_parallel(const std::string& line, size_t line_number,
                                      std::vector<ClassInfo>& local_classes,
                                      std::vector<FunctionInfo>& local_functions) {
        
        // パターン1: class ClassName
        std::regex class_pattern(R"(^\s*class\s+(\w+)(?:\s*:\s*(?:public|private|protected)\s+\w+)?\s*\{?)");
        std::smatch match;
        
        if (std::regex_search(line, match, class_pattern)) {
            std::string class_name = match[1].str();
            ClassInfo class_info;
            class_info.name = class_name;
            class_info.start_line = line_number;
            local_classes.push_back(class_info);
        }
        
        // パターン2: struct StructName
        std::regex struct_pattern(R"(^\s*struct\s+(\w+)(?:\s*:\s*(?:public|private|protected)\s+\w+)?\s*\{?)");
        if (std::regex_search(line, match, struct_pattern)) {
            std::string struct_name = match[1].str();
            ClassInfo struct_info;
            struct_info.name = "struct:" + struct_name;
            struct_info.start_line = line_number;
            local_classes.push_back(struct_info);
        }
        
        // パターン3: namespace NamespaceName
        std::regex namespace_pattern(R"(^\s*namespace\s+(\w+)\s*\{?)");
        if (std::regex_search(line, match, namespace_pattern)) {
            std::string ns_name = match[1].str();
            ClassInfo ns_info;
            ns_info.name = "namespace:" + ns_name;
            ns_info.start_line = line_number;
            local_classes.push_back(ns_info);
        }
        
        // パターン4: 関数定義（戻り値型付き）
        std::regex function_pattern(R"(^\s*(?:inline\s+|static\s+|virtual\s+|explicit\s+)*(?:\w+(?:\s*::\s*\w+)*\s*[&*]*)\s+(\w+)\s*\([^)]*\)\s*(?:const\s*)?(?:noexcept\s*)?(?:override\s*)?\s*\{?)");
        if (std::regex_search(line, match, function_pattern)) {
            std::string func_name = match[1].str();
            
            // C++キーワードを除外
            static const std::set<std::string> cpp_keywords = {
                "if", "for", "while", "switch", "return", "sizeof",
                "template", "typename", "class", "struct", "namespace", "using"
            };
            
            if (cpp_keywords.find(func_name) == cpp_keywords.end()) {
                FunctionInfo func_info;
                func_info.name = func_name;
                func_info.start_line = line_number;
                local_functions.push_back(func_info);
            }
        }
    }
    
    // 🆕 コメント収集機能付き前処理（オーバーロード）
    std::string preprocess_content(const std::string& content, std::vector<CommentInfo>* out_comments) {
        if (!out_comments) {
            return preprocess_content(content);  // 従来版にフォールバック
        }
        
        // std::cerr << "🔥 C++ preprocess_content called with comment collection!" << std::endl;
        
        // コメント除去処理と同時にコメント情報を収集
        std::string result = content;
        
        // 複数行コメント /* ... */ の除去と収集
        result = remove_multiline_comments(result, *out_comments);
        // std::cerr << "🔥 After multiline: " << out_comments->size() << " comments collected" << std::endl;
        
        // 単行コメント // の除去と収集
        result = remove_single_line_comments(result, *out_comments);
        // std::cerr << "🔥 After single line: " << out_comments->size() << " comments collected" << std::endl;
        
        return result;
    }
    
    // 🆕 従来版preprocess_content（後方互換性）
    std::string preprocess_content(const std::string& content) {
        // 既存のプリプロセッサ除去機能を活用
        return preprocess_cpp_content(content);
    }
    
    // 🆕 複数行コメント除去と収集
    std::string remove_multiline_comments(const std::string& content, std::vector<CommentInfo>& comments) {
        std::string result = content;
        size_t pos = 0;
        
        while ((pos = result.find("/*", pos)) != std::string::npos) {
            size_t end_pos = result.find("*/", pos + 2);
            if (end_pos == std::string::npos) {
                // 閉じられていない複数行コメント
                break;
            }
            
            end_pos += 2; // "*/"を含める
            
            // コメント内容を抽出
            std::string comment_content = result.substr(pos, end_pos - pos);
            
            // 行番号を計算
            uint32_t start_line = 1 + std::count(content.begin(), content.begin() + pos, '\n');
            uint32_t end_line = 1 + std::count(content.begin(), content.begin() + end_pos, '\n');
            
            // コメント情報を作成
            CommentInfo comment_info(start_line, end_line, "multi_line", comment_content);
            comment_info.looks_like_code = looks_like_code(comment_content);
            comments.push_back(comment_info);
            
            // コメントを空白で置換（行番号を維持）
            std::string replacement(end_pos - pos, ' ');
            for (size_t i = pos; i < end_pos; i++) {
                if (result[i] == '\n') {
                    replacement[i - pos] = '\n';
                }
            }
            result.replace(pos, end_pos - pos, replacement);
            
            pos = end_pos;
        }
        
        return result;
    }
    
    // 🆕 単行コメント除去と収集
    std::string remove_single_line_comments(const std::string& content, std::vector<CommentInfo>& comments) {
        std::istringstream stream(content);
        std::ostringstream result;
        std::string line;
        uint32_t line_number = 1;
        
        while (std::getline(stream, line)) {
            size_t comment_pos = line.find("//");
            
            if (comment_pos != std::string::npos) {
                // コメント内容を抽出
                std::string comment_content = line.substr(comment_pos);
                
                // コメント情報を作成
                CommentInfo comment_info(line_number, line_number, "single_line", comment_content);
                comment_info.looks_like_code = looks_like_code(comment_content);
                comments.push_back(comment_info);
                
                // コメント部分を除去
                line = line.substr(0, comment_pos);
            }
            
            result << line << '\n';
            line_number++;
        }
        
        return result.str();
    }
    
    // 🆕 コードらしさ判定（C++特化版）
    bool looks_like_code(const std::string& comment) {
        // C++キーワードを定義
        static const std::vector<std::string> cpp_keywords = {
            "if", "else", "for", "while", "do", "switch", "case", "break", "continue",
            "return", "class", "struct", "namespace", "public", "private", "protected",
            "virtual", "override", "const", "static", "inline", "template", "typename",
            "void", "int", "char", "bool", "float", "double", "string", "vector", "map",
            "new", "delete", "this", "throw", "try", "catch", "sizeof", "nullptr",
            "auto", "decltype", "constexpr", "noexcept", "final", "explicit"
        };
        
        // コメント記号を除去
        std::string content = comment;
        if (content.find("//") == 0) {
            content = content.substr(2);
        }
        if (content.find("/*") == 0 && content.size() >= 4) {
            content = content.substr(2, content.size() - 4);
        }
        
        // 前後の空白を除去
        content.erase(0, content.find_first_not_of(" \t\n\r"));
        content.erase(content.find_last_not_of(" \t\n\r") + 1);
        
        // 空の場合はコードではない
        if (content.empty()) return false;
        
        // C++のコード特徴をチェック
        int code_score = 0;
        
        // キーワードマッチング
        for (const auto& keyword : cpp_keywords) {
            if (content.find(keyword) != std::string::npos) {
                code_score += 2;
            }
        }
        
        // C++の構文特徴
        if (content.find("(") != std::string::npos && content.find(")") != std::string::npos) {
            code_score += 1; // 関数呼び出しっぽい
        }
        if (content.find(";") != std::string::npos) {
            code_score += 2; // セミコロンは強いC++の特徴
        }
        if (content.find("{") != std::string::npos || content.find("}") != std::string::npos) {
            code_score += 1; // ブロック構造
        }
        if (content.find("::") != std::string::npos) {
            code_score += 2; // C++のスコープ演算子
        }
        if (content.find("->") != std::string::npos || content.find(".") != std::string::npos) {
            code_score += 1; // メンバアクセス
        }
        if (content.find("==") != std::string::npos || content.find("!=") != std::string::npos ||
            content.find(">=") != std::string::npos || content.find("<=") != std::string::npos) {
            code_score += 1; // 比較演算子
        }
        if (content.find("&&") != std::string::npos || content.find("||") != std::string::npos) {
            code_score += 1; // 論理演算子
        }
        if (content.find("#include") != std::string::npos || content.find("#define") != std::string::npos) {
            code_score += 3; // プリプロセッサ指令
        }
        
        // 通常のコメント特徴（減点）
        if (content.find("TODO") != std::string::npos || content.find("FIXME") != std::string::npos ||
            content.find("NOTE") != std::string::npos || content.find("BUG") != std::string::npos) {
            code_score -= 1; // 通常のコメント
        }
        
        // 3点以上でコードらしいと判定
        return code_score >= 3;
    }
};

} // namespace nekocode