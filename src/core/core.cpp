#define NEKOCODE_FOUNDATION_CORE_CPP  // 基盤処理としてregex使用許可
#include "nekocode/core.hpp"
#include "nekocode/formatters.hpp"
#include "nekocode/utf8_utils.hpp"
#include "nekocode/language_detection.hpp"
#include "nekocode/cpp_analyzer.hpp"
#include "nekocode/tree_sitter_analyzer.hpp"
#include "nekocode/pegtl_analyzer.hpp"
// #include "nekocode/analyzers/csharp_analyzer.hpp" // regex版は削除済み
#include "../analyzers/base_analyzer.hpp"
#include "../utils/file_size_reporter.hpp"
#include "hybrid_stack_manager.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <execution>
#include <iostream>
#include <filesystem>
#include <regex>
#include <unordered_set>
#include <thread>
#include <nlohmann/json.hpp>
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
    std::unique_ptr<PEGTLAnalyzer> pegtl_analyzer_;
    std::unique_ptr<CppAnalyzer> cpp_analyzer_;
    std::unique_ptr<LanguageDetector> language_detector_;
    std::unique_ptr<FileScanner> file_scanner_;
    ProgressCallback progress_callback_;
    std::atomic<bool> parallel_enabled_{true};
    std::atomic<uint32_t> thread_count_{0};
    std::atomic<uint32_t> io_threads_{4};     // 🆕 同時ファイル読み込み数
    std::atomic<uint32_t> cpu_threads_{0};    // 🆕 解析スレッド数
    mutable std::mutex metrics_mutex_;

    explicit Impl(const AnalysisConfig& config) 
        : config_(config)
        , tree_sitter_analyzer_(std::make_unique<TreeSitterAnalyzer>())
        , pegtl_analyzer_(std::make_unique<PEGTLAnalyzer>())
        , cpp_analyzer_(std::make_unique<CppAnalyzer>())
        , language_detector_(std::make_unique<LanguageDetector>())
        , file_scanner_(std::make_unique<FileScanner>(config)) 
    {
        thread_count_ = config.max_threads;
        io_threads_ = config.io_threads;
        cpu_threads_ = config.cpu_threads;
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
        // 🎯 Hybrid Stack Manager による巨大ファイル対応
        HybridStackManager stack_mgr;
        
        return stack_mgr.analyze_with_smart_stack(file_path, [&]() -> Result<AnalysisResult> {
            // ファイル読み込み
            auto content_result = utils::read_file(file_path);
            if (content_result.is_error()) {
                return Result<AnalysisResult>(content_result.error());
            }
            
            return analyze_content(content_result.value(), file_path.string());
        });
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
        // 言語自動検出
        FilePath file_path(filename);
        Language detected_language = impl_->language_detector_->detect_language(file_path, content);
        
        // 🌳 Tree-sitter統一解析 - 全言語対応！
        AnalysisResult result;
        
        // ファイル基本情報解析
        result.file_info = analyze_file_structure(content, file_path);
        result.language = detected_language;
        
        if (impl_->config_.analyze_complexity) {
            result.complexity = analyze_complexity(content);
        }
        
        // 🔥 PEGTL解析（全言語統一・高精度）
        if (detected_language == Language::JAVASCRIPT || 
            detected_language == Language::TYPESCRIPT || 
            detected_language == Language::CPP || 
            detected_language == Language::C ||
            detected_language == Language::PYTHON) {
            auto pegtl_result = impl_->pegtl_analyzer_->analyze(content, filename, detected_language);
            if (pegtl_result.is_success()) {
                auto pg_result = pegtl_result.value();
                result.classes = pg_result.classes;
                result.functions = pg_result.functions;
                result.imports = pg_result.imports;
                result.exports = pg_result.exports;
                if (impl_->config_.analyze_complexity) {
                    result.complexity = pg_result.complexity;
                }
                
                // 🆕 CRITICAL FIX: commented_lines配列をコピー（欠落していた！）
                result.commented_lines = pg_result.commented_lines;
                
                // 🚀 Phase 5: Universal Symbolsをコピー（必須！）
                if (pg_result.universal_symbols) {
                    result.universal_symbols = pg_result.universal_symbols;
                    std::cerr << "[DEBUG core.cpp] Copied universal_symbols from PEGTL result" << std::endl;
                } else {
                    std::cerr << "[DEBUG core.cpp] No universal_symbols in PEGTL result" << std::endl;
                }
            }
        }
        
        // 🔥 PEGTLが既に依存関係と複雑度を解析済み
        // 従来の正規表現ベース・Tree-sitterプレースホルダーは不要
        
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
            // 🎯 Hybrid Stack Manager による巨大ファイル対応
            HybridStackManager stack_mgr;
            
            return stack_mgr.analyze_with_smart_stack(file_path, [&]() -> Result<MultiLanguageAnalysisResult> {
                // 🎯 大ファイル進捗表示（Claude Code向け）
                size_t file_size = 0;
                try {
                    file_size = std::filesystem::file_size(file_path);
                    if (FileSizeReporter::is_large_file(file_size)) {
                        FileSizeReporter::report_large_file_start(file_path.filename().string(), file_size);
                    }
                } catch (...) {
                    // ファイルサイズ取得失敗は無視して続行
                }
                
                // UTF-8 safe file reading
                auto safe_content = utf8::read_file_safe_utf8(file_path.string());
                if (!safe_content.conversion_success) {
                    return Result<MultiLanguageAnalysisResult>(
                        AnalysisError(ErrorCode::FILE_NOT_FOUND, safe_content.error_message, file_path));
                }
                
                // 言語自動検出
                Language detected_lang = impl_->language_detector_->detect_language(file_path, safe_content.content);
                
                auto analysis_result = analyze_content_multilang(safe_content.content, file_path.string(), detected_lang);
                
                // 🎯 大ファイル処理完了通知
                if (FileSizeReporter::is_large_file(file_size)) {
                    FileSizeReporter::report_large_file_complete(file_path.filename().string());
                }
                
                return analysis_result;
            });
            
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
            
            return analyze_content_multilang(safe_content.content, file_path.string(), language);
            
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
                // 🔥 PEGTL版アナライザーを使用（Claude Code支援作戦）
                auto analyzer = AnalyzerFactory::create_analyzer(result.detected_language);
                if (analyzer) {
                    auto analysis_result = analyzer->analyze(content, filename);
                    
                    // CppAnalysisResultを作成（PEGTL結果から変換）
                    CppAnalysisResult cpp_result;
                    cpp_result.file_info = analysis_result.file_info;
                    cpp_result.complexity = analysis_result.complexity;
                    
                    // 🔥 CRITICAL FIX: 統計情報を直接コピー（変換で失われるのを防ぐ）
                    cpp_result.stats = analysis_result.stats;
                    
                    // 🆕 CRITICAL FIX: commented_lines配列をコピー（欠落していた！）
                    cpp_result.commented_lines = analysis_result.commented_lines;
                    
                    // クラス・関数情報を変換
                    for (const auto& cls : analysis_result.classes) {
                        // デバッグクラスを除外
                        if (cls.name == "CPP_PEGTL_ANALYZER_CALLED") {
                            continue;
                        }
                        
                        CppClass cpp_class;
                        cpp_class.name = cls.name;
                        cpp_class.start_line = cls.start_line;
                        cpp_class.end_line = cls.end_line;
                        cpp_class.class_type = CppClass::CLASS; // デフォルト
                        
                        // 🔥 メンバ変数情報を変換
                        for (const auto& member : cls.member_variables) {
                            // MemberVariable -> string 変換（CppClassはstd::vector<std::string>）
                            cpp_class.member_variables.push_back(member.name);
                        }
                        
                        cpp_result.cpp_classes.push_back(cpp_class);
                    }
                    
                    for (const auto& func : analysis_result.functions) {
                        CppFunction cpp_func;
                        cpp_func.name = func.name;
                        cpp_func.start_line = func.start_line;
                        cpp_func.end_line = func.end_line;
                        // CppFunctionのreturn_typeを使用
                        cpp_func.return_type = "auto"; // PEGTLでは型推定が困難なのでデフォルト
                        // access_levelフィールドは存在しない
                        cpp_result.cpp_functions.push_back(cpp_func);
                    }
                    
                    // 🔥 統計再計算は不要！PEGTL結果を信頼
                    // cpp_result.update_statistics(); // ← これが問題の原因だった！
                    
                    // デバッグ出力削除（JSON出力に干渉するため）
                    // std::cerr << "✅ Stats preserved: classes=" << cpp_result.stats.class_count 
                    //           << ", functions=" << cpp_result.stats.function_count << std::endl;
                    
                    result.cpp_result = cpp_result;
                    result.file_info = analysis_result.file_info;
                    
                    // std::cerr << "✅ C++ PEGTL analyzer used successfully!" << std::endl;
                } else {
                    // フォールバック: 従来のCppAnalyzer
                    auto cpp_result = impl_->cpp_analyzer_->analyze_cpp_file(content, filename);
                    result.cpp_result = cpp_result;
                    result.file_info = cpp_result.file_info;
                    // std::cerr << "⚠️  Fallback to old CppAnalyzer" << std::endl;
                }
                break;
            }
            
            case Language::CSHARP: {
                // 🎮 Unity content detection + C# 解析
                // Unity analyzer を優先的に試行
                auto analyzer = AnalyzerFactory::create_unity_analyzer_from_file(filename, content);
                
                if (analyzer) {
                    auto csharp_result = analyzer->analyze(content, filename);
                    result.csharp_result = csharp_result;
                    result.file_info = csharp_result.file_info;
                    
                    // Unity analyzer が使用されたかをログ出力
                    if (content.find("using UnityEngine") != std::string::npos || 
                        content.find(": MonoBehaviour") != std::string::npos) {
                        std::cerr << "🎮 Unity analyzer used for: " << filename << std::endl;
                    } else {
                        std::cerr << "⚙️ C# PEGTL analyzer used for: " << filename << std::endl;
                    }
                } else {
                    // Unity判定に失敗した場合は通常のC#アナライザーを使用
                    analyzer = AnalyzerFactory::create_analyzer(Language::CSHARP);
                    if (analyzer) {
                        auto csharp_result = analyzer->analyze(content, filename);
                        result.csharp_result = csharp_result;
                        result.file_info = csharp_result.file_info;
                        std::cerr << "⚙️ C# PEGTL analyzer used (fallback) for: " << filename << std::endl;
                    } else {
                        std::cerr << "ERROR: Failed to create C# analyzer for: " << filename << std::endl;
                    }
                }
                break;
            }
            
            case Language::GO: {
                // 🐹 Go言語解析（Goroutine & Channel Detection）
                auto analyzer = AnalyzerFactory::create_analyzer(result.detected_language);
                if (analyzer) {
                    auto go_result = analyzer->analyze(content, filename);
                    
                    // Go固有の結果をJS結果として格納（共通構造がないため）
                    result.js_result = go_result;
                    result.file_info = go_result.file_info;
                    
                    std::cerr << "🐹 Go analyzer used successfully for: " << filename << std::endl;
                } else {
                    std::cerr << "ERROR: Failed to create Go analyzer for: " << filename << std::endl;
                }
                break;
            }
            
            case Language::RUST: {
                // 🦀 Rust言語解析（trait, impl, macro Detection）
                auto analyzer = AnalyzerFactory::create_analyzer(result.detected_language);
                if (analyzer) {
                    auto rust_result = analyzer->analyze(content, filename);
                    
                    // 🔧 Rust固有の結果を正しいフィールドに格納
                    result.rust_result = rust_result;
                    result.file_info = rust_result.file_info;
                    
                    // Rust analyzer successfully created
                } else {
                    // Failed to create Rust analyzer
                }
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
        
        // 🎯 完全解析モード: デッドコード検出を実行
        if (impl_->config_.complete_analysis) {
            perform_complete_analysis(result, filename);
        }
        
        return Result<MultiLanguageAnalysisResult>(std::move(result));
        
    } catch (const std::exception& e) {
        return Result<MultiLanguageAnalysisResult>(
            AnalysisError(ErrorCode::PARSING_ERROR, e.what()));
    }
}

