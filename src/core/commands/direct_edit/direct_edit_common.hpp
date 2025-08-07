#pragma once

//=============================================================================
// 🔥 Direct Edit Common - セッション不要編集機能の共通ユーティリティ
//
// Direct Mode編集機能で使用する共通的なファイル操作・バリデーション機能
// SessionData依存を完全に排除した軽量実装
//=============================================================================

#include <filesystem>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>
#include "../../config_manager.hpp"

namespace nekocode {
namespace DirectEdit {

//=============================================================================
// 🗂️ ファイルパス解決ユーティリティ
//=============================================================================

/**
 * ファイルパスを解決（相対パス→絶対パス変換）
 * SessionData不要でcurrent_path()ベース
 */
inline std::filesystem::path resolve_file_path(const std::string& file_path) {
    std::filesystem::path path(file_path);
    
    if (path.is_absolute()) {
        return path;
    }
    
    // 相対パスの場合は現在の作業ディレクトリベース
    return std::filesystem::current_path() / path;
}

/**
 * ファイル存在・読み取り権限チェック
 */
inline bool validate_file_access(const std::filesystem::path& file_path, 
                                 std::string& error_message) {
    if (!std::filesystem::exists(file_path)) {
        error_message = "ファイルが見つかりません: " + file_path.string();
        return false;
    }
    
    if (!std::filesystem::is_regular_file(file_path)) {
        error_message = "通常ファイルではありません: " + file_path.string();
        return false;
    }
    
    // 読み取り権限の簡易チェック
    std::error_code ec;
    auto perms = std::filesystem::status(file_path, ec).permissions();
    if (ec) {
        error_message = "ファイル権限チェックエラー: " + file_path.string();
        return false;
    }
    
    return true;
}

/**
 * ファイル書き込み権限チェック
 */
inline bool validate_write_access(const std::filesystem::path& file_path,
                                  std::string& error_message) {
    // ファイルが存在する場合は書き込み権限チェック
    if (std::filesystem::exists(file_path)) {
        std::error_code ec;
        auto perms = std::filesystem::status(file_path, ec).permissions();
        if (ec) {
            error_message = "書き込み権限チェックエラー: " + file_path.string();
            return false;
        }
        
        // 簡易的な書き込み権限チェック（owner write bit）
        using std::filesystem::perms;
        if ((perms & perms::owner_write) == perms::none) {
            error_message = "書き込み権限がありません: " + file_path.string();
            return false;
        }
    } else {
        // ファイルが存在しない場合は親ディレクトリの書き込み権限チェック
        auto parent_dir = file_path.parent_path();
        if (!std::filesystem::exists(parent_dir)) {
            error_message = "親ディレクトリが存在しません: " + parent_dir.string();
            return false;
        }
        
        std::error_code ec;
        auto perms = std::filesystem::status(parent_dir, ec).permissions();
        if (ec) {
            error_message = "親ディレクトリ権限チェックエラー: " + parent_dir.string();
            return false;
        }
    }
    
    return true;
}

//=============================================================================
// 🆔 Preview ID生成・管理
//=============================================================================

/**
 * ユニークなpreview_id生成
 */
inline std::string generate_preview_id(const std::string& operation_type) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << operation_type << "_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    return ss.str();
}

/**
 * 編集ID生成（confirm用）
 */
inline std::string generate_edit_id() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "edit_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    return ss.str();
}

/**
 * ISO8601形式タイムスタンプ生成
 */
inline std::string generate_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

//=============================================================================
// 📁 メモリディレクトリ管理
//=============================================================================

/**
 * メモリディレクトリ作成（edit_previews/edit_history用）
 */
inline void ensure_memory_directories() {
    std::filesystem::create_directories("memory/edit_previews");
    std::filesystem::create_directories("memory/edit_history");
}

/**
 * プレビューファイル容量ベース削除（設定可能、デフォルト5MB）
 */
