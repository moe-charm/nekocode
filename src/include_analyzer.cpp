//=============================================================================
// 🔍 Include Dependency Analyzer実装
//=============================================================================

#include "nekocode/include_analyzer.hpp"
#include "nekocode/utf8_utils.hpp"
#include <fstream>
#include <sstream>
#include <regex>
#include <queue>
#include <stack>
#include <algorithm>
#include <iostream>

namespace nekocode {

//=============================================================================
// 🏗️ IncludeAnalyzer::Impl - 実装詳細
//=============================================================================

class IncludeAnalyzer::Impl {
public:
    Config config_;
    std::map<std::string, IncludeNode> dependency_cache_;
    
    // Include文の正規表現パターン
    std::regex include_regex_{R"(^\s*#\s*include\s*([<"])([^>"]+)[>"])"};
    
    // ファイル拡張子判定
    bool is_header_file(const std::filesystem::path& path) {
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == ".h" || ext == ".hpp" || ext == ".hxx" || ext == ".hh";
    }
    
    // Include文解析
    std::vector<IncludeInfo> parse_includes(const std::string& content, const std::string& file_path) {
        std::vector<IncludeInfo> includes;
        
        std::istringstream stream(content);
        std::string line;
        uint32_t line_number = 1;
        
        while (std::getline(stream, line)) {
            std::smatch match;
            if (std::regex_search(line, match, include_regex_)) {
                IncludeInfo info;
                info.is_system_header = (match[1].str() == "<");
                info.path = match[2].str();
                info.line_number = line_number;
                info.raw_statement = line;
                
                // 無視パターンチェック
                bool should_ignore = false;
                for (const auto& pattern : config_.ignore_patterns) {
                    if (info.path.find(pattern) != std::string::npos) {
                        should_ignore = true;
                        break;
                    }
                }
                
                if (!should_ignore) {
                    includes.push_back(info);
                }
            }
            line_number++;
        }
        
        return includes;
    }
    
