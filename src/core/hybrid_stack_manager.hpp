#pragma once

#include <string>
#include <filesystem>
#include <exception>
#include <cstddef>
#include <type_traits>
#include <cstring>  // for strerror
#include "nekocode/types.hpp"

#ifdef __linux__
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace nekocode {

//=============================================================================
// 🎯 Hybrid Stack Manager - 最適なスタック管理戦略
//=============================================================================

class HybridStackManager {
public:
    // ハイブリッド戦略の閾値
    static constexpr size_t LARGE_FILE_THRESHOLD = 700 * 1024; // 700KB（786KBファイル対応）
    static constexpr size_t PREEMPTIVE_STACK_SIZE = 64 * 1024 * 1024; // 64MB
    
    HybridStackManager() = default;
    ~HybridStackManager() { restore_original_stack(); }
    
    /// メイン解析関数 - ファイルサイズに応じて最適な戦略を選択
    template<typename AnalyzeFunction>
    auto analyze_with_smart_stack(
        const FilePath& path, 
        AnalyzeFunction&& analyze_func
    ) -> decltype(analyze_func()) {
        try {
            size_t file_size = std::filesystem::file_size(path);
            
            if (file_size >= LARGE_FILE_THRESHOLD) {
                // 🥇 大ファイル: 事前拡張戦略
                return analyze_with_preemptive_expansion(path, analyze_func);
            } else {
                // 🥈 小中ファイル: エラードリブン戦略  
                return analyze_with_error_driven_retry(path, analyze_func);
            }
            
        } catch (const std::filesystem::filesystem_error& e) {
            return {AnalysisError(ErrorCode::FILE_NOT_FOUND, 
                                "Cannot access file: " + std::string(e.what()))};
        }
    }

private:
    struct rlimit original_stack_limit_;
    bool stack_expanded_ = false;
    
    /// 🥇 事前拡張戦略 - 大ファイル用
    template<typename AnalyzeFunction>
    auto analyze_with_preemptive_expansion(
        const FilePath& path,
        AnalyzeFunction&& analyze_func
    ) -> decltype(analyze_func()) {
        log_preemptive_expansion(path);
        
        if (expand_stack_preemptively()) {
            try {
                auto result = analyze_func();
                // メタデータ設定（型に応じて）
                if constexpr (std::is_same_v<decltype(result), Result<AnalysisResult>>) {
                    result.value().metadata["stack_strategy"] = "preemptive_expansion";
                }
                return result;
            } catch (const std::exception& e) {
                // 事前拡張してもダメ → フォールバック
                return fallback_to_simple_analysis<decltype(analyze_func())>(path, e);
            }
        } else {
            // 拡張失敗 → エラードリブンにフォールバック
            return analyze_with_error_driven_retry(path, analyze_func);
        }
    }
    
    /// 🥈 エラードリブン戦略 - 小中ファイル用
    template<typename AnalyzeFunction>
    auto analyze_with_error_driven_retry(
        const FilePath& path,
        AnalyzeFunction&& analyze_func
    ) -> decltype(analyze_func()) {
        try {
            // まず普通に解析してみる
            return analyze_func();
            
        } catch (const std::exception& e) {
            if (is_stack_overflow_error(e)) {
                return retry_with_expanded_stack(path, analyze_func, e);
            }
            // スタック関係ないエラーは再スロー
            throw;
        }
    }
    
    /// エラー検出後のリトライ
    template<typename AnalyzeFunction>
    auto retry_with_expanded_stack(
        const FilePath& path,
        AnalyzeFunction&& analyze_func,
        const std::exception& original_error
    ) -> decltype(analyze_func()) {
        log_error_driven_retry(path);
        
        if (expand_stack_after_error()) {
            try {
                auto result = analyze_func();
                // メタデータ設定（型に応じて）
                if constexpr (std::is_same_v<decltype(result), Result<AnalysisResult>>) {
                    result.value().metadata["stack_strategy"] = "error_driven_retry";
                    result.value().metadata["original_error"] = original_error.what();
                }
                return result;
                
            } catch (const std::exception& retry_error) {
                // それでもダメ → フォールバック
                return fallback_to_simple_analysis<decltype(analyze_func())>(path, retry_error);
            }
        } else {
            return {AnalysisError(ErrorCode::STACK_EXPANSION_FAILED,
                                "Stack expansion failed: " + std::string(original_error.what()))};
        }
    }
    
    /// スタックオーバーフロー検出
    bool is_stack_overflow_error(const std::exception& e) {
        std::string msg = e.what();
        
        // よくあるスタックオーバーフローのキーワード
        std::vector<std::string> stack_keywords = {
            "stack overflow", "stack space", "recursion limit",
            "maximum recursion", "call stack", "SIGSEGV",
            "segmentation fault", "access violation"
        };
        
        for (const auto& keyword : stack_keywords) {
            if (msg.find(keyword) != std::string::npos) {
                return true;
            }
        }
        
        return false;
    }
    
    /// 事前スタック拡張
    bool expand_stack_preemptively() {
#ifdef __linux__
        if (getrlimit(RLIMIT_STACK, &original_stack_limit_) != 0) {
            return false;
        }
        
        struct rlimit new_limit = original_stack_limit_;
        new_limit.rlim_cur = PREEMPTIVE_STACK_SIZE;
        
        if (setrlimit(RLIMIT_STACK, &new_limit) == 0) {
            stack_expanded_ = true;
            return true;
        }
#endif
        return false;
    }
    
    /// エラー後スタック拡張
    bool expand_stack_after_error() {
        // 事前拡張と同じロジックだが、将来的に段階的拡張も可能
        return expand_stack_preemptively();
    }
    
    /// スタックサイズ復元
    void restore_original_stack() {
#ifdef __linux__
        if (stack_expanded_) {
            setrlimit(RLIMIT_STACK, &original_stack_limit_);
            stack_expanded_ = false;
        }
#endif
    }
    
    /// フォールバック処理
    template<typename ResultType>
    ResultType fallback_to_simple_analysis(
        const FilePath& path,
        const std::exception& error
    ) {
        // TODO: 軽量パーサーによるフォールバック実装
        return {AnalysisError(ErrorCode::PARSING_ERROR,
                            "Analysis failed even with stack expansion: " + std::string(error.what()))};
    }
    
    /// ログ出力
    void log_preemptive_expansion(const FilePath& path) {
        size_t file_size_kb = std::filesystem::file_size(path) / 1024;
        std::cerr << "🔧 Large file detected (" << file_size_kb << " KB), optimizing memory..." << std::endl;
    }
    
    void log_error_driven_retry(const FilePath& path) {
        std::cerr << "🔄 Optimizing for complex file structure..." << std::endl;
    }
};

} // namespace nekocode