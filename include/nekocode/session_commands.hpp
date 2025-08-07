#pragma once

//=============================================================================
// 📋 Session Commands - セッションコマンド実装
//
// SessionManagerから分離したコマンド実装群
// 責任: 各種解析コマンドの具体的実装
//=============================================================================

#include "session_data.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace nekocode {

//=============================================================================
// 📋 Session Commands - セッションコマンド実装クラス
//=============================================================================

class SessionCommands {
public:
    SessionCommands() = default;
    ~SessionCommands() = default;

    //=========================================================================
    // 🔍 基本統計コマンド
    //=========================================================================
    
    /// 統計情報
    nlohmann::json cmd_stats(const SessionData& session) const;
    
    /// ファイル一覧
    nlohmann::json cmd_files(const SessionData& session) const;
    
    /// 複雑度ランキング
    nlohmann::json cmd_complexity(const SessionData& session) const;
    
    /// 複雑度ランキング（関数別）
    nlohmann::json cmd_complexity_ranking(const SessionData& session) const;
    
    /// 構造解析
    nlohmann::json cmd_structure(const SessionData& session) const;
    
    /// 関数呼び出し解析
    nlohmann::json cmd_calls(const SessionData& session) const;

    //=========================================================================
    // 🔍 詳細解析コマンド
    //=========================================================================
    
    /// シンボル検索
    nlohmann::json cmd_find(const SessionData& session, const std::string& term) const;
    
    /// 詳細構造解析
    nlohmann::json cmd_structure_detailed(const SessionData& session, const std::string& filename) const;
    
    /// メソッド複雑度解析
    nlohmann::json cmd_complexity_methods(const SessionData& session, const std::string& filename) const;
    
    /// 詳細呼び出し解析
    nlohmann::json cmd_calls_detailed(const SessionData& session, const std::string& function_name) const;
    
    /// クラス責任解析
    nlohmann::json cmd_analyze(const SessionData& session, const std::string& target = "", bool deep = false, bool complete = false) const;

    //=========================================================================
    // 🔍 プロジェクト品質コマンド
    //=========================================================================
    
    /// 大きなファイル検出
    nlohmann::json cmd_large_files(const SessionData& session, int threshold = 500) const;
    
    /// 重複ファイル検出
    nlohmann::json cmd_duplicates(const SessionData& session) const;
    
    /// TODO/FIXME検出
    nlohmann::json cmd_todo(const SessionData& session) const;

    //=========================================================================
    // 🔍 C++専用コマンド
    //=========================================================================
    
    /// インクルード依存関係グラフ
    nlohmann::json cmd_include_graph(const SessionData& session) const;
    
    /// インクルード循環依存検出
    nlohmann::json cmd_include_cycles(const SessionData& session) const;
    
    /// インクルード影響範囲分析
    nlohmann::json cmd_include_impact(const SessionData& session) const;
    
    /// 不要インクルード検出
    nlohmann::json cmd_include_unused(const SessionData& session) const;
    
    /// インクルード最適化提案
    nlohmann::json cmd_include_optimize(const SessionData& session) const;
    
    /// 依存関係分析（にゃー方式）
    nlohmann::json cmd_dependency_analyze(const SessionData& session, const std::string& filename = "") const;

    //=========================================================================
    // 🔍 シンボル検索（高度版）
    //=========================================================================
    
    /// 高度シンボル検索
    nlohmann::json cmd_find_symbols(const SessionData& session, 
                                    const std::string& symbol,
                                    const std::vector<std::string>& options,
                                    bool debug = false) const;

    //=========================================================================
    // 🌳 AST革命: 新セッションコマンド群
    //=========================================================================
    
    /// AST Query: 指定パスのノードを検索
    nlohmann::json cmd_ast_query(const SessionData& session, const std::string& query_path) const;
    
    /// スコープ解析: 指定行のスコープ情報を取得
    nlohmann::json cmd_scope_analysis(const SessionData& session, uint32_t line_number) const;
    
    /// AST Dump: AST構造を指定フォーマットで出力
    nlohmann::json cmd_ast_dump(const SessionData& session, const std::string& format = "tree") const;
    
    /// AST統計: AST基盤の詳細統計情報
    nlohmann::json cmd_ast_stats(const SessionData& session) const;

    // Note: 編集機能は EditCommands クラスに分離されました

    //=========================================================================
    // 🔄 シンボル移動コマンド（新機能）
    //=========================================================================
    
    /// クラス単位の移動
    nlohmann::json cmd_move_class(const SessionData& session,
                                  const std::vector<std::string>& args) const;

    //=========================================================================
    // 🔍 ヘルプ
    //=========================================================================
    
    /// ヘルプ表示
    nlohmann::json cmd_help() const;

private:
    //=========================================================================
    // 🛠️ 内部ヘルパー関数
    //=========================================================================
    
    /// ファイル解析ヘルパー
    nlohmann::json analyze_file(const AnalysisResult& file, bool deep = false) const;
    
    /// 複雑度計算ヘルパー
    uint32_t calculate_total_complexity(const AnalysisResult& file) const;
    
    /// 関数複雑度ソートヘルパー
    std::vector<std::pair<std::string, uint32_t>> get_sorted_function_complexity(const AnalysisResult& file) const;
};

} // namespace nekocode