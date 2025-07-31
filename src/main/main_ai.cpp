//=============================================================================
// 🤖 NekoCode AI Tool - Claude Code最適化実行ファイル
//
// 実行ファイル２個大作戦: AI専用ツール
//
// 特徴:
// - JSON構造化出力
// - Claude Code最適化
// - 高速処理優先
// - コンパクトデータ
//=============================================================================

#include "nekocode/core.hpp"
#include "nekocode/formatters.hpp"
#include "nekocode/session_manager.hpp"
#include "nekocode/progress_tracker.hpp"
#include "nekocode/command_dispatcher.hpp"
#include "nekocode/command_line_args.hpp"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <unistd.h>
#include <sys/wait.h>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace nekocode;

// 🔧 グローバルデバッグフラグ（analyzer_factory.cppで定義済み）
extern bool g_debug_mode;

//=============================================================================
// 📋 Command Line Parser - 共通ヘッダーを使用
//=============================================================================

//=============================================================================
// 📖 Help Display
//=============================================================================

void show_supported_languages() {
    nlohmann::json langs_json;
    langs_json["supported_languages"] = {
        {"javascript", {{"name", "JavaScript"}, {"extensions", {".js", ".mjs", ".jsx"}}}},
        {"typescript", {{"name", "TypeScript"}, {"extensions", {".ts", ".tsx", ".mts", ".cts"}}}},
        {"cpp", {{"name", "C++"}, {"extensions", {".cpp", ".cxx", ".cc", ".hpp", ".hxx", ".hh", ".h"}}}},
        {"c", {{"name", "C"}, {"extensions", {".c", ".h"}}}},
        {"python", {{"name", "Python"}, {"extensions", {".py", ".pyw", ".pyi"}}}},
        {"csharp", {{"name", "C#"}, {"extensions", {".cs", ".csx"}}}}
    };
    langs_json["auto_detection"] = true;
    langs_json["utf8_support"] = true;
    langs_json["unicode_identifiers"] = true;
    std::cout << langs_json.dump(2) << std::endl;
}

