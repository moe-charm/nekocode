/**
 * @file NyaMeshDebugLogger.h
 * @brief NyaMesh v2.1 グローバルデバッグロガー
 * 
 * 設計思想:
 * - send()メソッド一点集中でのデバッグ
 * - ゼロオーバーヘッド（無効時）
 * - Intent種別フィルタリング
 * - ファイル出力対応
 * 
 * @author nyamesh_v21 team
 * @date 2025-01-24
 */

#pragma once

#include <string>
#include <set>
#include <vector>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
#include "../nlohmann_json.hpp"
#include "../intents/IntentCommonOptions.h"

namespace nyamesh2 {

/**
 * @brief グローバルデバッグロガー
 * 
 * すべてのNyaMeshインスタンスで共有される単一のロガー。
 * send()メソッドでのみ使用され、すべてのIntentを追跡可能。
 */
class NyaMeshDebugLogger {
private:
    bool enabled_ = false;
    bool fileLoggingEnabled_ = false;
    std::ofstream logFile_;
    std::set<std::string> intentFilter_;  // 空の場合は全Intent対象
    mutable std::mutex mutex_;  // スレッドセーフ
    
    /**
     * @brief 現在時刻をフォーマット
     */
    std::string formatTime() const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    
public:
    /**
     * @brief デストラクタ - ファイルを安全にクローズ
     */
    ~NyaMeshDebugLogger() {
        if (logFile_.is_open()) {
            logFile_.close();
        }
    }
    
    /**
     * @brief デバッグモード有効化
     */
    void enable(bool enabled = true) {
        std::lock_guard<std::mutex> lock(mutex_);
        enabled_ = enabled;
    }
    
    /**
     * @brief デバッグモードの状態確認
     */
    bool isEnabled() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return enabled_;
    }
    
    /**
     * @brief ファイルロギング有効化
     * @param filename ログファイル名
     * @param append 追記モード（デフォルト: true）
     */
    void enableFileLogging(const std::string& filename, bool append = true) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (logFile_.is_open()) {
            logFile_.close();
        }
        
        auto mode = append ? std::ios::app : std::ios::out;
        logFile_.open(filename, mode);
        fileLoggingEnabled_ = logFile_.is_open();
        
        if (fileLoggingEnabled_) {
            logFile_ << "\n=== NyaMesh v2.1 Debug Log Started at " 
                     << formatTime() << " ===\n" << std::endl;
        }
    }
    
    /**
     * @brief Intent種別フィルタ設定
     * @param intents ログ対象のIntent種別リスト（空の場合は全対象）
     */
    void setIntentFilter(const std::vector<std::string>& intents) {
        std::lock_guard<std::mutex> lock(mutex_);
        intentFilter_.clear();
        intentFilter_.insert(intents.begin(), intents.end());
    }
    
    /**
     * @brief Intent送信をログ記録
     * @param type Intent種別
     * @param payload ペイロード
     * @param to 宛先（空の場合はブロードキャスト）
     * @param options 共通オプション（v2.1新機能）
     */
    void logIntent(const std::string& type,
                   const nlohmann::json& payload,
                   const std::string& to,
                   const IntentCommonOptions* options = nullptr) {
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!enabled_) return;
        
        // Intent種別フィルタリング
        if (!intentFilter_.empty() && 
            intentFilter_.find(type) == intentFilter_.end()) {
            return;  // フィルタ対象外
        }
        
        // ログメッセージ構築
        std::stringstream log;
        log << "[" << formatTime() << "] ";
        log << "INTENT: " << type;
        
        // 宛先情報
        if (to.empty()) {
            log << " -> [BROADCAST]";
        } else {
            log << " -> " << to;
        }
        
        // v2.1オプション情報（存在する場合）
        if (options) {
            log << " [opts:";
            std::vector<std::string> opts;
            if (options->broadcast) opts.push_back("broadcast");
            if (options->parallelSafe) opts.push_back("parallel");
            if (options->debug) opts.push_back("debug");
            if (options->priority != "normal") {
                opts.push_back("pri:" + options->priority);
            }
            
            // オプションをカンマ区切りで結合
            for (size_t i = 0; i < opts.size(); ++i) {
                if (i > 0) log << ",";
                log << opts[i];
            }
            log << "]";
        }
        
        // ペイロード情報
        try {
            if (payload.is_object()) {
                log << " payload:" << payload.size() << " fields";
                // 小さいペイロードは全体を出力
                if (payload.size() <= 3) {
                    log << " " << payload.dump();
                }
            } else {
                log << " payload:" << payload.dump();
            }
        } catch (const std::exception& e) {
            log << " payload:[parse error]";
        }
        
        log << "\n";
        
        // コンソール出力
        std::cout << log.str();
        
        // ファイル出力
        if (fileLoggingEnabled_ && logFile_.is_open()) {
            logFile_ << log.str();
            logFile_.flush();  // 即座に書き込み
        }
    }
    
    /**
     * @brief 統計情報をログに記録
     */
    void logStats(const nlohmann::json& stats) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!enabled_) return;
        
        std::stringstream log;
        log << "[" << formatTime() << "] ";
        log << "STATS: " << stats.dump() << "\n";
        
        std::cout << log.str();
        
        if (fileLoggingEnabled_ && logFile_.is_open()) {
            logFile_ << log.str();
            logFile_.flush();
        }
    }
    
    /**
     * @brief 簡易ログ出力（JSON不要版）
     * @param message ログメッセージ
     */
    void log(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!enabled_) return;
        
        std::stringstream log;
        log << "[" << formatTime() << "] ";
        log << "LOG: " << message << "\n";
        
        std::cout << log.str();
        
        if (fileLoggingEnabled_ && logFile_.is_open()) {
            logFile_ << log.str();
            logFile_.flush();
        }
    }
};

// グローバルインスタンス宣言（実体は.cppで定義）
extern NyaMeshDebugLogger globalDebugLogger;

} // namespace nyamesh2