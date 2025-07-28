//=============================================================================
// 🔍 Symbol Finder Implementation - シンボル検索実装
//=============================================================================

#include "nekocode/symbol_finder.hpp"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <cctype>

namespace nekocode {

//=============================================================================
// 🔍 SymbolFinder 実装
//=============================================================================

SymbolFinder::FindResults SymbolFinder::find(
    const std::string& symbol_name, 
    const FindOptions& options) {
    
    if (options.debug) {
        std::cerr << "[DEBUG SymbolFinder::find] Starting search for: " << symbol_name << std::endl;
        std::cerr << "[DEBUG SymbolFinder::find] Files count: " << files_.size() << std::endl;
    }
    
    return findInFiles(symbol_name, options);
}

SymbolFinder::FindResults SymbolFinder::findInFiles(
    const std::string& symbol, 
    const FindOptions& options) {
    
    FindResults results;
    
    // 検索対象ファイルのフィルタリング
    std::vector<FileInfo> target_files;
    
    if (options.search_paths.empty()) {
        // パス指定なしなら全ファイル
        target_files = files_;
    } else {
        // パス指定あり
        for (const auto& file : files_) {
            for (const auto& path : options.search_paths) {
                // シンプルなパスマッチング
                std::string file_path_str = file.path.string();
                if (file_path_str.find(path) == 0) {
                    target_files.push_back(file);
                    break;
                }
            }
        }
    }
    
    if (options.debug) {
        std::cerr << "[DEBUG findInFiles] Target files count: " << target_files.size() << std::endl;
    }
    
    // 各ファイルで検索
    for (const auto& file : target_files) {
        // ファイル内容を読み込む
        if (options.debug) {
            std::cerr << "[DEBUG findInFiles] Processing file: " << file.path << std::endl;
            
            // 詳細デバッグ: ファイル存在・パーミッション確認
            std::cerr << "[DEBUG findInFiles] File path string: '" << file.path.string() << "'" << std::endl;
            std::cerr << "[DEBUG findInFiles] File exists: " << std::filesystem::exists(file.path) << std::endl;
            std::cerr << "[DEBUG findInFiles] Is regular file: " << std::filesystem::is_regular_file(file.path) << std::endl;
            
            if (std::filesystem::exists(file.path)) {
                auto perms = std::filesystem::status(file.path).permissions();
                std::cerr << "[DEBUG findInFiles] File permissions readable: " 
                         << ((perms & std::filesystem::perms::owner_read) != std::filesystem::perms::none) << std::endl;
            }
        }
        
        std::ifstream ifs(file.path);
        if (!ifs.is_open()) {
            if (options.debug) {
                std::cerr << "[DEBUG findInFiles] Failed to open file: " << file.path << std::endl;
                std::cerr << "[DEBUG findInFiles] Current working directory: " << std::filesystem::current_path() << std::endl;
                std::cerr << "[DEBUG findInFiles] Absolute path: " << std::filesystem::absolute(file.path) << std::endl;
            }
            continue;  // ファイルが開けない場合はスキップ
        }
        
        std::string content((std::istreambuf_iterator<char>(ifs)),
                           std::istreambuf_iterator<char>());
        ifs.close();
        
        if (options.debug) {
            std::cerr << "[DEBUG findInFiles] File content size: " << content.size() << " bytes" << std::endl;
            if (content.size() > 0) {
                std::cerr << "[DEBUG findInFiles] First 100 chars: " 
                         << content.substr(0, std::min(content.size(), size_t(100))) << std::endl;
            }
        }
        
        auto file_results = findInFile(file.path.string(), content, symbol, options);
        if (options.debug) {
            std::cerr << "[DEBUG findInFiles] Found " << file_results.size() << " matches in this file" << std::endl;
        }
        
        for (const auto& loc : file_results) {
            results.addLocation(loc);
            
            // ファイル別カウント
            results.file_counts[loc.file_path]++;
        }
    }
    
    results.total_count = results.locations.size();
    return results;
}

std::vector<SymbolFinder::SymbolLocation> SymbolFinder::findInFile(
    const std::string& filename,
    const std::string& content,
    const std::string& symbol,
    const FindOptions& options) {
    
    if (options.debug) {
        std::cerr << "[DEBUG findInFile] Searching for '" << symbol << "' in " << filename << std::endl;
        std::cerr << "[DEBUG findInFile] Content size: " << content.size() << " bytes" << std::endl;
    }
    
    std::vector<SymbolLocation> locations;
    
    // 行ごとに処理
    std::istringstream stream(content);
    std::string line;
    uint32_t line_number = 0;
    
    while (std::getline(stream, line)) {
        line_number++;
        
        // 高速スキップ：シンボルが含まれていない行
        if (line.find(symbol) == std::string::npos) {
            continue;
        }
        
        if (options.debug) {
            std::cerr << "[DEBUG findInFile] Line " << line_number << " contains symbol: " << line << std::endl;
        }
        
        // シンボルの出現位置を全て検索
        size_t pos = 0;
        while ((pos = line.find(symbol, pos)) != std::string::npos) {
            // コメント内チェック
            if (isInComment(line, pos)) {
                pos += symbol.length();
                continue;
            }
            
            // 文字列内チェック
            if (isInString(line, pos)) {
                pos += symbol.length();
                continue;
            }
            
            // 単語境界チェック（前後が英数字でないこと）
            bool is_word_start = (pos == 0 || !std::isalnum(line[pos-1]));
            bool is_word_end = (pos + symbol.length() >= line.length() || 
                               !std::isalnum(line[pos + symbol.length()]));
            
            if (!is_word_start || !is_word_end) {
                pos += symbol.length();
                continue;
            }
            
            // 使用タイプとシンボルタイプを検出
            UseType use_type = detectUseType(line, pos, symbol);
            SymbolType symbol_type = detectSymbolType(line, pos, symbol);
            
            // オプションによるフィルタリング
            if (options.type == SymbolType::FUNCTION && 
                symbol_type != SymbolType::FUNCTION) {
                pos += symbol.length();
                continue;
            }
            if (options.type == SymbolType::VARIABLE && 
                symbol_type != SymbolType::VARIABLE) {
                pos += symbol.length();
                continue;
            }
            
            // 結果に追加
            SymbolLocation loc;
            loc.file_path = filename;
            loc.line_number = line_number;
            loc.line_content = line;
            loc.use_type = use_type;
            loc.symbol_type = symbol_type;
            
            locations.push_back(loc);
            
            pos += symbol.length();
        }
    }
    
    return locations;
}

bool SymbolFinder::isInComment(const std::string& line, size_t pos) {
    // 単純な単一行コメントチェック
    size_t comment_pos = line.find("//");
    if (comment_pos != std::string::npos && pos >= comment_pos) {
        return true;
    }
    
    // /* */ スタイルは複雑なので今回はスキップ
    return false;
}

bool SymbolFinder::isInString(const std::string& line, size_t pos) {
    // シンプルな文字列チェック（エスケープは考慮しない）
    int single_quotes = 0;
    int double_quotes = 0;
    int backticks = 0;
    
    for (size_t i = 0; i < pos; ++i) {
        if (line[i] == '\'' && (i == 0 || line[i-1] != '\\')) {
            single_quotes++;
        } else if (line[i] == '"' && (i == 0 || line[i-1] != '\\')) {
            double_quotes++;
        } else if (line[i] == '`') {
            backticks++;
        }
    }
    
    return (single_quotes % 2 == 1) || 
           (double_quotes % 2 == 1) || 
           (backticks % 2 == 1);
}

SymbolFinder::UseType SymbolFinder::detectUseType(
    const std::string& line, size_t pos, const std::string& symbol) {
    
    // 宣言パターン
    if (pos >= 4 && line.substr(pos - 4, 4) == "let " ||
        pos >= 6 && line.substr(pos - 6, 6) == "const " ||
        pos >= 4 && line.substr(pos - 4, 4) == "var " ||
        pos >= 9 && line.substr(pos - 9, 9) == "function ") {
        return UseType::DECLARATION;
    }
    
    // 代入パターン（シンボルの後に = があるか）
    size_t after_pos = pos + symbol.length();
    while (after_pos < line.length() && std::isspace(line[after_pos])) {
        after_pos++;
    }
    if (after_pos < line.length() && line[after_pos] == '=') {
        return UseType::ASSIGNMENT;
    }
    
    // 関数呼び出しパターン
    while (after_pos < line.length() && std::isspace(line[after_pos])) {
        after_pos++;
    }
    if (after_pos < line.length() && line[after_pos] == '(') {
        return UseType::CALL;
    }
    
    // その他は参照
    return UseType::REFERENCE;
}

SymbolFinder::SymbolType SymbolFinder::detectSymbolType(
    const std::string& line, size_t pos, const std::string& symbol) {
    
    // 関数パターン
    // 1. 関数宣言
    if (pos >= 9 && line.substr(pos - 9, 9) == "function ") {
        return SymbolType::FUNCTION;
    }
    
    // 2. 関数呼び出し
    size_t after_pos = pos + symbol.length();
    while (after_pos < line.length() && std::isspace(line[after_pos])) {
        after_pos++;
    }
    if (after_pos < line.length() && line[after_pos] == '(') {
        return SymbolType::FUNCTION;
    }
    
    // 3. アロー関数
    if (line.find("=>", after_pos) != std::string::npos) {
        return SymbolType::FUNCTION;
    }
    
    // その他は変数として扱う
    return SymbolType::VARIABLE;
}

void SymbolFinder::FindResults::addLocation(const SymbolLocation& loc) {
    locations.push_back(loc);
    
    // 統計情報更新
    if (loc.symbol_type == SymbolType::FUNCTION) {
        function_count++;
    } else {
        variable_count++;
    }
    
    use_type_counts[loc.use_type]++;
}

std::string SymbolFinder::SymbolLocation::toString() const {
    std::stringstream ss;
    ss << file_path << ":" << line_number << " " << line_content;
    return ss.str();
}

//=============================================================================
// 📋 FindOutputManager 実装
//=============================================================================

void FindOutputManager::display(
    const SymbolFinder::FindResults& results,
    const SymbolFinder::FindOptions& options,
    const std::string& symbol_name) {
    
    // 結果なし
    if (results.isEmpty()) {
        std::cerr << "\n❌ '" << symbol_name << "' は見つかりませんでした。\n\n";
        return;
    }
    
    // Claude Code対応：大量の結果は自動的にファイル出力
    bool should_output_to_file = false;
    std::string output_filename = options.output_file;
    
    if (results.total_count > options.display_limit) {
        should_output_to_file = true;
        if (output_filename.empty()) {
            output_filename = generateFilename(symbol_name);
        }
    } else if (!options.output_file.empty()) {
        should_output_to_file = true;
    }
    
    if (should_output_to_file) {
        saveToFile(results, output_filename, symbol_name);
    }
    
    displayToTerminal(results, options, symbol_name);
}

void FindOutputManager::displayToTerminal(
    const SymbolFinder::FindResults& results,
    const SymbolFinder::FindOptions& options,
    const std::string& symbol_name) {
    
    size_t display_count = std::min(results.total_count, options.display_limit);
    
    // ヘッダー
    std::cerr << "\n🔍 '" << symbol_name << "' の検索結果:\n\n";
    
    // 統計情報（結果が多い場合）
    if (results.total_count > 10) {
        if (results.function_count > 0 && results.variable_count > 0) {
            std::cerr << "📊 関数: " << results.function_count 
                     << "件, 変数: " << results.variable_count << "件\n\n";
        }
    }
    
    // 結果表示
    for (size_t i = 0; i < display_count && i < results.locations.size(); ++i) {
        const auto& loc = results.locations[i];
        
        // ファイルパス:行番号
        std::cerr << loc.file_path << ":" << loc.line_number;
        
        // 内容（インデントを整える）
        std::cerr << "  " << loc.line_content << "\n";
    }
    
    // 省略情報
    if (display_count < results.total_count) {
        std::string filename = options.output_file.empty() ? 
                              generateFilename(symbol_name) : options.output_file;
        displayOmissionInfo(display_count, results.total_count, filename);
    } else {
        std::cerr << "\n✅ 全" << results.total_count << "件を表示しました。\n\n";
    }
}

void FindOutputManager::saveToFile(
    const SymbolFinder::FindResults& results,
    const std::string& filename,
    const std::string& symbol_name) {
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "❌ ファイルを開けませんでした: " << filename << "\n";
        return;
    }
    
