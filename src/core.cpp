#include "nekocode/core.hpp"
#include "nekocode/formatters.hpp"
#include "nekocode/utf8_utils.hpp"
#include "nekocode/language_detection.hpp"
#include "nekocode/cpp_analyzer.hpp"
#include "nekocode/tree_sitter_analyzer.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <execution>
#include <filesystem>
#include <regex>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <atomic>
#include <numeric>

namespace nekocode {

//=============================================================================
// 🧠 NekoCodeCore::Impl - PIMPL実装
//=============================================================================

class NekoCodeCore::Impl {
public:
    AnalysisConfig config_;
    PerformanceMetrics metrics_;
    std::unique_ptr<TreeSitterAnalyzer> tree_sitter_analyzer_;
    std::unique_ptr<CppAnalyzer> cpp_analyzer_;
    std::unique_ptr<LanguageDetector> language_detector_;
    std::unique_ptr<FileScanner> file_scanner_;
    ProgressCallback progress_callback_;
    std::atomic<bool> parallel_enabled_{true};
    std::atomic<uint32_t> thread_count_{0};
    mutable std::mutex metrics_mutex_;

    explicit Impl(const AnalysisConfig& config) 
        : config_(config)
        , tree_sitter_analyzer_(std::make_unique<TreeSitterAnalyzer>())
        , cpp_analyzer_(std::make_unique<CppAnalyzer>())
        , language_detector_(std::make_unique<LanguageDetector>())
        , file_scanner_(std::make_unique<FileScanner>(config)) 
    {
        thread_count_ = config.max_threads;
    }
};

//=============================================================================
// 🏗️ NekoCodeCore Construction
//=============================================================================

NekoCodeCore::NekoCodeCore(const AnalysisConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

NekoCodeCore::~NekoCodeCore() = default;

NekoCodeCore::NekoCodeCore(NekoCodeCore&&) noexcept = default;
NekoCodeCore& NekoCodeCore::operator=(NekoCodeCore&&) noexcept = default;

//=============================================================================
// 📄 Single File Analysis
//=============================================================================

Result<AnalysisResult> NekoCodeCore::analyze_file(const FilePath& file_path) {
    auto [result, duration] = utils::measure_time([&]() -> Result<AnalysisResult> {
        // ファイル読み込み
        auto content_result = utils::read_file(file_path);
        if (content_result.is_error()) {
            return Result<AnalysisResult>(content_result.error());
        }
        
        return analyze_content(content_result.value(), file_path.filename().string());
    });
    
    // パフォーマンス統計更新
    {
        std::lock_guard<std::mutex> lock(impl_->metrics_mutex_);
        impl_->metrics_.parsing_time += duration;
        impl_->metrics_.files_processed++;
    }
    
    return result;
}

Result<AnalysisResult> NekoCodeCore::analyze_content(const std::string& content, const std::string& filename) {
    try {
        AnalysisResult result;
        
        // ファイル基本情報解析
        FilePath dummy_path(filename);
        result.file_info = analyze_file_structure(content, dummy_path);
        
        if (impl_->config_.analyze_complexity) {
            result.complexity = analyze_complexity(content);
        }
        
        // 🌳 Tree-sitter統合解析 - 正規表現地獄からの脱出！
        auto tree_result = impl_->tree_sitter_analyzer_->analyze(content, filename, Language::JAVASCRIPT);
        if (tree_result.is_success()) {
            auto ts_result = tree_result.value();
            result.classes = ts_result.classes;
            result.functions = ts_result.functions;
            result.imports = ts_result.imports;
            result.exports = ts_result.exports;
            if (impl_->config_.analyze_complexity) {
                result.complexity = ts_result.complexity;
            }
        } else {
            // Tree-sitter解析失敗時はエラーを返す
            return Result<AnalysisResult>(tree_result.error());
        }
        
        // 🌳 Tree-sitterが既に依存関係と複雑度を解析済み
        // 従来の正規表現ベース解析は不要
        
        // 統計更新
        result.update_statistics();
        
        // パフォーマンス統計
        {
            std::lock_guard<std::mutex> lock(impl_->metrics_mutex_);
            impl_->metrics_.lines_processed += result.file_info.total_lines;
            impl_->metrics_.bytes_processed += result.file_info.size_bytes;
        }
        
        return Result<AnalysisResult>(std::move(result));
        
    } catch (const std::exception& e) {
        return Result<AnalysisResult>(AnalysisError(ErrorCode::PARSING_ERROR, e.what()));
    }
}

Result<FileInfo> NekoCodeCore::get_file_info(const FilePath& file_path) {
    return utils::get_basic_file_info(file_path);
}

//=============================================================================
// 🌍 Multi-Language Analysis Implementation
//=============================================================================

Result<MultiLanguageAnalysisResult> NekoCodeCore::analyze_file_multilang(const FilePath& file_path) {
    auto [result, duration] = utils::measure_time([&]() -> Result<MultiLanguageAnalysisResult> {
        try {
            // UTF-8 safe file reading
            auto safe_content = utf8::read_file_safe_utf8(file_path.string());
            if (!safe_content.conversion_success) {
                return Result<MultiLanguageAnalysisResult>(
                    AnalysisError(ErrorCode::FILE_NOT_FOUND, safe_content.error_message, file_path));
            }
            
            // 言語自動検出
            Language detected_lang = impl_->language_detector_->detect_language(file_path, safe_content.content);
            
            return analyze_content_multilang(safe_content.content, file_path.filename().string(), detected_lang);
            
        } catch (const std::exception& e) {
            return Result<MultiLanguageAnalysisResult>(
                AnalysisError(ErrorCode::PARSING_ERROR, e.what(), file_path));
        }
    });
    
    // パフォーマンス統計更新
    {
        std::lock_guard<std::mutex> lock(impl_->metrics_mutex_);
        impl_->metrics_.parsing_time += duration;
        impl_->metrics_.files_processed++;
    }
    
    return result;
}

Result<MultiLanguageAnalysisResult> NekoCodeCore::analyze_file_with_language(const FilePath& file_path, Language language) {
    auto [result, duration] = utils::measure_time([&]() -> Result<MultiLanguageAnalysisResult> {
        try {
            // UTF-8 safe file reading
            auto safe_content = utf8::read_file_safe_utf8(file_path.string());
            if (!safe_content.conversion_success) {
                return Result<MultiLanguageAnalysisResult>(
                    AnalysisError(ErrorCode::FILE_NOT_FOUND, safe_content.error_message, file_path));
            }
            
            return analyze_content_multilang(safe_content.content, file_path.filename().string(), language);
            
        } catch (const std::exception& e) {
            return Result<MultiLanguageAnalysisResult>(
                AnalysisError(ErrorCode::PARSING_ERROR, e.what(), file_path));
        }
    });
    
    // パフォーマンス統計更新
    {
        std::lock_guard<std::mutex> lock(impl_->metrics_mutex_);
        impl_->metrics_.parsing_time += duration;
        impl_->metrics_.files_processed++;
    }
    
    return result;
}

Result<MultiLanguageAnalysisResult> NekoCodeCore::analyze_content_multilang(const std::string& content, 
                                                                            const std::string& filename, 
                                                                            Language language) {
    try {
        MultiLanguageAnalysisResult result;
        result.detected_language = language;
        
        // 言語が不明な場合は内容から推定
        if (language == Language::UNKNOWN && !content.empty()) {
            result.detected_language = impl_->language_detector_->detect_by_content(content);
        }
        
        // 言語別解析実行
        switch (result.detected_language) {
            case Language::JAVASCRIPT:
            case Language::TYPESCRIPT: {
                // JavaScript/TypeScript解析
                auto js_result = analyze_content(content, filename);
                if (js_result.is_success()) {
                    result.js_result = js_result.value();
                    result.file_info = js_result.value().file_info;
                } else {
                    return Result<MultiLanguageAnalysisResult>(js_result.error());
                }
                break;
            }
            
            case Language::CPP:
            case Language::C: {
                // C++/C解析
                auto cpp_result = impl_->cpp_analyzer_->analyze_cpp_file(content, filename);
                result.cpp_result = cpp_result;
                result.file_info = cpp_result.file_info;
                break;
            }
            
            case Language::UNKNOWN:
            default: {
                // 不明な言語の場合はJavaScriptとして解析を試行
                auto js_result = analyze_content(content, filename);
                if (js_result.is_success()) {
                    result.js_result = js_result.value();
                    result.file_info = js_result.value().file_info;
                    result.detected_language = Language::JAVASCRIPT;
                } else {
                    return Result<MultiLanguageAnalysisResult>(
                        AnalysisError(ErrorCode::PARSING_ERROR, "Unknown language and JavaScript parsing failed"));
                }
                break;
            }
        }
        
        return Result<MultiLanguageAnalysisResult>(std::move(result));
        
    } catch (const std::exception& e) {
        return Result<MultiLanguageAnalysisResult>(
            AnalysisError(ErrorCode::PARSING_ERROR, e.what()));
    }
}

//=============================================================================
// 📁 Directory Analysis
//=============================================================================

Result<DirectoryAnalysis> NekoCodeCore::analyze_directory(const FilePath& directory_path) {
    if (impl_->parallel_enabled_) {
        return analyze_directory_parallel(directory_path);
    }
    
    auto [result, duration] = utils::measure_time([&]() -> Result<DirectoryAnalysis> {
        try {
            DirectoryAnalysis analysis;
            analysis.directory_path = directory_path;
            
            // ファイル検索
            auto files = impl_->file_scanner_->scan_directory(directory_path);
            auto js_files = impl_->file_scanner_->filter_files(files);
            
            // 順次解析
            for (size_t i = 0; i < js_files.size(); ++i) {
                if (impl_->progress_callback_) {
                    impl_->progress_callback_(
                        static_cast<uint32_t>(i), 
                        static_cast<uint32_t>(js_files.size()), 
                        js_files[i].filename().string()
                    );
                }
                
                auto file_result = analyze_file(js_files[i]);
                if (file_result.is_success()) {
                    analysis.files.push_back(file_result.value());
                }
            }
            
            analysis.update_summary();
            return Result<DirectoryAnalysis>(std::move(analysis));
            
        } catch (const std::exception& e) {
            return Result<DirectoryAnalysis>(AnalysisError(ErrorCode::PARSING_ERROR, e.what()));
        }
    });
    
    // パフォーマンス統計更新
    {
        std::lock_guard<std::mutex> lock(impl_->metrics_mutex_);
        impl_->metrics_.analysis_time += duration;
    }
    
    return result;
}

Result<DirectoryAnalysis> NekoCodeCore::analyze_directory_parallel(const FilePath& directory_path) {
    auto [result, duration] = utils::measure_time([&]() -> Result<DirectoryAnalysis> {
        try {
            DirectoryAnalysis analysis;
            analysis.directory_path = directory_path;
            
            // ファイル検索
            auto files = impl_->file_scanner_->scan_directory_parallel(directory_path);
            auto js_files = impl_->file_scanner_->filter_files(files);
            
            // 並列解析
            std::vector<AnalysisResult> results(js_files.size());
            std::vector<bool> success_flags(js_files.size(), false);
            
            // 並列処理用のインデックスベクトル作成
            std::vector<size_t> indices(js_files.size());
            std::iota(indices.begin(), indices.end(), 0);
            
            std::for_each(std::execution::par_unseq, 
                indices.begin(), indices.end(),
                [&](size_t i) {
                    auto file_result = analyze_file(js_files[i]);
                    if (file_result.is_success()) {
                        results[i] = file_result.value();
                        success_flags[i] = true;
                    }
                });
            
            // 成功した結果のみ収集
            for (size_t i = 0; i < results.size(); ++i) {
                if (success_flags[i]) {
                    analysis.files.push_back(std::move(results[i]));
                }
            }
            
            analysis.update_summary();
            return Result<DirectoryAnalysis>(std::move(analysis));
            
        } catch (const std::exception& e) {
            return Result<DirectoryAnalysis>(AnalysisError(ErrorCode::PARSING_ERROR, e.what()));
        }
    });
    
    // パフォーマンス統計更新
    {
        std::lock_guard<std::mutex> lock(impl_->metrics_mutex_);
        impl_->metrics_.analysis_time += duration;
    }
    
    return result;
}

Result<DirectoryAnalysis> NekoCodeCore::analyze_files(const std::vector<FilePath>& file_paths) {
    try {
        DirectoryAnalysis analysis;
        analysis.directory_path = file_paths.empty() ? FilePath(".") : file_paths[0].parent_path();
        
        for (const auto& file_path : file_paths) {
            auto file_result = analyze_file(file_path);
            if (file_result.is_success()) {
                analysis.files.push_back(file_result.value());
            }
        }
        
        analysis.update_summary();
        return Result<DirectoryAnalysis>(std::move(analysis));
        
    } catch (const std::exception& e) {
        return Result<DirectoryAnalysis>(AnalysisError(ErrorCode::PARSING_ERROR, e.what()));
    }
}

//=============================================================================
// 🔍 File Discovery
//=============================================================================

std::vector<FilePath> NekoCodeCore::scan_javascript_files(const FilePath& directory_path) {
    return impl_->file_scanner_->scan_directory(directory_path);
}

std::vector<FilePath> NekoCodeCore::filter_files(const std::vector<FilePath>& files) {
    return impl_->file_scanner_->filter_files(files);
}

bool NekoCodeCore::should_exclude_file(const FilePath& file_path) const {
    return impl_->file_scanner_->should_exclude(file_path);
}

//=============================================================================
// 📊 Analysis Components
//=============================================================================

FileInfo NekoCodeCore::analyze_file_structure(const std::string& content, const FilePath& file_path) {
    FileInfo info(file_path);
    
    auto lines = utils::split_lines(content);
    info.total_lines = static_cast<uint32_t>(lines.size());
    info.size_bytes = content.size();
    
    uint32_t code_lines = 0;
    uint32_t comment_lines = 0;
    uint32_t empty_lines = 0;
    
    bool in_block_comment = false;
    
    for (const auto& line : lines) {
        std::string trimmed = utils::trim(line);
        
        if (trimmed.empty()) {
            empty_lines++;
        } else if (trimmed.length() >= 2 && trimmed.substr(0, 2) == "//") {
            comment_lines++;
        } else if (trimmed.length() >= 2 && trimmed.substr(0, 2) == "/*") {
            comment_lines++;
            in_block_comment = !(trimmed.length() >= 2 && trimmed.substr(trimmed.length() - 2) == "*/");
        } else if (in_block_comment) {
            comment_lines++;
            if (trimmed.length() >= 2 && trimmed.substr(trimmed.length() - 2) == "*/") {
                in_block_comment = false;
            }
        } else {
            code_lines++;
        }
    }
    
    info.code_lines = code_lines;
    info.comment_lines = comment_lines;
    info.empty_lines = empty_lines;
    info.code_ratio = info.total_lines > 0 ? 
        static_cast<double>(code_lines) / info.total_lines : 0.0;
    
    return info;
}

std::vector<ClassInfo> NekoCodeCore::analyze_classes(const std::string& content) {
    // 🌳 Tree-sitterが既にクラス解析済み - プレースホルダー実装
    (void)content; // 未使用パラメータ警告回避
    return {}; // 空の結果を返す（Tree-sitterで既に処理済み）
}

std::vector<FunctionInfo> NekoCodeCore::analyze_functions(const std::string& content) {
    // 🌳 Tree-sitterが既に関数解析済み - プレースホルダー実装
    (void)content; // 未使用パラメータ警告回避
    return {}; // 空の結果を返す（Tree-sitterで既に処理済み）
}

std::pair<std::vector<ImportInfo>, std::vector<ExportInfo>> 
NekoCodeCore::analyze_dependencies(const std::string& content) {
    // 🌳 Tree-sitterが既に依存関係解析済み - プレースホルダー実装
    (void)content; // 未使用パラメータ警告回避
    return {{}, {}}; // 空の結果を返す（Tree-sitterで既に処理済み）
}

std::pair<std::vector<FunctionCall>, FunctionCallFrequency> 
NekoCodeCore::analyze_function_calls(const std::string& content) {
    // 🌳 Tree-sitterが既に関数呼び出し解析済み - プレースホルダー実装
    (void)content; // 未使用パラメータ警告回避
    return {{}, {}}; // 空の結果を返す（Tree-sitterで既に処理済み）
}

ComplexityInfo NekoCodeCore::analyze_complexity(const std::string& content) {
    // 🌳 Tree-sitterが既に複雑度解析済み - プレースホルダー実装
    (void)content; // 未使用パラメータ警告回避
    
    ComplexityInfo complexity;
    complexity.cyclomatic_complexity = 1; // 基本値
    complexity.cognitive_complexity = 0;
    complexity.max_nesting_depth = 0;
    complexity.update_rating();
    
    return complexity;
}

//=============================================================================
// ⚙️ Configuration & Settings
//=============================================================================

void NekoCodeCore::set_config(const AnalysisConfig& config) {
    impl_->config_ = config;
    impl_->file_scanner_ = std::make_unique<FileScanner>(config);
}

const AnalysisConfig& NekoCodeCore::get_config() const {
    return impl_->config_;
}

void NekoCodeCore::enable_parallel_processing(bool enabled) {
    impl_->parallel_enabled_ = enabled;
}

void NekoCodeCore::set_thread_count(std::uint32_t count) {
    impl_->thread_count_ = count;
}

//=============================================================================
// 📈 Performance & Monitoring
//=============================================================================

const PerformanceMetrics& NekoCodeCore::get_performance_metrics() const {
    std::lock_guard<std::mutex> lock(impl_->metrics_mutex_);
    return impl_->metrics_;
}

void NekoCodeCore::reset_performance_metrics() {
    std::lock_guard<std::mutex> lock(impl_->metrics_mutex_);
    impl_->metrics_ = PerformanceMetrics{};
}

void NekoCodeCore::set_progress_callback(ProgressCallback callback) {
    impl_->progress_callback_ = callback;
}

//=============================================================================
// 🌍 Multi-Language Support Implementation
//=============================================================================

const LanguageDetector& NekoCodeCore::get_language_detector() const {
    return *impl_->language_detector_;
}

const CppAnalyzer& NekoCodeCore::get_cpp_analyzer() const {
    return *impl_->cpp_analyzer_;
}

std::vector<Language> NekoCodeCore::get_supported_languages() const {
    return impl_->language_detector_->get_supported_languages();
}

std::vector<FilePath> NekoCodeCore::scan_supported_files(const FilePath& directory_path) {
    std::vector<FilePath> all_files;
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory_path)) {
            if (entry.is_regular_file()) {
                Language detected = impl_->language_detector_->detect_by_extension(entry.path());
                if (detected != Language::UNKNOWN && !should_exclude_file(entry.path())) {
                    all_files.push_back(entry.path());
                }
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        // ディレクトリアクセスエラーは無視
    }
    
    return all_files;
}

std::vector<FilePath> NekoCodeCore::scan_files_for_language(const FilePath& directory_path, Language language) {
    std::vector<FilePath> language_files;
    auto extensions = impl_->language_detector_->get_extensions_for_language(language);
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory_path)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                
                if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end() &&
                    !should_exclude_file(entry.path())) {
                    language_files.push_back(entry.path());
                }
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        // ディレクトリアクセスエラーは無視
    }
    
    return language_files;
}