    // 依存グラフ構築（再帰的）
    void build_dependency_graph(const std::filesystem::path& file_path, 
                               std::map<std::string, IncludeNode>& graph,
                               std::set<std::string>& visited,
                               uint32_t depth = 0) {
        std::string normalized_path = std::filesystem::canonical(file_path).string();
        
        // 既に訪問済み
        if (visited.find(normalized_path) != visited.end()) {
            return;
        }
        visited.insert(normalized_path);
        
        // ファイル読み込み
        std::ifstream file(file_path);
        if (!file.is_open()) {
            return;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        
        // ノード作成
        IncludeNode& node = graph[normalized_path];
        node.file_path = normalized_path;
        node.is_header = is_header_file(file_path);
        node.include_statements = parse_includes(content, normalized_path);
        
        // 各include文を処理
        for (const auto& inc : node.include_statements) {
            // Include パス解決
            std::filesystem::path resolved_path;
            
            if (inc.is_system_header && !config_.analyze_system_headers) {
                continue;  // システムヘッダーはスキップ
            }
            
            // ローカルincludeの場合
            if (!inc.is_system_header) {
                // 現在のファイルからの相対パス
                resolved_path = file_path.parent_path() / inc.path;
                
                if (!std::filesystem::exists(resolved_path)) {
                    // インクルードパスから検索
                    for (const auto& inc_dir : config_.include_paths) {
                        resolved_path = std::filesystem::path(inc_dir) / inc.path;
                        if (std::filesystem::exists(resolved_path)) {
                            break;
                        }
                    }
                }
            }
            
            if (std::filesystem::exists(resolved_path)) {
                std::string resolved_str = std::filesystem::canonical(resolved_path).string();
                node.direct_includes.insert(resolved_str);
                
                // 再帰的に依存関係を構築
                build_dependency_graph(resolved_path, graph, visited, depth + 1);
                
                // 推移的includeを更新
                if (graph.find(resolved_str) != graph.end()) {
                    const auto& child_node = graph[resolved_str];
                    node.transitive_includes.insert(child_node.direct_includes.begin(),
                                                   child_node.direct_includes.end());
                    node.transitive_includes.insert(child_node.transitive_includes.begin(),
                                                   child_node.transitive_includes.end());
                }
            }
        }
        
        // Include深度更新
        node.include_depth = depth;
        
        // 推移的includeに自分の直接includeも追加
        node.transitive_includes.insert(node.direct_includes.begin(), 
                                       node.direct_includes.end());
    }
    
    // 循環依存検出（Tarjanのアルゴリズム）
    void detect_circular_dependencies(const std::map<std::string, IncludeNode>& graph,
                                     std::vector<CircularDependency>& cycles) {
        std::map<std::string, int> index_map;
        std::map<std::string, int> lowlink_map;
        std::map<std::string, bool> on_stack;
        std::stack<std::string> stack;
        int index_counter = 0;
        
        // 強連結成分を見つける
        std::function<void(const std::string&)> strongconnect;
        strongconnect = [&](const std::string& v) {
            index_map[v] = index_counter;
            lowlink_map[v] = index_counter;
            index_counter++;
            stack.push(v);
            on_stack[v] = true;
            
            if (graph.find(v) != graph.end()) {
                for (const auto& w : graph.at(v).direct_includes) {
                    if (index_map.find(w) == index_map.end()) {
                        strongconnect(w);
                        lowlink_map[v] = std::min(lowlink_map[v], lowlink_map[w]);
                    } else if (on_stack[w]) {
                        lowlink_map[v] = std::min(lowlink_map[v], index_map[w]);
                    }
                }
            }
            
            // 強連結成分のルートか？
            if (lowlink_map[v] == index_map[v]) {
                std::vector<std::string> component;
                std::string w;
                do {
                    w = stack.top();
                    stack.pop();
                    on_stack[w] = false;
                    component.push_back(w);
                } while (w != v);
                
                // 2つ以上の要素があれば循環
                if (component.size() > 1) {
                    CircularDependency cycle;
                    cycle.cycle_path = component;
                    cycle.cycle_path.push_back(component[0]); // 循環を明示
                    cycle.severity = component.size() > 3 ? "critical" : "warning";
                    cycles.push_back(cycle);
                }
            }
        };
        
        // 全ノードに対して実行
        for (const auto& [path, node] : graph) {
            if (index_map.find(path) == index_map.end()) {
                strongconnect(path);
            }
        }
    }
    
    // 不要include検出（シンボル使用ベース）
    void detect_unused_includes(const std::map<std::string, IncludeNode>& graph,
                               std::vector<UnusedInclude>& unused_includes) {
        for (const auto& [file_path, node] : graph) {
            // ヘッダーファイルは解析しない（複雑すぎる）
            if (node.is_header) continue;
            
            // ファイル内容を読み込み
            std::ifstream file(file_path);
            if (!file.is_open()) continue;
            
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            
            // 各includeについて使用チェック
            for (const auto& inc : node.include_statements) {
                if (inc.is_system_header) continue;  // システムヘッダーはスキップ
                
                // includeファイルから提供される可能性のあるシンボルを推測
                std::vector<std::string> potential_symbols = extract_potential_symbols(inc.path);
                
                // 使用されているか確認
                bool is_used = false;
                for (const auto& symbol : potential_symbols) {
                    // 単純な文字列検索（改善の余地あり）
                    if (content.find(symbol) != std::string::npos) {
                        // 単語境界チェック（簡易版）
                        std::regex symbol_regex("\\b" + symbol + "\\b");
                        if (std::regex_search(content, symbol_regex)) {
                            is_used = true;
                            break;
                        }
                    }
                }
                
                if (!is_used) {
                    UnusedInclude unused;
                    unused.file_path = file_path;
                    unused.included_file = inc.path;
                    unused.line_number = inc.line_number;
                    unused.reason = "No symbols from this include are used";
                    unused_includes.push_back(unused);
                }
            }
        }
    }
    
    // includeファイルから提供される可能性のあるシンボルを推測
    std::vector<std::string> extract_potential_symbols(const std::string& include_path) {
        std::vector<std::string> symbols;
        
        // ファイル名から推測
        std::filesystem::path path(include_path);
        std::string filename = path.stem().string();
        
        // ファイル名をシンボル候補に追加
        symbols.push_back(filename);
        
        // CamelCaseやsnake_caseを考慮
        // 例: my_class.hpp -> MyClass, my_class
        std::string camel_case;
        bool next_upper = true;
        for (char c : filename) {
            if (c == '_' || c == '-') {
                next_upper = true;
            } else if (next_upper) {
                camel_case += std::toupper(c);
                next_upper = false;
            } else {
                camel_case += c;
            }
        }
        if (!camel_case.empty() && camel_case != filename) {
            symbols.push_back(camel_case);
        }
        
        // 一般的なパターン
        symbols.push_back(filename + "_t");      // 型定義
        symbols.push_back(filename + "_ptr");    // ポインタ型
        symbols.push_back("I" + camel_case);     // インターフェース
        
        return symbols;
    }
    
    // ホットスポット検出
    void detect_hotspots(std::map<std::string, IncludeNode>& graph,
                        std::vector<IncludeAnalysisResult::HotspotHeader>& hotspots) {
        // 被include数を計算
        for (auto& [path, node] : graph) {
            for (const auto& included : node.direct_includes) {
                if (graph.find(included) != graph.end()) {
                    graph[included].included_by_count++;
                }
            }
        }
        
        // ホットスポット抽出
        for (const auto& [path, node] : graph) {
            if (node.included_by_count > 5) {  // 閾値
                IncludeAnalysisResult::HotspotHeader hotspot;
                hotspot.file_path = path;
                hotspot.included_by_count = node.included_by_count;
                
                // 影響度スコア計算（被include数 × 推移的include数）
                hotspot.impact_score = node.included_by_count * 
                    static_cast<uint32_t>(node.transitive_includes.size());
                
                hotspots.push_back(hotspot);
            }
        }
        
        // 影響度でソート
        std::sort(hotspots.begin(), hotspots.end(),
            [](const auto& a, const auto& b) {
                return a.impact_score > b.impact_score;
            });
    }
};

//=============================================================================
// 🔍 IncludeAnalyzer実装
//=============================================================================

IncludeAnalyzer::IncludeAnalyzer() 
    : impl_(std::make_unique<Impl>()) {
}

IncludeAnalyzer::~IncludeAnalyzer() = default;

void IncludeAnalyzer::set_config(const Config& config) {
    impl_->config_ = config;
}

IncludeAnalysisResult IncludeAnalyzer::analyze_file(const std::filesystem::path& file_path) {
    IncludeAnalysisResult result;
    
    if (!std::filesystem::exists(file_path)) {
        return result;
    }
    
    // 依存グラフ構築
    std::set<std::string> visited;
    impl_->build_dependency_graph(file_path, result.dependency_graph, visited);
    
    // 統計計算
    result.total_files = static_cast<uint32_t>(result.dependency_graph.size());
    
    uint32_t total_depth = 0;
    std::set<std::string> unique_includes_set;
    
    for (const auto& [path, node] : result.dependency_graph) {
        result.total_includes += static_cast<uint32_t>(node.direct_includes.size());
        unique_includes_set.insert(node.direct_includes.begin(), node.direct_includes.end());
        total_depth += node.include_depth;
    }
    
    result.unique_includes = static_cast<uint32_t>(unique_includes_set.size());
    result.average_include_depth = result.total_files > 0 ? 
        static_cast<float>(total_depth) / result.total_files : 0.0f;
    
    // 問題検出
    if (impl_->config_.detect_circular) {
        impl_->detect_circular_dependencies(result.dependency_graph, 
                                           result.circular_dependencies);
    }
    
    // 不要include検出
    if (impl_->config_.detect_unused) {
        impl_->detect_unused_includes(result.dependency_graph, result.unused_includes);
    }
    
    // ホットスポット検出
    impl_->detect_hotspots(result.dependency_graph, result.hotspot_headers);
    
    return result;
}

IncludeAnalysisResult IncludeAnalyzer::analyze_directory(const std::filesystem::path& dir_path) {
    IncludeAnalysisResult result;
    
    if (!std::filesystem::exists(dir_path) || !std::filesystem::is_directory(dir_path)) {
        return result;
    }
    
    std::set<std::string> visited;
    
    // 全ての C++ ファイルを処理
    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir_path)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            // C++ ソース/ヘッダーファイル
            if (ext == ".cpp" || ext == ".cc" || ext == ".cxx" || ext == ".c" ||
                ext == ".hpp" || ext == ".h" || ext == ".hxx" || ext == ".hh") {
                impl_->build_dependency_graph(entry.path(), result.dependency_graph, visited);
            }
        }
    }
    
    // 統計と問題検出（analyze_fileと同様）
    result.total_files = static_cast<uint32_t>(result.dependency_graph.size());
    
    uint32_t total_depth = 0;
    std::set<std::string> unique_includes_set;
    
    for (const auto& [path, node] : result.dependency_graph) {
        result.total_includes += static_cast<uint32_t>(node.direct_includes.size());
        unique_includes_set.insert(node.direct_includes.begin(), node.direct_includes.end());
        total_depth += node.include_depth;
    }
    
    result.unique_includes = static_cast<uint32_t>(unique_includes_set.size());
    result.average_include_depth = result.total_files > 0 ? 
        static_cast<float>(total_depth) / result.total_files : 0.0f;
    
    if (impl_->config_.detect_circular) {
        impl_->detect_circular_dependencies(result.dependency_graph, 
                                           result.circular_dependencies);
    }
    
    if (impl_->config_.detect_unused) {
        impl_->detect_unused_includes(result.dependency_graph, result.unused_includes);
    }
    
    impl_->detect_hotspots(result.dependency_graph, result.hotspot_headers);
    
    return result;
}

