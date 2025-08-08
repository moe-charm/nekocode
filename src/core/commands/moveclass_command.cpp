//=============================================================================
// 🚀 MoveClass Command - セッション統合コマンド実装
//
// プレビュー→確認の2段階実行パターン
// 複数ファイル変更の安全な実行
//=============================================================================

#include "nekocode/moveclass.hpp"
#include "nekocode/session_data.hpp"
#include <filesystem>
#include <chrono>
#include <sstream>
#include <fstream>
#include <iomanip>

namespace nekocode {

//=============================================================================
// プレビュー管理
//=============================================================================

struct MovePreview {
    std::string preview_id;
    std::string session_id;
    std::string symbol_id;
    std::string target_file;
    MoveOptions options;
    MoveResult preview_result;
    std::chrono::system_clock::time_point created_at;
    
    // 影響詳細
    struct FileChange {
        std::string file_path;
        std::string original_content;
        std::string modified_content;
        std::vector<std::pair<LineNumber, std::string>> changes;  // 行番号と変更内容
    };
    std::vector<FileChange> file_changes;
    
    nlohmann::json to_json() const {
        nlohmann::json j;
        j["preview_id"] = preview_id;
        j["session_id"] = session_id;
        j["symbol_id"] = symbol_id;
        j["target_file"] = target_file;
        j["created_at"] = std::chrono::system_clock::to_time_t(created_at);
        
        // 影響サマリー
        j["summary"] = {
            {"moved_symbols", preview_result.moved_symbols.size()},
            {"affected_files", file_changes.size()},
            {"added_imports", preview_result.added_imports.size()},
            {"removed_imports", preview_result.removed_imports.size()}
        };
        
        // ファイル変更詳細
        j["file_changes"] = nlohmann::json::array();
        for (const auto& fc : file_changes) {
            nlohmann::json change;
            change["file"] = fc.file_path;
            change["changes_count"] = fc.changes.size();
            
            // 最初の数行だけサンプル表示
            change["sample_changes"] = nlohmann::json::array();
            for (size_t i = 0; i < std::min(size_t(3), fc.changes.size()); ++i) {
                change["sample_changes"].push_back({
                    {"line", fc.changes[i].first},
                    {"change", fc.changes[i].second}
                });
            }
            
            j["file_changes"].push_back(change);
        }
        
        // エラーと警告
        if (!preview_result.errors.empty()) {
            j["errors"] = preview_result.errors;
        }
        if (!preview_result.warnings.empty()) {
            j["warnings"] = preview_result.warnings;
        }
        
        return j;
    }
};

// グローバルプレビューストレージ（本番では適切な管理が必要）
static std::unordered_map<std::string, std::shared_ptr<MovePreview>> g_move_previews;

//=============================================================================
// ユーティリティ関数
//=============================================================================

std::string generate_move_preview_id() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "move_preview_" << timestamp << "_" << rand() % 10000;
    return ss.str();
}

void cleanup_old_previews() {
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - std::chrono::minutes(15);  // 15分以上前のプレビューを削除
    
    for (auto it = g_move_previews.begin(); it != g_move_previews.end();) {
        if (it->second->created_at < cutoff) {
            it = g_move_previews.erase(it);
        } else {
            ++it;
        }
    }
}

//=============================================================================
// MoveClass Preview コマンド
//=============================================================================

