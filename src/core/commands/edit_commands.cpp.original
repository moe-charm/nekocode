//=============================================================================
// 🐱 Edit Commands - NekoCode独自編集機能実装
//
// SessionCommandsから分離した編集専用コマンド群
// 責任: ファイル編集、置換、挿入、行移動等の実装
//=============================================================================

#include "nekocode/edit_commands.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <regex>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace nekocode {

//=============================================================================
// 🐱 NekoCode独自編集機能実装
//=============================================================================

nlohmann::json EditCommands::cmd_replace(const SessionData& session,
                                         const std::string& file_path,
                                         const std::string& pattern,
                                         const std::string& replacement) const {
    
    nlohmann::json result = {
        {"command", "replace"},
        {"file_path", file_path},
        {"pattern", pattern},
        {"replacement", replacement}
    };
    
    try {
        // 1. ファイルパス解決 (SessionData活用)
        std::filesystem::path target_path;
        
        if (std::filesystem::path(file_path).is_absolute()) {
            target_path = file_path;
        } else {
            // SessionDataのプロジェクトルートからの相対パス
            if (session.is_directory) {
                target_path = session.target_path / file_path;
            } else {
                target_path = session.target_path.parent_path() / file_path;
            }
        }
        
        // 2. ファイル存在チェック
        if (!std::filesystem::exists(target_path)) {
            result["error"] = "ファイルが見つかりません: " + file_path;
            return result;
        }
        
        // 3. プロジェクト内ファイルかチェック (安全性)
        std::filesystem::path project_root = session.is_directory ? 
            session.target_path : session.target_path.parent_path();
        
        auto relative_check = std::filesystem::relative(target_path, project_root);
        std::string rel_str = relative_check.string();
        if (rel_str.length() >= 2 && rel_str.substr(0, 2) == "..") {
            result["error"] = "プロジェクト外のファイルは編集できません";
            return result;
        }
        
        // 4. ファイル読み込み
        std::ifstream file(target_path);
        if (!file.is_open()) {
            result["error"] = "ファイルを開けません: " + target_path.string();
            return result;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        // 5. 正規表現処理
        std::regex regex_pattern;
        try {
            regex_pattern = std::regex(pattern);
        } catch (const std::regex_error& e) {
            result["error"] = "正規表現エラー: " + std::string(e.what());
            return result;
        }
        
        // 6. マッチ検索
        std::smatch matches;
        if (!std::regex_search(content, matches, regex_pattern)) {
            result["warning"] = "パターンにマッチするものが見つかりませんでした";
            result["matches_found"] = 0;
            return result;
        }
        
        // 7. 置換実行
        std::string new_content = std::regex_replace(content, regex_pattern, replacement);
        
        // 8. 変更があるかチェック
        if (content == new_content) {
            result["warning"] = "変更がありませんでした";
            return result;
        }
        
        // 9. ファイル更新
        std::ofstream output_file(target_path);
        if (!output_file.is_open()) {
            result["error"] = "ファイルに書き込めません: " + target_path.string();
            return result;
        }
        
        output_file << new_content;
        output_file.close();
        
        // 10. 成功レポート
        result["success"] = true;
        result["file_updated"] = target_path.filename().string();
        result["size_before"] = content.size();
        result["size_after"] = new_content.size();
        
        // マッチした内容を表示（最初の1つだけ）
        if (matches.size() > 0) {
            result["matched_content"] = matches[0].str();
        }
        
    } catch (const std::exception& e) {
        result["error"] = std::string("Replace error: ") + e.what();
    }
    
    return result;
}

//=============================================================================
// 🔮 置換プレビュー機能
//=============================================================================

nlohmann::json EditCommands::cmd_replace_preview(const SessionData& session,
                                                 const std::string& file_path,
                                                 const std::string& pattern,
                                                 const std::string& replacement) const {
    nlohmann::json result = {
        {"command", "replace-preview"}
    };
    
    try {
        // ファイルパス解決（SessionData対応）
        std::filesystem::path target_path;
        if (std::filesystem::path(file_path).is_absolute()) {
            target_path = file_path;
        } else {
            // セッションがファイルの場合は親ディレクトリ基準
            if (session.is_directory) {
                target_path = session.target_path / file_path;
            } else {
                target_path = session.target_path.parent_path() / file_path;
            }
        }
        
        // プロジェクト境界チェック
        std::filesystem::path project_root = session.is_directory ? 
            session.target_path : session.target_path.parent_path();
        
        auto relative_check = std::filesystem::relative(target_path, project_root);
        std::string rel_str = relative_check.string();
        if (rel_str.length() >= 2 && rel_str.substr(0, 2) == "..") {
            result["error"] = "プロジェクト外のファイルは編集できません";
            return result;
        }
        
        // ファイル存在チェック
        if (!std::filesystem::exists(target_path)) {
            result["error"] = "ファイルが見つかりません: " + file_path;
            return result;
        }
        
        // ファイル読み込み
        std::ifstream file(target_path);
        if (!file.is_open()) {
            result["error"] = "ファイルを開けません: " + target_path.string();
            return result;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        // 正規表現でマッチング
        std::regex regex_pattern;
        try {
            regex_pattern = std::regex(pattern);
        } catch (const std::regex_error& e) {
            result["error"] = "正規表現エラー: " + std::string(e.what());
            return result;
        }
        
        // マッチ検索とサンプル収集
        std::vector<nlohmann::json> all_matches;
        std::vector<nlohmann::json> sample_matches;
        
        // memoryに保存する全マッチ
        auto matches_begin = std::sregex_iterator(content.begin(), content.end(), regex_pattern);
        auto matches_end = std::sregex_iterator();
        
        int match_count = 0;
        for (std::sregex_iterator it = matches_begin; it != matches_end; ++it) {
            std::smatch match = *it;
            
            // 行番号計算
            size_t line_start = content.rfind('\n', match.position());
            size_t line_number = std::count(content.begin(), content.begin() + match.position(), '\n') + 1;
            
            nlohmann::json match_info = {
                {"line", line_number},
                {"matched", match.str()},
                {"position", match.position()}
            };
            
            all_matches.push_back(match_info);
            
            // 詳細情報（memoryに保存）
            if (match_count < 50) { // 最大50個の詳細を保存
                match_info["context_before"] = "";
                match_info["context_after"] = "";
                // コンテキスト情報の追加は実装を簡略化
            }
            
            match_count++;
        }
        
        // サンプル（最初の5個のみ）
        for (size_t i = 0; i < std::min(all_matches.size(), size_t(5)); ++i) {
            sample_matches.push_back(all_matches[i]);
        }
        
        if (all_matches.empty()) {
            result["error"] = "パターンにマッチする内容が見つかりませんでした";
            result["pattern"] = pattern;
            return result;
        }
        
        // プレビューID生成
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << "preview_" << std::put_time(std::gmtime(&time_t), "%Y%m%d_%H%M%S");
        std::string preview_id = ss.str();
        
        // サイズ変更計算
        std::string new_content = std::regex_replace(content, regex_pattern, replacement);
        int size_change = static_cast<int>(new_content.size()) - static_cast<int>(content.size());
        
        // memoryディレクトリ作成
        std::filesystem::create_directories("memory");
        
        // 詳細情報をmemoryに保存
        nlohmann::json detailed_info = {
            {"preview_id", preview_id},
            {"file_path", target_path.string()},
            {"pattern", pattern},
            {"replacement", replacement},
            {"all_matches", all_matches},
            {"total_matches", match_count},
            {"size_change", size_change},
            {"content_original", content},
            {"content_new", new_content}
        };
        
        // 今後拡張
        std::string memory_file = "memory/" + preview_id + ".json";
        std::ofstream stream(memory_file);
        if (stream.is_open()) {
            stream << detailed_info.dump(2);
            stream.close();
        }
        
        // memoryに保存
        
        // 軽量応答を返却
        result = {
            {"preview_id", preview_id},
            {"file_path", target_path.string()},
            {"pattern", pattern},
            {"replacement", replacement},
            {"sample_matches", sample_matches},
            {"summary", {
                {"total_matches", match_count},
                {"size_change", std::to_string(size_change) + " bytes"},
                {"risk_level", match_count > 50 ? "high" : (match_count > 10 ? "medium" : "low")}
            }},
            {"more_details", "詳細は edit-show " + preview_id + " で確認"}
        };
        
    } catch (const std::exception& e) {
        result["error"] = std::string("Preview error: ") + e.what();
    }
    
    return result;
}

} // namespace nekocodenlohmann::json EditCommands::cmd_replace_confirm(const SessionData& session,
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
        std::string pattern = preview_data["operation"]["pattern"];
        std::string replacement = preview_data["operation"]["replacement"];
        
        // ファイル読み込み
        std::ifstream file(file_path);
        if (!file.is_open()) {
            result["error"] = "ファイルを開けません: " + file_path;
            return result;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
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
        before_stream << content;
        before_stream.close();
        
        // 置換実行
        std::regex regex_pattern(pattern);
        std::string new_content = std::regex_replace(content, regex_pattern, replacement);
        
        // ファイル書き込み
        std::ofstream out_file(file_path);
        if (!out_file.is_open()) {
            result["error"] = "ファイルに書き込めません: " + file_path;
            return result;
        }
        out_file << new_content;
        out_file.close();
        
        // memoryに変更後ファイル保存
        std::filesystem::path after_file = history_dir / (edit_id + "_after.txt");
        std::ofstream after_stream(after_file);
        after_stream << new_content;
        after_stream.close();
        
        // 履歴メタデータ保存
        std::stringstream time_str;
        time_str << std::put_time(std::localtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
        
        nlohmann::json history_data = {
            {"edit_id", edit_id},
            {"preview_id", preview_id},
            {"timestamp", time_str.str()},
            {"operation", "replace"},
            {"file_info", {
                {"path", file_path},
                {"size_before", content.length()},
                {"size_after", new_content.length()}
            }},
            {"change_details", {
                {"pattern", pattern},
                {"replacement", replacement},
                {"matches_count", preview_data["analysis"]["total_matches"]}
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
                // diff.txtがあれば削除（将来の拡張用）
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
            {"pattern", pattern},
            {"replacement", replacement},
            {"summary", "置換実行完了: " + file_path}
        };
        
    } catch (const std::exception& e) {
        result["error"] = std::string("置換実行エラー: ") + e.what();
    }
    
    return result;
}

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

//=============================================================================
// 📝 行移動機能（movelines）
//=============================================================================

nlohmann::json EditCommands::cmd_movelines_preview(const SessionData& session,
                                                    const std::string& srcfile,
                                                    const std::string& start_line_str,
                                                    const std::string& line_count_str,
                                                    const std::string& dstfile,
                                                    const std::string& insert_line_str) const {
    nlohmann::json result = {
        {"command", "movelines-preview"}
    };
    
    try {
        // 引数検証
        int start_line, line_count, insert_line;
        try {
            start_line = std::stoi(start_line_str);
            line_count = std::stoi(line_count_str);
            insert_line = std::stoi(insert_line_str);
        } catch (const std::exception& e) {
            result["error"] = "無効な行番号: " + std::string(e.what());
            return result;
        }
        
        if (start_line < 1 || line_count < 1 || insert_line < 1) {
            result["error"] = "行番号は1以上である必要があります";
            return result;
        }
        
        // プロジェクト内ファイルチェック
        std::filesystem::path project_root = session.target_path;
        if (session.is_directory) {
            project_root = session.target_path;
        } else {
            project_root = session.target_path.parent_path();
        }
        
        std::filesystem::path src_path = project_root / srcfile;
        std::filesystem::path dst_path = project_root / dstfile;
        
        // ファイル存在チェック
        if (!std::filesystem::exists(src_path)) {
            result["error"] = "ソースファイルが見つかりません: " + src_path.string();
            return result;
        }
        
        // 宛先ファイルが存在しない場合は作成準備
        bool dst_file_exists = std::filesystem::exists(dst_path);
        
        // ソースファイル読み込み
        std::ifstream src_file(src_path);
        if (!src_file.is_open()) {
            result["error"] = "ソースファイルを開けません: " + src_path.string();
            return result;
        }
        
        std::vector<std::string> src_lines;
        std::string line;
        while (std::getline(src_file, line)) {
            src_lines.push_back(line);
        }
        src_file.close();
        
        // 行範囲チェック
        if (start_line > (int)src_lines.size()) {
            result["error"] = "開始行がファイル行数を超えています: " + std::to_string(start_line) + " > " + std::to_string(src_lines.size());
            return result;
        }
        
        int end_line = start_line + line_count - 1;
        if (end_line > (int)src_lines.size()) {
            result["error"] = "終了行がファイル行数を超えています: " + std::to_string(end_line) + " > " + std::to_string(src_lines.size());
            return result;
        }
        
        // 移動対象行を抽出
        std::vector<std::string> moving_lines;
        for (int i = start_line - 1; i < start_line - 1 + line_count; i++) {
            moving_lines.push_back(src_lines[i]);
        }
        
        // 宛先ファイル読み込み（存在する場合）
        std::vector<std::string> dst_lines;
        if (dst_file_exists) {
            std::ifstream dst_file(dst_path);
            if (!dst_file.is_open()) {
                result["error"] = "宛先ファイルを開けません: " + dst_path.string();
                return result;
            }
            
            while (std::getline(dst_file, line)) {
                dst_lines.push_back(line);
            }
            dst_file.close();
            
            // 挿入位置チェック
            if (insert_line > (int)dst_lines.size() + 1) {
                result["error"] = "挿入位置がファイル行数を超えています: " + std::to_string(insert_line) + " > " + std::to_string(dst_lines.size() + 1);
                return result;
            }
        }
        
        // プレビュー作成
        std::string preview_id = "movelines_" + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
        
        // 移動内容
        std::string moving_content;
        for (const auto& mov_line : moving_lines) {
            if (!moving_content.empty()) moving_content += "\n";
            moving_content += mov_line;
        }
        
        // 結果作成
        result = {
            {"success", true},
            {"preview_id", preview_id},
            {"operation", {
                {"type", "movelines"},
                {"srcfile", srcfile},
                {"start_line", start_line},
                {"line_count", line_count},
                {"dstfile", dstfile},
                {"insert_line", insert_line}
            }},
            {"preview", {
                {"moving_content", moving_content},
                {"lines_to_move", line_count},
                {"source_range", std::to_string(start_line) + "-" + std::to_string(end_line)},
                {"destination", dstfile + ":" + std::to_string(insert_line)}
            }},
            {"summary", std::to_string(line_count) + " lines: " + srcfile + ":" + std::to_string(start_line) + "-" + std::to_string(end_line) + " → " + dstfile + ":" + std::to_string(insert_line)}
        };
        
        // メモリに保存（confirm用）
        std::filesystem::create_directories("memory/movelines_previews");
        std::string preview_file = "memory/movelines_previews/" + preview_id + ".json";
        
        nlohmann::json preview_data = {
            {"preview_id", preview_id},
            {"operation", result["operation"]},
            {"moving_lines", moving_lines},
            {"src_lines", src_lines},
            {"dst_lines", dst_lines},
            {"dst_file_exists", dst_file_exists},
            {"timestamp", std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()}
        };
        
        std::ofstream preview_json_file(preview_file);
        if (preview_json_file.is_open()) {
            preview_json_file << preview_data.dump(2);
            preview_json_file.close();
        }
        
    } catch (const std::exception& e) {
        result["error"] = std::string("movelines-preview エラー: ") + e.what();
    }
    
    return result;
}

nlohmann::json EditCommands::cmd_movelines_confirm(const SessionData& session,
                                                    const std::string& preview_id) const {
    nlohmann::json result = {
        {"command", "movelines-confirm"}
    };
    
    try {
        // プレビューデータ読み込み
        std::string preview_file = "memory/movelines_previews/" + preview_id + ".json";
        if (!std::filesystem::exists(preview_file)) {
            result["error"] = "プレビューが見つかりません: " + preview_id;
            return result;
        }
        
        std::ifstream file(preview_file);
        nlohmann::json preview_data;
        file >> preview_data;
        file.close();
        
        // パラメータ取得
        std::string srcfile = preview_data["operation"]["srcfile"];
        std::string dstfile = preview_data["operation"]["dstfile"];
        int start_line = preview_data["operation"]["start_line"];
        int line_count = preview_data["operation"]["line_count"];
        int insert_line = preview_data["operation"]["insert_line"];
        bool dst_file_exists = preview_data["dst_file_exists"];
        
        std::vector<std::string> moving_lines = preview_data["moving_lines"];
        std::vector<std::string> src_lines = preview_data["src_lines"];
        std::vector<std::string> dst_lines = preview_data["dst_lines"];
        
        // ファイルパス作成
        std::filesystem::path project_root = session.target_path;
        if (session.is_directory) {
            project_root = session.target_path;
        } else {
            project_root = session.target_path.parent_path();
        }
        
        std::filesystem::path src_path = project_root / srcfile;
        std::filesystem::path dst_path = project_root / dstfile;
        
        // ソースファイル編集（移動対象行を削除）
        std::vector<std::string> new_src_lines;
        for (int i = 0; i < (int)src_lines.size(); i++) {
            if (i < start_line - 1 || i >= start_line - 1 + line_count) {
                new_src_lines.push_back(src_lines[i]);
            }
        }
        
        // 宛先ファイル編集（行を挿入）
        std::vector<std::string> new_dst_lines;
        if (dst_file_exists) {
            for (int i = 0; i < (int)dst_lines.size(); i++) {
                if (i == insert_line - 1) {
                    // 挿入位置に移動行を挿入
                    for (const auto& moving_line : moving_lines) {
                        new_dst_lines.push_back(moving_line);
                    }
                }
                new_dst_lines.push_back(dst_lines[i]);
            }
            // 末尾に挿入の場合
            if (insert_line > (int)dst_lines.size()) {
                for (const auto& moving_line : moving_lines) {
                    new_dst_lines.push_back(moving_line);
                }
            }
        } else {
            // 新規ファイル作成
            new_dst_lines = moving_lines;
        }
        
        // ファイル保存
        std::ofstream src_out(src_path);
        if (!src_out.is_open()) {
            result["error"] = "ソースファイルを書き込めません: " + src_path.string();
            return result;
        }
        
        for (const auto& line : new_src_lines) {
            src_out << line << "\n";
        }
        src_out.close();
        
        std::ofstream dst_out(dst_path);
        if (!dst_out.is_open()) {
            result["error"] = "宛先ファイルを書き込めません: " + dst_path.string();
            return result;
        }
        
        for (const auto& line : new_dst_lines) {
            dst_out << line << "\n";
        }
        dst_out.close();
        
        // edit-history に記録
        std::string edit_id = "movelines_" + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
        
        std::filesystem::create_directories("memory/edit_history");
        std::string history_file = "memory/edit_history/" + edit_id + ".json";
        
        nlohmann::json history_data = {
            {"edit_id", edit_id},
            {"preview_id", preview_id},
            {"type", "movelines"},
            {"timestamp", std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()},
            {"operation", preview_data["operation"]},
            {"summary", "Moved " + std::to_string(line_count) + " lines from " + srcfile + " to " + dstfile},
            {"files", {srcfile, dstfile}}
        };
        
        std::ofstream history_json_file(history_file);
        if (history_json_file.is_open()) {
            history_json_file << history_data.dump(2);
            history_json_file.close();
        }
        
        // プレビューファイル削除
        std::filesystem::remove(preview_file);
        
        result = {
            {"success", true},
            {"edit_id", edit_id},
            {"preview_id", preview_id},
            {"files_modified", {srcfile, dstfile}},
            {"lines_moved", line_count},
            {"summary", "行移動完了: " + std::to_string(line_count) + " lines: " + srcfile + " → " + dstfile}
        };
        
    } catch (const std::exception& e) {
        result["error"] = std::string("movelines-confirm エラー: ") + e.what();
    }
    
    return result;
}

} // namespace nekocode