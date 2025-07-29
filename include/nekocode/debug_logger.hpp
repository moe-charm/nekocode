#pragma once

//=============================================================================
// 🐛 NekoCode Debug Logger - 統一デバッグログシステム
//
// 各言語アナライザーで使用する統一的なデバッグ機能
// パフォーマンス計測・詳細ログ・条件付きコンパイル対応
//=============================================================================

#include <iostream>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>

namespace nekocode {
namespace debug {

//=============================================================================
// 🎯 デバッグレベル定義
//=============================================================================
enum class LogLevel {
    TRACE = 0,  // 最詳細ログ
    DEBUG = 1,  // デバッグ情報
    INFO  = 2,  // 一般情報
    WARN  = 3,  // 警告
    ERROR = 4   // エラー
};

//=============================================================================
// 🔧 デバッグ設定（コンパイル時制御）
//=============================================================================
#ifndef NEKOCODE_DEBUG_LEVEL
    #ifdef NDEBUG
        #define NEKOCODE_DEBUG_LEVEL 2  // リリース版：INFO以上
    #else
        #define NEKOCODE_DEBUG_LEVEL 0  // デバッグ版：TRACE以上
    #endif
#endif

//=============================================================================
// 🐛 統一ログ関数群（前方宣言）
//=============================================================================

// 内部ヘルパー関数
inline std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

inline const char* level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNW";
    }
}

// コア出力関数
inline void log_output(LogLevel level, const std::string& category, 
                      const std::string& message) {
    if (static_cast<int>(level) >= NEKOCODE_DEBUG_LEVEL) {
        std::cerr << "[" << get_timestamp() << "] " 
                  << "[" << level_to_string(level) << "] "
                  << "[" << category << "] " 
                  << message << std::endl;
    }
}

// 便利関数群
inline void log_trace(const std::string& category, const std::string& message) {
    log_output(LogLevel::TRACE, category, message);
}

inline void log_debug(const std::string& category, const std::string& message) {
    log_output(LogLevel::DEBUG, category, message);
}

inline void log_info(const std::string& category, const std::string& message) {
    log_output(LogLevel::INFO, category, message);
}

inline void log_warn(const std::string& category, const std::string& message) {
    log_output(LogLevel::WARN, category, message);
}

inline void log_error(const std::string& category, const std::string& message) {
    log_output(LogLevel::ERROR, category, message);
}

//=============================================================================
// 📊 パフォーマンス計測クラス
//=============================================================================
class PerformanceTimer {
private:
    std::chrono::steady_clock::time_point start_time_;
    std::string operation_name_;
    
public:
    explicit PerformanceTimer(const std::string& operation) 
        : start_time_(std::chrono::steady_clock::now())
        , operation_name_(operation) {
        log_trace("Performance", "Started: " + operation_name_);
    }
    
    ~PerformanceTimer() {
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time_).count();
        
        std::ostringstream oss;
        oss << "Finished: " << operation_name_ 
            << " (" << duration << "μs)";
        log_debug("Performance", oss.str());
    }
    
    // 中間計測
    void checkpoint(const std::string& checkpoint_name) {
        auto current_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            current_time - start_time_).count();
            
        std::ostringstream oss;
        oss << operation_name_ << " - " << checkpoint_name 
            << " (" << duration << "μs)";
        log_trace("Performance", oss.str());
    }
};

//=============================================================================
// 📈 解析統計クラス
//=============================================================================
struct AnalysisStats {
    size_t total_lines = 0;
    size_t code_lines = 0;
    size_t functions_found = 0;
    size_t classes_found = 0;
    size_t imports_found = 0;
    size_t exports_found = 0;
    size_t complexity_score = 0;
    std::chrono::microseconds parse_time{0};
    
    void log_summary(const std::string& language, const std::string& filename) {
        std::ostringstream oss;
        oss << language << " Analysis Summary for " << filename << ":\n"
            << "  Lines: " << total_lines << " (code: " << code_lines << ")\n"
            << "  Functions: " << functions_found << ", Classes: " << classes_found << "\n"
            << "  Imports: " << imports_found << ", Exports: " << exports_found << "\n"
            << "  Complexity: " << complexity_score << "\n"
            << "  Parse Time: " << parse_time.count() << "μs";
        log_info("Analysis", oss.str());
    }
};

//=============================================================================
// 🎯 便利マクロ定義
//=============================================================================
#define NEKOCODE_PERF_TIMER(name) \
    nekocode::debug::PerformanceTimer perf_timer_(name)

#define NEKOCODE_PERF_CHECKPOINT(name) \
    if (static_cast<int>(nekocode::debug::LogLevel::TRACE) >= NEKOCODE_DEBUG_LEVEL) \
        perf_timer_.checkpoint(name)

#define NEKOCODE_LOG_TRACE(category, message) \
    nekocode::debug::log_trace(category, message)

#define NEKOCODE_LOG_DEBUG(category, message) \
    nekocode::debug::log_debug(category, message)

#define NEKOCODE_LOG_INFO(category, message) \
    nekocode::debug::log_info(category, message)

#define NEKOCODE_LOG_WARN(category, message) \
    nekocode::debug::log_warn(category, message)

#define NEKOCODE_LOG_ERROR(category, message) \
    nekocode::debug::log_error(category, message)

} // namespace debug
} // namespace nekocode