void show_help() {
    std::cout << R"(🤖 NekoCode AI Tool - 多言語対応Claude Code最適化版

USAGE:
    nekocode_ai <action> [args] [options]

ACTIONS:
    analyze <path>              単発解析（旧形式互換）
    session-create <path>       対話式セッション作成
    session-create-async <path> 🚀 非同期セッション作成（大規模プロジェクト用）
    session-status <id>         📊 セッション状態確認（非同期用）
    session-cmd <id> <cmd>      セッションコマンド実行
    <path>                      単発解析（後方互換）

INTERACTIVE COMMANDS:
    stats                       統計情報表示
    files                       ファイル一覧
    complexity                  複雑度分析
    complexity --methods [file] ファイル別メソッド複雑度ランキング
    large-files [--threshold N] 大きいファイル一覧（デフォルト500行以上）
    duplicates                  重複・バックアップファイル検出
    todo                        TODO/FIXME/BUGコメント検出
    complexity-ranking          関数複雑度ランキング（トップ50）
    structure                   構造解析（クラス・関数）
    structure --detailed [file] 詳細構造解析（クラス・メソッド情報）
    calls                       関数呼び出し分析
    calls --detailed <function> 特定関数の詳細呼び出し関係
    find <term> [options]       検索（--debug --limit N --function --variable）
    help                        コマンドヘルプ

OPTIONS:
    -h, --help          このヘルプを表示
    --compact           コンパクトJSON出力（改行なし）
    --stats-only        統計情報のみ出力（高速）
    --no-parallel       並列処理無効化
    --io-threads <N>    同時ファイル読み込み数（SSD: 8-16, HDD: 1-2, デフォルト: 4）
    --cpu-threads <N>   解析スレッド数（デフォルト: CPUコア数）
    --performance       パフォーマンス統計表示
    --format <type>     出力フォーマット (json|compact|stats)
    --lang <language>   言語指定 (auto|js|ts|cpp|c|python|csharp)
    --list-languages    サポート言語一覧表示
    --progress          進捗表示有効化（30,000ファイル対応）
    --debug             デバッグログ表示モード（詳細情報を表示）
    --no-check          事前チェックをスキップ（上級者向け）
    --force             確認なしで強制実行
    --check-only        プロジェクト規模のチェックのみ実行

SUPPORTED LANGUAGES:
    🟨 JavaScript       (.js, .mjs, .jsx)
    🔵 TypeScript       (.ts, .tsx, .mts, .cts)
    🔴 C++              (.cpp, .cxx, .cc, .hpp, .hxx, .hh, .h)
    ⚫ C                (.c, .h)
    🐍 Python           (.py, .pyw, .pyi)
    🟣 C#               (.cs, .csx)

EXAMPLES:
    # 🎮 対話式セッション作成
    nekocode_ai session-create charmflow_v5/
    nekocode_ai session-cmd ai_session_20250727_123456 stats
    nekocode_ai session-cmd ai_session_20250727_123456 complexity
    nekocode_ai session-cmd ai_session_20250727_123456 large-files
    nekocode_ai session-cmd ai_session_20250727_123456 "large-files --threshold 1000"
    nekocode_ai session-cmd ai_session_20250727_123456 duplicates
    nekocode_ai session-cmd ai_session_20250727_123456 todo
    nekocode_ai session-cmd ai_session_20250727_123456 complexity-ranking
    nekocode_ai session-cmd ai_session_20250727_123456 "find nyamesh --debug"
    nekocode_ai session-cmd ai_session_20250727_123456 "find std::cout --limit 10"
    
    # 🔍 Claude Code君向け詳細解析（NEW!）
    nekocode_ai session-cmd ai_session_20250727_123456 "structure --detailed UICore.cpp"
    nekocode_ai session-cmd ai_session_20250727_123456 "complexity --methods UICore.cpp"
    nekocode_ai session-cmd ai_session_20250727_123456 "calls --detailed createElement"
    
    # 🚀 大規模プロジェクト非同期処理（Claude Code最適化）
    nekocode_ai session-create-async large_project/ --progress
    nekocode_ai session-status ai_session_20250729_123456
    # （解析完了後）
    nekocode_ai session-cmd ai_session_20250729_123456 stats
    
    # 🔍 事前チェック機能
    nekocode_ai session-create typescript/TypeScript/ --check-only  # サイズ確認のみ
    nekocode_ai session-create huge_project/ --force               # 確認なしで実行
    nekocode_ai session-create auto_script/ --no-check             # チェックスキップ

    # 🔥 地獄のC++プロジェクト解析
    nekocode_ai analyze nyamesh_v22/ --lang cpp

    # 🌍 多言語プロジェクト自動検出
    nekocode_ai src/ --cpu-threads 8

    # 🤖 Claude用最適化出力
    nekocode_ai EditorCore_v22.cpp --compact

    # ⚡ 大規模プロジェクト高速統計
    nekocode_ai large_cpp_project/ --stats-only

    # 📊 サポート言語確認
    nekocode_ai --list-languages

OUTPUT:
    マルチ言語対応構造化JSON - Claude Codeでの解析に最適化

MULTI-LANGUAGE FEATURES:
    🌍 UTF-8完全対応 (日本語・Unicode)
    🔥 C++大規模プロジェクト対応
    ⚡ 言語別最適化エンジン
    🎯 実行ファイル２個大作戦 - AI専用

革命的多言語解析エンジン 🚀✨
)";
}

//=============================================================================
// ⚡ Performance Reporter
//=============================================================================

void show_performance_report(const PerformanceMetrics& metrics) {
    nlohmann::json perf_json;
    
    perf_json["performance"] = {
        {"analysis_time_ms", metrics.analysis_time.count()},
        {"files_processed", metrics.files_processed},
        {"lines_processed", metrics.lines_processed},
        {"bytes_processed", metrics.bytes_processed},
        {"throughput", {
            {"files_per_second", metrics.files_per_second()},
            {"lines_per_second", metrics.lines_per_second()},
            {"megabytes_per_second", metrics.megabytes_per_second()}
        }}
    };
    
    std::cerr << "\n🔥 Performance Report:\n" << perf_json.dump(2) << std::endl;
}