nlohmann::json moveclass_preview(const std::string& session_id,
                                 const std::string& symbol_id,
                                 const std::string& target_file,
                                 const MoveOptions& options = {}) {
    nlohmann::json result;
    result["command"] = "moveclass-preview";
    
    try {
        // 古いプレビューをクリーンアップ
        cleanup_old_previews();
        
        // セッション取得
        auto session_store = SessionStore::get_instance();
        auto session = session_store.get_session(session_id);
        if (!session) {
            result["error"] = "Session not found: " + session_id;
            return result;
        }
        
        // Universal Symbolsチェック
        if (!session->latest_result.universal_symbols) {
            result["error"] = "No Universal Symbols in session";
            return result;
        }
        
        // 依存グラフ構築
        auto dep_graph = std::make_shared<DependencyGraph>();
        dep_graph->build_from_symbol_table(*session->latest_result.universal_symbols);
        
        // MoveClassエンジン作成（dry-runモード）
        MoveOptions preview_opts = options;
        preview_opts.dry_run = true;
        preview_opts.verbose = true;
        
        MoveClassEngine engine(
            session->latest_result.universal_symbols,
            dep_graph,
            session->language,
            preview_opts
        );
        
        // プレビュー実行
        auto move_result = engine.preview_move(symbol_id, target_file);
        
        // プレビュー情報作成
        auto preview = std::make_shared<MovePreview>();
        preview->preview_id = generate_move_preview_id();
        preview->session_id = session_id;
        preview->symbol_id = symbol_id;
        preview->target_file = target_file;
        preview->options = options;
        preview->preview_result = move_result;
        preview->created_at = std::chrono::system_clock::now();
        
        // 影響分析詳細を追加
        auto impact = dep_graph->analyze_move_impact(symbol_id, target_file);
        
        // ファイル変更のシミュレーション
        for (const auto& file : impact.affected_files) {
            MovePreview::FileChange change;
            change.file_path = file;
            
            // TODO: 実際の変更内容をシミュレート
            change.changes.push_back({1, "- import { OldClass } from './old-path'"});
            change.changes.push_back({1, "+ import { OldClass } from '" + target_file + "'"});
            
            preview->file_changes.push_back(change);
        }
        
        // 保存
        g_move_previews[preview->preview_id] = preview;
        
        // 結果JSON作成
        result = preview->to_json();
        result["success"] = true;
        result["message"] = "プレビューを生成しました。moveclass-confirmで実行できます。";
        
        // 影響の可視化（ASCIIアート）
        std::stringstream viz;
        viz << "\n📦 MoveClass Preview\n";
        viz << "━━━━━━━━━━━━━━━━━━━━\n";
        viz << "🎯 Symbol: " << symbol_id << "\n";
        viz << "📂 Target: " << target_file << "\n";
        viz << "━━━━━━━━━━━━━━━━━━━━\n";
        viz << "📊 Impact Analysis:\n";
        viz << "  • Symbols to move: " << move_result.moved_symbols.size() << "\n";
        viz << "  • Files affected: " << impact.affected_files.size() << "\n";
        viz << "  • Imports to update: " << impact.required_imports.size() << "\n";
        if (impact.has_circular_dependency) {
            viz << "  ⚠️  Circular dependency detected!\n";
        }
        viz << "━━━━━━━━━━━━━━━━━━━━\n";
        
        result["visualization"] = viz.str();
        
    } catch (const std::exception& e) {
        result["error"] = e.what();
        result["success"] = false;
    }
    
    return result;
}

//=============================================================================
// MoveClass Confirm コマンド
//=============================================================================

nlohmann::json moveclass_confirm(const std::string& preview_id) {
    nlohmann::json result;
    result["command"] = "moveclass-confirm";
    
    try {
        // プレビュー取得
        auto it = g_move_previews.find(preview_id);
        if (it == g_move_previews.end()) {
            result["error"] = "Preview not found or expired: " + preview_id;
            return result;
        }
        
        auto preview = it->second;
        
        // セッション再取得
        auto session_store = SessionStore::get_instance();
        auto session = session_store.get_session(preview->session_id);
        if (!session) {
            result["error"] = "Session not found: " + preview->session_id;
            return result;
        }
        
        // 依存グラフ再構築
        auto dep_graph = std::make_shared<DependencyGraph>();
        dep_graph->build_from_symbol_table(*session->latest_result.universal_symbols);
        
        // 実際の実行（dry-run = false）
        MoveOptions exec_opts = preview->options;
        exec_opts.dry_run = false;
        
        MoveClassEngine engine(
            session->latest_result.universal_symbols,
            dep_graph,
            session->language,
            exec_opts
        );
        
        // 実行
        auto move_result = engine.move_class(preview->symbol_id, preview->target_file);
        
        if (move_result.success) {
            result["success"] = true;
            result["message"] = "MoveClass completed successfully";
            result["moved_symbols"] = move_result.moved_symbols;
            result["updated_files"] = move_result.updated_files;
            
            // 編集履歴に記録
            // TODO: edit_historyへの記録
            
            // プレビューを削除
            g_move_previews.erase(it);
        } else {
            result["success"] = false;
            result["errors"] = move_result.errors;
            result["warnings"] = move_result.warnings;
        }
        
    } catch (const std::exception& e) {
        result["error"] = e.what();
        result["success"] = false;
    }
    
    return result;
}

//=============================================================================
// MoveClass Status コマンド（プレビュー一覧）
//=============================================================================

nlohmann::json moveclass_status() {
    nlohmann::json result;
    result["command"] = "moveclass-status";
    
    cleanup_old_previews();
    
    result["previews"] = nlohmann::json::array();
    for (const auto& [id, preview] : g_move_previews) {
        nlohmann::json preview_info;
        preview_info["preview_id"] = id;
        preview_info["symbol_id"] = preview->symbol_id;
        preview_info["target_file"] = preview->target_file;
        preview_info["created_at"] = std::chrono::system_clock::to_time_t(preview->created_at);
        result["previews"].push_back(preview_info);
    }
    
    result["total"] = g_move_previews.size();
    result["success"] = true;
    
    return result;
}

//=============================================================================
// MoveClass Cancel コマンド（プレビューキャンセル）
//=============================================================================

nlohmann::json moveclass_cancel(const std::string& preview_id) {
    nlohmann::json result;
    result["command"] = "moveclass-cancel";
    
    auto it = g_move_previews.find(preview_id);
    if (it != g_move_previews.end()) {
        g_move_previews.erase(it);
        result["success"] = true;
        result["message"] = "Preview cancelled: " + preview_id;
    } else {
        result["error"] = "Preview not found: " + preview_id;
        result["success"] = false;
    }
    
    return result;
}

} // namespace nekocode