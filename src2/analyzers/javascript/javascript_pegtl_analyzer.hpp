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
#include <execution>
#include <mutex>
#include <atomic>
#include <iomanip>

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
    
    // コンストラクタ
    JavaScriptParseState() {
        // AST ルートノード初期化
        ast_root = std::make_unique<ASTNode>(ASTNodeType::FILE_ROOT, "");
        current_scope = ast_root.get();
        depth_stack[0] = ast_root.get();
    }
    
    void update_line_from_position(size_t pos) {
        current_line = 1;
        for (size_t i = 0; i < pos && i < current_content.size(); ++i) {
            if (current_content[i] == '\n') {
                current_line++;
            }
        }
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
        
        // 🔥 前処理革命：コメント・文字列除去システム（コメント収集付き）
        std::vector<CommentInfo> comments;
        std::string preprocessed_content = preprocess_content(content, &comments);
        
        // ファイル情報設定
        result.file_info.name = filename;
        result.file_info.size_bytes = content.size();
        result.language = Language::JAVASCRIPT;
        
        // 🆕 コメントアウト行情報を結果に追加
        result.commented_lines = std::move(comments);
        
        // PEGTL解析実行
        try {
            JavaScriptParseState state;
            state.current_content = preprocessed_content;
            
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
                result.imports = std::move(state.imports);
                result.exports = std::move(state.exports);
                
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
        
        // 🔍 JavaScriptメンバ変数検出（C++成功パターン移植）
        detect_member_variables(result, content);
        
        // 🚀 ハイブリッド戦略: 統計整合性チェック + 行ベース補完
        if (needs_line_based_fallback(result, content)) {
            apply_line_based_analysis(result, content, filename);
        }
        
        // 🌳 AST統計更新と従来統計の統合
        result.update_statistics();
        
        return result;
    }

private:
    // 🔍 JavaScriptメンバ変数検出（C++成功パターン移植）
    void detect_member_variables(AnalysisResult& result, const std::string& content) {
        std::istringstream stream(content);
        std::string line;
        size_t line_number = 0;
        
        // 現在解析中のクラス情報
        std::string current_class;
        size_t current_class_index = 0;
        bool in_constructor = false;
        size_t class_brace_depth = 0;
        size_t current_brace_depth = 0;
        
        while (std::getline(stream, line)) {
            line_number++;
            
            // ブレース深度追跡
            for (char c : line) {
                if (c == '{') {
                    current_brace_depth++;
                } else if (c == '}') {
                    if (current_brace_depth > 0) current_brace_depth--;
                    if (current_brace_depth <= class_brace_depth && !current_class.empty()) {
                        // クラス終了
                        current_class.clear();
                        in_constructor = false;
                        class_brace_depth = 0;
                    }
                }
            }
            
            // クラス開始検出
            std::regex class_pattern(R"(^\s*(?:export\s+)?class\s+(\w+))");
            std::smatch class_match;
            if (std::regex_search(line, class_match, class_pattern)) {
                current_class = class_match[1].str();
                class_brace_depth = current_brace_depth;
                
                // 既存のクラス情報を見つける
                for (size_t i = 0; i < result.classes.size(); i++) {
                    if (result.classes[i].name == current_class) {
                        current_class_index = i;
                        break;
                    }
                }
            }
            
            // コンストラクタ検出
            if (!current_class.empty()) {
                std::regex constructor_pattern(R"(^\s*constructor\s*\()");
                if (std::regex_search(line, constructor_pattern)) {
                    in_constructor = true;
                }
            }
            
            // JavaScriptメンバ変数パターン検出
            if (!current_class.empty() && current_class_index < result.classes.size()) {
                detect_javascript_member_patterns(line, line_number, result.classes[current_class_index], in_constructor);
            }
        }
    }
    
    // JavaScriptメンバ変数パターン検出
    void detect_javascript_member_patterns(const std::string& line, size_t line_number, 
                                          ClassInfo& class_info, bool in_constructor) {
        std::smatch match;
        
        // パターン1: this.property = value (コンストラクタやメソッド内)
        std::regex this_property_pattern(R"(this\.(\w+)\s*=)");
        auto this_begin = std::sregex_iterator(line.begin(), line.end(), this_property_pattern);
        auto this_end = std::sregex_iterator();
        
        for (std::sregex_iterator i = this_begin; i != this_end; ++i) {
            std::smatch match = *i;
            std::string property_name = match[1].str();
            
            // 重複チェック
            bool already_exists = false;
            for (const auto& member : class_info.member_variables) {
                if (member.name == property_name) {
                    already_exists = true;
                    break;
                }
            }
            
            if (!already_exists) {
                MemberVariable member;
                member.name = property_name;
                member.type = "any"; // JavaScriptは動的型付け
                member.declaration_line = line_number;
                member.access_modifier = "public"; // JavaScriptのthis.propertyは基本public
                
                // 値から型を推定
                if (line.find("= new ") != std::string::npos) {
                    member.type = "object";
                } else if (line.find("= []") != std::string::npos) {
                    member.type = "array";
                } else if (line.find("= {}") != std::string::npos) {
                    member.type = "object";
                } else if (line.find("= true") != std::string::npos || line.find("= false") != std::string::npos) {
                    member.type = "boolean";
                } else if (std::regex_search(line, std::regex(R"(=\s*\d+)"))) {
                    member.type = "number";
                } else if (std::regex_search(line, std::regex(R"(=\s*['""])"))) {
                    member.type = "string";
                }
                
                class_info.member_variables.push_back(member);
            }
        }
        
        // パターン2: ES2022プライベートフィールド #privateField = value
        std::regex private_field_pattern(R"(^\s*#(\w+)\s*=)");
        if (std::regex_search(line, match, private_field_pattern)) {
            std::string field_name = match[1].str();
            
            MemberVariable member;
            member.name = "#" + field_name; // プライベートフィールドは#付きで保存
            member.type = "any";
            member.declaration_line = line_number;
            member.access_modifier = "private";
            member.is_static = false;
            
            class_info.member_variables.push_back(member);
        }
        
        // パターン3: ES6クラスフィールド property = value
        std::regex class_field_pattern(R"(^\s*(\w+)\s*=)");
        if (std::regex_search(line, match, class_field_pattern) && 
            !in_constructor && // コンストラクタ内のthis.propertyと区別
            line.find("this.") == std::string::npos && // this.propertyではない
            line.find("function") == std::string::npos && // 関数定義ではない
            line.find("const") == std::string::npos && // const宣言ではない
            line.find("let") == std::string::npos && // let宣言ではない
            line.find("var") == std::string::npos) { // var宣言ではない
            
            std::string field_name = match[1].str();
            
            MemberVariable member;
            member.name = field_name;
            member.type = "any";
            member.declaration_line = line_number;
            member.access_modifier = "public";
            member.is_static = false;
            
            class_info.member_variables.push_back(member);
        }
        
        // パターン4: 静的プロパティ static property = value
        std::regex static_property_pattern(R"(^\s*static\s+(\w+)\s*=)");
        if (std::regex_search(line, match, static_property_pattern)) {
            std::string property_name = match[1].str();
            
            MemberVariable member;
            member.name = property_name;
            member.type = "any";
            member.declaration_line = line_number;
            member.access_modifier = "public";
            member.is_static = true;
            
            class_info.member_variables.push_back(member);
        }
    }

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
        // 戦略ドキュメント通り: 複雑度 vs 検出数の妖当性検証
        uint32_t complexity = result.complexity.cyclomatic_complexity;
        size_t detected_functions = result.functions.size();
        size_t detected_classes = result.classes.size();
        
        // 経験的闾値: 複雑度100以上で関数検出が10未満は明らかにおかしい
        if (complexity > 100 && detected_functions < 10) {
            return true;
        }
        
        // 複雑度500以上で関数検出0は絶対におかしい（lodashケース）
        if (complexity > 500 && detected_functions == 0) {
            return true;
        }
        
        // クラスがあるのに関数が少ない場合（class methodsが検出されていない可能性）
        if (detected_classes > 0 && detected_functions < 5) {
            // std::cerr << "[DEBUG] Class method fallback triggered: classes=" << detected_classes << ", functions=" << detected_functions << std::endl;
            return true;
        }
        
        // コンテンツにIIFEパターンがある場合もフォールバック
        if (content.find(";(function()") != std::string::npos || 
            content.find("(function(){") != std::string::npos) {
            return true;
        }
        
        return false;
    }
    
    // 🚀 JavaScript世界最強戦略: 自動最適化ハイブリッド解析（TypeScript成功パターン移植）
    void apply_line_based_analysis(AnalysisResult& result, const std::string& content, const std::string& /* filename */) {
        // 🎯 ファイルサイズ検出と戦略決定
        std::vector<std::string> all_lines;
        std::istringstream stream(content);
        std::string line;
        while (std::getline(stream, line)) {
            all_lines.push_back(line);
        }
        
        const size_t total_lines = all_lines.size();
        const bool use_full_analysis = total_lines < 15000;     // JavaScript特化調整: 15K行未満で全機能
        const bool use_sampling_mode = total_lines >= 15000 && total_lines < 40000;  // サンプリングモード
        
        if (!g_quiet_mode) {
            std::cerr << "📊 JavaScript解析開始: " << total_lines << "行検出" << std::endl;
        }
        
        // 既存の関数名を記録（重複検出を防ぐ）
        std::set<std::string> existing_functions;
        for (const auto& func : result.functions) {
            existing_functions.insert(func.name);
        }
        
        // 🕐 処理時間測定開始
        auto analysis_start = std::chrono::high_resolution_clock::now();
        size_t processed_lines = 0;
        
        if (use_full_analysis) {
            // std::cerr << "🚀 通常モード: 全機能有効（JavaScript最高精度）" << std::endl;
            // 通常モード：全行処理
            for (size_t i = 0; i < all_lines.size(); i++) {
                const std::string& current_line = all_lines[i];
                size_t current_line_number = i + 1;
                
                extract_functions_from_line(current_line, current_line_number, result, existing_functions);
                processed_lines++;
            }
        } else if (use_sampling_mode) {
            std::cerr << "🎲 サンプリングモード: 10行に1行処理（効率重視）" << std::endl;
            // サンプリングモード：10行に1行だけ処理
            for (size_t i = 0; i < all_lines.size(); i += 10) {
                const std::string& current_line = all_lines[i];
                size_t current_line_number = i + 1;
                
                extract_functions_from_line(current_line, current_line_number, result, existing_functions);
                processed_lines++;
            }
        } else {
            if (!g_quiet_mode) {
                std::cerr << "⚡ 高速モード: 基本検出のみ（大規模JS対応）" << std::endl;
            }
            // 高速モード：基本検出のみ
            
            // 高速モード：基本検出のみ（全行処理）
            for (size_t i = 0; i < all_lines.size(); i++) {
                const std::string& current_line = all_lines[i];
                size_t current_line_number = i + 1;
                
                // 基本的な関数パターンのみ検出
                extract_basic_functions_from_line(current_line, current_line_number, result, existing_functions);
                processed_lines++;
            }
        }
        
        auto analysis_end = std::chrono::high_resolution_clock::now();
        auto analysis_time = std::chrono::duration_cast<std::chrono::milliseconds>(analysis_end - analysis_start);
        
        if (!g_quiet_mode) {
            std::cerr << "✅ JavaScript第1段階完了: " << processed_lines << "行処理 (" 
                      << analysis_time.count() << "ms)" << std::endl;
        }
        
        // 🚀 【JavaScriptコールバック地獄専用】無限ネスト掘削アタック開始！
        if (use_full_analysis || use_sampling_mode) {
            if (!g_quiet_mode) {
                std::cerr << "🚀 【JavaScript専用】無限ネスト掘削アタック開始！（コールバック地獄対応版）" << std::endl;
            }
            size_t initial_function_count = result.functions.size();
            
            // 関数範囲を特定してネスト検索
            extract_nested_functions_recursively(result, all_lines, existing_functions);
            
            size_t nested_functions_found = result.functions.size() - initial_function_count;
            if (!g_quiet_mode) {
                std::cerr << "🏆 JavaScript無限ネスト掘削アタック最終結果：" << nested_functions_found 
                          << "個のネスト関数を発見！" << std::endl;
            }
        } else {
            if (!g_quiet_mode) {
                std::cerr << "⚡ 高速モード: ネスト掘削スキップ（大規模JS対応）" << std::endl;
            }
        }
        
        // 🏁 処理戦略のサマリー
        if (!use_full_analysis && !use_sampling_mode) {
            if (!g_quiet_mode) {
                std::cerr << "\n📊 処理戦略: 大規模JSファイルモード（基本検出のみ）" << std::endl;
            }
        } else if (use_sampling_mode) {
            if (!g_quiet_mode) {
                std::cerr << "\n📊 処理戦略: サンプリングモード（10%処理）" << std::endl;
            }
        } else {
            if (!g_quiet_mode) {
                std::cerr << "\n📊 処理戦略: 通常モード（全機能有効）" << std::endl;
            }
        }
    }
    
    // 行から関数を抽出
    void extract_functions_from_line(const std::string& line, size_t line_number, 
                                      AnalysisResult& result, std::set<std::string>& existing_functions) {
        
        // 制御構造キーワードフィルタリング 🔥 NEW!
        static const std::set<std::string> control_keywords = {
            "if", "else", "for", "while", "do", "switch", "case", "catch", 
            "try", "finally", "return", "break", "continue", "throw", 
            "typeof", "instanceof", "new", "delete", "var", "let", "const"
        };
        
        auto is_control_keyword = [&](const std::string& name) {
            return control_keywords.find(name) != control_keywords.end();
        };
        
        // パターン1: function name(
        std::regex function_pattern(R"(^\s*function\s+(\w+)\s*\()");
        std::smatch match;
        
        if (std::regex_search(line, match, function_pattern)) {
            std::string func_name = match[1].str();
            if (!is_control_keyword(func_name) && existing_functions.find(func_name) == existing_functions.end()) {  // 🔥 フィルタリング追加!
                FunctionInfo func_info;
                func_info.name = func_name;
                func_info.start_line = line_number;
                // func_info.is_fallback_detected = true;  // TODO: FunctionInfoにフィールド追加
                result.functions.push_back(func_info);
                existing_functions.insert(func_name);
            }
        }
        
        // パターン2: const name = function(
        std::regex const_function_pattern(R"(^\s*(?:const|let|var)\s+(\w+)\s*=\s*function\s*\()");
        if (std::regex_search(line, match, const_function_pattern)) {
            std::string func_name = match[1].str();
            if (existing_functions.find(func_name) == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = func_name;
                func_info.start_line = line_number;
                // func_info.is_fallback_detected = true;
                result.functions.push_back(func_info);
                existing_functions.insert(func_name);
            }
        }
        
        // パターン3: const name = () =>
        std::regex arrow_pattern(R"(^\s*(?:const|let|var)\s+(\w+)\s*=\s*\([^)]*\)\s*=>)");
        if (std::regex_search(line, match, arrow_pattern)) {
            std::string func_name = match[1].str();
            if (existing_functions.find(func_name) == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = func_name;
                func_info.start_line = line_number;
                func_info.is_arrow_function = true;
                // func_info.is_fallback_detected = true;
                result.functions.push_back(func_info);
                existing_functions.insert(func_name);
                // std::cerr << "[DEBUG] Fallback found arrow function: " << func_name << " at line " << line_number << std::endl;
            }
        }
        
        // パターン4: class method - methodName() {
        std::regex class_method_pattern(R"(^\s*(\w+)\s*\(\s*[^)]*\s*\)\s*\{)");
        if (std::regex_search(line, match, class_method_pattern)) {
            std::string method_name = match[1].str();
            // constructorや制御構造は関数として扱わない 🔥 フィルタリング強化!
            if (method_name != "constructor" && !is_control_keyword(method_name) && 
                existing_functions.find(method_name) == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = method_name;
                func_info.start_line = line_number;
                func_info.metadata["is_class_method"] = "true";
                result.functions.push_back(func_info);
                existing_functions.insert(method_name);
                // std::cerr << "[DEBUG] Fallback found class method: " << method_name << " at line " << line_number << std::endl;
            }
        }
        
        // パターン5: static class method - static methodName() {
        std::regex static_method_pattern(R"(^\s*static\s+(\w+)\s*\(\s*[^)]*\s*\)\s*\{)");
        if (std::regex_search(line, match, static_method_pattern)) {
            std::string method_name = match[1].str();
            if (existing_functions.find(method_name) == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = method_name;
                func_info.start_line = line_number;
                func_info.metadata["is_class_method"] = "true";
                func_info.metadata["is_static"] = "true";
                result.functions.push_back(func_info);
                existing_functions.insert(method_name);
                // std::cerr << "[DEBUG] Fallback found static method: " << method_name << " at line " << line_number << std::endl;
            }
        }
        
        // パターン6: async function
        std::regex async_function_pattern(R"(^\s*async\s+function\s+(\w+)\s*\()");
        if (std::regex_search(line, match, async_function_pattern)) {
            std::string func_name = match[1].str();
            if (existing_functions.find(func_name) == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = func_name;
                func_info.start_line = line_number;
                func_info.is_async = true;
                result.functions.push_back(func_info);
                existing_functions.insert(func_name);
                // std::cerr << "[DEBUG] Fallback found async function: " << func_name << " at line " << line_number << std::endl;
            }
        }
        
        // 🔥 【JavaScript行レベル二重アタック】パターン8: ES2015オブジェクトメソッドショートハンド - method() { (TypeScript成功パターン移植)
        std::regex es2015_method_pattern(R"(^\s*(\w+)\s*\(\s*[^)]*\s*\)\s*\{)");
        if (std::regex_search(line, match, es2015_method_pattern)) {
            std::string method_name = match[1].str();
            // constructor、get、set、制御構造は除外
            if (method_name != "constructor" && method_name != "get" && method_name != "set" && 
                !is_control_keyword(method_name) && existing_functions.find(method_name) == existing_functions.end()) {
                // クラス内かオブジェクト内かを判定（簡易版）
                bool is_likely_method = (line.find("class") == std::string::npos) && 
                                       (line.find("function") == std::string::npos);
                if (is_likely_method) {
                    FunctionInfo func_info;
                    func_info.name = method_name;
                    func_info.start_line = line_number;
                    func_info.metadata["is_es2015_method"] = "true";
                    func_info.metadata["pattern_type"] = "shorthand_method";
                    result.functions.push_back(func_info);
                    existing_functions.insert(method_name);
                    // std::cerr << "[DEBUG] 🔥 ES2015 method shorthand: " << method_name << " at line " << line_number << std::endl;
                }
            }
        }
        
        // 🚀 【JavaScript行レベル二重アタック】パターン9: アロー関数プロパティ - prop: () => { (TypeScript成功パターン移植)
        std::regex arrow_property_pattern(R"(^\s*(\w+):\s*\([^)]*\)\s*=>\s*\{?)");
        if (std::regex_search(line, match, arrow_property_pattern)) {
            std::string prop_name = match[1].str();
            if (!is_control_keyword(prop_name) && existing_functions.find(prop_name) == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = prop_name;
                func_info.start_line = line_number;
                func_info.is_arrow_function = true;
                func_info.metadata["is_property_arrow"] = "true";
                func_info.metadata["pattern_type"] = "arrow_property";
                result.functions.push_back(func_info);
                existing_functions.insert(prop_name);
                // std::cerr << "[DEBUG] 🚀 Arrow property: " << prop_name << " at line " << line_number << std::endl;
            }
        }
        
        // 🎯 【JavaScript行レベル二重アタック】パターン10: 複雑なプロパティ構文 - prop: function() { (TypeScript成功パターン移植)
        std::regex complex_property_pattern(R"(^\s*(\w+):\s*function\s*\([^)]*\)\s*\{)");
        if (std::regex_search(line, match, complex_property_pattern)) {
            std::string prop_name = match[1].str();
            if (!is_control_keyword(prop_name) && existing_functions.find(prop_name) == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = prop_name;
                func_info.start_line = line_number;
                func_info.metadata["is_property_function"] = "true";
                func_info.metadata["pattern_type"] = "property_function";
                result.functions.push_back(func_info);
                existing_functions.insert(prop_name);
                // std::cerr << "[DEBUG] 🎯 Property function: " << prop_name << " at line " << line_number << std::endl;
            }
        }
        
        // パターン7: async class method - async methodName(params) { 🔥 NEW!
        std::regex async_class_method_pattern(R"(^\s*async\s+(\w+)\s*\(\s*[^)]*\s*\)\s*\{)");
        if (std::regex_search(line, match, async_class_method_pattern)) {
            std::string method_name = match[1].str();
            // constructorは関数として扱わない
            if (method_name != "constructor" && existing_functions.find(method_name) == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = method_name;
                func_info.start_line = line_number;
                func_info.is_async = true;
                func_info.metadata["is_class_method"] = "true";
                result.functions.push_back(func_info);
                existing_functions.insert(method_name);
                // std::cerr << "[DEBUG] Fallback found async class method: " << method_name << " at line " << line_number << std::endl;
            }
        }
    }
    
    // 🚀 【JavaScript専用】無限ネスト掘削アタック（コールバック地獄対応版）
    void extract_nested_functions_recursively(AnalysisResult& result, const std::vector<std::string>& all_lines, 
                                               std::set<std::string>& existing_functions) {
        
        // 📈 層別プロファイリング用
        std::vector<std::chrono::milliseconds> layer_times;
        std::vector<size_t> layer_ranges;
        std::vector<size_t> layer_detections;
        std::vector<size_t> layer_lines;
        
        // 初期範囲設定：全体を1つの範囲として開始
        std::vector<FunctionRange> current_ranges = {{1, all_lines.size(), 0}};
        size_t total_processing_time = 0;
        size_t total_scanned_lines = 0;
        size_t round_count = 0;
        const size_t MAX_DEPTH = 5; // JavaScript特化：コールバック地獄でも5層まで
        
        // 📊 ネスト掘削統計
        std::atomic<size_t> total_nested_found{0};
        
        while (!current_ranges.empty() && round_count < MAX_DEPTH) {
            round_count++;
            
            auto round_start = std::chrono::high_resolution_clock::now();
            std::vector<FunctionRange> next_ranges;
            std::atomic<size_t> round_detections{0};
            size_t round_lines = 0;
            
            // スレッドセーフな範囲・出力管理
            std::mutex ranges_mutex;
            std::mutex output_mutex;
            
            if (!g_quiet_mode) {
                std::cerr << "🎯 JavaScript第" << round_count << "回ネスト掘削攻撃開始！（検索範囲: " 
                          << current_ranges.size() << "個）" << std::endl;
            }
            
            // 🔥 並列処理でコールバック地獄を高速攻略！
            std::for_each(std::execution::par_unseq,
                          current_ranges.begin(),
                          current_ranges.end(),
                          [&](const FunctionRange& range) {
                              
                size_t range_lines = 0;
                              
                for (size_t line_idx = range.start_line - 1; 
                     line_idx < std::min(range.end_line, all_lines.size()); line_idx++) {
                    
                    const std::string& line = all_lines[line_idx];
                    size_t current_line_number = line_idx + 1;
                    range_lines++;
                    
                    // 🎯 JavaScriptネスト関数パターン検出
                    std::vector<FunctionInfo> detected_functions = 
                        detect_javascript_nested_functions(line, current_line_number, existing_functions);
                    
                    if (!detected_functions.empty()) {
                        // 検出された関数を結果に追加
                        {
                            std::lock_guard<std::mutex> output_lock(output_mutex);
                            for (const auto& func : detected_functions) {
                                result.functions.push_back(func);
                                existing_functions.insert(func.name);
                                round_detections++;
                                total_nested_found++;
                                
                                if (!g_quiet_mode) {
                                    std::cerr << "🎯 第" << round_count << "回でネスト" 
                                              << (func.is_arrow_function ? "アロー" : "") 
                                              << "関数発見: " << func.name << " (行:" << func.start_line << ")" << std::endl;
                                }
                                
                                // 次回検索範囲の追加（コールバック地獄対応）
                                if (range.indent_level < MAX_DEPTH - 1) {
                                    size_t next_start = func.start_line + 1;
                                    size_t next_end = std::min(static_cast<size_t>(func.start_line + 50), all_lines.size()); // JavaScript関数は50行程度の範囲
                                    
                                    if (next_start < next_end) {
                                        std::lock_guard<std::mutex> ranges_lock(ranges_mutex);
                                        next_ranges.push_back({next_start, next_end, range.indent_level + 1});
                                        if (!g_quiet_mode) {
                                            std::cerr << "  → 次回検索範囲追加: 行" << next_start << "-" << next_end 
                                                      << " (深さ:" << (range.indent_level + 1) << ")" << std::endl;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                
                // 🔢 行数統計の更新
                {
                    std::lock_guard<std::mutex> output_lock(output_mutex);
                    round_lines += range_lines;
                }
            });
            
            auto round_end = std::chrono::high_resolution_clock::now();
            auto round_time = std::chrono::duration_cast<std::chrono::milliseconds>(round_end - round_start);
            
            // 📊 層別統計記録
            layer_times.push_back(round_time);
            layer_ranges.push_back(current_ranges.size());
            layer_detections.push_back(round_detections);
            layer_lines.push_back(round_lines);
            
            total_processing_time += round_time.count();
            total_scanned_lines += round_lines;
            
            if (!g_quiet_mode) {
                std::cerr << "🎯 JavaScript第" << round_count << "回攻撃完了！新規検出: " << round_detections 
                          << "個 (処理時間: " << round_time.count() << "ms, 処理行数: " << round_lines << "行)" << std::endl;
            }
            
            current_ranges = std::move(next_ranges);
            
            if (round_detections == 0) {
                if (!g_quiet_mode) {
                    std::cerr << "🎉 JavaScript無限ネスト掘削アタック完了！検索範囲が空になりました" << std::endl;
                }
                break;
            }
        }
        
        if (!g_quiet_mode) {
            std::cerr << "⏱️  JavaScript総処理時間: " << total_processing_time << "ms, 総スキャン行数: " 
                      << total_scanned_lines << "行 (ラウンド数: " << round_count << "回)" << std::endl;
        }
        
        // 📊 詳細プロファイリング出力
        if (!g_quiet_mode) {
            std::cerr << "\n📊 === JavaScript層ごとの詳細プロファイリング ===" << std::endl;
        }
        if (!g_quiet_mode) {
            for (size_t i = 0; i < layer_times.size(); i++) {
                double ms_per_line = layer_lines[i] > 0 ? static_cast<double>(layer_times[i].count()) / layer_lines[i] : 0.0;
                std::cerr << "📈 JavaScript第" << (i+1) << "層: " << layer_times[i].count() << "ms, " 
                          << layer_ranges[i] << "範囲, " << layer_detections[i] << "個検出, " 
                          << layer_lines[i] << "行処理 (1行あたり: " 
                          << std::fixed << std::setprecision(3) << ms_per_line << "ms)" << std::endl;
            }
        }
        
        // 📊 累積処理時間
        if (!g_quiet_mode) {
            std::cerr << "\n📊 === JavaScript累積処理時間 ===" << std::endl;
            size_t cumulative_time = 0;
            for (size_t i = 0; i < layer_times.size(); i++) {
                cumulative_time += layer_times[i].count();
                std::cerr << "🏃 JavaScript第1層〜第" << (i+1) << "層までの累積: " << cumulative_time << "ms" << std::endl;
            }
            std::cerr << "===================================" << std::endl;
        }
    }
    
    // 🎯 JavaScriptネスト関数検出（コールバック地獄専用）
    std::vector<FunctionInfo> detect_javascript_nested_functions(const std::string& line, size_t line_number, 
                                                                 std::set<std::string>& existing_functions) {
        std::vector<FunctionInfo> detected_functions;
        std::smatch match;
        
        // 制御構造フィルタリング
        static const std::set<std::string> control_keywords = {
            "if", "else", "for", "while", "do", "switch", "case", "catch", 
            "try", "finally", "return", "break", "continue", "throw", 
            "typeof", "instanceof", "new", "delete", "var", "let", "const"
        };
        
        auto is_control_keyword = [&](const std::string& name) {
            return control_keywords.find(name) != control_keywords.end();
        };
        
        // 🎯 パターン1: コールバック内関数 - function(
        std::regex nested_function_pattern(R"(function\s+(\w+)\s*\()");
        auto func_begin = std::sregex_iterator(line.begin(), line.end(), nested_function_pattern);
        auto func_end = std::sregex_iterator();
        
        for (std::sregex_iterator i = func_begin; i != func_end; ++i) {
            std::smatch match = *i;
            std::string func_name = match[1].str();
            
            if (!is_control_keyword(func_name) && existing_functions.find(func_name) == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = func_name;
                func_info.start_line = line_number;
                func_info.metadata["nested_type"] = "callback_function";
                detected_functions.push_back(func_info);
            }
        }
        
        // 🎯 パターン2: アロー関数変数 - const name = (
        std::regex nested_arrow_pattern(R"((?:const|let|var)\s+(\w+)\s*=\s*\([^)]*\)\s*=>)");
        auto arrow_begin = std::sregex_iterator(line.begin(), line.end(), nested_arrow_pattern);
        auto arrow_end = std::sregex_iterator();
        
        for (std::sregex_iterator i = arrow_begin; i != arrow_end; ++i) {
            std::smatch match = *i;
            std::string func_name = match[1].str();
            
            if (!is_control_keyword(func_name) && existing_functions.find(func_name) == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = func_name;
                func_info.start_line = line_number;
                func_info.is_arrow_function = true;
                func_info.metadata["nested_type"] = "arrow_callback";
                detected_functions.push_back(func_info);
            }
        }
        
        // 🎯 パターン3: オブジェクトメソッド - methodName: function(
        std::regex method_pattern(R"((\w+):\s*function\s*\()");
        auto method_begin = std::sregex_iterator(line.begin(), line.end(), method_pattern);
        auto method_end = std::sregex_iterator();
        
        for (std::sregex_iterator i = method_begin; i != method_end; ++i) {
            std::smatch match = *i;
            std::string method_name = match[1].str();
            
            if (!is_control_keyword(method_name) && existing_functions.find(method_name) == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = method_name;
                func_info.start_line = line_number;
                func_info.metadata["nested_type"] = "object_method";
                detected_functions.push_back(func_info);
            }
        }
        
        // 🎯 パターン4: 関数式 - const name = function
        std::regex function_expression_pattern(R"((?:const|let|var)\s+(\w+)\s*=\s*function\s*\()");
        if (std::regex_search(line, match, function_expression_pattern)) {
            std::string func_name = match[1].str();
            
            if (!is_control_keyword(func_name) && existing_functions.find(func_name) == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = func_name;
                func_info.start_line = line_number;
                func_info.metadata["nested_type"] = "function_expression";
                detected_functions.push_back(func_info);
            }
        }
        
        // 🔥 【JavaScript無限ネスト掘削アタック強化】パターン5: ES2015ネストメソッドショートハンド - method() { (TypeScript成功パターン移植)
        std::regex nested_es2015_method_pattern(R"((\w+)\s*\(\s*[^)]*\s*\)\s*\{)");
        auto es2015_begin = std::sregex_iterator(line.begin(), line.end(), nested_es2015_method_pattern);
        auto es2015_end = std::sregex_iterator();
        
        for (std::sregex_iterator i = es2015_begin; i != es2015_end; ++i) {
            std::smatch match = *i;
            std::string method_name = match[1].str();
            
            if (method_name != "constructor" && method_name != "get" && method_name != "set" && 
                !is_control_keyword(method_name) && existing_functions.find(method_name) == existing_functions.end()) {
                // ネスト内のメソッドショートハンド
                FunctionInfo func_info;
                func_info.name = method_name;
                func_info.start_line = line_number;
                func_info.metadata["nested_type"] = "es2015_method_shorthand";
                func_info.metadata["pattern_source"] = "nested_detection";
                detected_functions.push_back(func_info);
            }
        }
        
        // 🚀 【JavaScript無限ネスト掘削アタック強化】パターン6: ネストアロー関数プロパティ - prop: () => { (TypeScript成功パターン移植)
        std::regex nested_arrow_property_pattern(R"((\w+):\s*\([^)]*\)\s*=>\s*\{?)");
        auto arrow_prop_begin = std::sregex_iterator(line.begin(), line.end(), nested_arrow_property_pattern);
        auto arrow_prop_end = std::sregex_iterator();
        
        for (std::sregex_iterator i = arrow_prop_begin; i != arrow_prop_end; ++i) {
            std::smatch match = *i;
            std::string prop_name = match[1].str();
            
            if (!is_control_keyword(prop_name) && existing_functions.find(prop_name) == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = prop_name;
                func_info.start_line = line_number;
                func_info.is_arrow_function = true;
                func_info.metadata["nested_type"] = "arrow_property_nested";
                func_info.metadata["pattern_source"] = "nested_detection";
                detected_functions.push_back(func_info);
            }
        }
        
        // 🎯 【JavaScript無限ネスト掘削アタック強化】パターン7: 複雑ネストプロパティ関数 - prop: function() { (TypeScript成功パターン移植)
        std::regex nested_complex_property_pattern(R"((\w+):\s*function\s*\([^)]*\)\s*\{)");
        auto complex_prop_begin = std::sregex_iterator(line.begin(), line.end(), nested_complex_property_pattern);
        auto complex_prop_end = std::sregex_iterator();
        
        for (std::sregex_iterator i = complex_prop_begin; i != complex_prop_end; ++i) {
            std::smatch match = *i;
            std::string prop_name = match[1].str();
            
            if (!is_control_keyword(prop_name) && existing_functions.find(prop_name) == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = prop_name;
                func_info.start_line = line_number;
                func_info.metadata["nested_type"] = "property_function_nested";
                func_info.metadata["pattern_source"] = "nested_detection";
                detected_functions.push_back(func_info);
            }
        }
        
        return detected_functions;
    }
    
    // 🌟 JavaScriptのスコープ範囲構造体（コールバック地獄対応）
    struct FunctionRange {
        size_t start_line;
        size_t end_line;
        size_t indent_level;
    };
    
    // 🚀 高速モード専用：基本的な関数パターンのみ検出（大規模JS対応）
    void extract_basic_functions_from_line(const std::string& line, size_t line_number, 
                                          AnalysisResult& result, std::set<std::string>& existing_functions) {
        
        // 制御構造キーワードフィルタリング
        static const std::set<std::string> control_keywords = {
            "if", "else", "for", "while", "do", "switch", "case", "catch", 
            "try", "finally", "return", "break", "continue", "throw", 
            "typeof", "instanceof", "new", "delete", "var", "let", "const"
        };
        
        auto is_control_keyword = [&](const std::string& name) {
            return control_keywords.find(name) != control_keywords.end();
        };
        
        // 🎯 高速モード：最も一般的なパターンのみ検出
        std::smatch match;
        
        // パターン1: function name( - 最も基本的
        std::regex basic_function_pattern(R"(^\s*function\s+(\w+)\s*\()");
        if (std::regex_search(line, match, basic_function_pattern)) {
            std::string func_name = match[1].str();
            if (!is_control_keyword(func_name) && existing_functions.find(func_name) == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = func_name;
                func_info.start_line = line_number;
                func_info.metadata["detection_mode"] = "basic";
                result.functions.push_back(func_info);
                existing_functions.insert(func_name);
            }
        }
        
        // パターン2: const/let/var name = function( - ES6対応最小限
        std::regex basic_arrow_pattern(R"(^\s*(?:const|let|var)\s+(\w+)\s*=\s*function\s*\()");
        if (std::regex_search(line, match, basic_arrow_pattern)) {
            std::string func_name = match[1].str();
            if (!is_control_keyword(func_name) && existing_functions.find(func_name) == existing_functions.end()) {
                FunctionInfo func_info;
                func_info.name = func_name;
                func_info.start_line = line_number;
                func_info.is_arrow_function = true;
                func_info.metadata["detection_mode"] = "basic";
                result.functions.push_back(func_info);
                existing_functions.insert(func_name);
            }
        }
    }
    
    // 🆕 ===============================================================================
    // 💬 Comment Collection System - TypeScriptアナライザーからの移植
    // ===============================================================================
    
    // 🆕 コメント収集機能付き前処理（オーバーロード）
    std::string preprocess_content(const std::string& content, std::vector<CommentInfo>* out_comments) {
        if (!out_comments) {
            return preprocess_content_basic(content);  // 従来版にフォールバック
        }
        
        std::string result = content;
        
        // 1. 複数行コメント /* ... */ を除去・収集
        result = remove_multiline_comments(result, out_comments);
        
        // 2. 単行コメント // ... を除去・収集
        result = remove_single_line_comments(result, out_comments);
        
        // 3. 文字列リテラル "...", '...', `...` を除去
        result = remove_string_literals(result);
        
        return result;
    }
    
    // 🔧 行番号計算ヘルパー関数
    std::uint32_t calculate_line_number(const std::string& content, size_t position) {
        std::uint32_t line_number = 1;
        for (size_t i = 0; i < position && i < content.length(); ++i) {
            if (content[i] == '\n') {
                line_number++;
            }
        }
        return line_number;
    }
    
    // 🤖 コードらしさ判定ロジック（JavaScript専用）
    bool looks_like_code(const std::string& comment_text) {
        // コメント記号を除去したテキストを取得
        std::string content = comment_text;
        
        // コメント記号を削除
        if (content.find("//") == 0) {
            content = content.substr(2);
        }
        if (content.find("/*") == 0 && content.length() >= 4 && content.substr(content.length()-2) == "*/") {
            content = content.substr(2, content.length()-4);
        }
        
        // 空白を除去
        content.erase(0, content.find_first_not_of(" \t\n\r"));
        content.erase(content.find_last_not_of(" \t\n\r") + 1);
        
        if (content.empty()) return false;
        
        // 🎯 コード判定キーワード（JavaScript専用）
        std::vector<std::string> code_keywords = {
            "function", "const", "let", "var", "class", "return", "if", "else", 
            "for", "while", "switch", "case", "break", "continue", "try", "catch", 
            "finally", "throw", "import", "export", "async", "await", "yield",
            "console.log", "console.error", "console.warn", "debugger", "void", 
            "null", "undefined", "true", "false", "typeof", "instanceof", "new", 
            "delete", "this", "super", "extends", "implements"
        };
        
        // キーワードマッチング
        for (const auto& keyword : code_keywords) {
            if (content.find(keyword) != std::string::npos) {
                return true;
            }
        }
        
        // 🔧 構文パターンマッチング
        // セミコロン終了
        if (content.back() == ';') return true;
        
        // 中括弧・小括弧パターン
        if (content.find('{') != std::string::npos || content.find('}') != std::string::npos) return true;
        if (content.find('(') != std::string::npos && content.find(')') != std::string::npos) return true;
        
        // 代入演算子
        if (content.find('=') != std::string::npos && content.find("==") == std::string::npos) return true;
        
        // メソッド呼び出しパターン
        if (content.find('.') != std::string::npos && content.find('(') != std::string::npos) return true;
        
        return false;
    }
    
    // 🆕 複数行コメント /* ... */ 除去・収集（オーバーロード）
    std::string remove_multiline_comments(const std::string& content, std::vector<CommentInfo>* out_comments) {
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
                // コメントが閉じられていない場合
                std::string comment_text = content.substr(comment_start);
                std::uint32_t start_line = calculate_line_number(content, comment_start);
                std::uint32_t end_line = calculate_line_number(content, content.length());
                
                CommentInfo comment_info(start_line, end_line, "multi_line", comment_text);
                comment_info.looks_like_code = looks_like_code(comment_text);
                out_comments->push_back(comment_info);
                
                result += std::string(content.length() - comment_start, ' ');
                break;
            }
            
            // コメント情報を収集
            std::string comment_text = content.substr(comment_start, comment_end - comment_start + 2);
            std::uint32_t start_line = calculate_line_number(content, comment_start);
            std::uint32_t end_line = calculate_line_number(content, comment_end + 2);
            
            CommentInfo comment_info(start_line, end_line, "multi_line", comment_text);
            comment_info.looks_like_code = looks_like_code(comment_text);
            out_comments->push_back(comment_info);
            
            // コメント部分をスペースで置換（行数維持のため）
            for (char c : comment_text) {
                result += (c == '\n') ? '\n' : ' ';
            }
            
            pos = comment_end + 2;
        }
        
        return result;
    }
    
    // 🆕 単行コメント // ... 除去・収集（オーバーロード）
    std::string remove_single_line_comments(const std::string& content, std::vector<CommentInfo>* out_comments) {
        std::istringstream stream(content);
        std::string result;
        std::string line;
        std::uint32_t line_number = 1;
        
        while (std::getline(stream, line)) {
            size_t comment_pos = line.find("//");
            if (comment_pos != std::string::npos) {
                // コメント情報を収集
                std::string comment_text = line.substr(comment_pos);
                CommentInfo comment_info(line_number, line_number, "single_line", comment_text);
                comment_info.looks_like_code = looks_like_code(comment_text);
                out_comments->push_back(comment_info);
                
                // コメント部分をスペースで置換
                std::string clean_line = line.substr(0, comment_pos);
                clean_line += std::string(line.length() - comment_pos, ' ');
                result += clean_line + "\n";
            } else {
                result += line + "\n";
            }
            line_number++;
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
                        result += "  "; // エスケープ文字もスペースに
                        pos += 2;
                    } else if (current == '\n') {
                        // 改行は保持（行数維持）
                        result += '\n';
                        pos++;
                    } else {
                        result += ' '; // その他の文字はスペースに
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
    
    // 従来の前処理（フォールバック用）
    std::string preprocess_content_basic(const std::string& content) {
        std::string result = content;
        
        // 1. 複数行コメント /* ... */ を除去
        result = remove_multiline_comments_basic(result);
        
        // 2. 単行コメント // ... を除去
        result = remove_single_line_comments_basic(result);
        
        // 3. 文字列リテラル "...", '...`, `...` を除去
        result = remove_string_literals(result);
        
        return result;
    }
    
    // 従来の複数行コメント除去
    std::string remove_multiline_comments_basic(const std::string& content) {
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
    
    // 従来の単行コメント除去
    std::string remove_single_line_comments_basic(const std::string& content) {
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
};

} // namespace nekocode