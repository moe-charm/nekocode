#include "nekocode/moveclass.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <regex>

namespace nekocode {

//=============================================================================
// MoveClassEngine実装
//=============================================================================

MoveResult MoveClassEngine::move_class(const std::string& class_symbol_id,
                                       const std::string& target_file_path) {
    MoveResult result;
    
    // Symbolの存在確認
    auto symbol = symbol_table_->get_symbol(class_symbol_id);
    if (!symbol) {
        result.errors.push_back("Symbol not found: " + class_symbol_id);
        return result;
    }
    
    // 移動可能性チェック
    if (!RefactoringUtils::is_symbol_movable(*symbol, *dependency_graph_)) {
        result.errors.push_back("Symbol is not movable: " + class_symbol_id);
        return result;
    }
    
    // 依存関係分析
    auto impact = dependency_graph_->analyze_move_impact(class_symbol_id, target_file_path);
    
    if (!impact.is_safe_to_move) {
        result.errors.push_back("Move is not safe due to dependencies");
        result.warnings = impact.warnings;
        return result;
    }
    
    // 関連Symbolの収集
    std::vector<std::string> symbols_to_move = {class_symbol_id};
    if (options_.move_related_symbols) {
        auto related = dependency_graph_->get_required_symbols_for_move(class_symbol_id);
        symbols_to_move.insert(symbols_to_move.end(), related.begin(), related.end());
    }
    
    // dry-runモードの場合はここで終了
    if (options_.dry_run) {
        result.success = true;
        result.moved_symbols = symbols_to_move;
        result.warnings.push_back("Dry-run mode: No actual changes made");
        return result;
    }
    
    // TODO: 実際の移動処理を実装
    // 1. 元ファイルからコード抽出
    // 2. 元ファイルからコード削除
    // 3. 新ファイルにコード挿入
    // 4. import文の更新
    
    result.warnings.push_back("Move operation not fully implemented yet");
    return result;
}

MoveResult MoveClassEngine::move_symbols(const std::vector<std::string>& symbol_ids,
                                        const std::string& target_file_path) {
    MoveResult result;
    
    for (const auto& id : symbol_ids) {
        auto single_result = move_class(id, target_file_path);
        
        // 結果のマージ
        if (!single_result.success) {
            result.errors.insert(result.errors.end(), 
                               single_result.errors.begin(), 
                               single_result.errors.end());
        }
        
        result.moved_symbols.insert(result.moved_symbols.end(),
                                   single_result.moved_symbols.begin(),
                                   single_result.moved_symbols.end());
        
        result.warnings.insert(result.warnings.end(),
                             single_result.warnings.begin(),
                             single_result.warnings.end());
    }
    
    result.success = result.errors.empty();
    return result;
}

MoveResult MoveClassEngine::preview_move(const std::string& class_symbol_id,
                                        const std::string& target_file_path) {
    MoveOptions preview_opts = options_;
    preview_opts.dry_run = true;
    
    MoveClassEngine preview_engine(symbol_table_, dependency_graph_, language_, preview_opts);
    return preview_engine.move_class(class_symbol_id, target_file_path);
}

bool MoveClassEngine::rollback(const MoveResult& move_result) {
    if (!move_result.success || move_result.backups.empty()) {
        return false;
    }
    
    bool all_success = true;
    
    for (const auto& backup : move_result.backups) {
        if (!write_file(backup.file_path, backup.original_content)) {
            all_success = false;
            if (options_.verbose) {
                std::cerr << "Failed to rollback: " << backup.file_path << std::endl;
            }
        }
    }
    
    return all_success;
}

std::string MoveClassEngine::extract_symbol_code(const std::string& file_path,
                                                const UniversalSymbolInfo& symbol) {
    std::string content = read_file(file_path);
    if (content.empty()) {
        return "";
    }
    
    // 行番号ベースでコード抽出（簡易実装）
    std::istringstream stream(content);
    std::string line;
    std::string extracted;
    LineNumber current_line = 1;
    
    while (std::getline(stream, line)) {
        if (current_line >= symbol.start_line && current_line <= symbol.end_line) {
            extracted += line + "\n";
        }
        current_line++;
    }
    
    return extracted;
}

std::string MoveClassEngine::remove_symbol_code(const std::string& file_content,
                                               const UniversalSymbolInfo& symbol) {
    std::istringstream stream(file_content);
    std::string line;
    std::string result;
    LineNumber current_line = 1;
    
    while (std::getline(stream, line)) {
        if (current_line < symbol.start_line || current_line > symbol.end_line) {
            result += line + "\n";
        }
        current_line++;
    }
    
    return result;
}

std::string MoveClassEngine::insert_symbol_code(const std::string& file_content,
                                               const std::string& symbol_code,
                                               const std::string& target_position) {
    if (target_position == "end") {
        return file_content + "\n" + symbol_code;
    } else if (target_position == "start") {
        // import文の後に挿入する処理が必要
        return symbol_code + "\n" + file_content;
    }
    
    return file_content + "\n" + symbol_code;
}

std::string MoveClassEngine::update_imports_in_file(const std::string& file_content,
                                                   const std::string& old_path,
                                                   const std::string& new_path,
                                                   const std::vector<std::string>& moved_symbols) {
    // 言語別のimport文更新処理
    auto imports = ImportAnalyzer::parse_imports(file_content, language_);
    std::string updated = file_content;
    
    for (const auto& import : imports) {
        if (import.module_or_file == old_path) {
            // 移動したSymbolが含まれているかチェック
            bool needs_update = false;
            for (const auto& sym : import.symbols) {
                for (const auto& moved : moved_symbols) {
                    auto symbol = symbol_table_->get_symbol(moved);
                    if (symbol && symbol->name == sym) {
                        needs_update = true;
                        break;
                    }
                }
            }
            
            if (needs_update) {
                auto new_import = ImportAnalyzer::update_import_statement(
                    import, old_path, new_path, language_
                );
                
                // 元のimport文を新しいものに置換
                size_t pos = updated.find(import.raw_statement);
                if (pos != std::string::npos) {
                    updated.replace(pos, import.raw_statement.length(), new_import);
                }
            }
        }
    }
    
    return updated;
}

std::string MoveClassEngine::generate_import_statement(const std::string& from_path,
                                                      const std::vector<std::string>& symbols) {
    std::string import_stmt;
    
    switch (language_) {
        case Language::JAVASCRIPT:
        case Language::TYPESCRIPT:
            import_stmt = "import { ";
            for (size_t i = 0; i < symbols.size(); ++i) {
                if (i > 0) import_stmt += ", ";
                import_stmt += symbols[i];
            }
            import_stmt += " } from '" + from_path + "';";
            break;
            
        case Language::PYTHON:
            import_stmt = "from " + from_path + " import ";
            for (size_t i = 0; i < symbols.size(); ++i) {
                if (i > 0) import_stmt += ", ";
                import_stmt += symbols[i];
            }
            break;
            
        case Language::CPP:
        case Language::C:
            import_stmt = "#include \"" + from_path + "\"";
            break;
            
        case Language::CSHARP:
            import_stmt = "using " + from_path + ";";
            break;
            
        case Language::GO:
            import_stmt = "import \"" + from_path + "\"";
            break;
            
        case Language::RUST:
            import_stmt = "use " + from_path;
            if (!symbols.empty()) {
                import_stmt += "::{";
                for (size_t i = 0; i < symbols.size(); ++i) {
                    if (i > 0) import_stmt += ", ";
                    import_stmt += symbols[i];
                }
                import_stmt += "}";
            }
            import_stmt += ";";
            break;
            
        default:
            break;
    }
    
    return import_stmt;
}

std::string MoveClassEngine::read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool MoveClassEngine::write_file(const std::string& path, const std::string& content) {
    // バックアップ作成
    if (options_.create_backup) {
        std::string original = read_file(path);
        if (!original.empty()) {
            create_backup(path, original);
        }
    }
    
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    file << content;
    return file.good();
}

void MoveClassEngine::create_backup(const std::string& path, const std::string& content) {
    std::string backup_path = path + ".bak";
    std::ofstream backup(backup_path);
    if (backup.is_open()) {
        backup << content;
    }
}

//=============================================================================
// RefactoringUtils実装
//=============================================================================

std::string RefactoringUtils::calculate_relative_path(const std::string& from_file,
                                                     const std::string& to_file) {
    std::filesystem::path from(from_file);
    std::filesystem::path to(to_file);
    
    // 親ディレクトリを取得
    from = from.parent_path();
    
    // 相対パスを計算
    auto relative = std::filesystem::relative(to, from);
    
    // ./ で始まるように調整
    std::string result = relative.string();
    if (!result.empty() && result[0] != '.') {
        result = "./" + result;
    }
    
    return result;
}

std::string RefactoringUtils::normalize_import_path(const std::string& path, Language lang) {
    std::string normalized = path;
    
    // 言語別の正規化
    switch (lang) {
        case Language::JAVASCRIPT:
        case Language::TYPESCRIPT:
            // 拡張子を除去
            if (normalized.ends_with(".js") || normalized.ends_with(".ts") || 
                normalized.ends_with(".jsx") || normalized.ends_with(".tsx")) {
                auto pos = normalized.rfind('.');
                if (pos != std::string::npos) {
                    normalized = normalized.substr(0, pos);
                }
            }
            break;
            
        case Language::PYTHON:
            // スラッシュをドットに変換
            std::replace(normalized.begin(), normalized.end(), '/', '.');
            // .pyを除去
            if (normalized.ends_with(".py")) {
                normalized = normalized.substr(0, normalized.length() - 3);
            }
            break;
            
        default:
            break;
    }
    
    return normalized;
}

bool RefactoringUtils::is_symbol_movable(const UniversalSymbolInfo& symbol,
                                        const DependencyGraph& dep_graph) {
    // 基本的なチェック
    if (symbol.symbol_type == SymbolType::PARAMETER ||
        symbol.symbol_type == SymbolType::VARIABLE) {
        return false; // ローカル変数やパラメータは移動不可
    }
    
    // 循環依存チェック
    if (dep_graph.has_circular_dependency(symbol.symbol_id)) {
        return false;
    }
    
    // その他の移動可能性チェック
    auto node = dep_graph.get_node(symbol.symbol_id);
    if (node && !node->is_movable) {
        return false;
    }
    
    return true;
}


std::string RefactoringUtils::format_code(const std::string& code, Language lang) {
    // 簡易的なフォーマット処理
    // 実際にはclang-format、prettier等の外部ツールを呼び出すべき
    return code;
}

//=============================================================================
// MoveClassCommand実装
//=============================================================================

MoveClassCommand::Response MoveClassCommand::execute(const Request& request) {
    Response response;
    
    // TODO: Session Modeとの統合
    // 1. SessionからSymbolTableとDependencyGraphを取得
    // 2. MoveClassEngineを作成
    // 3. move_classを実行
    
    response.details["message"] = "MoveClass command not fully implemented";
    response.details["request"] = {
        {"session_id", request.session_id},
        {"symbol_id", request.symbol_id},
        {"target_file", request.target_file}
    };
    
    return response;
}

MoveClassCommand::Response MoveClassCommand::preview(const Request& request) {
    Request preview_request = request;
    preview_request.options.dry_run = true;
    return execute(preview_request);
}

nlohmann::json MoveClassCommand::to_json(const Response& response) {
    nlohmann::json json;
    json["success"] = response.success;
    
    if (response.success) {
        json["result"] = {
            {"moved_symbols", response.result.moved_symbols},
            {"updated_files", response.result.updated_files},
            {"added_imports", response.result.added_imports},
            {"removed_imports", response.result.removed_imports}
        };
    }
    
    if (!response.result.errors.empty()) {
        json["errors"] = response.result.errors;
    }
    
    if (!response.result.warnings.empty()) {
        json["warnings"] = response.result.warnings;
    }
    
    json["details"] = response.details;
    
    return json;
}

} // namespace nekocode