//=============================================================================
// 🚀 Main Function
//=============================================================================

// 対話式コマンド処理関数
int analyze_target(const std::string& target_path, const CommandLineArgs& args = CommandLineArgs());
int create_session(const std::string& target_path, const CommandLineArgs& args = CommandLineArgs{});
int create_session_async(const std::string& target_path, const CommandLineArgs& args = CommandLineArgs{});
int check_session_status(const std::string& session_id);
int execute_session_command(const std::string& session_id, const std::string& command);

int main(int argc, char* argv[]) {
    try {
        nekocode::CommandDispatcher dispatcher;
        return dispatcher.dispatch(argc, argv);
    } catch (const std::exception& e) {
        nlohmann::json error_json;
        error_json["error"] = {
            {"code", 500},
            {"message", e.what()},
            {"type", "exception"}
        };
        std::cout << error_json.dump(2) << std::endl;
        return 1;
    }
}

//=============================================================================
// 🎯 analyze_target実装
//=============================================================================

int analyze_target(const std::string& target_path, const CommandLineArgs& args) {
    try {
        // 🔧 グローバルデバッグフラグ設定
        g_debug_mode = args.debug_mode;
        
        // 設定作成（フルモード）
        AnalysisConfig config;
        config.analyze_complexity = true;  // 正規表現問題解決済み
        config.analyze_dependencies = true;
        config.analyze_function_calls = true;
        config.enable_parallel_processing = args.enable_parallel;
        
        // 新しい並列化設定
        config.io_threads = args.io_threads;
        config.cpu_threads = args.cpu_threads;
        
        // 自動ストレージモード設定
        config.storage_mode = StorageMode::AUTO;
        
        // ストレージモード別スレッド数再計算
        config.calculate_optimal_threads();
        
        // 解析エンジン作成
        NekoCodeCore analyzer(config);
        
        // パフォーマンス測定開始
        auto start_time = std::chrono::steady_clock::now();
        
        // AI専用フォーマッター作成
        auto formatter = FormatterFactory::create_formatter(OutputFormat::AI_JSON);
        
        std::filesystem::path path(target_path);
        
        if (std::filesystem::is_regular_file(path)) {
            //=========================================================================
            // 📄 Single File Analysis
            //=========================================================================
            
            auto result = analyzer.analyze_file_multilang(path);
            
            if (result.is_error()) {
                nlohmann::json error_json;
                error_json["error"] = {
                    {"code", static_cast<int>(result.error().code)},
                    {"message", result.error().message},
                    {"file_path", path.string()}
                };
                std::cout << error_json.dump(2) << std::endl;
                return 1;
            }
            
            // 結果からAnalysisResultを取得
            AnalysisResult analysis_result;
            auto multilang_result = result.value();
            
            if (multilang_result.csharp_result) {
                analysis_result = multilang_result.csharp_result.value();
            } else if (multilang_result.js_result) {
                analysis_result = multilang_result.js_result.value();
            } else if (multilang_result.cpp_result) {
                // 🔥 C++結果をAnalysisResultに手動変換（構造体が異なるため）
                auto cpp_result = multilang_result.cpp_result.value();
                analysis_result.file_info = cpp_result.file_info;
                analysis_result.complexity = cpp_result.complexity;
                analysis_result.stats = cpp_result.stats;
                analysis_result.language = Language::CPP;
                
                // C++クラス情報を変換
                for (const auto& cpp_class : cpp_result.cpp_classes) {
                    ClassInfo class_info;
                    class_info.name = cpp_class.name;
                    class_info.start_line = cpp_class.start_line;
                    class_info.end_line = cpp_class.end_line;
                    
                    // 🔥 メンバ変数情報を変換
                    for (const auto& member_name : cpp_class.member_variables) {
                        MemberVariable member;
                        member.name = member_name;
                        member.type = "auto"; // C++では型推定困難なのでデフォルト
                        member.access_modifier = "private"; // デフォルト
                        class_info.member_variables.push_back(member);
                    }
                    
                    analysis_result.classes.push_back(class_info);
                }
                
                // C++関数情報を変換
                for (const auto& cpp_func : cpp_result.cpp_functions) {
                    FunctionInfo func_info;
                    func_info.name = cpp_func.name;
                    func_info.start_line = cpp_func.start_line;
                    func_info.end_line = cpp_func.end_line;
                    analysis_result.functions.push_back(func_info);
                }
            } else {
                // フォールバック
                analysis_result.file_info = multilang_result.file_info;
                analysis_result.language = multilang_result.detected_language;
            }
            
            // 出力フォーマット選択
            std::string output = formatter->format_single_file(analysis_result);
            
            std::cout << output << std::endl;
            
        } else if (std::filesystem::is_directory(path)) {
            //=========================================================================
            // 📁 Directory Analysis
            //=========================================================================
            
            auto result = analyzer.analyze_directory(path);
            
            if (result.is_error()) {
                nlohmann::json error_json;
                error_json["error"] = {
                    {"code", static_cast<int>(result.error().code)},
                    {"message", result.error().message},
                    {"directory_path", path.string()}
                };
                std::cout << error_json.dump(2) << std::endl;
                return 1;
            }
            
            // ディレクトリ解析結果出力
            std::string output = formatter->format_directory(result.value());
            std::cout << output << std::endl;
            
        } else {
            nlohmann::json error_json;
            error_json["error"] = {
                {"code", 404},
                {"message", "File or directory not found"},
                {"path", path.string()}
            };
            std::cout << error_json.dump(2) << std::endl;
            return 1;
        }
        
        // パフォーマンス統計表示
        if (args.show_performance) {
            auto metrics = analyzer.get_performance_metrics();
            show_performance_report(metrics);
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        nlohmann::json error_json;
        error_json["error"] = {
            {"code", 500},
            {"message", e.what()},
            {"type", "exception"}
        };
        std::cout << error_json.dump(2) << std::endl;
        return 1;
    }
}

//=============================================================================
// 🎯 AI Tool Performance Notes
//
// このツールはClaude Code向けに最適化されています:
//
// 1. JSON出力: 構造化データでAI解析しやすい
// 2. 高速処理: 並列処理でレスポンス向上  
// 3. エラー情報: JSON形式で詳細エラー提供
// 4. コンパクト: --compactで最小データ転送
// 5. 統計特化: --stats-onlyで超高速概要取得
//
// Python版との比較:
// - 速度: 10-100倍高速
// - メモリ: 大幅削減
// - 精度: 型安全で高精度
// - 並列: マルチスレッド対応
//
// 実行ファイル２個大作戦: AI専用特化完了! 🤖
//=============================================================================

//=============================================================================
// 🎮 create_session実装
//=============================================================================

// 事前チェック用構造体
struct ProjectScanResult {
    size_t total_files = 0;
    size_t code_files = 0;
    size_t estimated_minutes = 0;
    std::string scale_category;
    bool proceed = true;
};

// セッションID生成関数
std::string generate_session_id() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    std::ostringstream oss;
    oss << "ai_session_" << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

// セッション状態ファイル作成/更新
void update_session_state(const std::string& session_id, const std::string& status, 
                          const std::string& target_path = "", pid_t pid = 0) {
    std::filesystem::create_directories("sessions");
    std::string state_file = "sessions/" + session_id + "_state.json";
    
    nlohmann::json state;
    state["session_id"] = session_id;
    state["status"] = status;
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    state["last_update"] = oss.str();
    
    if (!target_path.empty()) {
        state["target_path"] = target_path;
    }
    if (pid > 0) {
        state["pid"] = pid;
    }
    
    std::ofstream file(state_file);
    if (file.is_open()) {
        file << state.dump(2) << std::endl;
    }
}

// 高速ファイル数カウント
ProjectScanResult quick_project_scan(const std::filesystem::path& path, const CommandLineArgs& args) {
    ProjectScanResult result;
    
    if (args.skip_precheck) {
        result.proceed = true;
        return result;
    }
    
    std::cerr << "🔍 Quick project scan..." << std::endl;
    
    // ファイル数カウント
    for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
        if (entry.is_regular_file()) {
            result.total_files++;
            
            // コードファイル判定（簡易版）
            std::string ext = entry.path().extension().string();
            if (ext == ".ts" || ext == ".js" || ext == ".cpp" || ext == ".hpp" || 
                ext == ".py" || ext == ".cs" || ext == ".c" || ext == ".h") {
                result.code_files++;
            }
        }
    }
    
    // 時間予測（TypeScriptベース: 0.16秒/ファイル）
    result.estimated_minutes = (result.total_files * 0.16) / 60.0;
    
    // 規模分類
    if (result.total_files < 100) {
        result.scale_category = "Small";
    } else if (result.total_files < 1000) {
        result.scale_category = "Medium";
    } else if (result.total_files < 10000) {
        result.scale_category = "Large";
    } else {
        result.scale_category = "Massive";
    }
    
    // 結果表示
    std::cerr << "📊 Project Analysis:" << std::endl;
    std::cerr << "• Total files: " << result.total_files << std::endl;
    std::cerr << "• Code files: " << result.code_files << std::endl;
    std::cerr << "• Scale: " << result.scale_category << std::endl;
    std::cerr << "• Estimated time: " << result.estimated_minutes << " minutes" << std::endl;
    
    // ユーザー確認（大規模プロジェクトの場合）
    if (result.total_files >= 1000 && !args.force_execution && !args.check_only) {
        std::cerr << std::endl;
        std::cerr << "⚠️  Large project detected!" << std::endl;
        std::cerr << "This will block Claude Code for ~" << result.estimated_minutes << " minutes." << std::endl;
        std::cerr << std::endl;
        std::cerr << "Continue? [y/N]: ";
        
        std::string response;
        std::getline(std::cin, response);
        
        if (response != "y" && response != "Y" && response != "yes") {
            result.proceed = false;
            std::cerr << "✅ Cancelled. Consider using --check-only or analyzing a subdirectory." << std::endl;
        }
    }
    
    return result;
}

