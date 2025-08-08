#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "nekocode/types.hpp"
#include "nekocode/symbol_table.hpp"
#include "nekocode/dependency_graph.hpp"

namespace nekocode {

//=============================================================================
// 🚀 MoveClass - クラス移動リファクタリング機能
//=============================================================================

/// 移動操作の結果
struct MoveResult {
    bool success = false;
    std::vector<std::string> moved_symbols;           // 移動したSymbol ID
    std::vector<std::string> updated_files;           // 更新したファイル
    std::vector<std::string> added_imports;           // 追加したimport文
    std::vector<std::string> removed_imports;         // 削除したimport文
    std::vector<std::string> errors;                  // エラーメッセージ
    std::vector<std::string> warnings;                // 警告メッセージ
    
    // ロールバック用情報
    struct BackupInfo {
        std::string file_path;
        std::string original_content;
        std::string modified_content;
    };
    std::vector<BackupInfo> backups;                  // バックアップ情報
};

/// 移動操作のオプション
struct MoveOptions {
    bool update_imports = true;          // import文を自動更新
    bool move_related_symbols = true;    // 関連Symbolも一緒に移動
    bool create_backup = true;            // バックアップを作成
    bool dry_run = false;                 // 実際には変更を行わない（プレビューのみ）
    bool verbose = false;                 // 詳細ログ出力
};

/// MoveClassエンジン
class MoveClassEngine {
private:
    std::shared_ptr<SymbolTable> symbol_table_;
    std::shared_ptr<DependencyGraph> dependency_graph_;
    Language language_;
    MoveOptions options_;
    
public:
    MoveClassEngine(std::shared_ptr<SymbolTable> symbol_table,
                    std::shared_ptr<DependencyGraph> dep_graph,
                    Language lang,
                    const MoveOptions& opts = {})
        : symbol_table_(symbol_table)
        , dependency_graph_(dep_graph)
        , language_(lang)
        , options_(opts) {}
    
    /// クラスを別ファイルに移動
    MoveResult move_class(const std::string& class_symbol_id,
                          const std::string& target_file_path);
    
    /// 複数のSymbolを移動
    MoveResult move_symbols(const std::vector<std::string>& symbol_ids,
                           const std::string& target_file_path);
    
    /// 移動のプレビュー（dry-run）
    MoveResult preview_move(const std::string& class_symbol_id,
                           const std::string& target_file_path);
    
    /// ロールバック
    bool rollback(const MoveResult& move_result);
    
private:
    /// ファイルからSymbolのコードを抽出
    std::string extract_symbol_code(const std::string& file_path,
                                   const UniversalSymbolInfo& symbol);
    
    /// ファイルからSymbolのコードを削除
    std::string remove_symbol_code(const std::string& file_content,
                                  const UniversalSymbolInfo& symbol);
    
    /// ファイルにSymbolのコードを挿入
    std::string insert_symbol_code(const std::string& file_content,
                                  const std::string& symbol_code,
                                  const std::string& target_position = "end");
    
    /// import文の更新
    std::string update_imports_in_file(const std::string& file_content,
                                       const std::string& old_path,
                                       const std::string& new_path,
                                       const std::vector<std::string>& moved_symbols);
    
    /// 言語別のimport文生成
    std::string generate_import_statement(const std::string& from_path,
                                         const std::vector<std::string>& symbols);
    
    /// ファイルの読み書き
    std::string read_file(const std::string& path);
    bool write_file(const std::string& path, const std::string& content);
    
    /// バックアップ作成
    void create_backup(const std::string& path, const std::string& content);
};

//=============================================================================
// 🔧 リファクタリングユーティリティ
//=============================================================================

class RefactoringUtils {
public:
    /// 相対パスを計算
    static std::string calculate_relative_path(const std::string& from_file,
                                              const std::string& to_file);
    
    /// import文のパスを正規化
    static std::string normalize_import_path(const std::string& path, Language lang);
    
    /// Symbolが移動可能かチェック
    static bool is_symbol_movable(const UniversalSymbolInfo& symbol,
                                  const DependencyGraph& dep_graph);
    
    /// 名前空間/パッケージの調整
    static std::string adjust_namespace(const std::string& code,
                                       const std::string& old_namespace,
                                       const std::string& new_namespace,
                                       Language lang);
    
    /// コードフォーマット（言語別）
    static std::string format_code(const std::string& code, Language lang);
};

//=============================================================================
// 🎯 MoveClassコマンド（Session Mode統合用）
//=============================================================================

class MoveClassCommand {
public:
    struct Request {
        std::string session_id;
        std::string symbol_id;           // 移動するSymbol ID
        std::string target_file;          // 移動先ファイルパス
        MoveOptions options;
    };
    
    struct Response {
        bool success = false;
        MoveResult result;
        nlohmann::json details;
    };
    
    /// コマンド実行
    static Response execute(const Request& request);
    
    /// コマンドのJSON化
    static nlohmann::json to_json(const Response& response);
    
    /// プレビューモード
    static Response preview(const Request& request);
};

} // namespace nekocode