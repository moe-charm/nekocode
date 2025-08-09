#pragma once

//=============================================================================
// 🌟 JavaScript PEGTL Analyzer - C#成功パターン適用版
//
// 完全PEGTL移行：std::regex完全撤廃
// ES6+対応、クラス、関数、import/export検出
//=============================================================================

#include "../base_analyzer.hpp"
#include "javascript_minimal_grammar.hpp"
#include <tao/pegtl.hpp>
#include <vector>
#include <string>
#include <regex>
#include <sstream>
#include <set>
#include <iostream>
#include <chrono>
#include "nekocode/analyzers/script_postprocessing.hpp"
#include "nekocode/analyzers/script_preprocessing.hpp"
#include "nekocode/analyzers/script_detection_helpers.hpp" 
#include <execution>
#include <mutex>
#include <atomic>
#include <iomanip>

// 🚀 Phase 5: Universal Symbol直接生成
#include "nekocode/universal_symbol.hpp"
#include "nekocode/symbol_table.hpp"

// 🔧 グローバルフラグ（ユーザー制御可能）
extern bool g_debug_mode;
extern bool g_quiet_mode;

namespace nekocode {

//=============================================================================
// 🎯 JavaScript解析状態（C#パターン準拠）
//=============================================================================

struct JavaScriptParseState {
    // 🌳 従来の平面データ構造
    std::vector<ClassInfo> classes;
    std::vector<FunctionInfo> functions;
    std::vector<ImportInfo> imports;
    std::vector<ExportInfo> exports;
    
    // 現在の解析位置情報
    size_t current_line = 1;
    std::string current_content;
    std::vector<std::string> content_lines; // end_line計算用
    
    // 🌳 AST革命: リアルタイムAST構築システム
    std::unique_ptr<ASTNode> ast_root;              // AST ルートノード
    DepthStack depth_stack;                         // 深度スタック管理
    ASTNode* current_scope = nullptr;               // 現在のスコープノード
    std::uint32_t current_depth = 0;                // 現在の深度
    std::uint32_t brace_depth = 0;                  // ブレース深度追跡
    
    // AST構築フラグ
    bool ast_enabled = true;                        // AST構築の有効/無効
    bool in_class_body = false;                     // クラス本体内フラグ
    bool in_function_body = false;                  // 関数本体内フラグ
    std::string current_class_name;                 // 現在のクラス名
    std::string current_function_name;              // 現在の関数名
    
    // 🚀 Phase 5: Universal Symbol直接生成 
    std::shared_ptr<SymbolTable> symbol_table;      // Universal Symbolテーブル
    std::unordered_map<std::string, int> id_counters; // ID生成用カウンター
    
    // コンストラクタ
    JavaScriptParseState() {
        // AST ルートノード初期化
        ast_root = std::make_unique<ASTNode>(ASTNodeType::FILE_ROOT, "");
        current_scope = ast_root.get();
        depth_stack[0] = ast_root.get();
        
        // 🚀 Phase 5: Universal Symbol初期化
        symbol_table = std::make_shared<SymbolTable>();
    }
    
    void update_line_from_position(size_t pos) {
        current_line = 1;
        for (size_t i = 0; i < pos && i < current_content.size(); ++i) {
            if (current_content[i] == '\n') {
                current_line++;
            }
        }
    }
    
    // 🚀 Phase 5: Universal Symbol生成メソッド
    
    std::string generate_unique_id(const std::string& base) {
        int& counter = id_counters[base];
        return base + "_" + std::to_string(counter++);
    }
    
    /// 🌟 テスト用: 最小限のクラスSymbol生成
    void add_test_class_symbol(const std::string& class_name, std::uint32_t start_line) {
        UniversalSymbolInfo symbol;
        symbol.symbol_id = generate_unique_id("class_" + class_name);
        symbol.symbol_type = SymbolType::CLASS;
        symbol.name = class_name;
        symbol.start_line = start_line;
        symbol.metadata["language"] = "javascript";
        
#ifdef NEKOCODE_DEBUG_SYMBOLS
        std::cerr << "[Phase 5 Test] Adding class symbol: " << class_name 
                  << " with ID: " << symbol.symbol_id << std::endl;
#endif
        
        symbol_table->add_symbol(std::move(symbol));
    }
    
    /// 🌟 メソッドSymbol生成
    void add_test_method_symbol(const std::string& method_name, std::uint32_t start_line, const std::string& class_name = "") {
        UniversalSymbolInfo symbol;
        symbol.symbol_id = generate_unique_id("method_" + method_name);
        symbol.symbol_type = SymbolType::FUNCTION;
        symbol.name = method_name;
        symbol.start_line = start_line;
        symbol.metadata["language"] = "javascript";
        if (!class_name.empty()) {
            symbol.metadata["class"] = class_name;
        }
        
#ifdef NEKOCODE_DEBUG_SYMBOLS
        std::cerr << "[Phase 5 Test] Adding method symbol: " << method_name 
                  << " with ID: " << symbol.symbol_id << std::endl;
#endif
        
        symbol_table->add_symbol(std::move(symbol));
    }
    
    /// 🌟 関数Symbol生成
    void add_test_function_symbol(const std::string& func_name, std::uint32_t start_line) {
        UniversalSymbolInfo symbol;
        symbol.symbol_id = generate_unique_id("function_" + func_name);
        symbol.symbol_type = SymbolType::FUNCTION;
        symbol.name = func_name;
        symbol.start_line = start_line;
        symbol.metadata["language"] = "javascript";
        
#ifdef NEKOCODE_DEBUG_SYMBOLS
        std::cerr << "[Phase 5 Test] Adding function symbol: " << func_name 
                  << " with ID: " << symbol.symbol_id << std::endl;
#endif
        
        symbol_table->add_symbol(std::move(symbol));
    }
    
    // 🌳 AST構築メソッド
    