inline void cleanup_preview_files(const std::string& preview_dir = "memory/edit_previews") {
    // ConfigManagerから設定値を取得
    auto config = ConfigManager::instance().get_memory_config();
    const size_t MAX_SIZE_BYTES = config.get_preview_max_bytes();
    
    if (!std::filesystem::exists(preview_dir)) {
        return;
    }
    
    std::vector<std::filesystem::directory_entry> entries;
    size_t total_size = 0;
    
    for (const auto& entry : std::filesystem::directory_iterator(preview_dir)) {
        if (entry.path().extension() == ".json") {
            entries.push_back(entry);
            std::error_code ec;
            total_size += std::filesystem::file_size(entry.path(), ec);
        }
    }
    
    if (total_size <= MAX_SIZE_BYTES) {
        return; // 制限内なので削除不要
    }
    
    // 古い順にソート
    std::sort(entries.begin(), entries.end(), 
        [](const auto& a, const auto& b) {
            return std::filesystem::last_write_time(a) < std::filesystem::last_write_time(b);
        });
    
    // 削除実行（容量が制限内になるまで）
    for (const auto& entry : entries) {
        if (total_size <= MAX_SIZE_BYTES) {
            break;
        }
        
        std::error_code ec;
        auto file_size = std::filesystem::file_size(entry.path(), ec);
        std::filesystem::remove(entry.path(), ec);
        total_size -= file_size;
    }
}

/**
 * 容量ベース編集履歴削除（設定可能、デフォルト10MB/最低10件）
 */
inline void cleanup_history_files(const std::string& history_dir = "memory/edit_history") {
    // ConfigManagerから設定値を取得
    auto config = ConfigManager::instance().get_memory_config();
    const size_t MAX_SIZE_BYTES = config.get_history_max_bytes();
    const size_t MIN_FILES_KEEP = config.history_min_files;
    
    if (!std::filesystem::exists(history_dir)) {
        return;
    }
    
    // 全履歴ファイルを収集
    struct HistoryFile {
        std::filesystem::path json_file;
        std::filesystem::path before_file;  
        std::filesystem::path after_file;
        std::filesystem::file_time_type create_time;
        size_t total_size;
    };
    
    std::vector<HistoryFile> history_files;
    size_t total_size = 0;
    
    for (const auto& entry : std::filesystem::directory_iterator(history_dir)) {
        if (entry.path().extension() == ".json") {
            std::string base_name = entry.path().stem().string();
            
            HistoryFile hf;
            hf.json_file = entry.path();
            hf.before_file = std::filesystem::path(history_dir) / (base_name + "_before.txt");
            hf.after_file = std::filesystem::path(history_dir) / (base_name + "_after.txt");
            hf.create_time = std::filesystem::last_write_time(entry.path());
            
            // ファイルサイズ計算
            hf.total_size = 0;
            std::error_code ec;
            if (std::filesystem::exists(hf.json_file)) {
                hf.total_size += std::filesystem::file_size(hf.json_file, ec);
            }
            if (std::filesystem::exists(hf.before_file)) {
                hf.total_size += std::filesystem::file_size(hf.before_file, ec);
            }
            if (std::filesystem::exists(hf.after_file)) {
                hf.total_size += std::filesystem::file_size(hf.after_file, ec);
            }
            
            history_files.push_back(hf);
            total_size += hf.total_size;
        }
    }
    
    // 容量チェック
    if (total_size <= MAX_SIZE_BYTES) {
        return; // 制限内なので削除不要
    }
    
    // 古い順にソート
    std::sort(history_files.begin(), history_files.end(),
        [](const HistoryFile& a, const HistoryFile& b) {
            return a.create_time < b.create_time;
        });
    
    // 削除実行（容量が制限内になるまで）
    size_t files_removed = 0;
    for (const auto& hf : history_files) {
        // 最低保持件数チェック
        if (history_files.size() - files_removed <= MIN_FILES_KEEP) {
            break;
        }
        
        // 容量チェック
        if (total_size <= MAX_SIZE_BYTES) {
            break;
        }
        
        // ファイル削除
        std::error_code ec;
        std::filesystem::remove(hf.json_file, ec);
        std::filesystem::remove(hf.before_file, ec);
        std::filesystem::remove(hf.after_file, ec);
        
        total_size -= hf.total_size;
        files_removed++;
    }
}

//=============================================================================
// 📄 ファイル読み書きユーティリティ
//=============================================================================