std::unordered_map<Language, std::vector<FilePath>> NekoCodeCore::classify_files_by_language(const std::vector<FilePath>& files) {
    std::unordered_map<Language, std::vector<FilePath>> classified;
    
    for (const auto& file : files) {
        Language detected = impl_->language_detector_->detect_by_extension(file);
        classified[detected].push_back(file);
    }
    
    return classified;
}

//=============================================================================
// 🎯 JavaScript Analyzer Implementation
//=============================================================================

JavaScriptAnalyzer::JavaScriptAnalyzer() {
    try {
        initialize_patterns();
        initialize_standard_functions();
    } catch (const std::regex_error& e) {
        throw std::runtime_error(std::string("JavaScriptAnalyzer regex error: ") + e.what());
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("JavaScriptAnalyzer initialization error: ") + e.what());
    }
}

JavaScriptAnalyzer::~JavaScriptAnalyzer() = default;

void JavaScriptAnalyzer::initialize_patterns() {
    // ES6 クラス: class ClassName [extends Parent] {
    class_regex_ = std::regex(R"(class\s+(\w+)(?:\s+extends\s+(\w+))?\s*\{)");
    
    // プロトタイプクラス: function ClassName() { ... ClassName.prototype.method
    // 簡略化版 - 後方参照 \1 が問題の可能性があるため除去
    prototype_regex_ = std::regex(R"(function\s+(\w+)\s*\([^)]*\)\s*\{)");
    
    // 関数: function name() { ... }
    function_regex_ = std::regex(R"(function\s+(\w+)\s*\(([^)]*)\)\s*\{)");
    
    // アロー関数: const name = (...) => { ... }
    arrow_function_regex_ = std::regex(R"((?:const|let|var)\s+(\w+)\s*=\s*\([^)]*\)\s*=>)");
    
    // async関数
    async_function_regex_ = std::regex(R"(async\s+function\s+(\w+)\s*\([^)]*\)\s*\{)");
    
    // ES6 import
    es6_import_regex_ = std::regex(R"(import\s+(?:[\w\s,{}*]+\s+from\s+)?['""]([^'""]+)['""])");
    
    // CommonJS require
    commonjs_require_regex_ = std::regex(R"(require\s*\(\s*['""]([^'""]+)['""]\s*\))");
    
    // ES6 export
    es6_export_regex_ = std::regex(R"(export\s+(?:default\s+)?(?:const|let|var|function|class)?\s*(\w+))");
    
    // 関数呼び出し: functionName()
    function_call_regex_ = std::regex(R"((\w+)\s*\()");
    
    // メソッド呼び出し: object.method()
    method_call_regex_ = std::regex(R"((\w+)\.(\w+)\s*\()");
}