    /// 新しいASTノードを現在のスコープに追加
    ASTNode* add_ast_node(ASTNodeType type, const std::string& name, std::uint32_t start_line) {
        if (!ast_enabled || !current_scope) return nullptr;
        
        auto new_node = std::make_unique<ASTNode>(type, name);
        new_node->start_line = start_line;
        new_node->depth = current_depth;
        new_node->scope_path = build_scope_path(name);
        
        ASTNode* raw_ptr = new_node.get();
        current_scope->add_child(std::move(new_node));
        
        return raw_ptr;
    }
    
    /// スコープ開始（新しい深度レベルに入る）
    void enter_scope(ASTNode* scope_node) {
        if (!ast_enabled) return;
        
        current_depth++;
        current_scope = scope_node;
        depth_stack[current_depth] = scope_node;
        
        // デバッグ出力
        // std::cerr << "[AST] Enter scope: " << scope_node->name << " (depth: " << current_depth << ")" << std::endl;
    }
    
    /// スコープ終了（前の深度レベルに戻る）
    void exit_scope() {
        if (!ast_enabled || current_depth == 0) return;
        
        // 終了行を設定
        if (current_scope) {
            current_scope->end_line = current_line;
        }
        
        current_depth--;
        auto it = depth_stack.find(current_depth);
        current_scope = (it != depth_stack.end()) ? it->second : ast_root.get();
        
        // デバッグ出力
        // std::cerr << "[AST] Exit scope, return to: " << (current_scope ? current_scope->name : "root") << " (depth: " << current_depth << ")" << std::endl;
    }
    
    /// ブレース深度に基づく自動スコープ管理
    void update_brace_depth(char c) {
        if (!ast_enabled) return;
        
        if (c == '{') {
            brace_depth++;
            // 新しいブロックスコープの場合はBLOCKノードを追加
            if (brace_depth > current_depth + 1) {
                ASTNode* block_node = add_ast_node(ASTNodeType::BLOCK, "block", current_line);
                if (block_node) {
                    enter_scope(block_node);
                }
            }
        } else if (c == '}' && brace_depth > 0) {
            brace_depth--;
            // スコープを抜ける
            if (brace_depth < current_depth) {
                exit_scope();
                
                // クラス/関数終了フラグをリセット
                if (in_class_body && brace_depth == 0) {
                    in_class_body = false;
                    current_class_name.clear();
                }
                if (in_function_body && brace_depth <= 1) {
                    in_function_body = false;
                    current_function_name.clear();
                }
            }
        }
    }
    
    /// スコープパス構築
    std::string build_scope_path(const std::string& name) const {
        if (!current_scope || current_scope == ast_root.get()) {
            return name;
        }
        
        std::string parent_path = current_scope->scope_path;
        if (parent_path.empty()) {
            return name;
        }
        return parent_path + "::" + name;
    }
    
    /// クラス開始処理
    void start_class(const std::string& class_name, std::uint32_t start_line) {
        current_class_name = class_name;
        in_class_body = true;
        
        if (ast_enabled) {
            ASTNode* class_node = add_ast_node(ASTNodeType::CLASS, class_name, start_line);
            if (class_node) {
                enter_scope(class_node);
            }
        }
    }
    
    /// 関数開始処理
    void start_function(const std::string& function_name, std::uint32_t start_line, bool is_method = false) {
        current_function_name = function_name;
        in_function_body = true;
        
        if (ast_enabled) {
            ASTNodeType node_type = is_method ? ASTNodeType::METHOD : ASTNodeType::FUNCTION;
            ASTNode* func_node = add_ast_node(node_type, function_name, start_line);
            if (func_node) {
                enter_scope(func_node);
            }
        }
    }
    
    /// import文処理
    void add_import(const std::string& module_path, std::uint32_t line_number) {
        if (ast_enabled) {
            ASTNode* import_node = add_ast_node(ASTNodeType::IMPORT, module_path, line_number);
            if (import_node) {
                import_node->attributes["module_path"] = module_path;
            }
        }
    }
    
    /// export文処理
    void add_export(const std::string& export_name, std::uint32_t line_number) {
        if (ast_enabled) {
            ASTNode* export_node = add_ast_node(ASTNodeType::EXPORT, export_name, line_number);
            if (export_node) {
                export_node->attributes["export_name"] = export_name;
            }
        }
    }
    
    /// 制御構造処理（if, for, while等）
    void add_control_structure(ASTNodeType type, std::uint32_t line_number) {
        if (ast_enabled) {
            std::string name = get_control_structure_name(type);
            ASTNode* control_node = add_ast_node(type, name, line_number);
            if (control_node) {
                // 制御構造は一時的にスコープを作る場合がある
                if (type == ASTNodeType::IF_STATEMENT || type == ASTNodeType::FOR_LOOP || 
                    type == ASTNodeType::WHILE_LOOP || type == ASTNodeType::SWITCH_STATEMENT) {
                    enter_scope(control_node);
                }
            }
        }
    }
    
private:
    std::string get_control_structure_name(ASTNodeType type) const {
        switch (type) {
            case ASTNodeType::IF_STATEMENT: return "if";
            case ASTNodeType::FOR_LOOP: return "for";
            case ASTNodeType::WHILE_LOOP: return "while";
            case ASTNodeType::SWITCH_STATEMENT: return "switch";
            case ASTNodeType::TRY_BLOCK: return "try";
            case ASTNodeType::CATCH_BLOCK: return "catch";
            default: return "control";
        }
    }
};

//=============================================================================
// 🎮 PEGTL Action System - JavaScript特化
//=============================================================================

template<typename Rule>
struct javascript_action : tao::pegtl::nothing<Rule> {};

//=============================================================================
// 🌳 AST革命: ネストレベル管理システム 
//=============================================================================

// ⚡ ブレース開始検出（ネストレベル管理）
template<>
struct javascript_action<tao::pegtl::one<'{'>> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, JavaScriptParseState& state) {
        state.update_line_from_position(in.position().byte);
        state.update_brace_depth('{');
        
        // デバッグ出力
        // std::cerr << "[AST] Brace open: depth=" << state.brace_depth << ", line=" << state.current_line << std::endl;
    }
};