IncludeAnalyzer::ImpactAnalysis IncludeAnalyzer::analyze_impact(const std::filesystem::path& file_path) {
    ImpactAnalysis impact;
    impact.target_file = std::filesystem::canonical(file_path).string();
    
    // キャッシュされた依存グラフから影響範囲を計算
    for (const auto& [path, node] : impl_->dependency_cache_) {
        if (node.direct_includes.find(impact.target_file) != node.direct_includes.end()) {
            impact.directly_affected.insert(path);
        }
        if (node.transitive_includes.find(impact.target_file) != node.transitive_includes.end()) {
            impact.transitively_affected.insert(path);
        }
    }
    
    impact.total_affected_files = static_cast<uint32_t>(impact.transitively_affected.size());
    
    // .cpp ファイルのみカウント（再コンパイル単位）
    impact.recompilation_units = std::count_if(
        impact.transitively_affected.begin(),
        impact.transitively_affected.end(),
        [](const std::string& path) {
            size_t len = path.length();
            return (len > 4 && path.substr(len - 4) == ".cpp") ||
                   (len > 3 && path.substr(len - 3) == ".cc") ||
                   (len > 4 && path.substr(len - 4) == ".cxx");
        });
    
    return impact;
}

//=============================================================================
// 📊 JSON出力用API
//=============================================================================

