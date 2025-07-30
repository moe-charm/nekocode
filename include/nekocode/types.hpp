#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <chrono>
#include <filesystem>
#include <thread>
#include <optional>
#include <nlohmann/json.hpp>

namespace nekocode {

//=============================================================================
// 🌍 Language Support - 言語種別定義
//=============================================================================

/// サポート言語種別
enum class Language {
    JAVASCRIPT,     // JavaScript
    TYPESCRIPT,     // TypeScript
    CPP,           // C++
    C,             // C
    PYTHON,        // Python 🐍
    CSHARP,        // C# 🎯
    GO,            // Go 🐹
    RUST,          // Rust 🦀
    UNKNOWN        // 不明・未対応
};

//=============================================================================
// 🎯 Core Types - Python版互換・型安全設計
//=============================================================================

using FilePath = std::filesystem::path;
using FileSize = std::uintmax_t;
using LineNumber = std::uint32_t;
using Timestamp = std::chrono::system_clock::time_point;

//=============================================================================
// 📄 File Information - Python版FileInfo相当
//=============================================================================

struct FileInfo {
    std::string name;
    FilePath path;
    FileSize size_bytes = 0;
    LineNumber total_lines = 0;
    LineNumber code_lines = 0;
    LineNumber comment_lines = 0;
    LineNumber empty_lines = 0;
    double code_ratio = 0.0;
    Timestamp analyzed_at;
    
    FileInfo() : analyzed_at(std::chrono::system_clock::now()) {}
    
    explicit FileInfo(const FilePath& file_path) 
        : name(file_path.filename().string())
        , path(file_path)
        , analyzed_at(std::chrono::system_clock::now()) {}
};

//=============================================================================
// 🧮 Complexity Analysis - Python版複雑度解析拡張
//=============================================================================

enum class ComplexityRating {
    SIMPLE,      // <= 10
    MODERATE,    // 11-20
    COMPLEX,     // 21-50
    VERY_COMPLEX // > 50
};

struct ComplexityInfo {
    std::uint32_t cyclomatic_complexity = 1;
    std::uint32_t max_nesting_depth = 0;
    std::uint32_t cognitive_complexity = 0;  // C++版で追加
    ComplexityRating rating = ComplexityRating::SIMPLE;
    std::string rating_emoji;
    
    ComplexityInfo() { update_rating(); }
    
    void update_rating() {
        if (cyclomatic_complexity <= 10) {
            rating = ComplexityRating::SIMPLE;
            rating_emoji = "🟢";
        } else if (cyclomatic_complexity <= 20) {
            rating = ComplexityRating::MODERATE;
            rating_emoji = "🟡";
        } else if (cyclomatic_complexity <= 50) {
            rating = ComplexityRating::COMPLEX;
            rating_emoji = "🟠";
        } else {
            rating = ComplexityRating::VERY_COMPLEX;
            rating_emoji = "🔴";
        }
    }
    
    std::string to_string() const {
        switch (rating) {
            case ComplexityRating::SIMPLE: return "Simple " + rating_emoji;
            case ComplexityRating::MODERATE: return "Moderate " + rating_emoji;
            case ComplexityRating::COMPLEX: return "Complex " + rating_emoji;
            case ComplexityRating::VERY_COMPLEX: return "Very Complex " + rating_emoji;
        }
        return "Unknown";
    }
};

//=============================================================================
// 🏗️ Code Structure - クラス・関数情報
//=============================================================================

struct FunctionInfo {
    std::string name;
    LineNumber start_line = 0;
    LineNumber end_line = 0;
    std::vector<std::string> parameters;
    bool is_async = false;
    bool is_arrow_function = false;
    ComplexityInfo complexity;
    std::unordered_map<std::string, std::string> metadata;  // 🧩 Unity等の拡張情報
    
    FunctionInfo() = default;
    explicit FunctionInfo(const std::string& func_name) : name(func_name) {}
};

// 🔍 メンバ変数情報（analyze機能用）
struct MemberVariable {
    std::string name;
    std::string type;
    LineNumber declaration_line = 0;
    bool is_static = false;
    bool is_const = false;
    std::string access_modifier = "private";  // public/private/protected
    
    // Phase2で追加される情報
    std::vector<std::string> used_by_methods;
    std::vector<std::string> modified_by_methods;
    