int create_session(const std::string& target_path, const CommandLineArgs& args) {
    try {
        std::filesystem::path path(target_path);
        
        // 事前チェック実行
        if (std::filesystem::is_directory(path)) {
            auto scan_result = quick_project_scan(path, args);
            
            if (args.check_only) {
                // チェックのみモード
                std::cerr << "🎯 Analysis complete. Use without --check-only to proceed." << std::endl;
                return 0;
            }
            
            if (!scan_result.proceed) {
                return 1; // ユーザーがキャンセル
            }
        }
        
        // 設定作成
        AnalysisConfig config;
        config.analyze_complexity = true;
        config.analyze_dependencies = true;
        config.analyze_function_calls = true;
        config.enable_parallel_processing = args.enable_parallel;
        
        // 新しい並列化設定
        config.io_threads = args.io_threads;
        config.cpu_threads = args.cpu_threads;
        
        // 自動ストレージモード設定
        config.storage_mode = StorageMode::AUTO;
        
        // ストレージモード別スレッド数再計算
        config.calculate_optimal_threads();
        
        // 解析エンジン作成
        NekoCodeCore analyzer(config);
        SessionManager session_manager;
        std::string session_id;
        
        std::cerr << "🤖 NekoCode AI creating session: " << target_path << std::endl;
        
        if (std::filesystem::is_regular_file(path)) {
            auto result = analyzer.analyze_file_multilang(path);
            
            if (result.is_error()) {
                nlohmann::json error_json;
                error_json["error"] = {
                    {"code", static_cast<int>(result.error().code)},
                    {"message", result.error().message},
                    {"file_path", path.string()}
                };
                std::cout << error_json.dump(2) << std::endl;
                return 1;
            }
            
            // 結果からAnalysisResultを取得してセッション作成
            AnalysisResult analysis_result;
            auto multilang_result = result.value();
            
            if (multilang_result.csharp_result) {
                analysis_result = multilang_result.csharp_result.value();
            } else if (multilang_result.js_result) {
                analysis_result = multilang_result.js_result.value();
            } else if (multilang_result.cpp_result) {
                // 🔥 C++結果をAnalysisResultに手動変換（構造体が異なるため）
                auto cpp_result = multilang_result.cpp_result.value();
                analysis_result.file_info = cpp_result.file_info;
                analysis_result.complexity = cpp_result.complexity;
                analysis_result.stats = cpp_result.stats;
                analysis_result.language = Language::CPP;
                
                // C++クラス情報を変換
                for (const auto& cpp_class : cpp_result.cpp_classes) {
                    ClassInfo class_info;
                    class_info.name = cpp_class.name;
                    class_info.start_line = cpp_class.start_line;
                    class_info.end_line = cpp_class.end_line;
                    
                    // 🔥 メンバ変数情報を変換
                    for (const auto& member_name : cpp_class.member_variables) {
                        MemberVariable member;
                        member.name = member_name;
                        member.type = "auto"; // C++では型推定困難なのでデフォルト
                        member.access_modifier = "private"; // デフォルト
                        class_info.member_variables.push_back(member);
                    }
                    
                    analysis_result.classes.push_back(class_info);
                }
                
                // C++関数情報を変換
                for (const auto& cpp_func : cpp_result.cpp_functions) {
                    FunctionInfo func_info;
                    func_info.name = cpp_func.name;
                    func_info.start_line = cpp_func.start_line;
                    func_info.end_line = cpp_func.end_line;
                    analysis_result.functions.push_back(func_info);
                }
            } else {
                // フォールバック
                analysis_result.file_info = multilang_result.file_info;
                analysis_result.language = multilang_result.detected_language;
            }
            
            session_id = session_manager.create_session(path, analysis_result);
            
        } else if (std::filesystem::is_directory(path)) {
            // プログレス表示準備（session_id事前生成）
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto tm = *std::localtime(&time_t);
            std::ostringstream oss;
            oss << "ai_session_" << std::put_time(&tm, "%Y%m%d_%H%M%S");
            std::string temp_session_id = oss.str();
            SessionProgressTracker progress_tracker(temp_session_id, args.enable_progress);
            
            // ディレクトリ内ファイル数カウント
            size_t file_count = 0;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    file_count++;
                }
            }
            
            progress_tracker.start_directory_analysis(path, file_count);
            
            // プログレスコールバック設定
            if (args.enable_progress) {
                analyzer.set_progress_callback([&progress_tracker](std::uint32_t processed, std::uint32_t total, const std::string& current_file) {
                    // ファイルサイズは後で実装（とりあえず0）
                    progress_tracker.update_file_analysis(current_file, 0, true);
                });
            }
            
            auto result = analyzer.analyze_directory(path);
            
            progress_tracker.complete_analysis();
            
            if (result.is_error()) {
                nlohmann::json error_json;
                error_json["error"] = {
                    {"code", static_cast<int>(result.error().code)},
                    {"message", result.error().message},
                    {"directory_path", path.string()}
                };
                std::cout << error_json.dump(2) << std::endl;
                return 1;
            }
            
            session_id = session_manager.create_session(path, result.value());
            
        } else {
            nlohmann::json error_json;
            error_json["error"] = {
                {"code", 404},
                {"message", "File or directory not found"},
                {"path", path.string()}
            };
            std::cout << error_json.dump(2) << std::endl;
            return 1;
        }
        
        // セッション作成成功
        nlohmann::json result_json;
        result_json["session_id"] = session_id;
        result_json["commands"] = {"stats", "files", "complexity", "structure", "calls", "find <term>", "help"};
        result_json["message"] = "✅ AI Session created";
        
        std::cout << result_json.dump(2) << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        nlohmann::json error_json;
        error_json["error"] = {
            {"code", 500},
            {"message", std::string("Session creation failed: ") + e.what()},
            {"type", "exception"}
        };
        std::cout << error_json.dump(2) << std::endl;
        return 1;
    }
}

