/**
 * @file IntentCommonOptions.h
 * @brief NyaMesh v2.1 Intent Common Options Definition
 * 
 * Common options fully compatible with JavaScript v2.1.
 * Implements "free to send, responsible to interpret" philosophy.
 * 
 * @author nyamesh_v21 team
 * @date 2025-01-24
 */

#pragma once

#include <string>
#include <vector>
#include <set>

namespace nyamesh2 {

/**
 * @brief Intent common options structure
 * 
 * Common metadata available for all Intents.
 * Maintains same field names and default values as JavaScript version.
 */
struct IntentCommonOptions {
    /**
     * @brief Broadcast delivery flag
     * true: Send to all adjacent cores
     * false: 1-to-1 transmission (default)
     */
    bool broadcast = false;
    
    /**
     * @brief Parallel processing safety flag
     * true: Can be processed in parallel in MessagePool
     * false: Sequential processing required (default)
     */
    bool parallelSafe = false;
    
    /**
     * @brief Processing priority
     * "low", "normal", "high", "urgent"
     * Currently for future extension (not implemented)
     */
    std::string priority = "normal";
    
    /**
     * @brief Timeout value (milliseconds)
     * Currently for future extension (not implemented)
     */
    int timeout = 30000;
    
    /**
     * @brief Retry count
     * Currently for future extension (not implemented)
     */
    int retryCount = 0;
    
    /**
     * @brief Debug mode (C++ version specific extension)
     * true: Output detailed logs for this Intent
     * false: Normal logs only (default)
     */
    bool debug = false;
    
    /**
     * @brief Default constructor
     */
    IntentCommonOptions() = default;
    
    /**
     * @brief Convenient constructor (frequently used options only)
     */
    IntentCommonOptions(bool broadcast_, bool parallelSafe_ = false, bool debug_ = false)
        : broadcast(broadcast_)
        , parallelSafe(parallelSafe_)
        , debug(debug_) {}
};

/**
 * @brief Intent解釈ガイドクラス
 * 
 * 受信側が自律的にオプションの妥当性を判断するためのヘルパー。
 * JavaScript版のIntentInterpretationGuideをC++で実装。
 */
class IntentInterpreter {
private:
    // ブロードキャストが意味を持つIntent
    static const std::set<std::string> broadcastMeaningful_;
    
    // ブロードキャストが無意味/危険なIntent
    static const std::set<std::string> broadcastMeaningless_;
    
    // 並列処理が安全なIntent
    static const std::set<std::string> parallelSafeByNature_;
    
    // 並列処理に注意が必要なIntent
    static const std::set<std::string> parallelCaution_;
    
public:
    /**
     * @brief ブロードキャストが妥当かどうか判定
     */
    static bool isBroadcastMeaningful(const std::string& intentType) {
        return broadcastMeaningful_.find(intentType) != broadcastMeaningful_.end();
    }
    
    /**
     * @brief ブロードキャストを無視すべきか判定
     */
    static bool shouldIgnoreBroadcast(const std::string& intentType, bool broadcast) {
        return broadcast && 
               (broadcastMeaningless_.find(intentType) != broadcastMeaningless_.end());
    }
    
    /**
     * @brief 並列処理が自然に安全か判定
     */
    static bool isNaturallyParallelSafe(const std::string& intentType) {
        return parallelSafeByNature_.find(intentType) != parallelSafeByNature_.end();
    }
    
    /**
     * @brief 並列処理に注意が必要か判定
     */
    static bool needsParallelCaution(const std::string& intentType, bool parallelSafe) {
        return parallelSafe && 
               (parallelCaution_.find(intentType) != parallelCaution_.end());
    }
    
    /**
     * @brief Intent受信時の推奨アクション
     */
    struct Recommendation {
        bool shouldProcess = true;      // 処理すべきか
        bool forceSequential = false;   // 強制的に順次処理すべきか
        bool logWarning = false;        // 警告ログを出すべきか
        std::string warningMessage;     // 警告メッセージ
    };
    
    /**
     * @brief Intent受信時の自律的判断
     */
    static Recommendation analyzeIntent(const std::string& intentType, 
                                       const IntentCommonOptions& options) {
        Recommendation rec;
        
        // ブロードキャスト判定
        if (shouldIgnoreBroadcast(intentType, options.broadcast)) {
            rec.logWarning = true;
            rec.warningMessage = "Broadcast ignored for " + intentType + 
                                " (meaningless for this intent type)";
        }
        
        // 並列処理判定
        if (needsParallelCaution(intentType, options.parallelSafe)) {
            rec.forceSequential = true;
            rec.logWarning = true;
            rec.warningMessage = "Parallel processing forced to sequential for " + 
                                intentType + " (state changes detected)";
        }
        
        return rec;
    }
};

} // namespace nyamesh2