    MemberVariable() = default;
    MemberVariable(const std::string& var_name, const std::string& var_type, LineNumber line)
        : name(var_name), type(var_type), declaration_line(line) {}
};

struct ClassInfo {
    std::string name;
    std::string parent_class;
    LineNumber start_line = 0;
    LineNumber end_line = 0;
    std::vector<FunctionInfo> methods;
    std::vector<std::string> properties;
    std::vector<MemberVariable> member_variables;  // 🆕 メンバ変数リスト
    std::unordered_map<std::string, std::string> metadata;  // 🧩 Unity等の拡張情報
    
    ClassInfo() = default;
    explicit ClassInfo(const std::string& class_name) : name(class_name) {}
};

// 📊 クラス統計用構造体（analyze機能用）
struct ClassMetrics {
    std::uint32_t member_variable_count = 0;
    std::uint32_t method_count = 0;
    std::uint32_t total_lines = 0;
    std::uint32_t responsibility_score = 0;  // variables × methods
    float cohesion = 0.0f;  // 0.0-1.0
    std::uint32_t coupling = 0;  // 外部クラス参照数
    
    ClassMetrics() = default;
    
    void calculate_responsibility() {
        responsibility_score = member_variable_count * method_count;
    }
};

//=============================================================================
// 📦 Import/Export Analysis - 依存関係情報
//=============================================================================

enum class ImportType {
    ES6_IMPORT,      // import ... from
    COMMONJS_REQUIRE, // require()
    DYNAMIC_IMPORT   // import()
};

enum class ExportType {
    ES6_EXPORT,      // export ...
    ES6_DEFAULT,     // export default
    COMMONJS_EXPORTS // module.exports
};

struct ImportInfo {
    ImportType type = ImportType::ES6_IMPORT;
    std::string module_path;
    std::vector<std::string> imported_names;
    std::string alias;
    LineNumber line_number = 0;
    std::unordered_map<std::string, std::string> metadata;  // 🧩 C言語include等の拡張情報
    
    ImportInfo() = default;
    ImportInfo(ImportType t, const std::string& path) : type(t), module_path(path) {}
};

struct ExportInfo {
    ExportType type = ExportType::ES6_EXPORT;
    std::vector<std::string> exported_names;
    bool is_default = false;
    LineNumber line_number = 0;
    
    ExportInfo() = default;
    explicit ExportInfo(ExportType t) : type(t) {}
};

//=============================================================================
// 📞 Function Call Analysis - 関数呼び出し解析
//=============================================================================

struct FunctionCall {
    std::string function_name;
    std::string object_name;  // obj.method() の obj部分
    LineNumber line_number = 0;
    bool is_method_call = false;
    
    FunctionCall() = default;
    FunctionCall(const std::string& name, LineNumber line) 
        : function_name(name), line_number(line) {}
    
    std::string full_name() const {
        return is_method_call ? (object_name + "." + function_name) : function_name;
    }
};

using FunctionCallFrequency = std::unordered_map<std::string, std::uint32_t>;

//=============================================================================
// 📊 Analysis Results - 解析結果統合
//=============================================================================

struct AnalysisResult {
    // 基本情報
    FileInfo file_info;
    Language language = Language::UNKNOWN;
    
    // 多態性のため
    virtual ~AnalysisResult() = default;
    
    // 構造情報
    std::vector<ClassInfo> classes;
    std::vector<FunctionInfo> functions;
    
    // 依存関係
    std::vector<ImportInfo> imports;
    std::vector<ExportInfo> exports;
    
    // 関数呼び出し
    std::vector<FunctionCall> function_calls;
    FunctionCallFrequency call_frequency;
    
    // 複雑度
    ComplexityInfo complexity;
    
    // 拡張メタデータ（Unity等の特殊情報）
    std::unordered_map<std::string, std::string> metadata;
    
    // 統計（Python版互換）
    struct Statistics {
        std::uint32_t class_count = 0;
        std::uint32_t function_count = 0;
        std::uint32_t import_count = 0;
        std::uint32_t export_count = 0;
        std::uint32_t unique_calls = 0;
        std::uint32_t total_calls = 0;
    } stats;
    
    // 生成時刻
    Timestamp generated_at;
    
    AnalysisResult() : generated_at(std::chrono::system_clock::now()) {}
    
