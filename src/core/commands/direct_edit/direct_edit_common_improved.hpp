//=============================================================================
// 🔥 Direct Edit Common - 改善版容量ベース履歴管理
//=============================================================================

#pragma once
#include <filesystem>
#include <string>

namespace nekocode {
namespace DirectEdit {

//=============================================================================
// 📊 容量ベース履歴管理 (改善版)
//=============================================================================

struct HistoryConfig {
    static constexpr size_t DEFAULT_MAX_SIZE_BYTES = 10 * 1024 * 1024;  // 10MB
    static constexpr size_t DEFAULT_MIN_FILES_KEEP = 10;                // 最低保持件数
    static constexpr size_t DEFAULT_MAX_FILES_LIMIT = 200;             // 最大件数上限
};

/**
 * 容量ベース履歴クリーンアップ（改善版）
 * 
 * 特徴:
 * - 10MB制限（設定可能）
 * - 最低10件は保持（重要な履歴を保護）
 * - 古いものから削除
 * - 効率的なサイズ計算
 */
inline void cleanup_history_files_smart(const std::string& history_dir = "memory/edit_history") {
    if (!std::filesystem::exists(history_dir)) {
        return;
    }
    
    // 1. 全履歴ファイルを取得（作成時間でソート）
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
            
            // サイズ計算
            hf.total_size = 0;
            if (std::filesystem::exists(hf.json_file)) {
                hf.total_size += std::filesystem::file_size(hf.json_file);
            }
            if (std::filesystem::exists(hf.before_file)) {
                hf.total_size += std::filesystem::file_size(hf.before_file);
            }
            if (std::filesystem::exists(hf.after_file)) {
                hf.total_size += std::filesystem::file_size(hf.after_file);
            }
            
            history_files.push_back(hf);
            total_size += hf.total_size;
        }
    }
    
    // 2. 容量チェック
    if (total_size <= HistoryConfig::DEFAULT_MAX_SIZE_BYTES) {
        return; // 制限内なので何もしない
    }
    
    // 3. 古い順にソート
    std::sort(history_files.begin(), history_files.end(),
        [](const HistoryFile& a, const HistoryFile& b) {
            return a.create_time < b.create_time;
        });
    
    // 4. 削除実行（容量が制限内になるまで）
    size_t files_removed = 0;
    for (const auto& hf : history_files) {
        // 最低保持件数チェック
        if (history_files.size() - files_removed <= HistoryConfig::DEFAULT_MIN_FILES_KEEP) {
            break;
        }
        
        // 容量チェック
        if (total_size <= HistoryConfig::DEFAULT_MAX_SIZE_BYTES) {
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

/**
 * プレビューファイル容量ベース管理
 * プレビューは短期間なので5MB制限
 */
inline void cleanup_preview_files_smart(const std::string& preview_dir = "memory/edit_previews") {
    static constexpr size_t PREVIEW_MAX_SIZE = 5 * 1024 * 1024;  // 5MB
    
    if (!std::filesystem::exists(preview_dir)) {
        return;
    }
    
    std::vector<std::filesystem::directory_entry> entries;
    size_t total_size = 0;
    
    for (const auto& entry : std::filesystem::directory_iterator(preview_dir)) {
        if (entry.path().extension() == ".json") {
            entries.push_back(entry);
            total_size += std::filesystem::file_size(entry.path());
        }
    }
    
    if (total_size <= PREVIEW_MAX_SIZE) {
        return;
    }
    
    // 古い順にソート
    std::sort(entries.begin(), entries.end(),
        [](const auto& a, const auto& b) {
            return std::filesystem::last_write_time(a) < std::filesystem::last_write_time(b);
        });
    
    // 削除実行
    for (const auto& entry : entries) {
        if (total_size <= PREVIEW_MAX_SIZE) {
            break;
        }
        
        auto file_size = std::filesystem::file_size(entry.path());
        std::filesystem::remove(entry.path());
        total_size -= file_size;
    }
}

//=============================================================================
// 📊 統計情報取得
//=============================================================================

struct EditHistoryStats {
    size_t total_files = 0;
    size_t total_size_bytes = 0;
    size_t preview_files = 0;
    size_t preview_size_bytes = 0;
    std::string oldest_edit;
    std::string newest_edit;
    
    nlohmann::json to_json() const {
        return {
            {"history", {
                {"files", total_files},
                {"size_bytes", total_size_bytes},
                {"size_mb", total_size_bytes / 1024.0 / 1024.0},
                {"oldest", oldest_edit},
                {"newest", newest_edit}
            }},
            {"previews", {
                {"files", preview_files}, 
                {"size_bytes", preview_size_bytes},
                {"size_mb", preview_size_bytes / 1024.0 / 1024.0}
            }},
            {"limits", {
                {"max_history_mb", HistoryConfig::DEFAULT_MAX_SIZE_BYTES / 1024.0 / 1024.0},
                {"max_preview_mb", 5.0},
                {"min_files_keep", HistoryConfig::DEFAULT_MIN_FILES_KEEP}
            }}
        };
    }
};

inline EditHistoryStats get_edit_history_stats() {
    EditHistoryStats stats;
    
    // 履歴統計
    if (std::filesystem::exists("memory/edit_history")) {
        for (const auto& entry : std::filesystem::directory_iterator("memory/edit_history")) {
            if (entry.path().extension() == ".json") {
                stats.total_files++;
                stats.total_size_bytes += std::filesystem::file_size(entry.path());
                
                std::string base_name = entry.path().stem().string();
                auto before_file = std::filesystem::path("memory/edit_history") / (base_name + "_before.txt");
                auto after_file = std::filesystem::path("memory/edit_history") / (base_name + "_after.txt");
                
                if (std::filesystem::exists(before_file)) {
                    stats.total_size_bytes += std::filesystem::file_size(before_file);
                }
                if (std::filesystem::exists(after_file)) {
                    stats.total_size_bytes += std::filesystem::file_size(after_file);
                }
            }
        }
    }
    
    // プレビュー統計
    if (std::filesystem::exists("memory/edit_previews")) {
        for (const auto& entry : std::filesystem::directory_iterator("memory/edit_previews")) {
            if (entry.path().extension() == ".json") {
                stats.preview_files++;
                stats.preview_size_bytes += std::filesystem::file_size(entry.path());
            }
        }
    }
    
    return stats;
}

} // namespace DirectEdit
} // namespace nekocode