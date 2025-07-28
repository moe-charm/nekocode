/**
 * @file UltraLightCoreBase.cpp
 * @brief 究極軽量Coreベースクラス実装 - nyamesh純粋実装
 * 
 * ここでのみ重い依存関係を使用：
 * - nyamesh軽量メッセージング（純粋実装）
 * - 複雑なスレッド処理
 * - バイナリシリアライゼーション
 * 
 * ヘッダーには一切露出させない完全隠蔽実装
 * nyamesh_v21依存関係完全排除！
 * 
 * @version 2.0 - Pure nyamesh Implementation
 * @date 2025-07-25
 */

#include "UltraLightCoreBase.h"

// ここでのみ軽量依存関係をインクルード
// ヘッダーには一切露出させない
#include "LightMessaging.h"
#include "../debug/NyaMeshDebugLogger.h"

#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <random>
#include <sstream>
#include <iostream>

namespace charmcode {

// ===============================================
// UltraLightCoreBase::Impl - 完全隠蔽実装
// ===============================================

class UltraLightCoreBase::Impl {
public:
    explicit Impl(const std::string& coreName)
        : coreName_(coreName),
          coreId_(generateCoreId(coreName)),
          debugMode_(false),
          running_(false),
          statistics_{}
    {
        // 純粋nyamesh_v22実装のみ使用
        lightMessaging_ = nyamesh2::LightMessagingFactory::create(coreId_);
        debugLogger_ = std::make_unique<nyamesh2::NyaMeshDebugLogger>();
        
        startTime_ = std::chrono::high_resolution_clock::now();
        
        if (debugLogger_) {
            debugLogger_->log("UltraLightCoreBase created: " + coreName_ + " (ID: " + coreId_ + ")");
        }
    }
    
    ~Impl() {
        stopProcessing();
        if (debugLogger_) {
            debugLogger_->log("UltraLightCoreBase destroyed: " + coreName_);
        }
    }
    
    bool sendMessageImpl(const std::string& messageType,
                        const void* data, 
                        size_t size,
                        bool broadcast) {
        
        if (debugMode_ && debugLogger_) {
            debugLogger_->log("[" + coreName_ + "] Sending: " + messageType + 
                             " (size: " + std::to_string(size) + 
                             ", broadcast: " + (broadcast ? "true" : "false") + ")");
        }
        
        bool success = false;
        
        try {
            // 純粋nyamesh_v22軽量メッセージング使用
            if (lightMessaging_) {
                nyamesh2::LightOptions options(broadcast, debugMode_);
                
                // バイナリデータを直接送信（型安全・JSON不要）
                success = lightMessaging_->sendBinaryMessage(messageType, data, size, options);
                
                // 統計更新
                if (success) {
                    std::lock_guard<std::mutex> lock(statisticsMutex_);
                    statistics_.messagesSent++;
                }
            }
            
        } catch (const std::exception& e) {
            if (debugLogger_) {
                debugLogger_->log("[" + coreName_ + "] Send error: " + std::string(e.what()));
            }
            
            std::lock_guard<std::mutex> lock(statisticsMutex_);
            statistics_.errorsCount++;
            success = false;
        }
        
        return success;
    }
    
    bool subscribeMessageImpl(const std::string& messageType,
                             std::function<void(const void*, size_t)> handler) {
        
        if (debugMode_ && debugLogger_) {
            debugLogger_->log("[" + coreName_ + "] Subscribing to: " + messageType);
        }
        
        bool success = false;
        
        try {
            // 純粋nyamesh_v22軽量メッセージング購読
            if (lightMessaging_) {
                auto lightHandler = [this, handler](const void* data, size_t size) {
                    // バイナリデータを直接処理（変換不要）
                    handler(data, size);
                    
                    // 統計更新
                    std::lock_guard<std::mutex> lock(statisticsMutex_);
                    statistics_.messagesReceived++;
                };
                
                success = lightMessaging_->subscribeBinaryMessage(messageType, lightHandler);
            }
            
        } catch (const std::exception& e) {
            if (debugLogger_) {
                debugLogger_->log("[" + coreName_ + "] Subscribe error: " + std::string(e.what()));
            }
            success = false;
        }
        
        return success;
    }
    
    void startProcessing() {
        if (running_.exchange(true)) {
            return; // 既に実行中
        }
        
        if (lightMessaging_) {
            lightMessaging_->startProcessing();
        }
        
        if (debugLogger_) {
            debugLogger_->log("[" + coreName_ + "] Processing started");
        }
    }
    
    void stopProcessing() {
        if (!running_.exchange(false)) {
            return; // 既に停止中
        }
        
        if (lightMessaging_) {
            lightMessaging_->stopProcessing();
        }
        
        if (debugLogger_) {
            debugLogger_->log("[" + coreName_ + "] Processing stopped");
        }
    }
    
    std::string getCoreName() const {
        return coreName_;
    }
    
    std::string getCoreId() const {
        return coreId_;
    }
    