void JavaScriptAnalyzer::initialize_standard_functions() {
    standard_functions_ = {
        // JavaScript標準関数
        "console", "setTimeout", "setInterval", "clearTimeout", "clearInterval",
        "parseInt", "parseFloat", "isNaN", "isFinite", "encodeURI", "decodeURI",
        "eval", "typeof", "instanceof", "new", "delete", "void",
        
        // Array methods
        "push", "pop", "shift", "unshift", "slice", "splice", "concat",
        "join", "reverse", "sort", "indexOf", "lastIndexOf", "forEach",
        "map", "filter", "reduce", "reduceRight", "some", "every", "find",
        
        // Object methods
        "hasOwnProperty", "toString", "valueOf", "constructor",
        
        // String methods
        "charAt", "charCodeAt", "indexOf", "lastIndexOf", "substring",
        "substr", "slice", "toLowerCase", "toUpperCase", "trim",
        "replace", "split", "match", "search",
        
        // Math functions
        "abs", "ceil", "floor", "round", "max", "min", "random",
        "pow", "sqrt", "sin", "cos", "tan", "log",
        
        // Date functions
        "getTime", "getDate", "getDay", "getMonth", "getFullYear",
        "setDate", "setMonth", "setFullYear",
        
        // Promise/async
        "then", "catch", "finally", "resolve", "reject",
        
        // DOM (除外対象)
        "getElementById", "querySelector", "addEventListener",
        "createElement", "appendChild", "removeChild"
    };
}

