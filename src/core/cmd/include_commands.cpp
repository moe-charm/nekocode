//=============================================================================
// 🔗 Include Commands実装 - C++インクルード依存解析コマンド
//=============================================================================

#include "nekocode/session_commands.hpp"
#include "nekocode/include_analyzer.hpp"
#include <algorithm>
#include <iostream>
#include <numeric>
#include <fstream>
#include <sstream>
#include <set>
#include <map>
#include <filesystem>

namespace nekocode {

//=============================================================================
// 🔗 C++インクルード依存解析コマンド実装
//=============================================================================

nlohmann::json SessionCommands::cmd_include_graph(const SessionData& session) const {
    try {
        // IncludeAnalyzerを作成
        IncludeAnalyzer analyzer;
        
        // 設定
        IncludeAnalyzer::Config config;
        config.analyze_system_headers = false;  // システムヘッダーは除外
        config.detect_circular = true;
        config.detect_unused = true;
        analyzer.set_config(config);
        
        // ディレクトリ解析実行
        auto analysis_result = analyzer.analyze_directory(session.target_path);
        
        // グラフ情報をJSON形式で返却
        return analyzer.get_include_graph(analysis_result);
        
    } catch (const std::exception& e) {
        return {
            {"command", "include-graph"},
            {"error", e.what()},
            {"summary", "Include graph analysis failed"}
        };
    }
}

nlohmann::json SessionCommands::cmd_include_cycles(const SessionData& session) const {
    try {
        // IncludeAnalyzerを作成
        IncludeAnalyzer analyzer;
        
        // 設定
        IncludeAnalyzer::Config config;
        config.analyze_system_headers = false;
        config.detect_circular = true;
        config.detect_unused = false;  // 循環依存のみ検出
        analyzer.set_config(config);
        
        // ディレクトリ解析実行
        auto analysis_result = analyzer.analyze_directory(session.target_path);
        
        // 循環依存情報をJSON形式で返却
        return analyzer.get_circular_dependencies(analysis_result);
        
    } catch (const std::exception& e) {
        return {
            {"command", "include-cycles"},
            {"error", e.what()},
            {"summary", "Circular dependency detection failed"}
        };
    }
}

nlohmann::json SessionCommands::cmd_include_impact(const SessionData& session) const {
    return {
        {"command", "include-impact"},
        {"result", "Not implemented yet - moved to SessionCommands"},
        {"summary", "Include impact feature pending implementation"}
    };
}

nlohmann::json SessionCommands::cmd_include_unused(const SessionData& session) const {
    nlohmann::json result = {
        {"command", "include-unused"},
        {"unused_includes", nlohmann::json::array()},
        {"total_unused", 0}
    };
    
    if (!session.is_directory) {
        result["summary"] = "Single file analysis - unused include detection not applicable";
        return result;
    }
    
    try {
        // 🔥 ハイブリッドアプローチ: IncludeAnalyzer + SessionData実解析結果
        
        // 1. IncludeAnalyzerで#include情報を取得
        IncludeAnalyzer analyzer;
        IncludeAnalyzer::Config config;
        config.analyze_system_headers = false;
        analyzer.set_config(config);
        auto include_result = analyzer.analyze_directory(session.target_path);
        
        // 2. SessionDataから実際の提供シンボルを構築
        std::map<std::string, std::set<std::string>> provided_symbols; // ファイル→提供シンボル
        for (const auto& file : session.directory_result.files) {
            std::set<std::string> symbols;
            
            // クラス名を追加
            for (const auto& cls : file.classes) {
                symbols.insert(cls.name);
            }
            
            // 関数名を追加  
            for (const auto& func : file.functions) {
                symbols.insert(func.name);
            }
            
            // ファイル名でキー作成（フルパスから）
            std::string filename = std::filesystem::path(file.file_info.name).filename().string();
            provided_symbols[filename] = symbols;
        }
        
        // 3. 不要includeを検出
        nlohmann::json unused_array = nlohmann::json::array();
        int total_unused = 0;
        
        // include_resultから各ファイルのinclude情報を取得
        for (const auto& [file_path, node] : include_result.dependency_graph) {
            // .cppファイルのみチェック
            std::string ext = std::filesystem::path(file_path).extension().string();
            if (ext != ".cpp" && ext != ".cxx" && ext != ".cc") continue;
            
            // ファイル内容を一度だけ読み込み
            std::ifstream ifs(file_path);
            if (!ifs.is_open()) continue;
            std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            
            for (const auto& inc : node.include_statements) {
                // システムヘッダーはスキップ
                if (inc.is_system_header) continue;
                
                // included fileのファイル名を取得
                std::string included_filename = std::filesystem::path(inc.path).filename().string();
                
                // 提供シンボルを取得
                auto provided_it = provided_symbols.find(included_filename);
                if (provided_it == provided_symbols.end()) continue;
                
                // シンボル使用チェック (NO_REGEX準拠 + include文除外)
                bool is_used = false;
                for (const auto& symbol : provided_it->second) {
                    // include文の行を除外してチェック
                    std::istringstream iss(content);
                    std::string line;
                    size_t current_pos = 0;
                    
                    while (std::getline(iss, line)) {
                        // include文の行はスキップ
                        if (line.find("#include") != std::string::npos) {
                            current_pos += line.length() + 1; // +1 for newline
                            continue;
                        }
                        
                        // この行内でシンボル検索
                        size_t pos = line.find(symbol);
                        while (pos != std::string::npos) {
                            // 前の文字チェック（単語境界）
                            bool prev_ok = (pos == 0) || 
                                          (!std::isalnum(line[pos-1]) && line[pos-1] != '_');
                            // 後の文字チェック（単語境界）
                            bool next_ok = (pos + symbol.length() >= line.length()) || 
                                          (!std::isalnum(line[pos + symbol.length()]) && 
                                           line[pos + symbol.length()] != '_');
                            
                            if (prev_ok && next_ok) {
                                is_used = true;
                                break;
                            }
                            pos = line.find(symbol, pos + 1);
                        }
                        
                        if (is_used) break;
                        current_pos += line.length() + 1; // +1 for newline
                    }
                    if (is_used) break;
                }
                
                if (!is_used && !provided_it->second.empty()) {
                    unused_array.push_back({
                        {"file", std::filesystem::path(file_path).filename().string()},
                        {"unused_include", inc.path},
                        {"line", inc.line_number},
                        {"provided_symbols", std::vector<std::string>(provided_it->second.begin(), provided_it->second.end())},
                        {"reason", "None of the provided symbols are used in this file"}
                    });
                    total_unused++;
                }
            }
        }
        
        result["unused_includes"] = unused_array;
        result["total_unused"] = total_unused;
        result["summary"] = "Found " + std::to_string(total_unused) + " unused includes using hybrid analysis (IncludeAnalyzer + SessionData)";
        
        // 🎯 Production-ready output (debug info removed)
        
    } catch (const std::exception& e) {
        result["error"] = e.what();
        result["summary"] = "Unused include detection failed: " + std::string(e.what());
    }
    
    return result;
}

nlohmann::json SessionCommands::cmd_include_optimize(const SessionData& session) const {
    return {
        {"command", "include-optimize"},
        {"result", "Not implemented yet - moved to SessionCommands"},
        {"summary", "Include optimize feature pending implementation"}
    };
}

} // namespace nekocode