    void setDebugMode(bool enable) {
        debugMode_ = enable;
        
        if (lightMessaging_) {
            lightMessaging_->setDebugMode(enable);
        }
        
        if (debugLogger_) {
            debugLogger_->log("[" + coreName_ + "] Debug mode " + 
                             (enable ? "enabled" : "disabled"));
        }
    }
    
    UltraLightCoreBase::CoreStatistics getStatistics() const {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        
        UltraLightCoreBase::CoreStatistics stats = statistics_;
        
        // 稼働時間計算
        auto now = std::chrono::high_resolution_clock::now();
        auto uptime = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime_);
        stats.uptimeMs = uptime.count();
        
        return stats;
    }
    
    void log(const std::string& message) {
        if (debugLogger_) {
            debugLogger_->log("[" + coreName_ + "] " + message);
        }
    }
    
    std::string generateUniqueId() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);
        
        std::stringstream ss;
        ss << std::hex;
        for (int i = 0; i < 8; ++i) {
            ss << dis(gen);
        }
        
        return ss.str();
    }

private:
    std::string coreName_;
    std::string coreId_;
    std::atomic<bool> debugMode_;
    std::atomic<bool> running_;
    
    // 統計情報（スレッドセーフ）
    mutable std::mutex statisticsMutex_;
    UltraLightCoreBase::CoreStatistics statistics_;
    std::chrono::high_resolution_clock::time_point startTime_;
    
    // 純粋nyamesh_v22メッセージングシステム
    std::unique_ptr<nyamesh2::LightMessagingBus> lightMessaging_;
    std::unique_ptr<nyamesh2::NyaMeshDebugLogger> debugLogger_;
    
    /**
     * @brief Core ID生成
     */
    std::string generateCoreId(const std::string& coreName) {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        
        return coreName + "_" + std::to_string(timestamp) + "_" + std::to_string(dis(gen));
    }
    
    // 純粋バイナリメッセージング - 変換関数不要
};

// ===============================================
// UltraLightCoreBase - 公開インターフェース実装
// ===============================================

UltraLightCoreBase::UltraLightCoreBase(const std::string& coreName)
    : pImpl_(std::make_unique<Impl>(coreName)) {
}

UltraLightCoreBase::~UltraLightCoreBase() = default;

bool UltraLightCoreBase::sendMessageImpl(const std::string& messageType,
                                        const void* data, 
                                        size_t size,
                                        bool broadcast) {
    return pImpl_->sendMessageImpl(messageType, data, size, broadcast);
}

bool UltraLightCoreBase::subscribeMessageImpl(const std::string& messageType,
                                             std::function<void(const void*, size_t)> handler) {
    return pImpl_->subscribeMessageImpl(messageType, handler);
}

void UltraLightCoreBase::sendError(const std::string& error) {
    nyamesh2::messages::SystemError msg(error, getCoreName());
    sendMessage<nyamesh2::messages::SystemError>(
        nyamesh2::message_types::SYSTEM_ERROR, msg, true);
}

void UltraLightCoreBase::sendWarning(const std::string& warning) {
    nyamesh2::messages::SystemWarning msg(warning, getCoreName());
    sendMessage<nyamesh2::messages::SystemWarning>(
        nyamesh2::message_types::SYSTEM_WARNING, msg, true);
}

void UltraLightCoreBase::sendInfo(const std::string& info) {
    nyamesh2::messages::SystemInfo msg(info, getCoreName());
    sendMessage<nyamesh2::messages::SystemInfo>(
        nyamesh2::message_types::SYSTEM_INFO, msg, true);
}

void UltraLightCoreBase::sendInitialized(const std::string& version) {
    nyamesh2::messages::CoreInitialized msg(getCoreName(), version);
    sendMessage<nyamesh2::messages::CoreInitialized>(
        nyamesh2::message_types::CORE_INITIALIZED, msg, true);
}

std::string UltraLightCoreBase::getCoreName() const {
    return pImpl_->getCoreName();
}

std::string UltraLightCoreBase::getCoreId() const {
    return pImpl_->getCoreId();
}

void UltraLightCoreBase::startProcessing() {
    pImpl_->startProcessing();
}

void UltraLightCoreBase::stopProcessing() {
    pImpl_->stopProcessing();
}

void UltraLightCoreBase::setDebugMode(bool enable) {
    pImpl_->setDebugMode(enable);
}

UltraLightCoreBase::CoreStatistics UltraLightCoreBase::getStatistics() const {
    return pImpl_->getStatistics();
}

void UltraLightCoreBase::log(const std::string& message) {
    pImpl_->log(message);
}

void UltraLightCoreBase::logSubscription(const std::string& messageType) {
    log("Subscribed to: " + messageType);
}

std::string UltraLightCoreBase::generateUniqueId() {
    return pImpl_->generateUniqueId();
}

// ===============================================
// CoreFactory - ファクトリー実装
// ===============================================

CoreFactory::GlobalConfig CoreFactory::globalConfig_;

void CoreFactory::setGlobalConfig(const GlobalConfig& config) {
    globalConfig_ = config;
}

CoreFactory::GlobalConfig CoreFactory::getGlobalConfig() {
    return globalConfig_;
}

} // namespace charmcode