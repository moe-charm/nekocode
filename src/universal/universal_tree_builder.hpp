#pragma once

//=============================================================================
// 🌳 Universal Tree Builder - 言語統一AST構築エンジン
//
// 全言語共通のAST構築システム - JavaScript実装の成功パターンを統一化
// 既存の重複を排除し、99%共通処理 + 1%言語固有適応を実現
//=============================================================================

#include "nekocode/types.hpp"
#include <memory>
#include <stack>
#include <unordered_map>
#include <functional>

namespace nekocode {
namespace universal {

//=============================================================================
// 🎯 Language Traits Pattern - 言語固有特性の抽象化
//=============================================================================

template<typename LanguageTraits>
class UniversalTreeBuilder {
public:
    using NodePtr = std::unique_ptr<ASTNode>;
    using DepthStack = std::unordered_map<std::uint32_t, ASTNode*>;
    
private:
    // 🌳 共通AST構造（JavaScript成功パターンベース）
    NodePtr ast_root;
    DepthStack depth_stack;
    ASTNode* current_scope = nullptr;
    std::uint32_t current_depth = 0;
    
    // 📊 統計情報（リアルタイム更新）
    ASTStatistics ast_stats;
    
public:
    //=========================================================================
    // 🚀 初期化（JavaScript実装から統一化）
    //=========================================================================
    
    UniversalTreeBuilder() {
        // AST ルートノード初期化（全言語共通）
        ast_root = std::make_unique<ASTNode>(ASTNodeType::FILE_ROOT, "");
        current_scope = ast_root.get();
        depth_stack[0] = ast_root.get();
        
        ast_stats = ASTStatistics{};
    }
    
    //=========================================================================
    // 🔧 スコープ管理（JavaScript depth_stack成功パターン）
    //=========================================================================
    
    /// スコープ開始（関数、クラス、ブロック）
    void enter_scope(ASTNodeType type, const std::string& name, std::uint32_t line_number = 0) {
        // 🌳 新しいスコープノード作成
        auto scope_node = LanguageTraits::create_node(type, name);
        scope_node->start_line = line_number;
        scope_node->depth = current_depth + 1;
        
        // 親子関係構築
        current_scope->add_child(std::move(scope_node));
        ASTNode* raw_ptr = current_scope->children.back().get();
        
        // 深度スタック更新
        current_depth++;
        current_scope = raw_ptr;
        depth_stack[current_depth] = raw_ptr;
        
        // 統計更新
        update_statistics(type);
    }
    
    /// スコープ終了
    void exit_scope() {
        if (current_depth > 0) {
            current_depth--;
            auto it = depth_stack.find(current_depth);
            current_scope = (it != depth_stack.end()) ? it->second : ast_root.get();
        }
    }
    
    //=========================================================================
    // 🎯 シンボル追加（統一インターフェース）
    //=========================================================================
    
    /// 関数・メソッド追加
    void add_function(const std::string& name, std::uint32_t line_number = 0) {
        add_symbol(ASTNodeType::FUNCTION, name, line_number);
    }
    
    /// クラス追加
    void add_class(const std::string& name, std::uint32_t line_number = 0) {
        add_symbol(ASTNodeType::CLASS, name, line_number);
    }
    
    /// 変数・フィールド追加
    void add_variable(const std::string& name, std::uint32_t line_number = 0) {
        add_symbol(ASTNodeType::VARIABLE, name, line_number);
    }
    
    /// 制御構造追加（if, for, while等）
    void add_control_structure(ASTNodeType type, std::uint32_t line_number = 0) {
        add_symbol(type, "", line_number);
    }
    
    /// 汎用シンボル追加
    void add_symbol(ASTNodeType type, const std::string& name, std::uint32_t line_number = 0) {
        auto symbol_node = LanguageTraits::create_node(type, name);
        symbol_node->start_line = line_number;
        symbol_node->depth = current_depth + 1;
        
        current_scope->add_child(std::move(symbol_node));
        update_statistics(type);
    }
    
    //=========================================================================
    // 📊 情報取得・検索（JavaScript成功機能の統一化）
    //=========================================================================
    
    /// スコープパス構築
    std::string build_scope_path(const std::string& name) const {
        if (!current_scope || current_scope == ast_root.get()) {
            return name;
        }
        
        // パス構築（再帰的）
        std::string path;
        ASTNode* node = current_scope;
        while (node && node != ast_root.get()) {
            if (!path.empty()) path = "::" + path;
            path = node->name + path;
            // 親を辿る（簡易実装）
            break; // TODO: 親ポインタ実装後に完全実装
        }
        
        return path.empty() ? name : path + "::" + name;
    }
    
    /// AST検索（パス指定）
    ASTNode* query_ast(const std::string& path) const {
        // TODO: JavaScript実装のquery機能を統一化
        return nullptr; // 後で実装
    }
    
    /// 行番号からスコープ取得
    ASTNode* get_scope_at_line(std::uint32_t line_number) const {
        // TODO: scope-analysis機能統一化
        return nullptr; // 後で実装
    }
    
    //=========================================================================
    // 📤 結果出力（統一フォーマット）
    //=========================================================================
    
    /// AST統計取得
    const ASTStatistics& get_ast_statistics() const {
        return ast_stats;
    }
    
    /// AST構造取得
    const ASTNode* get_ast_root() const {
        return ast_root.get();
    }
    
    /// 従来フォーマットへ変換
    void extract_to_analysis_result(AnalysisResult& result) {
        // 🔄 AST → 平面構造変換（既存システム互換性）
        extract_classes_recursive(ast_root.get(), result.classes);
        extract_functions_recursive(ast_root.get(), result.functions);
        
        // 統計情報統合
        result.stats.class_count = ast_stats.classes;
        result.stats.function_count = ast_stats.functions;
    }
    
private:
    //=========================================================================
    // 🛠️ 内部ヘルパー
    //=========================================================================
    
    void update_statistics(ASTNodeType type) {
        switch (type) {
            case ASTNodeType::CLASS:
                ast_stats.classes++;
                break;
            case ASTNodeType::FUNCTION:
            case ASTNodeType::METHOD:
                ast_stats.functions++;
                break;
            case ASTNodeType::VARIABLE:
                ast_stats.variables++;
                break;
            case ASTNodeType::IF_STATEMENT:
            case ASTNodeType::FOR_LOOP:
            case ASTNodeType::WHILE_LOOP:
                ast_stats.control_structures++;
                break;
            default:
                break;
        }
        
        // 深度統計更新
        if (current_depth > ast_stats.max_depth) {
            ast_stats.max_depth = current_depth;
        }
    }
    
    void extract_classes_recursive(const ASTNode* node, std::vector<ClassInfo>& classes) {
        if (node->type == ASTNodeType::CLASS) {
            ClassInfo class_info;
            class_info.name = node->name;
            class_info.start_line = node->start_line;
            // TODO: end_lineの算出
            classes.push_back(class_info);
        }
        
        for (const auto& child : node->children) {
            extract_classes_recursive(child.get(), classes);
        }
    }
    
    void extract_functions_recursive(const ASTNode* node, std::vector<FunctionInfo>& functions) {
        if (node->type == ASTNodeType::FUNCTION || node->type == ASTNodeType::METHOD) {
            FunctionInfo func_info;
            func_info.name = node->name;
            func_info.start_line = node->start_line;
            // TODO: end_lineの算出
            functions.push_back(func_info);
        }
        
        for (const auto& child : node->children) {
            extract_functions_recursive(child.get(), functions);
        }
    }
};

} // namespace universal
} // namespace nekocode