    void update_statistics() {
        stats.class_count = static_cast<std::uint32_t>(classes.size());
        stats.function_count = static_cast<std::uint32_t>(functions.size());
        stats.import_count = static_cast<std::uint32_t>(imports.size());
        stats.export_count = static_cast<std::uint32_t>(exports.size());
        stats.unique_calls = static_cast<std::uint32_t>(call_frequency.size());
        stats.total_calls = static_cast<std::uint32_t>(function_calls.size());
    }
};

//=============================================================================
// 📁 Directory Analysis - ディレクトリ解析結果
//=============================================================================

struct DirectoryAnalysis {
    FilePath directory_path;
    std::vector<AnalysisResult> files;
    
    // 集計統計
    struct Summary {
        std::uint32_t total_files = 0;
        LineNumber total_lines = 0;
        FileSize total_size = 0;
        std::uint32_t large_files = 0;  // >500行
        std::uint32_t complex_files = 0; // Complex以上
        
        // コード構造統計
        std::uint32_t total_classes = 0;
        std::uint32_t total_functions = 0;
        
        // 全体複雑度統計
        std::uint32_t total_complexity = 0;
        double average_complexity = 0.0;
        std::uint32_t max_complexity = 0;
        std::string most_complex_file;
    } summary;
    
    Timestamp generated_at;
    
    DirectoryAnalysis() : generated_at(std::chrono::system_clock::now()) {}
    
    void update_summary() {
        summary.total_files = static_cast<std::uint32_t>(files.size());
        summary.total_lines = 0;
        summary.total_size = 0;
        summary.large_files = 0;
        summary.complex_files = 0;
        summary.total_classes = 0;
        summary.total_functions = 0;
        summary.total_complexity = 0;
        summary.max_complexity = 0;
        
        for (const auto& file : files) {
            summary.total_lines += file.file_info.total_lines;
            summary.total_size += file.file_info.size_bytes;
            summary.total_classes += file.stats.class_count;
            summary.total_functions += file.stats.function_count;
            
            if (file.file_info.total_lines > 500) {
                summary.large_files++;
            }
            
            if (file.complexity.rating >= ComplexityRating::COMPLEX) {
                summary.complex_files++;
            }
            
            summary.total_complexity += file.complexity.cyclomatic_complexity;
            
            if (file.complexity.cyclomatic_complexity > summary.max_complexity) {
                summary.max_complexity = file.complexity.cyclomatic_complexity;
                summary.most_complex_file = file.file_info.name;
            }
        }
        
        summary.average_complexity = summary.total_files > 0 ? 
            static_cast<double>(summary.total_complexity) / summary.total_files : 0.0;
    }
};

//=============================================================================
// 💾 Storage Mode - ストレージ最適化モード
//=============================================================================

/// ストレージタイプ別最適化モード
enum class StorageMode {
    AUTO,     ///< 自動検出 (CPU コア数)
    SSD,      ///< SSD最適化 (CPU コア数、並列I/O重視)
    HDD,      ///< HDD最適化 (1スレッド、シーケンシャル重視)
    MANUAL    ///< 手動指定 (ユーザー指定値)
};

//=============================================================================
// ⚙️ Configuration - 設定情報
//=============================================================================

struct AnalysisConfig {
    // ファイルフィルタ - マルチ言語対応
    std::vector<std::string> included_extensions = {
        // JavaScript/TypeScript
        ".js", ".mjs", ".jsx", ".ts", ".tsx",
        // C++
        ".cpp", ".cxx", ".cc", ".C",
        ".hpp", ".hxx", ".hh", ".H",
        // C
        ".c", ".h"
    };
    std::vector<std::string> excluded_patterns = {"node_modules", ".git", "dist", "build", "__pycache__"};
    
    // 解析オプション
    bool analyze_complexity = true;
    bool analyze_dependencies = true;
    bool analyze_function_calls = true;
    bool include_test_files = false;
    
    // パフォーマンス設定
    bool enable_parallel_processing = true;
    std::uint32_t max_threads = 0; // 0 = auto detect (廃止予定)
    std::uint32_t io_threads = 4;  // 🆕 同時ファイル読み込み数
    std::uint32_t cpu_threads = 0; // 🆕 解析スレッド数 (0 = auto)
    StorageMode storage_mode = StorageMode::AUTO; // ストレージ最適化モード
    
    // 出力設定
    bool verbose_output = false;
    bool include_line_numbers = true;
    
    AnalysisConfig() {
        // ストレージモード別スレッド数計算
        calculate_optimal_threads();
    }
    