// ⚡ ブレース終了検出（ネストレベル管理）
template<>
struct javascript_action<tao::pegtl::one<'}'>> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, JavaScriptParseState& state) {
        state.update_line_from_position(in.position().byte);
        state.update_brace_depth('}');
        
        // デバッグ出力
        // std::cerr << "[AST] Brace close: depth=" << state.brace_depth << ", line=" << state.current_line << std::endl;
    }
};

// 🎯 制御構造検出（if, for, while等）
template<>
struct javascript_action<nekocode::javascript::minimal_grammar::if_keyword> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, JavaScriptParseState& state) {
        state.update_line_from_position(in.position().byte);
        state.add_control_structure(ASTNodeType::IF_STATEMENT, state.current_line);
    }
};

template<>
struct javascript_action<nekocode::javascript::minimal_grammar::for_keyword> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, JavaScriptParseState& state) {
        state.update_line_from_position(in.position().byte);
        state.add_control_structure(ASTNodeType::FOR_LOOP, state.current_line);
    }
};

template<>
struct javascript_action<nekocode::javascript::minimal_grammar::while_keyword> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, JavaScriptParseState& state) {
        state.update_line_from_position(in.position().byte);
        state.add_control_structure(ASTNodeType::WHILE_LOOP, state.current_line);
    }
};

template<>
struct javascript_action<nekocode::javascript::minimal_grammar::switch_keyword> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, JavaScriptParseState& state) {
        state.update_line_from_position(in.position().byte);
        state.add_control_structure(ASTNodeType::SWITCH_STATEMENT, state.current_line);
    }
};

template<>
struct javascript_action<nekocode::javascript::minimal_grammar::try_keyword> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, JavaScriptParseState& state) {
        state.update_line_from_position(in.position().byte);
        state.add_control_structure(ASTNodeType::TRY_BLOCK, state.current_line);
    }
};

template<>
struct javascript_action<nekocode::javascript::minimal_grammar::catch_keyword> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, JavaScriptParseState& state) {
        state.update_line_from_position(in.position().byte);
        state.add_control_structure(ASTNodeType::CATCH_BLOCK, state.current_line);
    }
};

// 🧪 テスト用: simple function 検出 + 🌳 AST構築
template<>
struct javascript_action<javascript::minimal_grammar::simple_function> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, JavaScriptParseState& state) {
        std::string matched = in.string();
        // std::cerr << "[DEBUG] simple_function matched: " << matched.substr(0, 50) << "..." << std::endl;
        
        // function name から名前抽出
        size_t func_pos = matched.find("function");
        if (func_pos != std::string::npos) {
            size_t name_start = func_pos + 8; // "function"の長さ
            while (name_start < matched.size() && std::isspace(matched[name_start])) {
                name_start++;
            }
            
            size_t name_end = name_start;
            while (name_end < matched.size() && 
                   (std::isalnum(matched[name_end]) || matched[name_end] == '_' || matched[name_end] == '$')) {
                name_end++;
            }
            
            if (name_end > name_start) {
                std::string func_name = matched.substr(name_start, name_end - name_start);
                state.update_line_from_position(in.position().byte);
                
                // 🌳 AST革命: リアルタイムAST構築
                state.start_function(func_name, state.current_line, state.in_class_body);
                
                // 従来の平面データ構造も維持（後方互換性）
                FunctionInfo func_info;
                func_info.name = func_name;
                func_info.start_line = state.current_line;
                state.functions.push_back(func_info);
                
                // 🚀 Phase 5: Universal Symbol生成
                state.add_test_function_symbol(func_name, state.current_line);
                
                // std::cerr << "[AST] Found simple function: " << func_name << " at line " << state.current_line << std::endl;
            }
        }
    }
};

// ⚡ async関数検出
template<>
struct javascript_action<javascript::minimal_grammar::async_function> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, JavaScriptParseState& state) {
        std::string matched = in.string();
        // std::cerr << "[DEBUG] async_function matched: " << matched.substr(0, 50) << "..." << std::endl;
        
        // async function name() { から名前抽出
        size_t func_pos = matched.find("function");
        if (func_pos != std::string::npos) {
            size_t name_start = func_pos + 8; // "function"の長さ
            while (name_start < matched.size() && std::isspace(matched[name_start])) {
                name_start++;
            }
            
            size_t name_end = name_start;
            while (name_end < matched.size() && 
                   (std::isalnum(matched[name_end]) || matched[name_end] == '_' || matched[name_end] == '$')) {
                name_end++;
            }
            
            if (name_end > name_start) {
                FunctionInfo func_info;
                func_info.name = matched.substr(name_start, name_end - name_start);
                state.update_line_from_position(in.position().byte);  // 🔥 行番号更新追加
                func_info.start_line = state.current_line;
                func_info.is_async = true;
                state.functions.push_back(func_info);
                
                // 🚀 Phase 5: Universal Symbol生成
                state.add_test_function_symbol(func_info.name, state.current_line);
                // std::cerr << "[DEBUG] Found async function: " << func_info.name << " at line " << func_info.start_line << std::endl;
            }
        }
    }
};

// 🏹 アロー関数検出（簡易版）
template<>
struct javascript_action<javascript::minimal_grammar::simple_arrow> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, JavaScriptParseState& state) {
        std::string matched = in.string();
        
        // const name = () => { から名前抽出
        size_t const_pos = matched.find("const");
        if (const_pos != std::string::npos) {
            size_t name_start = const_pos + 5; // "const"の長さ
            while (name_start < matched.size() && std::isspace(matched[name_start])) {
                name_start++;
            }
            
            size_t name_end = name_start;
            while (name_end < matched.size() && 
                   (std::isalnum(matched[name_end]) || matched[name_end] == '_' || matched[name_end] == '$')) {
                name_end++;
            }
            
            if (name_end > name_start) {
                FunctionInfo func_info;
                func_info.name = matched.substr(name_start, name_end - name_start);
                state.update_line_from_position(in.position().byte);  // 🔥 行番号更新追加
                func_info.start_line = state.current_line;
                func_info.is_arrow_function = true;
                state.functions.push_back(func_info);
                
                // 🚀 Phase 5: Universal Symbol生成
                state.add_test_function_symbol(func_info.name, state.current_line);
            }
        }
    }
};

