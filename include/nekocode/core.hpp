#pragma once

#include "types.hpp"
#include "language_detection.hpp"
#include "utf8_utils.hpp"
#include "cpp_analyzer.hpp"
#include <memory>
#include <regex>
#include <string_view>
#include <thread>
#include <future>
#include <optional>

namespace nekocode {

//=============================================================================
// 🧠 NekoCodeCore - Python版と互換の高速解析エンジン
//
// Python版の特徴を継承:
// - JavaScript/TypeScript解析
// - 複雑度計算
// - 依存関係解析
// - 関数呼び出し解析
//
// C++版の利点:
// - 10-100倍高速処理
// - 低メモリ消費
// - 並列処理対応
// - 型安全性
//=============================================================================

class NekoCodeCore {
public:
    //=========================================================================
    // 🏗️ Construction & Configuration
    //=========================================================================
    
    explicit NekoCodeCore(const AnalysisConfig& config = AnalysisConfig{});
    ~NekoCodeCore();
    
    // コピー・ムーブ（リソース管理明確化）
    NekoCodeCore(const NekoCodeCore&) = delete;
    NekoCodeCore& operator=(const NekoCodeCore&) = delete;
    NekoCodeCore(NekoCodeCore&&) noexcept;
    NekoCodeCore& operator=(NekoCodeCore&&) noexcept;
    
    //=========================================================================
    // 📄 Single File Analysis - Python版analyze_file相当
    //=========================================================================
    
    /// 単一ファイル解析（マルチ言語対応）
    Result<MultiLanguageAnalysisResult> analyze_file_multilang(const FilePath& file_path);
    
    /// 言語指定ファイル解析
    Result<MultiLanguageAnalysisResult> analyze_file_with_language(const FilePath& file_path, Language language);
    
    /// 文字列コンテンツ直接解析（マルチ言語）
    Result<MultiLanguageAnalysisResult> analyze_content_multilang(const std::string& content, 
                                                                  const std::string& filename = "", 
                                                                  Language language = Language::UNKNOWN);
    
    /// 従来JavaScript専用メソッド（後方互換性）
    Result<AnalysisResult> analyze_file(const FilePath& file_path);
    Result<AnalysisResult> analyze_content(const std::string& content, const std::string& filename = "");
    
    /// ファイル情報のみ取得（高速）
    Result<FileInfo> get_file_info(const FilePath& file_path);
    
    //=========================================================================
    // 📁 Directory Analysis - Python版analyze_directory相当
    //=========================================================================
    
    /// ディレクトリ解析
    Result<DirectoryAnalysis> analyze_directory(const FilePath& directory_path);
    
    /// 並列ディレクトリ解析（C++版独自）
    Result<DirectoryAnalysis> analyze_directory_parallel(const FilePath& directory_path);
    
    /// ファイルリスト解析
    Result<DirectoryAnalysis> analyze_files(const std::vector<FilePath>& file_paths);
    
    //=========================================================================
    // 🔍 File Discovery - ファイル検索・フィルタリング
    //=========================================================================
    
    /// 対応言語ファイル検索（全言語）
    std::vector<FilePath> scan_supported_files(const FilePath& directory_path);
    
    /// 特定言語ファイル検索
    std::vector<FilePath> scan_files_for_language(const FilePath& directory_path, Language language);
    
    /// 言語別ファイル分類
    std::unordered_map<Language, std::vector<FilePath>> classify_files_by_language(const std::vector<FilePath>& files);
    
    /// JavaScriptファイル検索（後方互換性）
    std::vector<FilePath> scan_javascript_files(const FilePath& directory_path);
    
    /// ファイルフィルタリング
    std::vector<FilePath> filter_files(const std::vector<FilePath>& files);
    
    /// 除外パターンチェック
    bool should_exclude_file(const FilePath& file_path) const;
    
    //=========================================================================
    // 📊 Analysis Components - 個別解析機能
    //=========================================================================
    
    /// ファイル基本情報解析
    FileInfo analyze_file_structure(const std::string& content, const FilePath& file_path);
    
    /// クラス解析
    std::vector<ClassInfo> analyze_classes(const std::string& content);
    
    /// 関数解析
    std::vector<FunctionInfo> analyze_functions(const std::string& content);
    
    /// import/export解析
    std::pair<std::vector<ImportInfo>, std::vector<ExportInfo>> analyze_dependencies(const std::string& content);
    
    /// 関数呼び出し解析
    std::pair<std::vector<FunctionCall>, FunctionCallFrequency> analyze_function_calls(const std::string& content);
    
    /// 複雑度解析
    ComplexityInfo analyze_complexity(const std::string& content);
    
    //=========================================================================
    // ⚙️ Configuration & Settings
    //=========================================================================
    