    void calculate_optimal_threads() {
        std::uint32_t cores = std::thread::hardware_concurrency();
        if (cores == 0) cores = 4; // fallback
        
        // cpu_threadsのデフォルト設定
        if (cpu_threads == 0) {
            cpu_threads = cores;
        }
        
        // 後方互換性: max_threadsからio_threads/cpu_threadsへマップ
        if (max_threads != 0) {
            cpu_threads = max_threads;
            // ストレージモードに基づいてio_threadsを設定
            switch (storage_mode) {
                case StorageMode::HDD:
                    io_threads = 1;  // HDDモード: シーケンシャル読み込み
                    break;
                case StorageMode::SSD:
                    io_threads = std::min(cores, 8u);  // SSDモード: 並列読み込み（最大8）
                    break;
                default:
                    // io_threadsはデフォルト値(4)のまま
                    break;
            }
        }
        
        // max_threadsも更新（後方互換性）
        max_threads = cpu_threads;
    }
};

//=============================================================================
// 🎯 Output Formats - 出力フォーマット（実行ファイル２個大作戦）
//=============================================================================

enum class OutputFormat {
    AI_JSON,        // AI用: 構造化JSON
    HUMAN_TEXT,     // Human用: 美しいテキスト
    BOTH           // 両方
};

//=============================================================================
// 📈 Performance Metrics - パフォーマンス測定
//=============================================================================

struct PerformanceMetrics {
    std::chrono::milliseconds analysis_time{0};
    std::chrono::milliseconds file_scan_time{0};
    std::chrono::milliseconds parsing_time{0};
    std::chrono::milliseconds report_generation_time{0};
    
    std::uint32_t files_processed = 0;
    std::uint32_t lines_processed = 0;
    FileSize bytes_processed = 0;
    
    // スループット計算
    double files_per_second() const {
        if (analysis_time.count() == 0) return 0.0;
        return static_cast<double>(files_processed) * 1000.0 / analysis_time.count();
    }
    
    double lines_per_second() const {
        if (analysis_time.count() == 0) return 0.0;
        return static_cast<double>(lines_processed) * 1000.0 / analysis_time.count();
    }
    
    double megabytes_per_second() const {
        if (analysis_time.count() == 0) return 0.0;
        return static_cast<double>(bytes_processed) / (1024.0 * 1024.0) * 1000.0 / analysis_time.count();
    }
};

//=============================================================================
// 🚨 Error Handling - エラー処理
//=============================================================================

enum class ErrorCode {
    SUCCESS = 0,
    FILE_NOT_FOUND = 1,
    PERMISSION_DENIED = 2,
    INVALID_FILE_FORMAT = 3,
    PARSING_ERROR = 4,
    OUT_OF_MEMORY = 5,
    TIMEOUT = 6,
    UNKNOWN_ERROR = 99
};

struct AnalysisError {
    ErrorCode code = ErrorCode::SUCCESS;
    std::string message;
    FilePath file_path;
    LineNumber line_number = 0;
    
    AnalysisError() = default;
    AnalysisError(ErrorCode c, const std::string& msg) : code(c), message(msg) {}
    AnalysisError(ErrorCode c, const std::string& msg, const FilePath& path) 
        : code(c), message(msg), file_path(path) {}
    
    bool is_error() const { return code != ErrorCode::SUCCESS; }
    explicit operator bool() const { return is_error(); }
};

//=============================================================================
// 🎯 Result Wrapper - Result<T>型（エラーハンドリング）
//=============================================================================

template<typename T>
class Result {
private:
    T value_;
    AnalysisError error_;
    bool has_value_;
    
public:
    Result(T&& val) : value_(std::move(val)), has_value_(true) {}
    Result(const T& val) : value_(val), has_value_(true) {}
    Result(const AnalysisError& err) : error_(err), has_value_(false) {}
    
    bool is_success() const { return has_value_; }
    bool is_error() const { return !has_value_; }
    
    const T& value() const { 
        if (!has_value_) throw std::runtime_error("Accessing value of failed Result");
        return value_; 
    }
    
    T& value() { 
        if (!has_value_) throw std::runtime_error("Accessing value of failed Result");
        return value_; 
    }
    
    const AnalysisError& error() const { return error_; }
    
    // Monadic operations
    template<typename F>
    auto map(F&& func) -> Result<decltype(func(value_))> {
        if (is_error()) return Result<decltype(func(value_))>(error_);
        try {
            return Result<decltype(func(value_))>(func(value_));
        } catch (const std::exception& e) {
            return Result<decltype(func(value_))>(AnalysisError(ErrorCode::UNKNOWN_ERROR, e.what()));
        }
    }
};

} // namespace nekocode