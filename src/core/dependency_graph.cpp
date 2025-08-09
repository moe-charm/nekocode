#include "nekocode/dependency_graph.hpp"
#include "commands/direct_edit/pcre2_engine.hpp"
#include <regex>
#include <sstream>
#include <queue>
#include <algorithm>

namespace nekocode {

//=============================================================================
// DependencyGraph実装
//=============================================================================

void DependencyGraph::add_node(const std::string& symbol_id,
                               const std::string& symbol_name,
                               const std::string& file_path,
                               SymbolType type) {
    if (nodes_.find(symbol_id) == nodes_.end()) {
        auto node = std::make_unique<DependencyNode>();
        node->symbol_id = symbol_id;
        node->symbol_name = symbol_name;
        node->file_path = file_path;
        node->symbol_type = type;
        nodes_[symbol_id] = std::move(node);
        
        // ファイル→Symbolマッピング更新
        file_to_symbols_[file_path].push_back(symbol_id);
    }
}

void DependencyGraph::add_edge(const std::string& from_id,
                               const std::string& to_id,
                               DependencyType type,
                               LineNumber line,
                               const std::string& context) {
    // ノードが存在しない場合は作成
    if (nodes_.find(from_id) == nodes_.end() || nodes_.find(to_id) == nodes_.end()) {
        return; // エラー処理：ノードが存在しない
    }
    
    // エッジ情報作成
    DependencyEdge edge;
    edge.from_symbol_id = from_id;
    edge.to_symbol_id = to_id;
    edge.type = type;
    edge.line_number = line;
    edge.context = context;
    
    // 依存関係を記録
    nodes_[from_id]->depends_on.push_back(to_id);
    nodes_[to_id]->depended_by.push_back(from_id);
    nodes_[from_id]->edges[to_id] = edge;
}

void DependencyGraph::build_from_symbol_table(const SymbolTable& symbol_table) {
    // Symbol Tableから全Symbolをノードとして追加
    for (const auto& symbol : symbol_table.get_all_symbols()) {
        const auto& symbol_id = symbol.symbol_id;
        const auto& symbol_info = symbol;
        // SymbolTypeはそのまま使用
        SymbolType type = symbol_info.symbol_type;
        
        add_node(symbol_id, 
                symbol_info.name,
                "", // TODO: file_pathをUniversalSymbolInfoに追加するか、metadataから取得
                type);
        
        // 親子関係から依存を推定
        if (!symbol_info.parent_id.empty()) {
            add_edge(symbol_id, symbol_info.parent_id, DependencyType::COMPOSITION);
        }
    }
}

void DependencyGraph::analyze_imports(const std::string& file_path,
                                      const std::vector<std::string>& import_statements) {
    // ファイル内の全Symbolを取得
    auto symbols_in_file = get_symbols_in_file(file_path);
    
    for (const auto& import_stmt : import_statements) {
        // import文を解析（簡易版）
        // TODO: 言語別の詳細な解析を実装
        for (const auto& symbol_id : symbols_in_file) {
            // import先のSymbolを探す（簡易実装）
            // 本来はimport文のパスとSymbol名から正確に特定する必要がある
        }
    }
}

std::vector<std::string> DependencyGraph::get_dependencies(const std::string& symbol_id) const {
    auto it = nodes_.find(symbol_id);
    if (it != nodes_.end()) {
        return it->second->depends_on;
    }
    return {};
}

std::vector<std::string> DependencyGraph::get_dependents(const std::string& symbol_id) const {
    auto it = nodes_.find(symbol_id);
    if (it != nodes_.end()) {
        return it->second->depended_by;
    }
    return {};
}

bool DependencyGraph::has_circular_dependency(const std::string& symbol_id) const {
    return cyclic_dependencies_.find(symbol_id) != cyclic_dependencies_.end();
}

void DependencyGraph::detect_circular_dependencies() {
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> rec_stack;
    
    for (const auto& [symbol_id, node] : nodes_) {
        if (visited.find(symbol_id) == visited.end()) {
            if (dfs_detect_cycle(symbol_id, visited, rec_stack)) {
                // 循環依存を検出
                for (const auto& id : rec_stack) {
                    cyclic_dependencies_.insert(id);
                }
            }
        }
    }
}

bool DependencyGraph::dfs_detect_cycle(const std::string& node_id,
                                       std::unordered_set<std::string>& visited,
                                       std::unordered_set<std::string>& rec_stack) const {
    visited.insert(node_id);
    rec_stack.insert(node_id);
    
    auto it = nodes_.find(node_id);
    if (it != nodes_.end()) {
        for (const auto& dep_id : it->second->depends_on) {
            if (visited.find(dep_id) == visited.end()) {
                if (dfs_detect_cycle(dep_id, visited, rec_stack)) {
                    return true;
                }
            } else if (rec_stack.find(dep_id) != rec_stack.end()) {
                return true; // 循環を検出
            }
        }
    }
    
    rec_stack.erase(node_id);
    return false;
}

DependencyGraph::MoveImpact DependencyGraph::analyze_move_impact(
    const std::string& symbol_id,
    const std::string& target_file) const {
    
    MoveImpact impact;
    
    auto node = get_node(symbol_id);
    if (!node) {
        impact.is_safe_to_move = false;
        impact.warnings.push_back("Symbol not found: " + symbol_id);
        return impact;
    }
    
    // 循環依存チェック
    if (has_circular_dependency(symbol_id)) {
        impact.has_circular_dependency = true;
        impact.warnings.push_back("Symbol has circular dependencies");
    }
    
    // 依存しているSymbolをチェック
    for (const auto& dep_id : node->depends_on) {
        auto dep_node = get_node(dep_id);
        if (dep_node) {
            // 同じファイルから移動する場合の影響
            if (dep_node->file_path == node->file_path && 
                dep_node->file_path != target_file) {
                impact.affected_symbols.push_back(dep_id);
                impact.required_imports.push_back(
                    "Need to import " + dep_node->symbol_name + " from " + target_file
                );
            }
        }
    }
    
    // このSymbolに依存しているものをチェック
    for (const auto& dependent_id : node->depended_by) {
        auto dep_node = get_node(dependent_id);
        if (dep_node) {
            impact.affected_files.push_back(dep_node->file_path);
            impact.affected_symbols.push_back(dependent_id);
            impact.required_imports.push_back(
                "Update import of " + node->symbol_name + " in " + dep_node->file_path
            );
        }
    }
    
    // 重複を除去
    std::sort(impact.affected_files.begin(), impact.affected_files.end());
    impact.affected_files.erase(
        std::unique(impact.affected_files.begin(), impact.affected_files.end()),
        impact.affected_files.end()
    );
    
    return impact;
}

std::vector<std::string> DependencyGraph::get_required_symbols_for_move(
    const std::string& class_id) const {
    
    std::vector<std::string> required_symbols;
    std::unordered_set<std::string> visited;
    std::queue<std::string> to_process;
    
    to_process.push(class_id);
    visited.insert(class_id);
    
    while (!to_process.empty()) {
        std::string current = to_process.front();
        to_process.pop();
        required_symbols.push_back(current);
        
        auto node = get_node(current);
        if (node) {
            // 直接依存しているSymbolを追加
            for (const auto& dep_id : node->depends_on) {
                if (visited.find(dep_id) == visited.end()) {
                    auto dep_node = get_node(dep_id);
                    if (dep_node && dep_node->file_path == node->file_path) {
                        // 同じファイル内のSymbolは一緒に移動する必要がある
                        to_process.push(dep_id);
                        visited.insert(dep_id);
                    }
                }
            }
        }
    }
    
    return required_symbols;
}

std::vector<std::string> DependencyGraph::topological_sort() const {
    std::vector<std::string> result;
    std::unordered_map<std::string, int> in_degree;
    
    // 入次数を計算
    for (const auto& [id, node] : nodes_) {
        in_degree[id] = 0;
    }
    for (const auto& [id, node] : nodes_) {
        for (const auto& dep : node->depends_on) {
            in_degree[dep]++;
        }
    }
    
    // 入次数0のノードをキューに追加
    std::queue<std::string> q;
    for (const auto& [id, degree] : in_degree) {
        if (degree == 0) {
            q.push(id);
        }
    }
    
    // トポロジカルソート
    while (!q.empty()) {
        std::string current = q.front();
        q.pop();
        result.push_back(current);
        
        auto node = get_node(current);
        if (node) {
            for (const auto& dep : node->depends_on) {
                in_degree[dep]--;
                if (in_degree[dep] == 0) {
                    q.push(dep);
                }
            }
        }
    }
    
    return result;
}

nlohmann::json DependencyGraph::to_json() const {
    nlohmann::json result;
    result["nodes"] = nlohmann::json::array();
    result["edges"] = nlohmann::json::array();
    
    for (const auto& [id, node] : nodes_) {
        nlohmann::json node_json;
        node_json["id"] = node->symbol_id;
        node_json["name"] = node->symbol_name;
        node_json["file"] = node->file_path;
        node_json["type"] = static_cast<int>(node->symbol_type);
        node_json["depends_on"] = node->depends_on;
        node_json["depended_by"] = node->depended_by;
        node_json["is_movable"] = node->is_movable;
        node_json["has_circular_dependency"] = has_circular_dependency(id);
        result["nodes"].push_back(node_json);
        
        for (const auto& [to_id, edge] : node->edges) {
            nlohmann::json edge_json;
            edge_json["from"] = edge.from_symbol_id;
            edge_json["to"] = edge.to_symbol_id;
            edge_json["type"] = static_cast<int>(edge.type);
            edge_json["line"] = edge.line_number;
            edge_json["context"] = edge.context;
            result["edges"].push_back(edge_json);
        }
    }
    
    result["circular_dependencies"] = std::vector<std::string>(
        cyclic_dependencies_.begin(), cyclic_dependencies_.end()
    );
    
    return result;
}

std::string DependencyGraph::to_dot() const {
    std::stringstream ss;
    ss << "digraph DependencyGraph {\n";
    ss << "  rankdir=LR;\n";
    ss << "  node [shape=box];\n\n";
    
    // ノード定義
    for (const auto& [id, node] : nodes_) {
        std::string color = has_circular_dependency(id) ? "red" : "black";
        ss << "  \"" << node->symbol_name << "\" [color=" << color << "];\n";
    }
    
    ss << "\n";
    
    // エッジ定義
    for (const auto& [id, node] : nodes_) {
        for (const auto& [to_id, edge] : node->edges) {
            auto to_node = get_node(to_id);
            if (to_node) {
                std::string style = "solid";
                if (edge.type == DependencyType::INHERITANCE) {
                    style = "dashed";
                } else if (edge.type == DependencyType::IMPORT) {
                    style = "dotted";
                }
                ss << "  \"" << node->symbol_name << "\" -> \"" 
                   << to_node->symbol_name << "\" [style=" << style << "];\n";
            }
        }
    }
    
    ss << "}\n";
    return ss.str();
}

//=============================================================================
// ImportAnalyzer実装
//=============================================================================

std::vector<ImportAnalyzer::ImportStatement> ImportAnalyzer::parse_imports(
    const std::string& content, Language language) {
    
    switch (language) {
        case Language::JAVASCRIPT:
        case Language::TYPESCRIPT:
            return parse_js_imports(content);
        case Language::PYTHON:
            return parse_python_imports(content);
        case Language::CPP:
        case Language::C:
            return parse_cpp_includes(content);
        case Language::CSHARP:
            return parse_csharp_usings(content);
        case Language::GO:
            return parse_go_imports(content);
        case Language::RUST:
            return parse_rust_uses(content);
        default:
            return {};
    }
}

std::vector<ImportAnalyzer::ImportStatement> ImportAnalyzer::parse_js_imports(
    const std::string& content) {
    
    std::vector<ImportStatement> imports;
    using namespace DirectEdit;
    
    // PCRE2革命: 行ベースでJavaScript import解析
    std::istringstream stream(content);
    std::string line;
    LineNumber line_num = 0;
    
    while (std::getline(stream, line)) {
        line_num++;
        
        // import文を含む行の処理
        if (line.find("import") != std::string::npos && line.find("from") != std::string::npos) {
            ImportStatement stmt = parse_js_import_line_pcre2(line, line_num);
            if (!stmt.raw_statement.empty()) {
                imports.push_back(stmt);
            }
        }
    }
    
    return imports;
}

std::vector<ImportAnalyzer::ImportStatement> ImportAnalyzer::parse_python_imports(
    const std::string& content) {
    
    std::vector<ImportStatement> imports;
    
    // import文のパターン
    std::regex import_regex(R"(^\s*import\s+(.+?)$)", std::regex::multiline);
    std::regex from_import_regex(R"(^\s*from\s+(.+?)\s+import\s+(.+?)$)", std::regex::multiline);
    
    // 通常のimport文
    auto begin = std::sregex_iterator(content.begin(), content.end(), import_regex);
    auto end = std::sregex_iterator();
    
    for (auto i = begin; i != end; ++i) {
        ImportStatement stmt;
        stmt.raw_statement = i->str();
        stmt.module_or_file = (*i)[1];
        stmt.symbols.push_back((*i)[1]);
        imports.push_back(stmt);
    }
    
    // from ... import文
    begin = std::sregex_iterator(content.begin(), content.end(), from_import_regex);
    for (auto i = begin; i != end; ++i) {
        ImportStatement stmt;
        stmt.raw_statement = i->str();
        stmt.module_or_file = (*i)[1];
        
        std::string import_part = (*i)[2];
        if (import_part == "*") {
            stmt.is_wildcard = true;
        } else {
            // カンマ区切りのシンボルを解析
            std::stringstream ss(import_part);
            std::string symbol;
            while (std::getline(ss, symbol, ',')) {
                // トリム処理
                symbol.erase(0, symbol.find_first_not_of(" \t"));
                symbol.erase(symbol.find_last_not_of(" \t") + 1);
                stmt.symbols.push_back(symbol);
            }
        }
        
        // 相対importチェック
        if (stmt.module_or_file.find('.') == 0) {
            stmt.is_relative = true;
        }
        
        imports.push_back(stmt);
    }
    
    return imports;
}

std::vector<ImportAnalyzer::ImportStatement> ImportAnalyzer::parse_cpp_includes(
    const std::string& content) {
    
    std::vector<ImportStatement> includes;
    std::regex include_regex(R"(^\s*#include\s+[<"](.+?)[>"])", std::regex::multiline);
    
    auto begin = std::sregex_iterator(content.begin(), content.end(), include_regex);
    auto end = std::sregex_iterator();
    
    for (auto i = begin; i != end; ++i) {
        ImportStatement stmt;
        stmt.raw_statement = i->str();
        stmt.module_or_file = (*i)[1];
        
        // C++ではファイル全体がincludeされる
        stmt.is_wildcard = true;
        
        // <>はシステムヘッダー、""はユーザーヘッダー（相対）
        if (i->str().find('"') != std::string::npos) {
            stmt.is_relative = true;
        }
        
        includes.push_back(stmt);
    }
    
    return includes;
}

std::vector<ImportAnalyzer::ImportStatement> ImportAnalyzer::parse_csharp_usings(
    const std::string& content) {
    
    std::vector<ImportStatement> usings;
    std::regex using_regex(R"(^\s*using\s+(.+?);)", std::regex::multiline);
    
    auto begin = std::sregex_iterator(content.begin(), content.end(), using_regex);
    auto end = std::sregex_iterator();
    
    for (auto i = begin; i != end; ++i) {
        ImportStatement stmt;
        stmt.raw_statement = i->str();
        std::string using_part = (*i)[1];
        
        // エイリアスチェック: using Foo = Bar.Baz;
        if (using_part.find('=') != std::string::npos) {
            auto eq_pos = using_part.find('=');
            stmt.symbols.push_back(using_part.substr(0, eq_pos));
            stmt.module_or_file = using_part.substr(eq_pos + 1);
        } else {
            stmt.module_or_file = using_part;
            stmt.is_wildcard = true; // C#のusingは名前空間全体
        }
        
        usings.push_back(stmt);
    }
    
    return usings;
}

std::vector<ImportAnalyzer::ImportStatement> ImportAnalyzer::parse_go_imports(
    const std::string& content) {
    
    std::vector<ImportStatement> imports;
    
    // 単一import: import "fmt"
    std::regex single_import_regex(R"(^\s*import\s+\"(.+?)\")", std::regex::multiline);
    
    // 複数import: import ( ... )
    std::regex multi_import_regex(R"(import\s*\(([\s\S]*?)\))", std::regex::multiline);
    
    // 単一import解析
    auto begin = std::sregex_iterator(content.begin(), content.end(), single_import_regex);
    auto end = std::sregex_iterator();
    
    for (auto i = begin; i != end; ++i) {
        ImportStatement stmt;
        stmt.raw_statement = i->str();
        stmt.module_or_file = (*i)[1];
        stmt.is_wildcard = true; // Goはパッケージ全体をimport
        imports.push_back(stmt);
    }
    
    // 複数import解析
    begin = std::sregex_iterator(content.begin(), content.end(), multi_import_regex);
    for (auto i = begin; i != end; ++i) {
        std::string import_block = (*i)[1];
        std::regex pkg_regex(R"(\"(.+?)\")");
        auto pkg_begin = std::sregex_iterator(import_block.begin(), import_block.end(), pkg_regex);
        auto pkg_end = std::sregex_iterator();
        
        for (auto j = pkg_begin; j != pkg_end; ++j) {
            ImportStatement stmt;
            stmt.module_or_file = (*j)[1];
            stmt.is_wildcard = true;
            imports.push_back(stmt);
        }
    }
    
    return imports;
}

std::vector<ImportAnalyzer::ImportStatement> ImportAnalyzer::parse_rust_uses(
    const std::string& content) {
    
    std::vector<ImportStatement> uses;
    std::regex use_regex(R"(^\s*use\s+(.+?);)", std::regex::multiline);
    
    auto begin = std::sregex_iterator(content.begin(), content.end(), use_regex);
    auto end = std::sregex_iterator();
    
    for (auto i = begin; i != end; ++i) {
        ImportStatement stmt;
        stmt.raw_statement = i->str();
        std::string use_part = (*i)[1];
        
        // パス解析: std::collections::HashMap
        auto last_sep = use_part.rfind("::");
        if (last_sep != std::string::npos) {
            stmt.module_or_file = use_part.substr(0, last_sep);
            std::string symbol_part = use_part.substr(last_sep + 2);
            
            // {HashMap, HashSet}のような複数import
            if (symbol_part.find('{') != std::string::npos) {
                std::regex symbol_regex(R"(\w+)");
                auto sym_begin = std::sregex_iterator(symbol_part.begin(), symbol_part.end(), symbol_regex);
                auto sym_end = std::sregex_iterator();
                for (auto j = sym_begin; j != sym_end; ++j) {
                    stmt.symbols.push_back(j->str());
                }
            }
            // 単一Symbol
            else if (symbol_part != "*") {
                stmt.symbols.push_back(symbol_part);
            }
            // ワイルドカード
            else {
                stmt.is_wildcard = true;
            }
        } else {
            stmt.module_or_file = use_part;
        }
        
        uses.push_back(stmt);
    }
    
    return uses;
}

std::string ImportAnalyzer::update_import_statement(
    const ImportStatement& import_stmt,
    const std::string& old_path,
    const std::string& new_path,
    Language language) {
    
    std::string updated = import_stmt.raw_statement;
    
    // パスの置換
    size_t pos = updated.find(old_path);
    if (pos != std::string::npos) {
        updated.replace(pos, old_path.length(), new_path);
    }
    
    return updated;
}

// 🐍 PCRE2革命のJavaScript import行解析
ImportAnalyzer::ImportStatement ImportAnalyzer::parse_js_import_line_pcre2(
    const std::string& line, LineNumber line_num) {
    
    ImportStatement stmt;
    stmt.raw_statement = line;
    stmt.line_number = line_num;
    
    using namespace DirectEdit;
    
    // PCRE2パターンでimport文解析
    std::string pattern = R"(^\s*import\s+(.+?)\s+from\s+['"](.+?)['"];?\s*$)";
    
    auto result = re_sub(pattern, "$1|$2", line);
    if (result.success && result.new_content != line) {
        // マッチング成功 - パイプで分離された結果を解析
        size_t pipe_pos = result.new_content.find('|');
        if (pipe_pos != std::string::npos) {
            std::string import_part = result.new_content.substr(0, pipe_pos);
            stmt.module_or_file = result.new_content.substr(pipe_pos + 1);
            
            // TypeScript type import検出
            if (import_part.find("type") != std::string::npos) {
                stmt.is_type_import = true;
                // "type"キーワードを除去
                size_t type_pos = import_part.find("type");
                import_part.erase(type_pos, 4);
            }
            
            // 相対パス検出
            stmt.is_relative = (stmt.module_or_file.find("./") == 0 || 
                               stmt.module_or_file.find("../") == 0);
            
            // ワイルドカード検出
            stmt.is_wildcard = (import_part.find("* as") != std::string::npos);
            
            // シンボル名抽出
            if (import_part.find('{') != std::string::npos) {
                // 名前付きimport: { Foo, Bar }
                size_t start = import_part.find('{') + 1;
                size_t end = import_part.find('}');
                if (end != std::string::npos) {
                    std::string symbols_str = import_part.substr(start, end - start);
                    parse_symbol_list(symbols_str, stmt.symbols);
                }
            } else if (!stmt.is_wildcard) {
                // デフォルトimport
                std::string symbol = trim_string(import_part);
                if (!symbol.empty()) {
                    stmt.symbols.push_back(symbol);
                }
            }
        }
    }
    
    return stmt;
}

} // namespace nekocode