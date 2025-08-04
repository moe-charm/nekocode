#pragma once

//=============================================================================
// 🔍 Include Dependency Analyzer - C++ Include地獄解決エンジン
//
// 複雑なinclude依存関係を解析・可視化・最適化
//
// 特徴:
// - Include依存グラフ構築
// - 循環依存検出
// - 不要include検出
// - 最適化提案
//=============================================================================

#include "types.hpp"
#include <filesystem>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>

namespace nekocode {

//=============================================================================
// 📊 Include情報構造体
//=============================================================================

struct IncludeInfo {
    std::string path;                    // includeファイルパス
    bool is_system_header = false;       // <> でinclude
    uint32_t line_number = 0;            // include行番号
    std::string raw_statement;           // 元のinclude文
};

struct IncludeNode {
    std::string file_path;                          // ファイルパス
    std::set<std::string> direct_includes;          // 直接include
    std::set<std::string> transitive_includes;     // 推移的include（全て）
    std::vector<IncludeInfo> include_statements;    // include文詳細
    uint32_t include_depth = 0;                     // 最大include深度
    bool is_header = false;                         // ヘッダーファイルか
    bool is_system = false;                         // システムヘッダーか
    uint32_t included_by_count = 0;                 // 被include数
};

struct CircularDependency {
    std::vector<std::string> cycle_path;            // 循環パス
    std::string severity = "warning";               // critical/warning
};

struct UnusedInclude {
    std::string file_path;                          // ファイル
    std::string included_file;                      // 不要なinclude
    std::string reason;                             // 理由
    uint32_t line_number = 0;                       // 行番号
};

struct IncludeOptimization {
    enum Type {
        FORWARD_DECLARATION,     // 前方宣言で十分
        MOVE_TO_IMPLEMENTATION,  // .cppに移動可能
        PIMPL_CANDIDATE,        // PIMPL適用候補
        REMOVE_UNUSED,          // 削除可能
        COMBINE_INCLUDES        // 統合可能
    };
    
    Type type;
    std::string target_file;
    std::string target_include;
    std::string suggestion;
    uint32_t estimated_impact;  // 推定改善度(0-100)
};

//=============================================================================
// 📈 Include解析結果
//=============================================================================

struct IncludeAnalysisResult {
    // 基本統計
    uint32_t total_files = 0;
    uint32_t total_includes = 0;
    uint32_t unique_includes = 0;
    float average_include_depth = 0.0f;
    
    // 依存グラフ
    std::map<std::string, IncludeNode> dependency_graph;
    
    // 問題検出
    std::vector<CircularDependency> circular_dependencies;
    std::vector<UnusedInclude> unused_includes;
    
    // ホットスポット（多くincludeされるファイル）
    struct HotspotHeader {
        std::string file_path;
        uint32_t included_by_count = 0;
        uint32_t impact_score = 0;  // 変更時の影響度
    };
    std::vector<HotspotHeader> hotspot_headers;
    
    // 最適化提案
    std::vector<IncludeOptimization> optimizations;
    
    // 推定改善
    struct OptimizationPotential {
        uint32_t removable_includes = 0;
        uint32_t forward_declaration_candidates = 0;
        float estimated_compile_time_reduction = 0.0f;  // パーセント
    } optimization_potential;
};

//=============================================================================
// 🔍 Include Analyzer クラス
//=============================================================================

class IncludeAnalyzer {
public:
    IncludeAnalyzer();
    ~IncludeAnalyzer();
    
    // 解析設定
    struct Config {
        bool analyze_system_headers = false;     // システムヘッダーも解析
        bool detect_circular = true;             // 循環依存検出
        bool detect_unused = true;               // 不要include検出
        bool suggest_optimizations = true;       // 最適化提案
        std::vector<std::string> include_paths;  // インクルードパス
        std::set<std::string> ignore_patterns;   // 無視パターン
    };
    
    void set_config(const Config& config);
    
    // 単一ファイル解析
    IncludeAnalysisResult analyze_file(const std::filesystem::path& file_path);
    
    // ディレクトリ全体解析
    IncludeAnalysisResult analyze_directory(const std::filesystem::path& dir_path);
    
    // 特定ファイルの影響範囲解析
    struct ImpactAnalysis {
        std::string target_file;
        std::set<std::string> directly_affected;     // 直接影響
        std::set<std::string> transitively_affected; // 推移的影響
        uint32_t total_affected_files = 0;
        uint32_t recompilation_units = 0;
    };
    ImpactAnalysis analyze_impact(const std::filesystem::path& file_path);
    
    // セッションコマンド用API
    nlohmann::json get_include_graph(const IncludeAnalysisResult& result);
    nlohmann::json get_circular_dependencies(const IncludeAnalysisResult& result);
    nlohmann::json get_unused_includes(const IncludeAnalysisResult& result);
    nlohmann::json get_optimization_suggestions(const IncludeAnalysisResult& result);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

//=============================================================================
// 🛠️ ユーティリティ関数
//=============================================================================

// Include文のパース
IncludeInfo parse_include_statement(const std::string& line, uint32_t line_number);

// ファイルパスの正規化
std::string normalize_include_path(const std::string& base_path, 
                                   const std::string& include_path,
                                   const std::vector<std::string>& include_dirs);

} // namespace nekocode