std::vector<ClassInfo> JavaScriptAnalyzer::find_es6_classes(const std::string& content) {
    std::vector<ClassInfo> classes;
    std::sregex_iterator iter(content.begin(), content.end(), class_regex_);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        ClassInfo class_info;
        class_info.name = (*iter)[1].str();
        
        if ((*iter)[2].matched) {
            class_info.parent_class = (*iter)[2].str();
        }
        
        // メソッド抽出
        class_info.methods = extract_class_methods(content, class_info);
        
        classes.push_back(class_info);
    }
    
    return classes;
}

std::vector<ClassInfo> JavaScriptAnalyzer::find_prototype_classes(const std::string& content) {
    std::vector<ClassInfo> classes;
    std::sregex_iterator iter(content.begin(), content.end(), prototype_regex_);
    std::sregex_iterator end;
    
    std::unordered_set<std::string> found_classes;
    
    for (; iter != end; ++iter) {
        std::string class_name = (*iter)[1].str();
        
        if (found_classes.find(class_name) == found_classes.end()) {
            ClassInfo class_info;
            class_info.name = class_name;
            
            // プロトタイプメソッド検索
            // エスケープ処理を簡略化
            std::regex proto_method_regex(class_name + R"(\.prototype\.(\w+)\s*=)");
            std::sregex_iterator method_iter(content.begin(), content.end(), proto_method_regex);
            
            for (; method_iter != end; ++method_iter) {
                FunctionInfo method;
                method.name = (*method_iter)[1].str();
                class_info.methods.push_back(method);
            }
            
            classes.push_back(class_info);
            found_classes.insert(class_name);
        }
    }
    
    return classes;
}