//=============================================================================
// 🎯 execute_session_command実装
//=============================================================================

int execute_session_command(const std::string& session_id, const std::string& command) {
    try {
        SessionManager session_manager;
        
        nlohmann::json result = session_manager.execute_command(session_id, command);
        
        std::cout << result.dump(2) << std::endl;
        
        return result.contains("error") ? 1 : 0;
        
    } catch (const std::exception& e) {
        nlohmann::json error_json;
        error_json["error"] = {
            {"code", 500},
            {"message", std::string("Command execution failed: ") + e.what()},
            {"type", "exception"}
        };
        std::cout << error_json.dump(2) << std::endl;
        return 1;
    }
}

//=============================================================================
// 🚀 create_session_async実装 - Claude Code最適化
//=============================================================================

int create_session_async(const std::string& target_path, const CommandLineArgs& args) {
    try {
        std::filesystem::path path(target_path);
        
        // セッションID生成
        std::string session_id = generate_session_id();
        
        // 事前チェック（--check-onlyの場合は同期実行）
        if (std::filesystem::is_directory(path)) {
            auto scan_result = quick_project_scan(path, args);
            
            if (args.check_only) {
                std::cerr << "🎯 Analysis complete. Use session-create-async without --check-only to proceed." << std::endl;
                return 0;
            }
            
            if (!scan_result.proceed) {
                return 1;
            }
        }
        
        // 初期状態ファイル作成
        update_session_state(session_id, "STARTING", target_path);
        
        std::cerr << "🚀 Starting async session: " << session_id << std::endl;
        
        // プロセス分離
        pid_t pid = fork();
        
        if (pid == -1) {
            // fork失敗
            update_session_state(session_id, "ERROR");
            nlohmann::json error_json;
            error_json["error"] = {
                {"code", 500},
                {"message", "Failed to create background process"},
                {"type", "fork_error"}
            };
            std::cout << error_json.dump(2) << std::endl;
            return 1;
        }
        else if (pid == 0) {
            //=================================================================
            // 子プロセス: 実際の解析実行
            //=================================================================
            
            // 状態更新
            update_session_state(session_id, "RUNNING", target_path, getpid());
            
            // 通常のセッション作成と同じ処理
            AnalysisConfig config;
            config.analyze_complexity = true;
            config.analyze_dependencies = true;
            config.analyze_function_calls = true;
            config.enable_parallel_processing = args.enable_parallel;
            
            // ストレージモード設定
            config.storage_mode = StorageMode::AUTO;
            
            config.calculate_optimal_threads();
            
            NekoCodeCore analyzer(config);
            SessionManager session_manager;
            
            try {
                if (std::filesystem::is_regular_file(path)) {
                    auto result = analyzer.analyze_file_multilang(path);
                    
                    if (result.is_error()) {
                        update_session_state(session_id, "ERROR");
                        exit(1);
                    }
                    
                    AnalysisResult analysis_result;
                    auto multilang_result = result.value();
                    
                    if (multilang_result.csharp_result) {
                        analysis_result = multilang_result.csharp_result.value();
                    } else if (multilang_result.js_result) {
                        analysis_result = multilang_result.js_result.value();
                    } else if (multilang_result.cpp_result) {
                        analysis_result.file_info = multilang_result.cpp_result->file_info;
                        analysis_result.complexity = multilang_result.cpp_result->complexity;
                        analysis_result.language = Language::CPP;
                    } else {
                        analysis_result.file_info = multilang_result.file_info;
                        analysis_result.language = multilang_result.detected_language;
                    }
                    
                    session_manager.create_session(path, analysis_result);
                    
                } else if (std::filesystem::is_directory(path)) {
                    // プログレストラッカー設定（バックグラウンド用）
                    SessionProgressTracker progress_tracker(session_id, args.enable_progress);
                    
                    size_t file_count = 0;
                    for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                        if (entry.is_regular_file()) {
                            file_count++;
                        }
                    }
                    
                    progress_tracker.start_directory_analysis(path, file_count);
                    
                    if (args.enable_progress) {
                        analyzer.set_progress_callback([&progress_tracker](std::uint32_t processed, std::uint32_t total, const std::string& current_file) {
                            progress_tracker.update_file_analysis(current_file, 0, true);
                        });
                    }
                    
                    auto result = analyzer.analyze_directory(path);
                    
                    progress_tracker.complete_analysis();
                    
                    if (result.is_error()) {
                        update_session_state(session_id, "ERROR");
                        exit(1);
                    }
                    
                    session_manager.create_session(path, result.value());
                    
                } else {
                    update_session_state(session_id, "ERROR");
                    exit(1);
                }
                
                // 成功
                update_session_state(session_id, "COMPLETED");
                exit(0);
                
            } catch (const std::exception& e) {
                update_session_state(session_id, "ERROR");
                exit(1);
            }
        }
        else {
            //=================================================================
            // 親プロセス: 即座にレスポンス返す
            //=================================================================
            
            // 状態更新
            update_session_state(session_id, "RUNNING", target_path, pid);
            
            // JSON レスポンス
            nlohmann::json result_json;
            result_json["session_id"] = session_id;
            result_json["status"] = "started";
            result_json["mode"] = "async";
            result_json["pid"] = pid;
            result_json["progress_file"] = "sessions/" + session_id + "_progress.txt";
            result_json["state_file"] = "sessions/" + session_id + "_state.json";
            result_json["message"] = "✅ Background analysis started. Use session-status to check progress.";
            
            std::cout << result_json.dump(2) << std::endl;
            
            return 0;
        }
        
    } catch (const std::exception& e) {
        nlohmann::json error_json;
        error_json["error"] = {
            {"code", 500},
            {"message", std::string("Async session creation failed: ") + e.what()},
            {"type", "exception"}
        };
        std::cout << error_json.dump(2) << std::endl;
        return 1;
    }
}

