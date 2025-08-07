//=============================================================================
// 📥 Direct Insert - セッション不要挿入機能
//
// SessionData依存を完全に排除した軽量挿入実装
// current_path()ベースでの位置指定挿入
//=============================================================================

#include "direct_edit_common.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace nekocode {
namespace DirectEdit {

//=============================================================================
// 📥 統一挿入プレビュー機能
//=============================================================================

nlohmann::json insert_preview(const std::string& file_path,
                              const std::string& position,
                              const std::string& content) {
    nlohmann::json result = {
        {"command", "insert-preview"}
    };
    
    try {
        // 1. ファイルパス解決（SessionData不要）
        auto target_file = resolve_file_path(file_path);
        
        // 2. ファイルアクセス検証
        std::string error_message;
        if (!validate_file_access(target_file, error_message)) {
            result["error"] = error_message;
            return result;
        }
        
        // 3. ファイル読み込み
        std::vector<std::string> lines;
        if (!read_file_lines(target_file, lines, error_message)) {
            result["error"] = error_message;
            return result;
        }
        
        int total_lines = lines.size();
        
        // 4. 位置解決
        int insert_line = 0;  // 0-based index for vector
        std::string position_description;
        
        if (position == "start" || position == "top" || position == "0") {
            insert_line = 0;
            position_description = "ファイル先頭";
        } else if (position == "end" || position == "bottom") {
            insert_line = total_lines;
            position_description = "ファイル末尾";
        } else if (position.substr(0, 7) == "before:") {
            // パターンマッチ（将来実装）
            result["error"] = "パターンマッチはまだ実装されていません";
            return result;
        } else if (position.substr(0, 6) == "after:") {
            // パターンマッチ（将来実装）
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
        
        // 5. コンテキスト収集（前後3行）
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
        
        // 6. プレビューID生成とメモリディレクトリ準備
        ensure_memory_directories();
        std::string preview_id = generate_preview_id("insert");
        
        // 7. 詳細情報をメモリに保存
        nlohmann::json detailed_context = {
            {"before_10_lines", nlohmann::json::array()},
            {"after_10_lines", nlohmann::json::array()}
        };
        
        // 詳細コンテキスト（memory用、前後10行）
        for (int i = std::max(0, insert_line - 10); i < insert_line && i < total_lines; i++) {
            detailed_context["before_10_lines"].push_back({
                {"line", i + 1},
                {"content", lines[i]}
            });
        }
        for (int i = insert_line; i < std::min(total_lines, insert_line + 10); i++) {
            detailed_context["after_10_lines"].push_back({
                {"line", i + 1},
                {"content", lines[i]}
            });
        }
        
        nlohmann::json preview_details = {
            {"preview_id", preview_id},
            {"created_at", generate_timestamp()},
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
            {"detailed_context", detailed_context}
        };
        
        // 8. メモリに保存
        std::string preview_file = "memory/edit_previews/" + preview_id + ".json";
        std::ofstream memory_file(preview_file);
        if (memory_file.is_open()) {
            memory_file << preview_details.dump(2);
            memory_file.close();
        }
        
        // 9. プレビューファイルクリーンアップ
        cleanup_preview_files();
        
        // 10. 軽量応答を返却
        result = {
            {"success", true},
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

nlohmann::json insert_confirm(const std::string& preview_id) {
    nlohmann::json result = {
        {"command", "insert-confirm"}
    };
    
    try {
        // 1. プレビューファイル読み込み
        std::filesystem::path preview_file = "memory/edit_previews/" + preview_id + ".json";
        if (!std::filesystem::exists(preview_file)) {
            result["error"] = "プレビューが見つかりません: " + preview_id;
            return result;
        }
        
        std::ifstream preview_stream(preview_file);
        nlohmann::json preview_data;
        preview_stream >> preview_data;
        preview_stream.close();
        
        // 2. ファイル情報取得
        std::string file_path = preview_data["file_info"]["path"];
        int insert_line = preview_data["operation"]["resolved_line"].get<int>() - 1;  // 1-based to 0-based
        std::string insert_content = preview_data["operation"]["content"];
        
        // 3. 書き込み権限チェック
        std::string error_message;
        if (!validate_write_access(file_path, error_message)) {
            result["error"] = error_message;
            return result;
        }
        
        // 4. ファイル読み込み（最新状態確認）
        std::vector<std::string> lines;
        if (!read_file_lines(file_path, lines, error_message)) {
            result["error"] = error_message;
            return result;
        }
        
        // 5. 履歴ID生成とディレクトリ準備
        ensure_memory_directories();
        std::string edit_id = generate_edit_id();
        
        // 6. 変更前ファイル保存
        std::filesystem::path before_file = "memory/edit_history/" + edit_id + "_before.txt";
        std::ofstream before_stream(before_file);
        for (const auto& l : lines) {
            before_stream << l << "\\n";
        }
        before_stream.close();
        
        // 7. 挿入実行
        // contentを行ごとに分割
        std::vector<std::string> insert_lines;
        std::istringstream content_stream(insert_content);
        std::string content_line;
        while (std::getline(content_stream, content_line)) {
            insert_lines.push_back(content_line);
        }
        
        // 新しい内容を構築
        std::vector<std::string> new_lines;
        for (int i = 0; i < insert_line && i < (int)lines.size(); i++) {
            new_lines.push_back(lines[i]);
        }
        for (const auto& l : insert_lines) {
            new_lines.push_back(l);
        }
        for (int i = insert_line; i < (int)lines.size(); i++) {
            new_lines.push_back(lines[i]);
        }
        
        // 8. ファイル書き込み
        if (!write_file_lines(file_path, new_lines, error_message)) {
            result["error"] = error_message;
            return result;
        }
        
        // 9. 変更後ファイル保存
        std::filesystem::path after_file = "memory/edit_history/" + edit_id + "_after.txt";
        std::ofstream after_stream(after_file);
        for (const auto& l : new_lines) {
            after_stream << l << "\\n";
        }
        after_stream.close();
        
        // 10. 履歴メタデータ保存
        nlohmann::json history_data = {
            {"edit_id", edit_id},
            {"preview_id", preview_id},
            {"timestamp", generate_timestamp()},
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
        
        std::filesystem::path history_meta = "memory/edit_history/" + edit_id + ".json";
        std::ofstream history_stream(history_meta);
        history_stream << history_data.dump(2);
        history_stream.close();
        
        // 11. 履歴ファイルクリーンアップ
        cleanup_history_files();
        
        // 12. プレビューファイル削除
        std::filesystem::remove(preview_file);
        
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
// 🚀 Direct挿入（即実行版）
//=============================================================================

nlohmann::json insert_direct(const std::string& file_path,
                             const std::string& position,
                             const std::string& content) {
    nlohmann::json result = {
        {"command", "insert"}
    };
    
    try {
        // 1. ファイルパス解決
        auto target_file = resolve_file_path(file_path);
        
        // 2. ファイルアクセス・書き込み権限検証
        std::string error_message;
        if (!validate_file_access(target_file, error_message) ||
            !validate_write_access(target_file, error_message)) {
            result["error"] = error_message;
            return result;
        }
        
        // 3. ファイル読み込み
        std::vector<std::string> lines;
        if (!read_file_lines(target_file, lines, error_message)) {
            result["error"] = error_message;
            return result;
        }
        
        int total_lines = lines.size();
        
        // 4. 位置解決（insert_previewと同じロジック）
        int insert_line = 0;
        std::string position_description;
        
        if (position == "start" || position == "top" || position == "0") {
            insert_line = 0;
            position_description = "ファイル先頭";
        } else if (position == "end" || position == "bottom") {
            insert_line = total_lines;
            position_description = "ファイル末尾";
        } else {
            try {
                int line_num = std::stoi(position);
                if (line_num < 0) {
                    insert_line = 0;
                } else if (line_num > total_lines) {
                    insert_line = total_lines;
                } else {
                    insert_line = line_num - 1;
                }
                position_description = std::to_string(line_num) + "行目";
            } catch (...) {
                result["error"] = "無効な位置指定: " + position;
                return result;
            }
        }
        
        // 5. 挿入実行
        std::vector<std::string> insert_lines;
        std::istringstream content_stream(content);
        std::string content_line;
        while (std::getline(content_stream, content_line)) {
            insert_lines.push_back(content_line);
        }
        
        std::vector<std::string> new_lines;
        for (int i = 0; i < insert_line && i < (int)lines.size(); i++) {
            new_lines.push_back(lines[i]);
        }
        for (const auto& l : insert_lines) {
            new_lines.push_back(l);
        }
        for (int i = insert_line; i < (int)lines.size(); i++) {
            new_lines.push_back(lines[i]);
        }
        
        // 6. ファイル書き込み
        if (!write_file_lines(target_file, new_lines, error_message)) {
            result["error"] = error_message;
            return result;
        }
        
        // 7. 成功レポート
        result["success"] = true;
        result["file_updated"] = target_file.filename().string();
        result["position"] = position_description;
        result["lines_before"] = total_lines;
        result["lines_after"] = new_lines.size();
        result["inserted_lines"] = insert_lines.size();
        
    } catch (const std::exception& e) {
        result["error"] = std::string("直接挿入エラー: ") + e.what();
    }
    
    return result;
}

} // namespace DirectEdit
} // namespace nekocode