std::vector<FunctionInfo> JavaScriptAnalyzer::extract_class_methods(const std::string& content, const ClassInfo& class_info) {
    std::vector<FunctionInfo> methods;
    
    // クラス内のメソッド検索
    std::regex method_regex(R"((\w+)\s*\([^)]*\)\s*\{)");
    
    // クラス定義の開始位置を見つける
    // エスケープ処理を簡略化
    std::regex class_start_regex("class\\s+" + class_info.name + R"((?:\s+extends\s+\w+)?\s*\{)");
    std::smatch class_match;
    
    if (std::regex_search(content, class_match, class_start_regex)) {
        size_t class_start = class_match.position() + class_match.length();
        
        // 対応する}を見つける（簡易実装）
        int brace_count = 1;
        size_t class_end = class_start;
        
        for (size_t i = class_start; i < content.length() && brace_count > 0; ++i) {
            if (content[i] == '{') brace_count++;
            else if (content[i] == '}') brace_count--;
            class_end = i;
        }
        
        // クラス内容を抽出
        std::string class_content = content.substr(class_start, class_end - class_start);
        
        // メソッド検索
        std::sregex_iterator iter(class_content.begin(), class_content.end(), method_regex);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            FunctionInfo method;
            method.name = (*iter)[1].str();
            
            // constructor以外のメソッドのみ
            if (method.name != "constructor") {
                methods.push_back(method);
            }
        }
    }
    
    return methods;
}

