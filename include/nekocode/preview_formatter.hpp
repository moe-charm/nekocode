#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <nlohmann/json.hpp>
#include "nekocode/types.hpp"

namespace nekocode {

//=============================================================================
// 🎨 Preview Formatter - 全機能統一プレビュー表示
//=============================================================================

class PreviewFormatter {
public:
    struct DiffLine {
        LineNumber line_number;
        std::string content;
        enum Type { CONTEXT, ADDED, REMOVED, MODIFIED } type;
        std::string annotation;  // 説明や注釈
    };
    
    struct DiffSection {
        std::string title;                    // セクションタイトル
        std::string description;              // 変更の説明
        LineNumber start_line;                // 開始行
        LineNumber end_line;                  // 終了行
        std::vector<DiffLine> lines;          // 差分行
        int context_lines = 5;                // 前後のコンテキスト行数
    };
    
    struct PreviewResult {
        std::string file_path;                // ファイルパス
        std::string operation;                // 操作種別（replace/insert/move等）
        std::vector<DiffSection> sections;    // 変更セクション
        std::string summary;                  // 変更サマリー
        std::vector<std::string> warnings;    // 警告
        
        // 統計情報
        int total_changes = 0;
        int lines_added = 0;
        int lines_removed = 0;
        int lines_modified = 0;
    };
    
    /// 統一的なプレビュー生成
    static std::string format_preview(const PreviewResult& preview);
    
    /// Replace操作のプレビュー生成
    static PreviewResult create_replace_preview(
        const std::string& file_path,
        const std::string& original_content,
        const std::string& pattern,
        const std::string& replacement,
        const std::vector<std::pair<LineNumber, std::string>>& matches,
        int context_lines = 5);
    
    /// Insert操作のプレビュー生成
    static PreviewResult create_insert_preview(
        const std::string& file_path,
        const std::string& original_content,
        const std::string& content_to_insert,
        LineNumber insert_position,
        int context_lines = 5);
    
    /// MoveClass操作のプレビュー生成
    static PreviewResult create_moveclass_preview(
        const std::string& source_file,
        const std::string& target_file,
        const std::string& symbol_name,
        const std::vector<std::pair<std::string, std::string>>& import_changes,
        int context_lines = 5);
    
    /// 行番号付きコンテンツ生成
    static std::vector<std::string> get_numbered_lines(
        const std::string& content,
        LineNumber start_line = 1,
        LineNumber end_line = 0);
    
    /// 見やすいdiff表示（GitHub風）
    static std::string format_as_github_diff(const PreviewResult& preview);
    
    /// コンパクトな表示（Claude Code用）
    static std::string format_compact(const PreviewResult& preview);
    
    /// 詳細な表示（人間用）
    static std::string format_detailed(const PreviewResult& preview);
    
    /// JSON形式（API用）
    static nlohmann::json format_as_json(const PreviewResult& preview);
    
private:
    /// 色付け（ターミナル用）
    static std::string colorize(const std::string& text, const std::string& color);
    
    /// 行の切り出し
    static std::vector<std::string> extract_lines(
        const std::string& content,
        LineNumber start,
        LineNumber end);
    
