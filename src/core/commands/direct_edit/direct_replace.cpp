//=============================================================================
// 🔄 Direct Replace - セッション不要置換機能
//
// SessionData依存を完全に排除した軽量置換実装
// current_path()ベースでのファイル操作
//=============================================================================

#include "direct_edit_common.hpp"
#include "pcre2_engine.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace nekocode {
namespace DirectEdit {

//=============================================================================
// 🔄 Direct置換プレビュー機能
//=============================================================================

nlohmann::json replace_preview(const std::string& file_path,
                               const std::string& pattern,
                               const std::string& replacement) {
    nlohmann::json result = {
        {"command", "replace-preview"}
    };
    
    try {
        // 1. ファイルパス解決（SessionData不要）
        auto target_path = resolve_file_path(file_path);
        
        // 2. ファイルアクセス検証
        std::string error_message;
        if (!validate_file_access(target_path, error_message)) {
            result["error"] = error_message;
            return result;
        }
        
        // 3. ファイル読み込み
        std::ifstream file(target_path);
        if (!file.is_open()) {
            result["error"] = "ファイルを開けません: " + target_path.string();
            return result;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        // 4. PCRE2 革命的置換処理 🐍
        auto pcre_result = smart_replace(pattern, replacement, content);
        
        if (!pcre_result.success) {
            result["error"] = "パターン処理エラー: " + pcre_result.error_message;
            return result;
        }
        
        // 5. マッチ情報収集
        std::vector<nlohmann::json> all_matches;
        std::vector<nlohmann::json> sample_matches;
        
        int match_count = pcre_result.total_replacements;
        
        for (size_t i = 0; i < pcre_result.match_positions.size(); ++i) {
            nlohmann::json match_info = {
                {"line", pcre_result.match_lines[i]},
                {"position", pcre_result.match_positions[i]}
            };
            
            all_matches.push_back(match_info);
            
            // サンプル（最初の5個のみ）
            if (i < 5) {
                sample_matches.push_back(match_info);
            }
        }
        
        if (all_matches.empty()) {
            result["error"] = "パターンにマッチする内容が見つかりませんでした";
            result["pattern"] = pattern;
            return result;
        }
        
        // 6. プレビューID生成とメモリディレクトリ準備
        ensure_memory_directories();
        std::string preview_id = generate_preview_id("preview");
        
        // 7. サイズ変更計算
        std::string new_content = pcre_result.new_content;
        int size_change = static_cast<int>(new_content.size()) - static_cast<int>(content.size());
        
        // 8. 詳細情報をメモリに保存
        nlohmann::json detailed_info = {
            {"preview_id", preview_id},
            {"file_path", target_path.string()},
            {"pattern", pattern},
            {"replacement", replacement},
            {"all_matches", all_matches},
            {"total_matches", match_count},
            {"size_change", size_change},
            {"content_original", content},
            {"content_new", new_content},
            {"timestamp", generate_timestamp()}
        };
        
        std::string memory_file = "memory/edit_previews/" + preview_id + ".json";
        std::ofstream stream(memory_file);
        if (stream.is_open()) {
            stream << detailed_info.dump(2);
            stream.close();
        }
        
        // 9. プレビューファイルクリーンアップ
        cleanup_preview_files();
        
        // 10. 軽量応答を返却
        result = {
            {"success", true},
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
        result["error"] = std::string("置換プレビューエラー: ") + e.what();
    }
    
    return result;
}

//=============================================================================
// ✅ Direct置換確定実行
//=============================================================================

nlohmann::json replace_confirm(const std::string& preview_id) {
    nlohmann::json result = {
        {"command", "replace-confirm"}
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
        std::string file_path = preview_data["file_path"];
        std::string pattern = preview_data["pattern"];
        std::string replacement = preview_data["replacement"];
        
        // 3. 書き込み権限チェック
        std::string error_message;
        if (!validate_write_access(file_path, error_message)) {
            result["error"] = error_message;
            return result;
        }
        
        // 4. ファイル読み込み（最新状態確認）
        std::ifstream file(file_path);
        if (!file.is_open()) {
            result["error"] = "ファイルを開けません: " + file_path;
            return result;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        // 5. 履歴ID生成とディレクトリ準備
        ensure_memory_directories();
        std::string edit_id = generate_edit_id();
        
        // 6. 変更前ファイル保存
        std::filesystem::path before_file = "memory/edit_history/" + edit_id + "_before.txt";
        std::ofstream before_stream(before_file);
        before_stream << content;
        before_stream.close();
        
        // 7. PCRE2革命的置換実行 🐍
        auto pcre_result = smart_replace(pattern, replacement, content);
        
        if (!pcre_result.success) {
            result["error"] = "置換エラー: " + pcre_result.error_message;
            return result;
        }
        
        std::string new_content = pcre_result.new_content;
        
        // 8. ファイル書き込み
        std::ofstream out_file(file_path);
        if (!out_file.is_open()) {
            result["error"] = "ファイルに書き込めません: " + file_path;
            return result;
        }
        out_file << new_content;
        out_file.close();
        
        // 9. 変更後ファイル保存
        std::filesystem::path after_file = "memory/edit_history/" + edit_id + "_after.txt";
        std::ofstream after_stream(after_file);
        after_stream << new_content;
        after_stream.close();
        
        // 10. 履歴メタデータ保存
        nlohmann::json history_data = {
            {"edit_id", edit_id},
            {"preview_id", preview_id},
            {"timestamp", generate_timestamp()},
            {"operation", "replace"},
            {"file_info", {
                {"path", file_path},
                {"size_before", content.length()},
                {"size_after", new_content.length()}
            }},
            {"change_details", {
                {"pattern", pattern},
                {"replacement", replacement},
                {"matches_count", preview_data["total_matches"]}
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
// 🚀 Direct置換（即実行版）
//=============================================================================

nlohmann::json replace_direct(const std::string& file_path,
                             const std::string& pattern,
                             const std::string& replacement) {
    nlohmann::json result = {
        {"command", "replace"}
    };
    
    try {
        // 1. ファイルパス解決
        auto target_path = resolve_file_path(file_path);
        
        // 2. ファイルアクセス検証
        std::string error_message;
        if (!validate_file_access(target_path, error_message) ||
            !validate_write_access(target_path, error_message)) {
            result["error"] = error_message;
            return result;
        }
        
        // 3. ファイル読み込み
        std::ifstream file(target_path);
        if (!file.is_open()) {
            result["error"] = "ファイルを開けません: " + target_path.string();
            return result;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        // 4. PCRE2革命的処理 🐍
        auto pcre_result = smart_replace(pattern, replacement, content);
        
        if (!pcre_result.success) {
            result["error"] = "パターン処理エラー: " + pcre_result.error_message;
            return result;
        }
        
        // 5. マッチ確認
        if (pcre_result.total_replacements == 0) {
            result["warning"] = "パターンにマッチするものが見つかりませんでした";
            result["matches_found"] = 0;
            return result;
        }
        
        // 6. 置換結果取得
        std::string new_content = pcre_result.new_content;
        
        // 7. 変更チェック
        if (content == new_content) {
            result["warning"] = "変更がありませんでした";
            return result;
        }
        
        // 8. ファイル更新
        std::ofstream output_file(target_path);
        if (!output_file.is_open()) {
            result["error"] = "ファイルに書き込めません: " + target_path.string();
            return result;
        }
        
        output_file << new_content;
        output_file.close();
        
        // 9. 成功レポート
        result["success"] = true;
        result["file_updated"] = target_path.filename().string();
        result["size_before"] = content.size();
        result["size_after"] = new_content.size();
        
        // マッチした内容を表示（統計情報）
        result["matches_found"] = pcre_result.total_replacements;
        result["match_positions"] = pcre_result.match_positions;
        result["match_lines"] = pcre_result.match_lines;
        
    } catch (const std::exception& e) {
        result["error"] = std::string("直接置換エラー: ") + e.what();
    }
    
    return result;
}

} // namespace DirectEdit
} // namespace nekocode