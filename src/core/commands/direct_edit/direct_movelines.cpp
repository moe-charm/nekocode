//=============================================================================
// 🔄 Direct Movelines - セッション不要行移動機能
//
// SessionData依存を完全に排除したファイル間行移動実装
// current_path()ベースでの複数ファイル操作
//=============================================================================

#include "direct_edit_common.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>

namespace nekocode {
namespace DirectEdit {

//=============================================================================
// 🔄 Direct行移動プレビュー機能
//=============================================================================

nlohmann::json movelines_preview(const std::string& srcfile,
                                 int start_line, int line_count,
                                 const std::string& dstfile, 
                                 int insert_line) {
    nlohmann::json result = {
        {"command", "movelines-preview"}
    };
    
    try {
        // 1. 引数検証
        if (start_line < 1 || line_count < 1 || insert_line < 1) {
            result["error"] = "行番号は1以上である必要があります";
            return result;
        }
        
        // 2. ファイルパス解決（SessionData不要）
        auto src_path = resolve_file_path(srcfile);
        auto dst_path = resolve_file_path(dstfile);
        
        // 3. ソースファイル検証・読み込み
        std::string error_message;
        if (!validate_file_access(src_path, error_message)) {
            result["error"] = "ソースファイル: " + error_message;
            return result;
        }
        
        std::vector<std::string> src_lines;
        if (!read_file_lines(src_path, src_lines, error_message)) {
            result["error"] = "ソースファイル読み込み: " + error_message;
            return result;
        }
        
        // 4. 行範囲チェック
        if (start_line > (int)src_lines.size()) {
            result["error"] = "開始行がファイル行数を超えています: " + 
                             std::to_string(start_line) + " > " + std::to_string(src_lines.size());
            return result;
        }
        
        int end_line = start_line + line_count - 1;
        if (end_line > (int)src_lines.size()) {
            result["error"] = "終了行がファイル行数を超えています: " + 
                             std::to_string(end_line) + " > " + std::to_string(src_lines.size());
            return result;
        }
        
        // 5. 移動対象行を抽出
        std::vector<std::string> moving_lines;
        for (int i = start_line - 1; i < start_line - 1 + line_count; i++) {
            moving_lines.push_back(src_lines[i]);
        }
        
        // 6. 宛先ファイル処理
        std::vector<std::string> dst_lines;
        bool dst_file_exists = std::filesystem::exists(dst_path);
        
        if (dst_file_exists) {
            // 宛先ファイルアクセス検証・読み込み
            if (!validate_file_access(dst_path, error_message)) {
                result["error"] = "宛先ファイル: " + error_message;
                return result;
            }
            
            if (!read_file_lines(dst_path, dst_lines, error_message)) {
                result["error"] = "宛先ファイル読み込み: " + error_message;
                return result;
            }
            
            // 挿入位置チェック
            if (insert_line > (int)dst_lines.size() + 1) {
                result["error"] = "挿入位置がファイル行数を超えています: " + 
                                 std::to_string(insert_line) + " > " + std::to_string(dst_lines.size() + 1);
                return result;
            }
        } else {
            // 宛先ファイルが存在しない場合の親ディレクトリチェック
            auto parent_dir = dst_path.parent_path();
            if (!std::filesystem::exists(parent_dir)) {
                result["error"] = "宛先ファイルの親ディレクトリが存在しません: " + parent_dir.string();
                return result;
            }
        }
        
        // 7. 書き込み権限チェック
        if (!validate_write_access(src_path, error_message)) {
            result["error"] = "ソースファイル書き込み権限: " + error_message;
            return result;
        }
        if (!validate_write_access(dst_path, error_message)) {
            result["error"] = "宛先ファイル書き込み権限: " + error_message;
            return result;
        }
        
        // 8. プレビューID生成とメモリ準備
        ensure_memory_directories();
        std::string preview_id = generate_preview_id("movelines");
        
        // 9. 移動内容文字列作成
        std::string moving_content;
        for (size_t i = 0; i < moving_lines.size(); i++) {
            if (i > 0) moving_content += "\\n";
            moving_content += moving_lines[i];
        }
        
        // 10. 詳細情報をメモリに保存
        nlohmann::json preview_data = {
            {"preview_id", preview_id},
            {"created_at", generate_timestamp()},
            {"operation", {
                {"type", "movelines"},
                {"srcfile", src_path.string()},
                {"start_line", start_line},
                {"line_count", line_count},
                {"dstfile", dst_path.string()},
                {"insert_line", insert_line}
            }},
            {"moving_lines", moving_lines},
            {"src_lines", src_lines},
            {"dst_lines", dst_lines},
            {"dst_file_exists", dst_file_exists}
        };
        
        std::string preview_file = "memory/edit_previews/" + preview_id + ".json";
        std::ofstream memory_stream(preview_file);
        if (memory_stream.is_open()) {
            memory_stream << preview_data.dump(2);
            memory_stream.close();
        }
        
        // 11. プレビューファイルクリーンアップ
        cleanup_preview_files();
        
        // 12. 軽量応答を返却
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
            {"summary", std::to_string(line_count) + " lines: " + srcfile + ":" + 
                       std::to_string(start_line) + "-" + std::to_string(end_line) + 
                       " → " + dstfile + ":" + std::to_string(insert_line)}
        };
        
    } catch (const std::exception& e) {
        result["error"] = std::string("行移動プレビューエラー: ") + e.what();
    }
    