    /// 設定更新
    void set_config(const AnalysisConfig& config);
    
    /// 現在設定取得
    const AnalysisConfig& get_config() const;
    
    /// 並列処理有効/無効
    void enable_parallel_processing(bool enabled);
    
    /// スレッド数設定
    void set_thread_count(std::uint32_t count);
    
    //=========================================================================
    // 📈 Performance & Monitoring
    //=========================================================================
    
    /// パフォーマンス統計取得
    const PerformanceMetrics& get_performance_metrics() const;
    
    /// 統計リセット
    void reset_performance_metrics();
    
    /// プログレスコールバック設定
    using ProgressCallback = std::function<void(std::uint32_t processed, std::uint32_t total, const std::string& current_file)>;
    void set_progress_callback(ProgressCallback callback);
    
    //=========================================================================
    // 🌍 Multi-Language Support
    //=========================================================================
    
    /// 言語検出器取得
    const LanguageDetector& get_language_detector() const;
    
    /// C++解析エンジン取得
    const CppAnalyzer& get_cpp_analyzer() const;
    
    /// サポート言語一覧
    std::vector<Language> get_supported_languages() const;

private:
    //=========================================================================
    // 🔒 Private Implementation
    //=========================================================================
    
    class Impl; // PIMPL idiom for implementation hiding
    std::unique_ptr<Impl> impl_;
};

//=============================================================================
// 🎯 JavaScript Analyzer - 専用JavaScript解析エンジン
//=============================================================================

class JavaScriptAnalyzer {
public:
    JavaScriptAnalyzer();
    ~JavaScriptAnalyzer();
    
    //=========================================================================
    // 🏗️ Class Detection - クラス検出
    //=========================================================================
    
    /// ES6クラス検出
    std::vector<ClassInfo> find_es6_classes(const std::string& content);
    
    /// プロトタイプクラス検出
    std::vector<ClassInfo> find_prototype_classes(const std::string& content);
    
    /// クラスメソッド抽出
    std::vector<FunctionInfo> extract_class_methods(const std::string& content, const ClassInfo& class_info);
    
    //=========================================================================
    // ⚙️ Function Detection - 関数検出
    //=========================================================================
    
    /// 通常関数検出
    std::vector<FunctionInfo> find_regular_functions(const std::string& content);
    
    /// アロー関数検出
    std::vector<FunctionInfo> find_arrow_functions(const std::string& content);
    
    /// async関数検出
    std::vector<FunctionInfo> find_async_functions(const std::string& content);
    
    /// 関数パラメータ解析
    std::vector<std::string> parse_function_parameters(const std::string& params_str);
    
    //=========================================================================
    // 📦 Import/Export Analysis - 依存関係解析
    //=========================================================================
    
    /// ES6 import解析
    std::vector<ImportInfo> parse_es6_imports(const std::string& content);
    
    /// CommonJS require解析  
    std::vector<ImportInfo> parse_commonjs_requires(const std::string& content);
    
    /// ES6 export解析
    std::vector<ExportInfo> parse_es6_exports(const std::string& content);
    
    /// CommonJS exports解析
    std::vector<ExportInfo> parse_commonjs_exports(const std::string& content);
    
    //=========================================================================
    // 📞 Function Call Analysis - 関数呼び出し解析
    //=========================================================================
    
    /// 関数呼び出し検出
    std::vector<FunctionCall> find_function_calls(const std::string& content);
    
    /// メソッド呼び出し検出
    std::vector<FunctionCall> find_method_calls(const std::string& content);
    
    /// 頻度計算
    FunctionCallFrequency calculate_call_frequency(const std::vector<FunctionCall>& calls);
    
    /// 標準関数フィルタリング
    std::vector<FunctionCall> filter_standard_functions(const std::vector<FunctionCall>& calls);

private:
    //=========================================================================
    // 🎯 Regular Expressions - 高速パターンマッチング
    //=========================================================================
    
    // クラス検出用
    std::regex class_regex_;
    std::regex prototype_regex_;
    
    // 関数検出用
    std::regex function_regex_;
    std::regex arrow_function_regex_;
    std::regex async_function_regex_;
    std::regex method_regex_;
    
    // import/export検出用
    std::regex es6_import_regex_;
    std::regex commonjs_require_regex_;
    std::regex es6_export_regex_;
    std::regex commonjs_export_regex_;
    
    // 関数呼び出し検出用
    std::regex function_call_regex_;
    std::regex method_call_regex_;
    
    // 除外パターン
    std::unordered_set<std::string> standard_functions_;
    std::unordered_set<std::string> excluded_objects_;
    
    /// 正規表現初期化
    void initialize_patterns();
    
