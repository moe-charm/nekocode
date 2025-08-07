//=============================================================================
// 📋 Edit History Commands実装 - NekoCode編集履歴機能
//
// edit_commands.cpp分割 - History系機能群
// 責任: 編集履歴一覧、編集詳細表示の実装
//=============================================================================

#include "nekocode/edit_commands.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace nekocode {

//=============================================================================
// 📋 編集履歴一覧
//=============================================================================

nlohmann::json EditCommands::cmd_edit_history(const SessionData& session) const {
    nlohmann::json result;
    
    try {
        std::filesystem::path history_dir = "memory/edit_history";
        std::vector<nlohmann::json> history_list;
        
        if (std::filesystem::exists(history_dir)) {
            // JSONファイルを収集
            std::vector<std::filesystem::directory_entry> entries;
            for (const auto& entry : std::filesystem::directory_iterator(history_dir)) {
                if (entry.path().extension() == ".json") {
                    entries.push_back(entry);
                }
            }
            
            // 新しい順にソート
            std::sort(entries.begin(), entries.end(), 
                [](const auto& a, const auto& b) {
                    return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
                });
            
            // 履歴読み込み（最新20件）
            for (size_t i = 0; i < std::min(entries.size(), size_t(20)); i++) {
                std::ifstream file(entries[i].path());
                nlohmann::json history_data;
                file >> history_data;
                
                // 簡易情報のみ
                history_list.push_back({
                    {"edit_id", history_data["edit_id"]},
                    {"timestamp", history_data["timestamp"]},
                    {"file", history_data["file_info"]["path"]},
                    {"operation", history_data["operation"]},
                    {"pattern", history_data["change_details"]["pattern"]}
                });
            }
        }
        
        result = {
            {"command", "edit-history"},
            {"total_count", history_list.size()},
            {"history", history_list},
            {"summary", "最新20件の編集履歴"}
        };
        
    } catch (const std::exception& e) {
        result["error"] = std::string("履歴取得エラー: ") + e.what();
    }
    
    return result;
}

//=============================================================================
// 🔍 編集詳細表示
//=============================================================================

nlohmann::json EditCommands::cmd_edit_show(const SessionData& session,
                                             const std::string& id) const {
    nlohmann::json result;
    
    try {
        // preview_かedit_で判定
        std::filesystem::path target_file;
        if (id.substr(0, 8) == "preview_") {
            target_file = "memory/edit_previews/" + id + ".json";
        } else if (id.substr(0, 5) == "edit_") {
            target_file = "memory/edit_history/" + id + ".json";
        } else {
            result["error"] = "無効なID形式: " + id;
            return result;
        }
        
        if (!std::filesystem::exists(target_file)) {
            result["error"] = "指定されたIDが見つかりません: " + id;
            return result;
        }
        
        // JSON読み込み
        std::ifstream file(target_file);
        nlohmann::json data;
        file >> data;
        file.close();
        
        result = {
            {"command", "edit-show"},
            {"id", id},
            {"details", data}
        };
        
        // edit_の場合は差分も表示可能にする
        if (id.substr(0, 5) == "edit_") {
            std::filesystem::path before_path = "memory/edit_history/" + id + "_before.txt";
            std::filesystem::path after_path = "memory/edit_history/" + id + "_after.txt";
            
            if (std::filesystem::exists(before_path) && std::filesystem::exists(after_path)) {
                result["files_available"] = {
                    {"before", before_path.string()},
                    {"after", after_path.string()}
                };
            }
        }
        
    } catch (const std::exception& e) {
        result["error"] = std::string("詳細取得エラー: ") + e.what();
    }
    
    return result;
}

} // namespace nekocode