nlohmann::json IncludeAnalyzer::get_include_graph(const IncludeAnalysisResult& result) {
    nlohmann::json graph_json;
    
    graph_json["statistics"] = {
        {"total_files", result.total_files},
        {"total_includes", result.total_includes},
        {"unique_includes", result.unique_includes},
        {"average_include_depth", result.average_include_depth}
    };
    
    nlohmann::json nodes = nlohmann::json::array();
    for (const auto& [path, node] : result.dependency_graph) {
        nodes.push_back({
            {"file", path},
            {"direct_includes", node.direct_includes.size()},
            {"transitive_includes", node.transitive_includes.size()},
            {"include_depth", node.include_depth},
            {"included_by", node.included_by_count},
            {"is_header", node.is_header}
        });
    }
    graph_json["nodes"] = nodes;
    
    return graph_json;
}

nlohmann::json IncludeAnalyzer::get_circular_dependencies(const IncludeAnalysisResult& result) {
    nlohmann::json cycles_json = nlohmann::json::array();
    
    for (const auto& cycle : result.circular_dependencies) {
        cycles_json.push_back({
            {"cycle", cycle.cycle_path},
            {"severity", cycle.severity},
            {"length", cycle.cycle_path.size() - 1}  // 最後は最初の要素の繰り返し
        });
    }
    
    return {
        {"circular_dependencies", cycles_json},
        {"total_cycles", result.circular_dependencies.size()}
    };
}