// ⚡ async arrow関数検出
template<>
struct javascript_action<javascript::minimal_grammar::async_arrow> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, JavaScriptParseState& state) {
        std::string matched = in.string();
        // std::cerr << "[DEBUG] async_arrow matched: " << matched.substr(0, 50) << "..." << std::endl;
        
        // const name = async () => { から名前抽出
        size_t const_pos = matched.find("const");
        if (const_pos != std::string::npos) {
            size_t name_start = const_pos + 5; // "const"の長さ
            while (name_start < matched.size() && std::isspace(matched[name_start])) {
                name_start++;
            }
            
            size_t name_end = name_start;
            while (name_end < matched.size() && 
                   (std::isalnum(matched[name_end]) || matched[name_end] == '_' || matched[name_end] == '$')) {
                name_end++;
            }
            
            if (name_end > name_start) {
                FunctionInfo func_info;
                func_info.name = matched.substr(name_start, name_end - name_start);
                state.update_line_from_position(in.position().byte);
                func_info.start_line = state.current_line;
                func_info.is_arrow_function = true;
                func_info.is_async = true;
                state.functions.push_back(func_info);
                
                // 🚀 Phase 5: Universal Symbol生成
                state.add_test_function_symbol(func_info.name, state.current_line);
                // std::cerr << "[DEBUG] Found async arrow: " << func_info.name << " at line " << func_info.start_line << std::endl;
            }
        }
    }
};

// 📦 import文検出（簡易版）+ 🌳 AST構築
template<>
struct javascript_action<javascript::minimal_grammar::simple_import> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, JavaScriptParseState& state) {
        std::string matched = in.string();
        state.update_line_from_position(in.position().byte);
        
        ImportInfo import_info;
        import_info.line_number = state.current_line;
        import_info.type = ImportType::ES6_IMPORT;
        
        // import { name } from 'module' から抽出
        size_t brace_start = matched.find('{');
        size_t brace_end = matched.find('}');
        size_t quote1 = matched.find('\'');
        size_t quote2 = matched.find('\'', quote1 + 1);
        
        if (brace_start != std::string::npos && brace_end != std::string::npos) {
            std::string import_names = matched.substr(brace_start + 1, brace_end - brace_start - 1);
            import_info.imported_names.push_back(import_names);
        }
        
        std::string module_path;
        if (quote1 != std::string::npos && quote2 != std::string::npos) {
            module_path = matched.substr(quote1 + 1, quote2 - quote1 - 1);
            import_info.module_path = module_path;
        }
        
        // 🌳 AST革命: import文をASTに追加
        state.add_import(module_path, state.current_line);
        
        // 従来の平面データ構造も維持
        state.imports.push_back(import_info);
    }
};

// 🏛️ class検出 + 🌳 AST構築
template<>
struct javascript_action<javascript::minimal_grammar::simple_class> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, JavaScriptParseState& state) {
        std::string matched = in.string();
        // std::cerr << "[DEBUG] simple_class matched: " << matched.substr(0, 50) << "..." << std::endl;
        
        // class Name { から名前抽出
        size_t class_pos = matched.find("class");
        if (class_pos != std::string::npos) {
            size_t name_start = class_pos + 5; // "class"の長さ
            while (name_start < matched.size() && std::isspace(matched[name_start])) {
                name_start++;
            }
            
            size_t name_end = name_start;
            while (name_end < matched.size() && 
                   (std::isalnum(matched[name_end]) || matched[name_end] == '_' || matched[name_end] == '$')) {
                name_end++;
            }
            
            if (name_end > name_start) {
                std::string class_name = matched.substr(name_start, name_end - name_start);
                state.update_line_from_position(in.position().byte);
                
                // 🌳 AST革命: リアルタイムクラス構築
                state.start_class(class_name, state.current_line);
                
                // 従来の平面データ構造も維持（後方互換性）
                ClassInfo class_info;
                class_info.name = class_name;
                class_info.start_line = state.current_line;
                state.classes.push_back(class_info);
                
                // 🚀 Phase 5 テスト: Universal Symbol直接生成
                state.add_test_class_symbol(class_name, state.current_line);
                
                // std::cerr << "[AST] Found simple class: " << class_name << " at line " << state.current_line << std::endl;
            }
        }
    }
};

// 🌍 export class検出
template<>
struct javascript_action<javascript::minimal_grammar::export_class> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, JavaScriptParseState& state) {
        std::string matched = in.string();
        
        // export class Name { から名前抽出
        size_t class_pos = matched.find("class");
        if (class_pos != std::string::npos) {
            size_t name_start = class_pos + 5; // "class"の長さ
            while (name_start < matched.size() && std::isspace(matched[name_start])) {
                name_start++;
            }
            
            size_t name_end = name_start;
            while (name_end < matched.size() && 
                   (std::isalnum(matched[name_end]) || matched[name_end] == '_' || matched[name_end] == '$')) {
                name_end++;
            }
            
            if (name_end > name_start) {
                ClassInfo class_info;
                class_info.name = matched.substr(name_start, name_end - name_start);
                class_info.start_line = state.current_line;
                // TODO: is_exported フィールド追加予定
                state.classes.push_back(class_info);
            }
        }
    }
};

// 🎯 関数宣言検出
template<>
struct javascript_action<javascript::minimal_grammar::function_decl> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, JavaScriptParseState& state) {
        std::string matched = in.string();
        