    /// 標準関数リスト初期化
    void initialize_standard_functions();
};

//=============================================================================
// 🧮 Complexity Calculator - 複雑度計算エンジン
//=============================================================================

class ComplexityCalculator {
public:
    //=========================================================================
    // 📈 Cyclomatic Complexity - サイクロマチック複雑度
    //=========================================================================
    
    /// サイクロマチック複雑度計算
    static std::uint32_t calculate_cyclomatic_complexity(const std::string& content);
    
    /// 認知複雑度計算（C++版独自）
    static std::uint32_t calculate_cognitive_complexity(const std::string& content);
    
    /// 最大ネスト深度計算
    static std::uint32_t calculate_max_nesting_depth(const std::string& content);
    
    /// 関数別複雑度計算
    static std::vector<std::pair<std::string, ComplexityInfo>> calculate_function_complexities(
        const std::string& content, 
        const std::vector<FunctionInfo>& functions
    );
    
    //=========================================================================
    // 🎯 Complexity Metrics - 複雑度指標
    //=========================================================================
    
    /// Halstead複雑度（C++版独自）
    struct HalsteadMetrics {
        std::uint32_t operators = 0;
        std::uint32_t operands = 0;
        std::uint32_t unique_operators = 0;
        std::uint32_t unique_operands = 0;
        double difficulty = 0.0;
        double effort = 0.0;
        double time_to_implement = 0.0; // seconds
    };
    
    static HalsteadMetrics calculate_halstead_metrics(const std::string& content);
    
    /// 保守性指数計算
    static double calculate_maintainability_index(const std::string& content, const ComplexityInfo& complexity);

private:
    /// 制御構造キーワード
    static const std::vector<std::string> control_keywords_;
    
    /// 演算子・オペランド抽出
    static std::pair<std::vector<std::string>, std::vector<std::string>> extract_operators_operands(const std::string& content);
};

//=============================================================================
// 📄 File Scanner - 高速ファイルスキャナー
//=============================================================================

class FileScanner {
public:
    explicit FileScanner(const AnalysisConfig& config);
    
    //=========================================================================
    // 🔍 File Discovery
    //=========================================================================
    
    /// 再帰的ファイル検索
    std::vector<FilePath> scan_directory(const FilePath& directory_path);
    
    /// 並列ファイル検索
    std::vector<FilePath> scan_directory_parallel(const FilePath& directory_path);
    
    /// ファイル拡張子チェック
    bool is_javascript_file(const FilePath& file_path) const;
    
    /// 除外パターンチェック
    bool should_exclude(const FilePath& file_path) const;
    
    /// ファイルサイズチェック
    bool is_file_too_large(const FilePath& file_path, FileSize max_size = 10 * 1024 * 1024) const; // 10MB default
    
    /// ファイルフィルタリング
    std::vector<FilePath> filter_files(const std::vector<FilePath>& files);
    
    //=========================================================================
    // 📊 Statistics
    //=========================================================================
    
    struct ScanStats {
        std::uint32_t total_files_found = 0;
        std::uint32_t javascript_files = 0;
        std::uint32_t excluded_files = 0;
        std::uint32_t large_files = 0;
        std::chrono::milliseconds scan_time{0};
    };
    
    const ScanStats& get_scan_stats() const { return stats_; }
    void reset_stats() { stats_ = ScanStats{}; }

private:
    AnalysisConfig config_;
    ScanStats stats_;
    
    /// ワイルドカードマッチング
    bool wildcard_match(const std::string& pattern, const std::string& text) const;
};

//=============================================================================
// 🎯 Utility Functions - ユーティリティ関数
//=============================================================================

namespace utils {
    /// 文字列トリミング
    std::string trim(const std::string& str);
    
    /// 行分割
    std::vector<std::string> split_lines(const std::string& content);
    
    /// コメント除去
    std::string remove_comments(const std::string& content);
    
    /// 文字列内の文字列リテラル除去
    std::string remove_string_literals(const std::string& content);
    
    /// ファイル読み込み（エラーハンドリング付き）
    Result<std::string> read_file(const FilePath& file_path);
    
    /// ファイル情報取得
    Result<FileInfo> get_basic_file_info(const FilePath& file_path);
    
    /// 現在時刻のフォーマット
    std::string format_timestamp(const Timestamp& timestamp);
    
    /// ファイルサイズのフォーマット
    std::string format_file_size(FileSize size);
    
    /// 実行時間測定ヘルパー
    template<typename F>
    auto measure_time(F&& func) -> std::pair<decltype(func()), std::chrono::milliseconds> {
        auto start = std::chrono::steady_clock::now();
        auto result = func();
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return {std::move(result), duration};
    }
}

} // namespace nekocode