//=============================================================================
// 📥 Insert Commands実装 - NekoCode挿入機能
//
// edit_commands.cpp分割 - Insert系機能群
// 責任: 挿入プレビュー、挿入実行の実装
//=============================================================================

#include "nekocode/edit_commands.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace nekocode {

//=============================================================================
// 🎯 統一挿入プレビュー機能
//=============================================================================

nlohmann::json EditCommands::cmd_insert_preview(const SessionData& session,
                                                   const std::string& file_path,
                                                   const std::string& position,
                                                   const std::string& content) const {
    nlohmann::json result;
    
    try {
        // ファイルパス解決（SessionData対応）
        std::filesystem::path target_file;
        if (std::filesystem::path(file_path).is_absolute()) {
            target_file = file_path;
        } else {
            // セッションがファイルの場合は親ディレクトリ基準
            if (!session.is_directory) {
                target_file = session.target_path.parent_path() / file_path;
            } else {
                target_file = session.target_path / file_path;
            }
        }
        
        // プロジェクト境界チェック
        std::filesystem::path base_path = session.is_directory ? 
            session.target_path : session.target_path.parent_path();
        auto relative_check = std::filesystem::relative(target_file, base_path);
        std::string rel_str = relative_check.string();
        if (rel_str.length() >= 2 && rel_str.substr(0, 2) == "..") {
            result["error"] = "プロジェクト外のファイルは編集できません";
            return result;
        }
        
        // ファイル存在チェック
        if (!std::filesystem::exists(target_file)) {
            result["error"] = "ファイルが見つかりません: " + target_file.string();
            return result;
        }
        
        // ファイル読み込み
        std::ifstream file(target_file);
        if (!file.is_open()) {
            result["error"] = "ファイルを開けません: " + target_file.string();
            return result;
        }
        
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        file.close();
        
        int total_lines = lines.size();
        
        // 位置解決
        int insert_line = 0;  // 0-based index for vector
        std::string position_description;
        bool is_pattern_based = false;
        std::string pattern;
        
        // position解析
        if (position == "start" || position == "top" || position == "0") {
            insert_line = 0;
            position_description = "ファイル先頭";
        } else if (position == "end" || position == "bottom") {
            insert_line = total_lines;
            position_description = "ファイル末尾";
        } else if (position.substr(0, 7) == "before:") {
            // パターンマッチ（将来実装）
            pattern = position.substr(7);
            is_pattern_based = true;
            result["error"] = "パターンマッチはまだ実装されていません";
            return result;
        } else if (position.substr(0, 6) == "after:") {
            // パターンマッチ（将来実装）
            pattern = position.substr(6);
            is_pattern_based = true;
            result["error"] = "パターンマッチはまだ実装されていません";
            return result;
        } else {
            // 行番号として解析（1-based → 0-based）
            try {
                int line_num = std::stoi(position);
                if (line_num < 0) {
                    insert_line = 0;
                } else if (line_num > total_lines) {
                    insert_line = total_lines;
                } else {
                    insert_line = line_num - 1;  // 1-based to 0-based
                }
                position_description = std::to_string(line_num) + "行目";
            } catch (...) {
                result["error"] = "無効な位置指定: " + position;
                return result;
            }
        }
        
        // コンテキスト収集（前後3行）
        const int CONTEXT_LINES = 3;
        std::vector<nlohmann::json> before_lines;
        std::vector<nlohmann::json> after_lines;
        
        // 前の行
        for (int i = std::max(0, insert_line - CONTEXT_LINES); i < insert_line && i < total_lines; i++) {
            before_lines.push_back({
                {"line", i + 1},  // 1-based for display
                {"content", lines[i]}
            });
        }
        
        // 後の行
        for (int i = insert_line; i < std::min(total_lines, insert_line + CONTEXT_LINES); i++) {
            after_lines.push_back({
                {"line", i + 1},  // 1-based for display
                {"content", lines[i]}
            });
        }
        
        // プレビューID生成
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << "insert_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
        std::string preview_id = ss.str();
        
        // memoryディレクトリ作成
        std::filesystem::path memory_dir = "memory/edit_previews";
        std::filesystem::create_directories(memory_dir);
        
        // 詳細情報をmemoryに保存
        std::stringstream time_str;
        time_str << std::put_time(std::localtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
        
        nlohmann::json preview_details = {
            {"preview_id", preview_id},
            {"created_at", time_str.str()},
            {"file_info", {
                {"path", target_file.string()},
                {"total_lines", total_lines}
            }},
            {"operation", {
                {"type", "insert"},
                {"position", position},
                {"resolved_line", insert_line + 1},  // 1-based for display
                {"content", content}
            }},
            {"detailed_context", {
                {"before_10_lines", nlohmann::json::array()},
                {"after_10_lines", nlohmann::json::array()}
            }}
        };
        
        // 詳細コンテキスト（memory用、前後10行）
        for (int i = std::max(0, insert_line - 10); i < insert_line && i < total_lines; i++) {
            preview_details["detailed_context"]["before_10_lines"].push_back({
                {"line", i + 1},
                {"content", lines[i]}
            });
        }
        for (int i = insert_line; i < std::min(total_lines, insert_line + 10); i++) {
            preview_details["detailed_context"]["after_10_lines"].push_back({
                {"line", i + 1},
                {"content", lines[i]}
            });
        }
        
        // memoryに保存
        std::filesystem::path preview_file = memory_dir / (preview_id + ".json");
        std::ofstream memory_file(preview_file);
        if (memory_file.is_open()) {
            memory_file << preview_details.dump(2);
            memory_file.close();
        }
        
        // 軽量応答を返却
        result = {
            {"preview_id", preview_id},
            {"file_path", target_file.string()},
            {"position", position},
            {"resolved_line", insert_line + 1},  // 1-based for display
            {"insert_content", content},
            {"context", {
                {"before_lines", before_lines},
                {"after_lines", after_lines}
            }},
            {"summary", position_description + "に挿入"},
            {"more_details", "詳細は edit-show " + preview_id + " で確認"}
        };
        
    } catch (const std::exception& e) {
        result["error"] = std::string("挿入プレビューエラー: ") + e.what();
    }
    
    return result;
}

//=============================================================================
// 🚀 挿入実行確定
//=============================================================================

nlohmann::json EditCommands::cmd_insert_confirm(const SessionData& session,
                                                   const std::string& preview_id) const {
    nlohmann::json result;
    
    try {
        // プレビューファイル読み込み
        std::filesystem::path preview_file = "memory/edit_previews/" + preview_id + ".json";
        if (!std::filesystem::exists(preview_file)) {
            result["error"] = "プレビューが見つかりません: " + preview_id;
            return result;
        }
        
        std::ifstream preview_stream(preview_file);
        nlohmann::json preview_data;
        preview_stream >> preview_data;
        preview_stream.close();
        
        // ファイル情報取得
        std::string file_path = preview_data["file_info"]["path"];
        int insert_line = preview_data["operation"]["resolved_line"].get<int>() - 1;  // 1-based to 0-based
        std::string insert_content = preview_data["operation"]["content"];
        
        // ファイル読み込み
        std::ifstream file(file_path);
        if (!file.is_open()) {
            result["error"] = "ファイルを開けません: " + file_path;
            return result;
        }
        
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }
        file.close();
        
        // 履歴ID生成
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << "edit_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
        std::string edit_id = ss.str();
        
        // memoryに変更前ファイル保存
        std::filesystem::path history_dir = "memory/edit_history";
        std::filesystem::create_directories(history_dir);
        
        std::filesystem::path before_file = history_dir / (edit_id + "_before.txt");
        std::ofstream before_stream(before_file);
        for (const auto& l : lines) {
            before_stream << l << "\n";
        }
        before_stream.close();
        
        // 挿入実行
        // contentを行ごとに分割
        std::vector<std::string> insert_lines;
        std::istringstream content_stream(insert_content);
        std::string content_line;
        while (std::getline(content_stream, content_line)) {
            insert_lines.push_back(content_line);
        }
        
        // 新しい内容を構築
        std::vector<std::string> new_lines;
        for (int i = 0; i < insert_line && i < lines.size(); i++) {
            new_lines.push_back(lines[i]);
        }
        for (const auto& l : insert_lines) {
            new_lines.push_back(l);
        }
        for (int i = insert_line; i < lines.size(); i++) {
            new_lines.push_back(lines[i]);
        }
        
        // ファイル書き込み
        std::ofstream out_file(file_path);
        if (!out_file.is_open()) {
            result["error"] = "ファイルに書き込めません: " + file_path;
            return result;
        }
        for (size_t i = 0; i < new_lines.size(); i++) {
            out_file << new_lines[i];
            if (i < new_lines.size() - 1) {
                out_file << "\n";
            }
        }
        out_file.close();
        
        // memoryに変更後ファイル保存
        std::filesystem::path after_file = history_dir / (edit_id + "_after.txt");
        std::ofstream after_stream(after_file);
        for (const auto& l : new_lines) {
            after_stream << l << "\n";
        }
        after_stream.close();
        
        // 履歴メタデータ保存
        std::stringstream time_str;
        time_str << std::put_time(std::localtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
        
        nlohmann::json history_data = {
            {"edit_id", edit_id},
            {"preview_id", preview_id},
            {"timestamp", time_str.str()},
            {"operation", "insert"},
            {"file_info", {
                {"path", file_path},
                {"lines_before", lines.size()},
                {"lines_after", new_lines.size()}
            }},
            {"change_details", {
                {"position", preview_data["operation"]["position"]},
                {"resolved_line", insert_line + 1},
                {"inserted_lines", insert_lines.size()}
            }}
        };
        
        std::filesystem::path history_meta = history_dir / (edit_id + ".json");
        std::ofstream history_stream(history_meta);
        history_stream << history_data.dump(2);
        history_stream.close();
        
        // 100件制限チェック（古いものを削除）
        auto entries = std::vector<std::filesystem::directory_entry>();
        for (const auto& entry : std::filesystem::directory_iterator(history_dir)) {
            if (entry.path().extension() == ".json") {
                entries.push_back(entry);
            }
        }
        
        if (entries.size() > 100) {
            // 古い順にソート
            std::sort(entries.begin(), entries.end(), 
                [](const auto& a, const auto& b) {
                    return std::filesystem::last_write_time(a) < std::filesystem::last_write_time(b);
                });
            
            // 古いものを削除（100件を超える分）
            for (size_t i = 0; i < entries.size() - 100; i++) {
                std::string base_name = entries[i].path().stem().string();
                std::filesystem::remove(history_dir / (base_name + ".json"));
                std::filesystem::remove(history_dir / (base_name + "_before.txt"));
                std::filesystem::remove(history_dir / (base_name + "_after.txt"));
                if (std::filesystem::exists(history_dir / (base_name + "_diff.txt"))) {
                    std::filesystem::remove(history_dir / (base_name + "_diff.txt"));
                }
            }
        }
        
        result = {
            {"success", true},
            {"edit_id", edit_id},
            {"preview_id", preview_id},
            {"file_path", file_path},
            {"position", preview_data["operation"]["position"]},
            {"resolved_line", insert_line + 1},
            {"summary", "挿入実行完了: " + file_path}
        };
        
    } catch (const std::exception& e) {
        result["error"] = std::string("挿入実行エラー: ") + e.what();
    }
    
    return result;
}

} // namespace nekocode