    // ヘッダー
    file << "NekoCode 検索結果レポート\n";
    file << "================================================================================\n";
    file << "検索語: " << symbol_name << "\n";
    
    // タイムスタンプ
    auto now = std::time(nullptr);
    file << "検索日時: " << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S") << "\n";
    file << "================================================================================\n\n";
    
    // サマリー
    file << "【サマリー】\n";
    file << "- 総ヒット数: " << results.total_count << "件\n";
    file << "- 関数として: " << results.function_count << "件\n";
    file << "- 変数として: " << results.variable_count << "件\n";
    file << "- 影響ファイル数: " << results.file_counts.size() << "ファイル\n\n";
    
    // 詳細結果
    file << "【詳細結果】\n\n";
    
    std::string current_file;
    for (const auto& loc : results.locations) {
        // ファイル名が変わったら区切り
        if (loc.file_path != current_file) {
            current_file = loc.file_path;
            file << "\n" << std::string(50, '=') << "\n";
            file << "📁 " << current_file << "\n";
            file << std::string(50, '=') << "\n\n";
        }
        
        file << "  " << loc.line_number << ": " << loc.line_content << "\n";
    }
    
    file.close();
    
    std::cerr << "💾 結果をファイルに保存しました: " << filename << "\n";
}

void FindOutputManager::displayOmissionInfo(
    size_t displayed, size_t total, const std::string& filename) {
    
    size_t omitted = total - displayed;
    
    std::cerr << "\n" << std::string(50, '-') << "\n";
    std::cerr << "📊 表示: " << displayed << "件 / 全" << total << "件";
    std::cerr << "（" << omitted << "件省略）\n";
    std::cerr << "📁 残り" << omitted << "件は以下のファイルに保存されました:\n";
    std::cerr << "   → " << filename << "\n";
    std::cerr << std::string(50, '-') << "\n\n";
}

std::string FindOutputManager::generateFilename(const std::string& symbol_name) {
    auto now = std::time(nullptr);
    std::stringstream ss;
    ss << "find_results_" << symbol_name << "_" 
       << std::put_time(std::localtime(&now), "%Y%m%d_%H%M%S") << ".txt";
    return ss.str();
}

} // namespace nekocode