std::vector<FunctionInfo> JavaScriptAnalyzer::find_regular_functions(const std::string& content) {
    std::vector<FunctionInfo> functions;
    std::sregex_iterator iter(content.begin(), content.end(), function_regex_);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        FunctionInfo func;
        func.name = (*iter)[1].str();
        
        // パラメータ解析
        std::string params_str = (*iter)[2].str();
        func.parameters = parse_function_parameters(params_str);
        
        functions.push_back(func);
    }
    
    return functions;
}

std::vector<FunctionInfo> JavaScriptAnalyzer::find_arrow_functions(const std::string& content) {
    std::vector<FunctionInfo> functions;
    std::sregex_iterator iter(content.begin(), content.end(), arrow_function_regex_);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        FunctionInfo func;
        func.name = (*iter)[1].str();
        func.is_arrow_function = true;
        
        functions.push_back(func);
    }
    
    return functions;
}

std::vector<FunctionInfo> JavaScriptAnalyzer::find_async_functions(const std::string& content) {
    std::vector<FunctionInfo> functions;
    std::sregex_iterator iter(content.begin(), content.end(), async_function_regex_);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        FunctionInfo func;
        func.name = (*iter)[1].str();
        func.is_async = true;
        
        functions.push_back(func);
    }
    
    return functions;
}

std::vector<std::string> JavaScriptAnalyzer::parse_function_parameters(const std::string& params_str) {
    std::vector<std::string> parameters;
    
    if (params_str.empty()) return parameters;
    
    std::stringstream ss(params_str);
    std::string param;
    
    while (std::getline(ss, param, ',')) {
        param = utils::trim(param);
        if (!param.empty()) {
            // デフォルト値を除去: param = defaultValue → param
            size_t eq_pos = param.find('=');
            if (eq_pos != std::string::npos) {
                param = param.substr(0, eq_pos);
                param = utils::trim(param);
            }
            parameters.push_back(param);
        }
    }
    
    return parameters;
}

std::vector<ImportInfo> JavaScriptAnalyzer::parse_es6_imports(const std::string& content) {
    std::vector<ImportInfo> imports;
    std::sregex_iterator iter(content.begin(), content.end(), es6_import_regex_);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        ImportInfo import;
        import.type = ImportType::ES6_IMPORT;
        import.module_path = (*iter)[1].str();
        
        imports.push_back(import);
    }
    
    return imports;
}

std::vector<ImportInfo> JavaScriptAnalyzer::parse_commonjs_requires(const std::string& content) {
    std::vector<ImportInfo> imports;
    std::sregex_iterator iter(content.begin(), content.end(), commonjs_require_regex_);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        ImportInfo import;
        import.type = ImportType::COMMONJS_REQUIRE;
        import.module_path = (*iter)[1].str();
        
        imports.push_back(import);
    }
    
    return imports;
}

std::vector<ExportInfo> JavaScriptAnalyzer::parse_es6_exports(const std::string& content) {
    std::vector<ExportInfo> exports;
    std::sregex_iterator iter(content.begin(), content.end(), es6_export_regex_);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        ExportInfo export_info;
        export_info.type = ExportType::ES6_EXPORT;
        
        std::string exported_name = (*iter)[1].str();
        if (!exported_name.empty()) {
            export_info.exported_names.push_back(exported_name);
        }
        
        exports.push_back(export_info);
    }
    
    return exports;
}