    return result;
}

//=============================================================================
// ✅ Direct行移動確定実行
//=============================================================================

nlohmann::json movelines_confirm(const std::string& preview_id) {
    nlohmann::json result = {
        {"command", "movelines-confirm"}
    };
    
    try {
        // 1. プレビューデータ読み込み
        std::string preview_file = "memory/edit_previews/" + preview_id + ".json";
        if (!std::filesystem::exists(preview_file)) {
            result["error"] = "プレビューが見つかりません: " + preview_id;
            return result;
        }
        
        std::ifstream file(preview_file);
        nlohmann::json preview_data;
        file >> preview_data;
        file.close();
        
        // 2. パラメータ取得
        std::string srcfile = preview_data["operation"]["srcfile"];
        std::string dstfile = preview_data["operation"]["dstfile"];
        int start_line = preview_data["operation"]["start_line"];
        int line_count = preview_data["operation"]["line_count"];
        int insert_line = preview_data["operation"]["insert_line"];
        bool dst_file_exists = preview_data["dst_file_exists"];
        
        std::vector<std::string> moving_lines = preview_data["moving_lines"];
        std::vector<std::string> src_lines = preview_data["src_lines"];
        std::vector<std::string> dst_lines = preview_data["dst_lines"];
        
        // 3. 権限再チェック
        std::string error_message;
        if (!validate_write_access(srcfile, error_message)) {
            result["error"] = "ソースファイル書き込み権限: " + error_message;
            return result;
        }
        if (!validate_write_access(dstfile, error_message)) {
            result["error"] = "宛先ファイル書き込み権限: " + error_message;
            return result;
        }
        
        // 4. ソースファイル編集（移動対象行を削除）
        std::vector<std::string> new_src_lines;
        for (int i = 0; i < (int)src_lines.size(); i++) {
            if (i < start_line - 1 || i >= start_line - 1 + line_count) {
                new_src_lines.push_back(src_lines[i]);
            }
        }
        
        // 5. 宛先ファイル編集（行を挿入）
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
        
        // 6. 履歴ID生成とディレクトリ準備
        ensure_memory_directories();
        std::string edit_id = generate_edit_id();
        
        // 7. 変更前ファイル保存
        std::filesystem::path before_src = "memory/edit_history/" + edit_id + "_src_before.txt";
        std::ofstream before_src_stream(before_src);
        for (const auto& line : src_lines) {
            before_src_stream << line << "\\n";
        }
        before_src_stream.close();
        
        if (dst_file_exists) {
            std::filesystem::path before_dst = "memory/edit_history/" + edit_id + "_dst_before.txt";
            std::ofstream before_dst_stream(before_dst);
            for (const auto& line : dst_lines) {
                before_dst_stream << line << "\\n";
            }
            before_dst_stream.close();
        }
        
        // 8. ファイル保存実行
        if (!write_file_lines(srcfile, new_src_lines, error_message)) {
            result["error"] = "ソースファイル書き込み: " + error_message;
            return result;
        }
        
        if (!write_file_lines(dstfile, new_dst_lines, error_message)) {
            result["error"] = "宛先ファイル書き込み: " + error_message;
            return result;
        }
        
        // 9. 変更後ファイル保存
        std::filesystem::path after_src = "memory/edit_history/" + edit_id + "_src_after.txt";
        std::ofstream after_src_stream(after_src);
        for (const auto& line : new_src_lines) {
            after_src_stream << line << "\\n";
        }
        after_src_stream.close();
        
        std::filesystem::path after_dst = "memory/edit_history/" + edit_id + "_dst_after.txt";
        std::ofstream after_dst_stream(after_dst);
        for (const auto& line : new_dst_lines) {
            after_dst_stream << line << "\\n";
        }
        after_dst_stream.close();
        
        // 10. 履歴メタデータ保存
        nlohmann::json history_data = {
            {"edit_id", edit_id},
            {"preview_id", preview_id},
            {"type", "movelines"},
            {"timestamp", generate_timestamp()},
            {"operation", preview_data["operation"]},
            {"summary", "Moved " + std::to_string(line_count) + " lines from " + srcfile + " to " + dstfile},
            {"files", {srcfile, dstfile}}
        };
        
        std::string history_file = "memory/edit_history/" + edit_id + ".json";
        std::ofstream history_json_file(history_file);
        if (history_json_file.is_open()) {
            history_json_file << history_data.dump(2);
            history_json_file.close();
        }
        
        // 11. 履歴ファイルクリーンアップ
        cleanup_history_files();
        
        // 12. プレビューファイル削除
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
        result["error"] = std::string("行移動実行エラー: ") + e.what();
    }
    
    return result;
}

//=============================================================================
// 🚀 Direct行移動（即実行版）
//=============================================================================

nlohmann::json movelines_direct(const std::string& srcfile,
                                int start_line, int line_count,
                                const std::string& dstfile, 
                                int insert_line) {
    nlohmann::json result = {
        {"command", "movelines"}
    };
    
    try {
        // 1. まずプレビューを作成
        auto preview_result = movelines_preview(srcfile, start_line, line_count, dstfile, insert_line);
        
        if (preview_result.contains("error")) {
            result["error"] = preview_result["error"];
            return result;
        }
        
        // 2. プレビューが成功した場合は確定実行
        std::string preview_id = preview_result["preview_id"];
        auto confirm_result = movelines_confirm(preview_id);
        
        if (confirm_result.contains("error")) {
            result["error"] = confirm_result["error"];
            return result;
        }
        
        // 3. 成功レポート
        result["success"] = true;
        result["edit_id"] = confirm_result["edit_id"];
        result["files_modified"] = {srcfile, dstfile};
        result["lines_moved"] = line_count;
        result["source_range"] = std::to_string(start_line) + "-" + std::to_string(start_line + line_count - 1);
        result["destination"] = dstfile + ":" + std::to_string(insert_line);
        result["summary"] = "直接行移動完了: " + std::to_string(line_count) + " lines: " + 
                           srcfile + " → " + dstfile;
        
    } catch (const std::exception& e) {
        result["error"] = std::string("直接行移動エラー: ") + e.what();
    }
    
    return result;
}

} // namespace DirectEdit
} // namespace nekocode