nlohmann::json IncludeAnalyzer::get_unused_includes(const IncludeAnalysisResult& result) {
    nlohmann::json unused_json = nlohmann::json::array();
    
    for (const auto& unused : result.unused_includes) {
        unused_json.push_back({
            {"file", unused.file_path},
            {"unused_include", unused.included_file},
            {"line", unused.line_number},
            {"reason", unused.reason}
        });
    }
    
    return {
        {"unused_includes", unused_json},
        {"total_unused", result.unused_includes.size()}
    };
}

nlohmann::json IncludeAnalyzer::get_optimization_suggestions(const IncludeAnalysisResult& result) {
    nlohmann::json opt_json;
    
    opt_json["hotspot_headers"] = nlohmann::json::array();
    for (const auto& hotspot : result.hotspot_headers) {
        opt_json["hotspot_headers"].push_back({
            {"file", hotspot.file_path},
            {"included_by_count", hotspot.included_by_count},
            {"impact_score", hotspot.impact_score}
        });
    }
    
    opt_json["optimization_potential"] = {
        {"removable_includes", result.optimization_potential.removable_includes},
        {"forward_declaration_candidates", result.optimization_potential.forward_declaration_candidates},
        {"estimated_compile_time_reduction", 
         std::to_string(result.optimization_potential.estimated_compile_time_reduction) + "%"}
    };
    
    return opt_json;
}

//=============================================================================
// 🛠️ ユーティリティ関数
//=============================================================================

IncludeInfo parse_include_statement(const std::string& line, uint32_t line_number) {
    IncludeInfo info;
    info.line_number = line_number;
    info.raw_statement = line;
    
    std::regex include_regex(R"(^\s*#\s*include\s*([<"])([^>"]+)[>"])");
    std::smatch match;
    
    if (std::regex_search(line, match, include_regex)) {
        info.is_system_header = (match[1].str() == "<");
        info.path = match[2].str();
    }
    
    return info;
}

std::string normalize_include_path(const std::string& base_path,
                                   const std::string& include_path,
                                   const std::vector<std::string>& include_dirs) {
    // 相対パスを解決
    std::filesystem::path resolved = std::filesystem::path(base_path).parent_path() / include_path;
    
    if (std::filesystem::exists(resolved)) {
        return std::filesystem::canonical(resolved).string();
    }
    
    // インクルードディレクトリから検索
    for (const auto& dir : include_dirs) {
        resolved = std::filesystem::path(dir) / include_path;
        if (std::filesystem::exists(resolved)) {
            return std::filesystem::canonical(resolved).string();
        }
    }
    
    return include_path;  // 解決できない場合は元のパスを返す
}

} // namespace nekocode