        // function name() { から名前抽出
        size_t func_pos = matched.find("function");
        if (func_pos != std::string::npos) {
            size_t name_start = func_pos + 8; // "function"の長さ
            while (name_start < matched.size() && std::isspace(matched[name_start])) {
                name_start++;
            }
            
            size_t name_end = name_start;
            while (name_end < matched.size() && 
                   (std::isalnum(matched[name_end]) || matched[name_end] == '_' || matched[name_end] == '$')) {
                name_end++;
            }
            
            if (name_end > name_start) {
                FunctionInfo func_info;
                func_info.name = matched.substr(name_start, name_end - name_start);
                func_info.start_line = state.current_line;
                func_info.is_async = (matched.find("async") != std::string::npos);
                state.functions.push_back(func_info);
                
                // 🚀 Phase 5: Universal Symbol生成
                state.add_test_function_symbol(func_info.name, state.current_line);
            }
        }
    }
};

// 🌐 export関数検出 (TypeScript対応)
template<>
struct javascript_action<javascript::minimal_grammar::export_function> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, JavaScriptParseState& state) {
        std::string matched = in.string();
        
        // export function name() { から名前抽出
        size_t func_pos = matched.find("function");
        if (func_pos != std::string::npos) {
            size_t name_start = func_pos + 8; // "function"の長さ
            while (name_start < matched.size() && std::isspace(matched[name_start])) {
                name_start++;
            }
            
            size_t name_end = name_start;
            while (name_end < matched.size() && 
                   (std::isalnum(matched[name_end]) || matched[name_end] == '_' || matched[name_end] == '$')) {
                name_end++;
            }
            
            if (name_end > name_start) {
                FunctionInfo func_info;
                func_info.name = matched.substr(name_start, name_end - name_start);
                state.update_line_from_position(in.position().byte);
                func_info.start_line = state.current_line;
                // func_info.is_exported = true;  // TODO: FunctionInfoにこのフィールド追加
                state.functions.push_back(func_info);
                
                // 🚀 Phase 5: Universal Symbol生成
                state.add_test_function_symbol(matched.substr(name_start, name_end - name_start), state.current_line);
            }
        }
    }
};

// 🏹 アロー関数検出
template<>
struct javascript_action<javascript::minimal_grammar::arrow_function> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, JavaScriptParseState& state) {
        std::string matched = in.string();
        
        // const name = () => { から名前抽出
        size_t name_start = 0;
        if (matched.find("const") == 0) {
            name_start = 5; // "const"の長さ
        } else if (matched.find("let") == 0) {
            name_start = 3;
        } else if (matched.find("var") == 0) {
            name_start = 3;
        }
        
        while (name_start < matched.size() && std::isspace(matched[name_start])) {
            name_start++;
        }
        
        size_t name_end = name_start;
        while (name_end < matched.size() && 
               (std::isalnum(matched[name_end]) || matched[name_end] == '_' || matched[name_end] == '$')) {
            name_end++;
        }
        
        if (name_end > name_start) {
            FunctionInfo func_info;
            func_info.name = matched.substr(name_start, name_end - name_start);
            func_info.start_line = state.current_line;
            func_info.is_arrow_function = true;
            func_info.is_async = (matched.find("async") != std::string::npos);
            state.functions.push_back(func_info);
            
            // 🚀 Phase 5: Universal Symbol生成
            state.add_test_function_symbol(func_info.name, state.current_line);
        }
    }
};

// 🏛️ ES6クラス検出
template<>
struct javascript_action<javascript::minimal_grammar::class_header> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, JavaScriptParseState& state) {
        std::string matched = in.string();
        
        // class ClassName [extends Parent] から名前抽出
        size_t class_pos = matched.find("class");
        if (class_pos != std::string::npos) {
            size_t name_start = class_pos + 5; // "class"の長さ
            while (name_start < matched.size() && std::isspace(matched[name_start])) {
                name_start++;
            }
            
            size_t name_end = name_start;
            while (name_end < matched.size() && 
                   (std::isalnum(matched[name_end]) || matched[name_end] == '_' || matched[name_end] == '$')) {
                name_end++;
            }
            
            if (name_end > name_start) {
                ClassInfo class_info;
                class_info.name = matched.substr(name_start, name_end - name_start);
                state.update_line_from_position(in.position().byte);
                class_info.start_line = state.current_line;
                
                // extends Parent 検出
                size_t extends_pos = matched.find("extends");
                if (extends_pos != std::string::npos) {
                    size_t parent_start = extends_pos + 7; // "extends"の長さ
                    while (parent_start < matched.size() && std::isspace(matched[parent_start])) {
                        parent_start++;
                    }
                    
                    size_t parent_end = parent_start;
                    while (parent_end < matched.size() && 
                           (std::isalnum(matched[parent_end]) || matched[parent_end] == '_' || matched[parent_end] == '$')) {
                        parent_end++;
                    }
                    
                    if (parent_end > parent_start) {
                        class_info.parent_class = matched.substr(parent_start, parent_end - parent_start);
                    }
                }
                
                state.classes.push_back(class_info);
                
                // 🚀 Phase 5: Universal Symbol生成
                state.add_test_class_symbol(class_info.name, class_info.start_line);
            }
        }
    }
};

// 🏛️ class method検出 + 🌳 AST構築
template<>
struct javascript_action<javascript::minimal_grammar::class_method> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, JavaScriptParseState& state) {
        std::string matched = in.string();
        // std::cerr << "[DEBUG] class_method matched: " << matched.substr(0, 50) << "..." << std::endl;
        
        // static methodName() { または methodName() { から名前抽出
        bool is_static = (matched.find("static") != std::string::npos);
        bool is_async = (matched.find("async") != std::string::npos);
        
        // メソッド名抽出位置を決定
        size_t name_start = 0;
        if (is_async) {
            size_t async_pos = matched.find("async");
            name_start = async_pos + 5; // "async"の長さ
        }
        if (is_static) {
            size_t static_pos = matched.find("static");
            name_start = std::max(name_start, static_pos + 6); // "static"の長さ
        }
        
        // 空白をスキップ
        while (name_start < matched.size() && std::isspace(matched[name_start])) {
            name_start++;
        }
        