/**
 * ファイル内容を行ベクターとして読み込み
 */
inline bool read_file_lines(const std::filesystem::path& file_path, 
                           std::vector<std::string>& lines, 
                           std::string& error_message) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        error_message = "ファイルを開けません: " + file_path.string();
        return false;
    }
    
    lines.clear();
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    
    return true;
}

/**
 * 行ベクターをファイルに書き込み
 */
inline bool write_file_lines(const std::filesystem::path& file_path,
                            const std::vector<std::string>& lines,
                            std::string& error_message) {
    std::ofstream file(file_path);
    if (!file.is_open()) {
        error_message = "ファイルに書き込めません: " + file_path.string();
        return false;
    }
    
    for (size_t i = 0; i < lines.size(); i++) {
        file << lines[i];
        if (i < lines.size() - 1) {
            file << "\n";
        }
    }
    
    return true;
}

//=============================================================================
// 📊 編集履歴統計情報
//=============================================================================

struct EditHistoryStats {
    size_t history_files = 0;
    size_t history_size_bytes = 0;
    size_t preview_files = 0;
    size_t preview_size_bytes = 0;
    
    nlohmann::json to_json() const {
        // ConfigManagerから現在の設定値を取得
        auto config = ConfigManager::instance().get_memory_config();
        
        return {
            {"edit_history", {
                {"files", history_files},
                {"size_bytes", history_size_bytes},
                {"size_mb", history_size_bytes / 1024.0 / 1024.0}
            }},
            {"edit_previews", {
                {"files", preview_files}, 
                {"size_bytes", preview_size_bytes},
                {"size_mb", preview_size_bytes / 1024.0 / 1024.0}
            }},
            {"limits", {
                {"history_max_mb", static_cast<double>(config.history_max_mb)},
                {"preview_max_mb", static_cast<double>(config.preview_max_mb)},
                {"history_min_files_keep", config.history_min_files},
                {"configured", ConfigManager::instance().is_configured()}
            }},
            {"summary", {
                {"total_size_mb", (history_size_bytes + preview_size_bytes) / 1024.0 / 1024.0},
                {"history_usage_percent", (history_size_bytes / static_cast<double>(config.get_history_max_bytes())) * 100},
                {"preview_usage_percent", (preview_size_bytes / static_cast<double>(config.get_preview_max_bytes())) * 100}
            }}
        };
    }
};

inline EditHistoryStats get_edit_history_stats() {
    EditHistoryStats stats;
    
    // 編集履歴統計
    if (std::filesystem::exists("memory/edit_history")) {
        for (const auto& entry : std::filesystem::directory_iterator("memory/edit_history")) {
            if (entry.path().extension() == ".json") {
                stats.history_files++;
                std::error_code ec;
                stats.history_size_bytes += std::filesystem::file_size(entry.path(), ec);
                
                std::string base_name = entry.path().stem().string();
                auto before_file = std::filesystem::path("memory/edit_history") / (base_name + "_before.txt");
                auto after_file = std::filesystem::path("memory/edit_history") / (base_name + "_after.txt");
                
                if (std::filesystem::exists(before_file)) {
                    stats.history_size_bytes += std::filesystem::file_size(before_file, ec);
                }
                if (std::filesystem::exists(after_file)) {
                    stats.history_size_bytes += std::filesystem::file_size(after_file, ec);
                }
            }
        }
    }
    
    // プレビュー統計
    if (std::filesystem::exists("memory/edit_previews")) {
        for (const auto& entry : std::filesystem::directory_iterator("memory/edit_previews")) {
            if (entry.path().extension() == ".json") {
                stats.preview_files++;
                std::error_code ec;
                stats.preview_size_bytes += std::filesystem::file_size(entry.path(), ec);
            }
        }
    }
    
    return stats;
}

//=============================================================================
// 🔄 Direct行移動機能宣言
//=============================================================================

/// 行移動プレビュー生成
nlohmann::json movelines_preview(const std::string& srcfile,
                                 int start_line, int line_count,
                                 const std::string& dstfile, 
                                 int insert_line);

/// 行移動実行
nlohmann::json movelines_confirm(const std::string& preview_id);

} // namespace DirectEdit
} // namespace nekocode