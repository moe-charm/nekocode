#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>
#include "nekocode/types.hpp"
#include "nekocode/symbol_table.hpp"
#include "nekocode/universal_symbol.hpp"

namespace nekocode {

//=============================================================================
// 🔗 Dependency Graph - Symbol依存関係解析
//=============================================================================

/// 依存関係の種類
enum class DependencyType {
    IMPORT,        // import/include文による依存
    INHERITANCE,   // 継承関係
    COMPOSITION,   // コンポジション（メンバー変数）
    PARAMETER,     // パラメータ型として使用
    RETURN_TYPE,   // 戻り値型として使用
    REFERENCE,     // その他の参照
    UNKNOWN
};

/// 依存関係エッジ
struct DependencyEdge {
    std::string from_symbol_id;     // 依存元シンボルID
    std::string to_symbol_id;       // 依存先シンボルID
    DependencyType type;            // 依存の種類
    LineNumber line_number = 0;     // 依存が発生している行番号
    std::string context;             // 依存のコンテキスト（import文など）
    bool is_direct = true;          // 直接依存かどうか
};

/// Symbolノード（依存グラフ用）
struct DependencyNode {
    std::string symbol_id;                              // Symbol ID
    std::string symbol_name;                            // Symbol名
    std::string file_path;                              // 定義ファイル
    SymbolType symbol_type;                             // Symbolタイプ
    std::vector<std::string> depends_on;                // このSymbolが依存するSymbol ID一覧
    std::vector<std::string> depended_by;               // このSymbolに依存されているSymbol ID一覧
    std::unordered_map<std::string, DependencyEdge> edges;  // エッジ詳細情報
    
    // 移動可能性チェック用
    bool is_movable = true;                             // 移動可能かどうか
    std::vector<std::string> move_blockers;             // 移動を妨げる要因
};

/// 依存グラフ
class DependencyGraph {
private:
    std::unordered_map<std::string, std::unique_ptr<DependencyNode>> nodes_;
    std::unordered_map<std::string, std::vector<std::string>> file_to_symbols_;  // ファイル→Symbol IDマッピング
    std::unordered_set<std::string> cyclic_dependencies_;  // 循環依存しているSymbol ID群
    
public:
    DependencyGraph() = default;
    
    /// ノード追加
    void add_node(const std::string& symbol_id, 
                  const std::string& symbol_name,
                  const std::string& file_path,
                  SymbolType type);
    
    /// エッジ追加（依存関係）
    void add_edge(const std::string& from_id,
                  const std::string& to_id,
                  DependencyType type,
                  LineNumber line = 0,
                  const std::string& context = "");
    
    /// Symbol Tableから依存グラフを構築
    void build_from_symbol_table(const SymbolTable& symbol_table);
    
    /// import/include文を解析して依存関係を追加
    void analyze_imports(const std::string& file_path,
                        const std::vector<std::string>& import_statements);
    
    /// 特定Symbolの依存関係を取得
    std::vector<std::string> get_dependencies(const std::string& symbol_id) const;
    
    /// 特定Symbolに依存しているSymbolを取得
    std::vector<std::string> get_dependents(const std::string& symbol_id) const;
    
    /// 循環依存をチェック
    bool has_circular_dependency(const std::string& symbol_id) const;
    
    /// 循環依存を検出して記録
    void detect_circular_dependencies();
    
    /// Symbol移動時の影響範囲を計算
    struct MoveImpact {
        std::vector<std::string> affected_files;      // 影響を受けるファイル
        std::vector<std::string> affected_symbols;    // 影響を受けるSymbol
        std::vector<std::string> required_imports;    // 必要なimport文の更新
        bool has_circular_dependency = false;         // 循環依存の有無
        bool is_safe_to_move = true;                  // 安全に移動可能か
        std::vector<std::string> warnings;            // 警告メッセージ
    };
    
    /// Symbol移動の影響分析
    MoveImpact analyze_move_impact(const std::string& symbol_id,
                                   const std::string& target_file) const;
    
    /// クラス移動に必要なSymbol群を特定
    std::vector<std::string> get_required_symbols_for_move(const std::string& class_id) const;
    
    /// JSON出力
    nlohmann::json to_json() const;
    
    /// デバッグ用：グラフの可視化データ生成（DOT形式）
    std::string to_dot() const;
    
    /// ノード取得
    const DependencyNode* get_node(const std::string& symbol_id) const {
        auto it = nodes_.find(symbol_id);
        return it != nodes_.end() ? it->second.get() : nullptr;
    }
    
    /// ファイル内のSymbol一覧取得
    std::vector<std::string> get_symbols_in_file(const std::string& file_path) const {
        auto it = file_to_symbols_.find(file_path);
        return it != file_to_symbols_.end() ? it->second : std::vector<std::string>{};
    }
    
private:
    /// DFSで循環依存を検出
    bool dfs_detect_cycle(const std::string& node_id,
                         std::unordered_set<std::string>& visited,
                         std::unordered_set<std::string>& rec_stack) const;
    
    /// トポロジカルソート（依存順序の決定）
    std::vector<std::string> topological_sort() const;
};

//=============================================================================
// 🔍 Import/Include解析ヘルパー
//=============================================================================

class ImportAnalyzer {
public:
    struct ImportStatement {
        std::string raw_statement;           // 元のimport/include文
        std::string module_or_file;          // モジュール名/ファイル名
        std::vector<std::string> symbols;    // importされているSymbol名
        bool is_wildcard = false;            // * importかどうか
        bool is_relative = false;            // 相対importかどうか
        bool is_type_import = false;         // TypeScript type importかどうか
        LineNumber line_number = 0;          // 行番号
    };
    
    /// 言語別のimport文解析
    static std::vector<ImportStatement> parse_imports(const std::string& content,
                                                      Language language);
    
    /// JavaScript/TypeScript import文解析
    static std::vector<ImportStatement> parse_js_imports(const std::string& content);
    
    /// Python import文解析
    static std::vector<ImportStatement> parse_python_imports(const std::string& content);
    
    /// C/C++ include文解析
    static std::vector<ImportStatement> parse_cpp_includes(const std::string& content);
    
    /// C# using文解析
    static std::vector<ImportStatement> parse_csharp_usings(const std::string& content);
    
    /// Go import文解析
    static std::vector<ImportStatement> parse_go_imports(const std::string& content);
    
    /// Rust use文解析
    static std::vector<ImportStatement> parse_rust_uses(const std::string& content);
    
    /// import文の更新（クラス移動時）
    static std::string update_import_statement(const ImportStatement& import_stmt,
                                              const std::string& old_path,
                                              const std::string& new_path,
                                              Language language);

private:
    // 🐍 PCRE2革命的JavaScript Import解析用ヘルパー関数
    static ImportStatement parse_js_import_line_smart(const std::string& line, LineNumber line_num);
    static ImportStatement parse_js_import_line_pcre2(const std::string& line, LineNumber line_num);
    static void parse_symbol_list(const std::string& symbols_str, std::vector<std::string>& symbols);
    static std::string trim_string(const std::string& str);
};

} // namespace nekocode