        // メソッド名を抽出
        size_t name_end = name_start;
        while (name_end < matched.size() && 
               (std::isalnum(matched[name_end]) || matched[name_end] == '_' || matched[name_end] == '$')) {
            name_end++;
        }
        
        if (name_end > name_start) {
            std::string method_name = matched.substr(name_start, name_end - name_start);
            state.update_line_from_position(in.position().byte);
            
            // 🌳 AST革命: メソッドをASTに追加（クラス内メソッドとして）
            state.start_function(method_name, state.current_line, true); // is_method = true
            
            // 従来の平面データ構造も維持
            FunctionInfo func_info;
            func_info.name = method_name;
            func_info.start_line = state.current_line;
            func_info.is_async = is_async;
            if (is_static) {
                func_info.metadata["is_static"] = "true";
            }
            func_info.metadata["is_class_method"] = "true";
            state.functions.push_back(func_info);
            
            // 🚀 Phase 5修正: クラスのmethodsフィールドにも追加
            if (!state.classes.empty()) {
                state.classes.back().methods.push_back(func_info);
                // Universal Symbol生成
                state.add_test_method_symbol(method_name, state.current_line, state.classes.back().name);
            } else {
                // クラス外のメソッド（稀なケース）
                state.add_test_method_symbol(method_name, state.current_line);
            }
            
            // std::cerr << "[AST] Found class method: " << method_name << " at line " << state.current_line << std::endl;
        }
    }
};

// 📦 import文検出
template<>
struct javascript_action<javascript::minimal_grammar::import_stmt> {
    template<typename ParseInput>
    static void apply(const ParseInput& in, JavaScriptParseState& state) {
        std::string matched = in.string();
        
        ImportInfo import_info;
        import_info.line_number = state.current_line;
        import_info.type = ImportType::ES6_IMPORT;
        
        // import name from 'module' パターン解析
        size_t import_pos = matched.find("import");
        size_t from_pos = matched.find("from");
        
        if (import_pos != std::string::npos && from_pos != std::string::npos) {
            // 名前抽出 (import と from の間)
            size_t name_start = import_pos + 6; // "import"の長さ
            while (name_start < from_pos && std::isspace(matched[name_start])) {
                name_start++;
            }
            
            size_t name_end = from_pos;
            while (name_end > name_start && std::isspace(matched[name_end - 1])) {
                name_end--;
            }
            
            if (name_end > name_start) {
                std::string import_name = matched.substr(name_start, name_end - name_start);
                import_info.imported_names.push_back(import_name);
            }
            
            // モジュールパス抽出 (fromの後の文字列リテラル)
            size_t module_start = from_pos + 4; // "from"の長さ
            while (module_start < matched.size() && std::isspace(matched[module_start])) {
                module_start++;
            }
            
            if (module_start < matched.size() && (matched[module_start] == '\'' || matched[module_start] == '"')) {
                char quote = matched[module_start];
                size_t module_end = matched.find(quote, module_start + 1);
                if (module_end != std::string::npos) {
                    import_info.module_path = matched.substr(module_start + 1, module_end - module_start - 1);
                }
            }
        }
        
        state.imports.push_back(import_info);
    }
};

//=============================================================================
// 🌟 JavaScript PEGTL Analyzer 本体
//=============================================================================

class JavaScriptPEGTLAnalyzer : public BaseAnalyzer {
public:
    JavaScriptPEGTLAnalyzer() = default;
    ~JavaScriptPEGTLAnalyzer() = default;
    
    Language get_language() const override {
        return Language::JAVASCRIPT;
    }
    
    std::string get_language_name() const override {
        return "JavaScript (PEGTL)";
    }
    
    std::vector<std::string> get_supported_extensions() const override {
        return {".js", ".mjs", ".jsx", ".cjs"};
    }
    