std::vector<ExportInfo> JavaScriptAnalyzer::parse_commonjs_exports(const std::string& content) {
    std::vector<ExportInfo> exports;
    
    // module.exports = ... または exports.name = ...
    std::regex cjs_export_regex(R"((?:module\.)?exports(?:\.(\w+))?\s*=)");
    std::sregex_iterator iter(content.begin(), content.end(), cjs_export_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        ExportInfo export_info;
        export_info.type = ExportType::COMMONJS_EXPORTS;
        
        if ((*iter)[1].matched) {
            export_info.exported_names.push_back((*iter)[1].str());
        }
        
        exports.push_back(export_info);
    }
    
    return exports;
}

std::vector<FunctionCall> JavaScriptAnalyzer::find_function_calls(const std::string& content) {
    std::vector<FunctionCall> calls;
    std::sregex_iterator iter(content.begin(), content.end(), function_call_regex_);
    std::sregex_iterator end;
    
    uint32_t line_number = 1;
    size_t line_start = 0;
    
    for (; iter != end; ++iter) {
        FunctionCall call;
        call.function_name = (*iter)[1].str();
        
        // 行番号計算
        size_t pos = iter->position();
        line_number += std::count(content.begin() + line_start, content.begin() + pos, '\n');
        call.line_number = line_number;
        line_start = pos;
        
        calls.push_back(call);
    }
    
    return calls;
}

std::vector<FunctionCall> JavaScriptAnalyzer::find_method_calls(const std::string& content) {
    std::vector<FunctionCall> calls;
    std::sregex_iterator iter(content.begin(), content.end(), method_call_regex_);
    std::sregex_iterator end;
    
    uint32_t line_number = 1;
    size_t line_start = 0;
    
    for (; iter != end; ++iter) {
        FunctionCall call;
        call.object_name = (*iter)[1].str();
        call.function_name = (*iter)[2].str();
        call.is_method_call = true;
        
        // 行番号計算
        size_t pos = iter->position();
        line_number += std::count(content.begin() + line_start, content.begin() + pos, '\n');
        call.line_number = line_number;
        line_start = pos;
        
        calls.push_back(call);
    }
    
    return calls;
}

FunctionCallFrequency JavaScriptAnalyzer::calculate_call_frequency(const std::vector<FunctionCall>& calls) {
    FunctionCallFrequency frequency;
    
    for (const auto& call : calls) {
        std::string call_name = call.full_name();
        frequency[call_name]++;
    }
    
    return frequency;
}

std::vector<FunctionCall> JavaScriptAnalyzer::filter_standard_functions(const std::vector<FunctionCall>& calls) {
    std::vector<FunctionCall> filtered;
    
    for (const auto& call : calls) {
        // 標準関数・メソッドをフィルタリング
        if (standard_functions_.find(call.function_name) == standard_functions_.end()) {
            filtered.push_back(call);
        }
    }
    
    return filtered;
}

//=============================================================================
// 🧮 Complexity Calculator Implementation  
//=============================================================================

const std::vector<std::string> ComplexityCalculator::control_keywords_ = {
    "if", "else", "while", "for", "do", "switch", "case", "catch", "try",
    "&&", "||", "?", ":", "return"
};

std::uint32_t ComplexityCalculator::calculate_cyclomatic_complexity(const std::string& content) {
    std::uint32_t complexity = 1; // 基本パス
    
    for (const auto& keyword : control_keywords_) {
        std::regex keyword_regex("\\b" + keyword + "\\b");
        std::sregex_iterator iter(content.begin(), content.end(), keyword_regex);
        std::sregex_iterator end;
        
        complexity += std::distance(iter, end);
    }
    
    return complexity;
}

std::uint32_t ComplexityCalculator::calculate_cognitive_complexity(const std::string& content) {
    std::uint32_t cognitive = 0;
    std::uint32_t nesting_level = 0;
    
    // 簡易実装: ネストレベルを考慮した複雑度
    auto lines = utils::split_lines(content);
    
    for (const auto& line : lines) {
        std::string trimmed = utils::trim(line);
        
        // ネストレベル更新
        if (trimmed.find('{') != std::string::npos) {
            nesting_level++;
        }
        if (trimmed.find('}') != std::string::npos && nesting_level > 0) {
            nesting_level--;
        }
        
        // 制御構造検出
        for (const auto& keyword : control_keywords_) {
            if (trimmed.find(keyword) != std::string::npos) {
                cognitive += (1 + nesting_level); // ネストレベル分ペナルティ
                break;
            }
        }
    }
    
    return cognitive;
}

std::uint32_t ComplexityCalculator::calculate_max_nesting_depth(const std::string& content) {
    std::uint32_t max_depth = 0;
    std::uint32_t current_depth = 0;
    
    for (char c : content) {
        if (c == '{') {
            current_depth++;
            max_depth = std::max(max_depth, current_depth);
        } else if (c == '}' && current_depth > 0) {
            current_depth--;
        }
    }
    
    return max_depth;
}

//=============================================================================
// 📄 File Scanner Implementation
//=============================================================================

