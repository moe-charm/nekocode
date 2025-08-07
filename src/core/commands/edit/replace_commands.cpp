//=============================================================================
// 🔄 Replace Commands実装 - NekoCode置換機能
//
// edit_commands.cpp分割 - Replace系機能群
// 責任: 置換プレビュー、置換実行、置換確認の実装
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
// 🐱 NekoCode独自編集機能実装 - Replace系
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

nlohmann::json EditCommands::cmd_replace_confirm(const SessionData& session,
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

} // namespace nekocode