    AnalysisResult analyze(const std::string& content, const std::string& filename) override {
        AnalysisResult result;
        
        // 🔥 統一前処理システム使用（重複削除済み）
        auto preprocess_result = script_preprocessing::ScriptPreprocessor::preprocess_script_content(
            content, "JS", g_debug_mode
        );
        std::string preprocessed_content = preprocess_result.content;
        
        // ファイル情報設定
        result.file_info.name = filename;
        result.file_info.size_bytes = content.size();
        result.language = Language::JAVASCRIPT;
        
        // 🆕 コメントアウト行情報を統一前処理から取得
        result.commented_lines = std::move(preprocess_result.comments);
        
        // PEGTL解析実行
        try {
            JavaScriptParseState state;
            state.current_content = preprocessed_content;
            
            // Split content into lines for end_line calculation
            std::istringstream stream(preprocessed_content);
            std::string line;
            while (std::getline(stream, line)) {
                state.content_lines.push_back(line);
            }
            
            tao::pegtl::string_input input(preprocessed_content, filename);
            bool success = tao::pegtl::parse<javascript::minimal_grammar::javascript_minimal, 
                                          javascript_action>(input, state);
            
            // デバッグ: パース結果を強制確認
            if (success) {
                // 🌳 AST革命: AST情報は内部に保存（TODO: セッション作成時に取得）
                // 将来的にsession_manager.cppから取得できるように保存
                // if (state.ast_root) {
                //     // AST情報をクラスメンバーに保存して後で取得可能にする
                // }
                
                // 従来の平面データ構造を移動（後方互換性）
                result.classes = std::move(state.classes);
                result.functions = std::move(state.functions);
                
                // Calculate end_line for all functions
                for (auto& func : result.functions) {
                    if (func.start_line > 0) {
                        func.end_line = find_function_end_line(state.content_lines, func.start_line - 1);
                    }
                }
                
                // Calculate end_line for all classes (バグ修正: MoveClass機能に必要)
                for (auto& cls : result.classes) {
                    if (cls.start_line > 0) {
                        cls.end_line = find_class_end_line(state.content_lines, cls.start_line - 1);
                    }
                }
                result.imports = std::move(state.imports);
                result.exports = std::move(state.exports);
                
                // 🚀 Phase 5 テスト: Universal Symbols結果設定
                // 注意：ここで設定すると、後のメソッド検出が反映されないため、
                // apply_javascript_unified_detectionで統一的に設定する
#ifdef NEKOCODE_DEBUG_SYMBOLS
                std::cerr << "[DEBUG JS] SKIPPING early symbol table assignment. Will be set in unified detection." << std::endl;
                std::cerr << "[DEBUG JS] state.symbol_table has " 
                          << (state.symbol_table ? state.symbol_table->size() : 0) << " symbols" << std::endl;
#endif
                // result.universal_symbols = state.symbol_table; // コメントアウト：後で設定
#ifdef NEKOCODE_DEBUG_SYMBOLS
                std::cerr << "[Phase 5 Test] JS Universal Symbols generated: " 
                          << (state.symbol_table ? state.symbol_table->size() : 0) << " symbols" << std::endl;
#endif
                
                // デバッグ出力
                //// std::cerr << "[DEBUG] Functions found: " << result.functions.size() << std::endl;
                //for (const auto& f : result.functions) {
                //    std::cerr << "  - " << f.name << " at line " << f.start_line << std::endl;
                //}
            }
            
            // デバッグコード削除済み
            
        } catch (const tao::pegtl::parse_error& e) {
            // パースエラーは警告として記録（完全失敗ではない）
            // TODO: エラー記録方法を検討
            // result.file_info.notes = "PEGTL parse warning: " + std::string(e.what());
        }
        
        // 複雑度計算（既存ロジック流用）
        result.complexity = calculate_javascript_complexity(content);
        
        // 🚀 ハイブリッド戦略: 統計整合性チェック + 統一検出システム補完
        if (needs_line_based_fallback(result, content)) {
            auto unified_start = std::chrono::high_resolution_clock::now();
            apply_javascript_unified_detection(result, preprocessed_content, filename);
            auto unified_end = std::chrono::high_resolution_clock::now();
            
            if (!g_quiet_mode) {
                auto unified_duration = std::chrono::duration_cast<std::chrono::milliseconds>(unified_end - unified_start).count();
                std::cerr << "⏱️ [JS] Unified detection took: " << unified_duration << "ms" << std::endl;
            }
        }
        
        // 🎯 統一後処理実行（メンバ変数検出・統計更新・ログ出力統合）
        script_postprocessing::ScriptPostprocessor::finalize_analysis_result(
            result, content, filename, Language::JAVASCRIPT, "JS"
        );
        
        // 🚀 Phase 5 テスト: Universal Symbols結果設定はtryブロック内で実行済み
        
#ifdef NEKOCODE_DEBUG_SYMBOLS
        std::cerr << "[DEBUG JS FINAL] Before return: result.universal_symbols is " 
                  << (result.universal_symbols ? "NOT NULL" : "NULL") << std::endl;
#endif
        
        return result;
    }

protected:

