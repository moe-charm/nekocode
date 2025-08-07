//=============================================================================
// 🔄 Movelines Commands実装 - NekoCode行移動機能
//
// edit_commands.cpp分割 - Movelines系機能群
// 責任: 行移動プレビュー、行移動実行の実装
//=============================================================================

#include "nekocode/edit_commands.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>

namespace nekocode {

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