//=============================================================================
// 📊 check_session_status実装
//=============================================================================

int check_session_status(const std::string& session_id) {
    try {
        std::string state_file = "sessions/" + session_id + "_state.json";
        std::string progress_file = "sessions/" + session_id + "_progress.txt";
        
        nlohmann::json status_json;
        status_json["session_id"] = session_id;
        
        // 状態ファイル読み取り
        if (std::filesystem::exists(state_file)) {
            std::ifstream file(state_file);
            if (file.is_open()) {
                nlohmann::json state;
                file >> state;
                status_json["status"] = state["status"];
                status_json["target_path"] = state["target_path"];
                status_json["last_update"] = state["last_update"];
                if (state.contains("pid")) {
                    status_json["pid"] = state["pid"];
                }
            }
        } else {
            status_json["status"] = "NOT_FOUND";
            status_json["error"] = "Session not found";
            std::cout << status_json.dump(2) << std::endl;
            return 1;
        }
        
        // 進捗ファイル読み取り（最後の数行）
        if (std::filesystem::exists(progress_file)) {
            std::ifstream file(progress_file);
            if (file.is_open()) {
                std::string line;
                std::string last_line;
                while (std::getline(file, line)) {
                    if (!line.empty()) {
                        last_line = line;
                    }
                }
                
                if (!last_line.empty()) {
                    status_json["last_progress"] = last_line;
                    
                    // 進捗パーセント抽出（簡易版）
                    if (last_line.find("COMPLETE") != std::string::npos) {
                        status_json["progress_percent"] = 100;
                    } else if (last_line.find("PROCESSING") != std::string::npos) {
                        // "2847/20732 (13.7%)" のような形式から抽出
                        size_t paren_pos = last_line.find('(');
                        size_t percent_pos = last_line.find('%');
                        if (paren_pos != std::string::npos && percent_pos != std::string::npos) {
                            std::string percent_str = last_line.substr(paren_pos + 1, percent_pos - paren_pos - 1);
                            try {
                                status_json["progress_percent"] = std::stod(percent_str);
                            } catch (...) {
                                status_json["progress_percent"] = 0;
                            }
                        }
                    }
                }
            }
        }
        
        std::cout << status_json.dump(2) << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        nlohmann::json error_json;
        error_json["error"] = {
            {"code", 500},
            {"message", std::string("Status check failed: ") + e.what()},
            {"type", "exception"}
        };
        std::cout << error_json.dump(2) << std::endl;
        return 1;
    }
}