    // 複雑度計算（C#成功パターン準拠）
    ComplexityInfo calculate_javascript_complexity(const std::string& content) {
        ComplexityInfo complexity;
        complexity.cyclomatic_complexity = 1;
        
        // JavaScript固有の複雑度キーワード
        std::vector<std::string> complexity_keywords = {
            "if ", "else if", "else ", "for ", "while ", "do ",
            "switch ", "case ", "catch ", "&&", "||", "? ",
            ".then(", ".catch(", "async ", "await "
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
    
    // 🚀 ハイブリッド戦略: 統計整合性チェック
    bool needs_line_based_fallback(const AnalysisResult& result, const std::string& content) {
        // 🎯 シンプルな判定: 15,000行以内は無条件で完全解析！
        size_t total_lines = std::count(content.begin(), content.end(), '\n') + 1;
        
        // 15,000行以内は常に行ベース解析を実行（完全解析保証）
        if (total_lines < 15000) {
            return true;  // 無条件で実行！
        }
        
        // 15,000行以上の大規模ファイルのみ条件判定
        // （高速モードが適用されるため、最低限の検出を保証）
        uint32_t complexity = result.complexity.cyclomatic_complexity;
        size_t detected_functions = result.functions.size();
        
        // 大規模でも複雑度が高いのに検出が少なすぎる場合は実行
        if (complexity > 500 && detected_functions < 10) {
            return true;
        }
        
        // 大規模ファイルで十分検出されていればスキップ
        if (detected_functions >= 50) {
            return false;
        }
        
        // デフォルトで行ベース解析を実行
        return true;
    }
    
    // 🚀 JavaScript世界最強戦略: 自動最適化ハイブリッド解析（TypeScript成功パターン移植）
    void apply_line_based_analysis(AnalysisResult& result, const std::string& content, const std::string& filename) {
        if (!g_quiet_mode) {
            std::cerr << "📊 JavaScript解析開始: 統一検出システム使用" << std::endl;
        }
        
        // 統一検出システムを使用
        apply_javascript_unified_detection(result, content, filename);
    }
    
    // 行から関数を抽出
    // 🔥 クラス検出メソッド（行ベース）
    
    void apply_javascript_unified_detection(AnalysisResult& result, const std::string& content, const std::string& filename) {
        using namespace script_detection;
        
        // 既存の関数/クラス名セットを構築
        auto existing_names = ScriptDetectionHelpers::build_existing_names_set(result.functions, result.classes);
        
        // 統一システムで基本検出を実行
        auto export_functions = ScriptDetectionHelpers::detect_export_functions(content, existing_names);
        auto basic_functions = ScriptDetectionHelpers::detect_basic_functions(content, existing_names);
        auto classes = ScriptDetectionHelpers::detect_classes(content, existing_names);
        
        // 結果をマージ
        result.functions.insert(result.functions.end(), export_functions.begin(), export_functions.end());
        result.functions.insert(result.functions.end(), basic_functions.begin(), basic_functions.end());
        result.classes.insert(result.classes.end(), classes.begin(), classes.end());
        
        // 🚀 Phase 5緊急対応: クラスメソッドを検出
        ScriptDetectionHelpers::detect_class_methods(result.classes, content);
        
        // 🚀 Phase 5: 統一検出システムでもUniversal Symbol生成
        // 注意：早期設定を削除したので、ここで必ず生成する
#ifdef NEKOCODE_DEBUG_SYMBOLS
        std::cerr << "[DEBUG] Starting Universal Symbol generation for JavaScript" << std::endl;
        std::cerr << "[DEBUG] Classes: " << result.classes.size() 
                  << ", Functions: " << result.functions.size() << std::endl;
#endif
        
        auto symbol_table = std::make_shared<SymbolTable>();
        int class_counter = 0;
        int function_counter = 0;
        
        // クラスとメソッドのUniversal Symbol生成
        int method_counter = 0;
        // classesではなくresult.classesを使用（統一検出システムの結果を使用）
        for (const auto& class_info : result.classes) {
            // クラス自体のシンボル
            UniversalSymbolInfo symbol;
            symbol.symbol_id = "class_" + class_info.name + "_" + std::to_string(class_counter++);
            symbol.symbol_type = SymbolType::CLASS;
            symbol.name = class_info.name;
            symbol.start_line = class_info.start_line;
            symbol.metadata["language"] = "javascript";
            
#ifdef NEKOCODE_DEBUG_SYMBOLS
            std::cerr << "[Phase 5 Unified] Adding class symbol: " << class_info.name 
                      << " with ID: " << symbol.symbol_id << std::endl;
#endif
            
            symbol_table->add_symbol(std::move(symbol));
            
            // クラスメソッドのシンボル生成
#ifdef NEKOCODE_DEBUG_SYMBOLS
            std::cerr << "[DEBUG] Class " << class_info.name << " has " << class_info.methods.size() << " methods" << std::endl;
#endif
            for (const auto& method : class_info.methods) {
                UniversalSymbolInfo method_symbol;
                method_symbol.symbol_id = "method_" + method.name + "_" + std::to_string(method_counter++);
                method_symbol.symbol_type = SymbolType::FUNCTION;
                method_symbol.name = method.name;
                method_symbol.start_line = method.start_line;
                method_symbol.metadata["language"] = "javascript";
                method_symbol.metadata["class"] = class_info.name;
                
#ifdef NEKOCODE_DEBUG_SYMBOLS
                std::cerr << "[DEBUG] Adding method symbol: " << method.name 
                          << " from class " << class_info.name
                          << " with ID: " << method_symbol.symbol_id << std::endl;
#endif
                
                symbol_table->add_symbol(std::move(method_symbol));
            }
        }
        
        // 関数のUniversal Symbol生成
        for (const auto& func_info : export_functions) {
            UniversalSymbolInfo symbol;
            symbol.symbol_id = "function_" + func_info.name + "_" + std::to_string(function_counter++);
            symbol.symbol_type = SymbolType::FUNCTION;
            symbol.name = func_info.name;
            symbol.start_line = func_info.start_line;
            symbol.metadata["language"] = "javascript";
            symbol_table->add_symbol(std::move(symbol));
        }
        for (const auto& func_info : basic_functions) {
            UniversalSymbolInfo symbol;
            symbol.symbol_id = "function_" + func_info.name + "_" + std::to_string(function_counter++);
            symbol.symbol_type = SymbolType::FUNCTION;
            symbol.name = func_info.name;
            symbol.start_line = func_info.start_line;
            symbol.metadata["language"] = "javascript";
            symbol_table->add_symbol(std::move(symbol));
        }
        
        // Universal Symbol結果設定
        if (symbol_table->size() > 0) {
            result.universal_symbols = symbol_table;
#ifdef NEKOCODE_DEBUG_SYMBOLS
            std::cerr << "[Phase 5 Unified] JS Universal Symbols total: " 
                      << symbol_table->size() << " symbols" << std::endl;
#endif
        }
        
        if (!g_quiet_mode && (!export_functions.empty() || !basic_functions.empty() || !classes.empty())) {
            std::cerr << "🎯 [JS] Unified detection added: +" << export_functions.size() 
                      << " exports, +" << basic_functions.size() << " functions, +" 
                      << classes.size() << " classes" << std::endl;
        }
    }

    uint32_t find_function_end_line(const std::vector<std::string>& lines, size_t start_line) {
        if (start_line >= lines.size()) {
            return static_cast<uint32_t>(start_line + 1);
        }
        
        // Simple brace matching to find function end
        uint32_t brace_count = 0;
        bool found_opening_brace = false;
        
        for (size_t i = start_line; i < lines.size(); i++) {
            const std::string& line = lines[i];
            
            for (char c : line) {
                if (c == '{') {
                    brace_count++;
                    found_opening_brace = true;
                } else if (c == '}' && found_opening_brace) {
                    brace_count--;
                    if (brace_count == 0) {
                        return static_cast<uint32_t>(i + 1);
                    }
                }
            }
        }
        
        // If no matching brace found, return a reasonable default
        return static_cast<uint32_t>(std::min(start_line + 10, lines.size()));
    }
    
    uint32_t find_class_end_line(const std::vector<std::string>& lines, size_t start_line) {
        if (start_line >= lines.size()) {
            return static_cast<uint32_t>(start_line + 1);
        }
        
        // Classes use the same brace matching as functions
        uint32_t brace_count = 0;
        bool found_opening_brace = false;
        
        for (size_t i = start_line; i < lines.size(); i++) {
            const std::string& line = lines[i];
            
            for (char c : line) {
                if (c == '{') {
                    brace_count++;
                    found_opening_brace = true;
                } else if (c == '}' && found_opening_brace) {
                    brace_count--;
                    if (brace_count == 0) {
                        return static_cast<uint32_t>(i + 1);
                    }
                }
            }
        }
        
        // If no matching brace found, return a reasonable default (classes tend to be larger)
        return static_cast<uint32_t>(std::min(start_line + 50, lines.size()));
    }
};

} // namespace nekocode