//=============================================================================
// 🎯 Complete Analysis Implementation - 完全解析処理
//=============================================================================

void NekoCodeCore::perform_complete_analysis(MultiLanguageAnalysisResult& result, const std::string& filename) {
    try {
        // 🐍 Pythonスクリプトを呼び出してデッドコード検出
        std::string command = "python3 src/tools/universal_deadcode_analyzer.py \"" + filename + "\" --complete";
        
        // システムコマンド実行
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::cerr << "⚠️ Failed to execute dead code analysis for: " << filename << std::endl;
            return;
        }
        
        // 結果読み取り
        std::string output;
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }
        pclose(pipe);
        
        // JSON結果を探す（Pythonスクリプトは装飾付きで出力）
        size_t json_start = output.rfind("{");
        if (json_start != std::string::npos) {
            // JSON部分を抽出
            std::string json_str = output.substr(json_start);
            size_t json_end = json_str.find_last_of("}");
            if (json_end != std::string::npos) {
                json_str = json_str.substr(0, json_end + 1);
                
                try {
                    // JSONパース
                    nlohmann::json dead_code_result = nlohmann::json::parse(json_str);
                    
                    // dead_code情報を取得
                    if (dead_code_result.contains("dead_code") && 
                        dead_code_result["dead_code"].contains("status")) {
                        
                        // メタデータに詳細情報を保存
                        std::string dead_code_json = dead_code_result["dead_code"].dump();
                        
                        // 言語別にメタデータに保存
                        if (result.cpp_result.has_value()) {
                            result.cpp_result->file_info.metadata["dead_code"] = dead_code_json;
                        } 
                        if (result.js_result.has_value()) {
                            result.js_result->file_info.metadata["dead_code"] = dead_code_json;
                        } 
                        if (result.csharp_result.has_value()) {
                            result.csharp_result->file_info.metadata["dead_code"] = dead_code_json;
                        } 
                        if (result.rust_result.has_value()) {
                            result.rust_result->file_info.metadata["dead_code"] = dead_code_json;
                        }
                        // 共通メタデータにも保存（どの言語でも参照可能）
                        result.file_info.metadata["dead_code"] = dead_code_json;
                        
                        std::cerr << "✅ Dead code analysis completed for: " << filename << std::endl;
                        
                        // 検出数を表示
                        if (dead_code_result["dead_code"].contains("total_found")) {
                            int total_found = dead_code_result["dead_code"]["total_found"];
                            if (total_found > 0) {
                                std::cerr << "   🔍 Found " << total_found << " dead code items" << std::endl;
                            }
                        }
                    } else {
                        // std::cerr << "⚠️ Dead code analysis returned no results for: " << filename << std::endl;
                    }
                } catch (const nlohmann::json::exception& e) {
                    // JSON出力に干渉するため無効化
                    // std::cerr << "⚠️ Failed to parse dead code analysis JSON: " << e.what() << std::endl;
                }
            }
        } else {
            // std::cerr << "⚠️ No JSON output from dead code analysis for: " << filename << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Complete analysis error: " << e.what() << std::endl;
    }
}