FileScanner::FileScanner(const AnalysisConfig& config) : config_(config) {}

std::vector<FilePath> FileScanner::scan_directory(const FilePath& directory_path) {
    std::vector<FilePath> files;
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory_path)) {
            if (entry.is_regular_file() && 
                is_javascript_file(entry.path()) && 
                !should_exclude(entry.path())) {
                files.push_back(entry.path());
                stats_.javascript_files++;
            }
            stats_.total_files_found++;
        }
    } catch (const std::filesystem::filesystem_error&) {
        // ディレクトリアクセスエラーは無視
    }
    
    return files;
}

std::vector<FilePath> FileScanner::scan_directory_parallel(const FilePath& directory_path) {
    // 単純実装: 現在は通常のスキャンと同じ
    // 本格実装では並列ディレクトリ走査が必要
    return scan_directory(directory_path);
}

bool FileScanner::is_javascript_file(const FilePath& file_path) const {
    std::string extension = file_path.extension().string();
    
    return std::find(config_.included_extensions.begin(), 
                    config_.included_extensions.end(), 
                    extension) != config_.included_extensions.end();
}

bool FileScanner::should_exclude(const FilePath& file_path) const {
    std::string path_str = file_path.string();
    
    for (const auto& pattern : config_.excluded_patterns) {
        if (wildcard_match(pattern, path_str)) {
            return true;
        }
    }
    
    return false;
}

bool FileScanner::is_file_too_large(const FilePath& file_path, FileSize max_size) const {
    try {
        return std::filesystem::file_size(file_path) > max_size;
    } catch (const std::filesystem::filesystem_error&) {
        return true; // エラー時は大きすぎると判定
    }
}

std::vector<FilePath> FileScanner::filter_files(const std::vector<FilePath>& files) {
    std::vector<FilePath> filtered_files;
    for (const auto& file : files) {
        if (is_javascript_file(file) && !should_exclude(file)) {
            filtered_files.push_back(file);
        }
    }
    return filtered_files;
}

bool FileScanner::wildcard_match(const std::string& pattern, const std::string& text) const {
    // 簡易ワイルドカードマッチング
    return text.find(pattern) != std::string::npos;
}

//=============================================================================
// 🎯 Utility Functions Implementation
//=============================================================================

namespace utils {

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::vector<std::string> split_lines(const std::string& content) {
    std::vector<std::string> lines;
    std::stringstream ss(content);
    std::string line;
    
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }
    
    return lines;
}

std::string remove_comments(const std::string& content) {
    std::string result = content;
    
    // 単行コメント削除
    std::regex single_comment_regex(R"(//.*$)", std::regex_constants::multiline);
    result = std::regex_replace(result, single_comment_regex, "");
    
    // 複数行コメント削除
    std::regex multi_comment_regex(R"(/\*[\s\S]*?\*/)");
    result = std::regex_replace(result, multi_comment_regex, "");
    
    return result;
}

std::string remove_string_literals(const std::string& content) {
    std::string result = content;
    
    // 文字列リテラル削除（簡易実装）
    std::regex string_regex(R"(["'][^"']*["'])");
    result = std::regex_replace(result, string_regex, "");
    
    return result;
}

Result<std::string> read_file(const FilePath& file_path) {
    try {
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            return Result<std::string>(AnalysisError(ErrorCode::FILE_NOT_FOUND, 
                "Cannot open file: " + file_path.string()));
        }
        
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::string content(size, '\0');
        file.read(&content[0], size);
        
        return Result<std::string>(std::move(content));
        
    } catch (const std::exception& e) {
        return Result<std::string>(AnalysisError(ErrorCode::UNKNOWN_ERROR, e.what()));
    }
}

Result<FileInfo> get_basic_file_info(const FilePath& file_path) {
    try {
        FileInfo info(file_path);
        
        if (!std::filesystem::exists(file_path)) {
            return Result<FileInfo>(AnalysisError(ErrorCode::FILE_NOT_FOUND, 
                "File not found: " + file_path.string()));
        }
        
        info.size_bytes = std::filesystem::file_size(file_path);
        
        // ファイル読み込みして行数カウント
        auto content_result = read_file(file_path);
        if (content_result.is_error()) {
            return Result<FileInfo>(content_result.error());
        }
        
        auto lines = split_lines(content_result.value());
        info.total_lines = static_cast<uint32_t>(lines.size());
        
        return Result<FileInfo>(std::move(info));
        
    } catch (const std::exception& e) {
        return Result<FileInfo>(AnalysisError(ErrorCode::UNKNOWN_ERROR, e.what()));
    }
}

std::string format_timestamp(const Timestamp& timestamp) {
    auto time = std::chrono::system_clock::to_time_t(timestamp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string format_file_size(FileSize size) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    double file_size = static_cast<double>(size);
    int unit_index = 0;
    
    while (file_size >= 1024.0 && unit_index < 3) {
        file_size /= 1024.0;
        unit_index++;
    }
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << file_size << " " << units[unit_index];
    return ss.str();
}

} // namespace utils

} // namespace nekocode