    /// 変更箇所のハイライト
    static std::string highlight_changes(
        const std::string& line,
        const std::string& pattern,
        const std::string& replacement);
};

//=============================================================================
// 実装
//=============================================================================

inline std::string PreviewFormatter::format_preview(const PreviewResult& preview) {
    std::stringstream ss;
    
    // ヘッダー
    ss << "\n╔════════════════════════════════════════════════════════════╗\n";
    ss << "║ 📝 " << preview.operation << " Preview";
    ss << std::string(56 - preview.operation.length(), ' ') << "║\n";
    ss << "╠════════════════════════════════════════════════════════════╣\n";
    
    // ファイル情報
    ss << "║ 📂 File: " << preview.file_path;
    ss << std::string(51 - preview.file_path.length(), ' ') << "║\n";
    
    // 統計情報
    ss << "║ 📊 Changes: " << preview.total_changes 
       << " (+" << preview.lines_added 
       << " -" << preview.lines_removed
       << " ~" << preview.lines_modified << ")";
    
    int stats_len = 13 + std::to_string(preview.total_changes).length() 
                   + 3 + std::to_string(preview.lines_added).length()
                   + 2 + std::to_string(preview.lines_removed).length()
                   + 2 + std::to_string(preview.lines_modified).length() + 1;
    ss << std::string(61 - stats_len, ' ') << "║\n";
    
    ss << "╠════════════════════════════════════════════════════════════╣\n";
    
    // 各セクション
    for (const auto& section : preview.sections) {
        // セクションヘッダー
        ss << "║ 🔍 " << section.title;
        ss << std::string(56 - section.title.length(), ' ') << "║\n";
        
        if (!section.description.empty()) {
            ss << "║    " << section.description;
            ss << std::string(56 - section.description.length(), ' ') << "║\n";
        }
        
        ss << "║" << std::string(60, '─') << "║\n";
        
        // 差分表示（拡張コンテキスト付き）
        for (const auto& line : section.lines) {
            std::string prefix;
            std::string marker;
            
            switch (line.type) {
                case DiffLine::ADDED:
                    prefix = "║ + ";
                    marker = "🟢";
                    break;
                case DiffLine::REMOVED:
                    prefix = "║ - ";
                    marker = "🔴";
                    break;
                case DiffLine::MODIFIED:
                    prefix = "║ ~ ";
                    marker = "🟡";
                    break;
                default:
                    prefix = "║   ";
                    marker = "  ";
            }
            
            // 行番号と内容
            ss << prefix << std::setw(4) << line.line_number << " │ " 
               << marker << " " << line.content;
            
            // 行末調整
            int content_len = line.content.length() + 11;
            if (content_len < 60) {
                ss << std::string(60 - content_len, ' ');
            }
            ss << "║\n";
            
            // 注釈があれば表示
            if (!line.annotation.empty()) {
                ss << "║      └─> " << line.annotation;
                int ann_len = 10 + line.annotation.length();
                if (ann_len < 60) {
                    ss << std::string(60 - ann_len, ' ');
                }
                ss << "║\n";
            }
        }
        
        ss << "║" << std::string(60, '─') << "║\n";
    }
    
    // 警告
    if (!preview.warnings.empty()) {
        ss << "║ ⚠️  Warnings:" << std::string(45, ' ') << "║\n";
        for (const auto& warning : preview.warnings) {
            ss << "║    • " << warning;
            int warn_len = 6 + warning.length();
            if (warn_len < 60) {
                ss << std::string(60 - warn_len, ' ');
            }
            ss << "║\n";
        }
    }
    
    // フッター
    ss << "╚════════════════════════════════════════════════════════════╝\n";
    
    return ss.str();
}

inline PreviewFormatter::PreviewResult PreviewFormatter::create_replace_preview(
    const std::string& file_path,
    const std::string& original_content,
    const std::string& pattern,
    const std::string& replacement,
    const std::vector<std::pair<LineNumber, std::string>>& matches,
    int context_lines) {
    
    PreviewResult result;
    result.file_path = file_path;
    result.operation = "Replace";
    result.total_changes = matches.size();
    
    auto lines = get_numbered_lines(original_content);
    
    for (const auto& [line_num, matched_text] : matches) {
        DiffSection section;
        section.title = "Match at line " + std::to_string(line_num);
        section.description = "Pattern: \"" + pattern + "\" → \"" + replacement + "\"";
        section.start_line = std::max(1, static_cast<int>(line_num) - context_lines);
        section.end_line = std::min(static_cast<int>(lines.size()), 
                                   static_cast<int>(line_num) + context_lines);
        
        // コンテキスト行を追加
        for (int i = section.start_line; i <= section.end_line; ++i) {
            DiffLine diff_line;
            diff_line.line_number = i;
            
            if (i == line_num) {
                // 変更される行
                diff_line.type = DiffLine::REMOVED;
                diff_line.content = lines[i - 1];
                section.lines.push_back(diff_line);
                
                // 新しい行
                DiffLine new_line;
                new_line.line_number = i;
                new_line.type = DiffLine::ADDED;
                // TODO: 実際の置換処理
                new_line.content = lines[i - 1];  // 仮実装
                new_line.annotation = "After replacement";
                section.lines.push_back(new_line);
                
                result.lines_modified++;
            } else {
                // コンテキスト行
                diff_line.type = DiffLine::CONTEXT;
                diff_line.content = lines[i - 1];
                section.lines.push_back(diff_line);
            }
        }
        
        result.sections.push_back(section);
    }
    
    result.summary = "Replace " + std::to_string(matches.size()) + 
                    " occurrences of pattern in " + file_path;
    
    return result;
}

inline std::vector<std::string> PreviewFormatter::get_numbered_lines(
    const std::string& content,
    LineNumber start_line,
    LineNumber end_line) {
    
    std::vector<std::string> lines;
    std::istringstream stream(content);
    std::string line;
    
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    
    return lines;
}

inline std::string PreviewFormatter::format_as_github_diff(const PreviewResult& preview) {
    std::stringstream ss;
    
    ss << "```diff\n";
    ss << "--- " << preview.file_path << "\n";
    ss << "+++ " << preview.file_path << " (modified)\n";
    
    for (const auto& section : preview.sections) {
        ss << "@@ -" << section.start_line << "," 
           << (section.end_line - section.start_line + 1)
           << " +" << section.start_line << ","
           << (section.end_line - section.start_line + 1) << " @@\n";
        
        for (const auto& line : section.lines) {
            switch (line.type) {
                case DiffLine::ADDED:
                    ss << "+ ";
                    break;
                case DiffLine::REMOVED:
                    ss << "- ";
                    break;
                default:
                    ss << "  ";
            }
            ss << line.content << "\n";
        }
    }
    
    ss << "```\n";
    
    return ss.str();
}

} // namespace nekocode