//=============================================================================
// 📁 Directory Analysis
//=============================================================================

Result<DirectoryAnalysis> NekoCodeCore::analyze_directory(const FilePath& directory_path) {
    // 🐛 TEMPORARY FIX: 並列処理を強制無効化してセグフォルト回避
    if (false && impl_->parallel_enabled_) {
        std::cerr << "🔄 Using parallel processing path" << std::endl;
        return analyze_directory_parallel(directory_path);
    }
    std::cerr << "🐌 Using sequential processing path" << std::endl;
    
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
            auto files = impl_->file_scanner_->scan_directory(directory_path);
            auto js_files = impl_->file_scanner_->filter_files(files);
            
            // 🚀 新しい並列化戦略: I/O並列度とCPU並列度を分離
            std::vector<AnalysisResult> results(js_files.size());
            std::vector<bool> success_flags(js_files.size(), false);
            
            // I/O並列度を制限するための簡易実装（C++17互換）
            std::atomic<size_t> active_io_count{0};
            const size_t max_io_threads = impl_->io_threads_;
            
            // デバッグ用: 開始時刻を記録
            auto start_time = std::chrono::high_resolution_clock::now();
            
            // CPU並列度の制御（execution policyで自動調整）
            // TODO: 将来的にはthread poolでより細かく制御
            
            // 並列処理用のインデックスベクトル作成
            std::vector<size_t> indices(js_files.size());
            std::iota(indices.begin(), indices.end(), 0);
            
            std::for_each(std::execution::par_unseq, 
                indices.begin(), indices.end(),
                [&](size_t i) {
                    // I/O並列度制限（簡易実装）
                    while (active_io_count.load() >= max_io_threads) {
                        std::this_thread::yield();  // 待機
                    }
                    active_io_count.fetch_add(1);
                    
                    // ファイル解析実行
                    auto file_result = analyze_file(js_files[i]);
                    
                    // I/Oカウント減少
                    active_io_count.fetch_sub(1);
                    
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
            
            // デバッグ用: 処理時間を記録
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            std::cerr << "🚀 Parallel analysis completed: " 
                      << js_files.size() << " files in " 
                      << duration.count() << "ms with --io-threads=" 
                      << max_io_threads << std::endl;
            
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