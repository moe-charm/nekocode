/**
 * @file LightMessaging.cpp
 * @brief nyamesh 革命的軽量メッセージバス実装
 * 
 * ここでのみ重い依存関係を使用：
 * - std::thread（メッセージ処理）
 * - std::atomic（スレッドセーフ操作）  
 * - std::mutex（排他制御）
 * - 複雑なメッセージキューイング
 * - バイナリシリアライゼーション
 * 
 * ヘッダーには一切露出させない完全隠蔽実装
 * JSON依存関係完全排除！
 * 
 * @version 2.0 - Pure Implementation
 * @date 2025-07-25
 */

#include "LightMessaging.h"

// 軽量依存関係のみインクルード - JSON完全排除
#include <thread>
#include "../debug/NyaMeshDebugLogger.h"

#include <mutex>
#include <thread>
#include <atomic>
#include <queue>
#include <unordered_map>
#include <condition_variable>
#include <chrono>
#include <iostream>

namespace nyamesh2 {

// ===============================================
// 内部メッセージ構造体（完全純粋・JSON不要）
// ===============================================

struct InternalMessage {
    std::string messageType;
    std::string nodeId;
    std::chrono::high_resolution_clock::time_point timestamp;
    bool broadcast = false;
    bool debug = false;
    std::vector<uint8_t> data;
    
    InternalMessage() = default;
};

// ===============================================
// LightMessagingBus::Impl - Pimpl内部実装
// ===============================================

class LightMessagingBus::Impl {
public:
    explicit Impl(const std::string& nodeId) 
        : nodeId_(nodeId), 
          running_(false),
          debugMode_(false),
          statistics_{}
    {
        // 内部でのみJSON等の重い処理を初期化
        debugLogger_ = std::make_unique<NyaMeshDebugLogger>();
        debugLogger_->log("LightMessagingBus created: " + nodeId_);
    }
    
    ~Impl() {
        stopProcessing();
        if (debugLogger_) {
            debugLogger_->log("LightMessagingBus destroyed: " + nodeId_);
        }
    }
    
    bool sendMessageImpl(const std::string& messageType,
                        const void* data, 
                        size_t size,
                        const LightOptions& options) {
        
        if (debugMode_ && debugLogger_) {
            debugLogger_->log("Sending message: " + messageType + " (size: " + std::to_string(size) + ")");
        }
        
        try {
            // 完全純粋バイナリ通信 - JSON排除
            InternalMessage msg;
            msg.messageType = messageType;
            msg.nodeId = nodeId_;
            msg.timestamp = std::chrono::high_resolution_clock::now();
            msg.broadcast = options.broadcast;
            msg.debug = options.debug;
            
            // バイナリデータ直接保存
            if (data && size > 0) {
                msg.data.resize(size);
                std::memcpy(msg.data.data(), data, size);
            }
            
            // メッセージをキューに送信（純粋バイナリ）
            {
                std::lock_guard<std::mutex> lock(messageQueueMutex_);
                messageQueue_.push(std::move(msg));
            }
            queueCondition_.notify_one();
            
            // 統計更新
            {
                std::lock_guard<std::mutex> lock(statisticsMutex_);
                statistics_.messagesSent++;
            }
            
            // メッセージ送信完了（純粋バイナリキューイング済み）
            
            return true;
            
        } catch (const std::exception& e) {
            if (debugLogger_) {
                debugLogger_->log("Send error: " + std::string(e.what()));
            }
            
            std::lock_guard<std::mutex> lock(statisticsMutex_);
            statistics_.errorsCount++;
            return false;
        }
    }
    
    bool subscribeMessageImpl(const std::string& messageType,
                             std::function<void(const void*, size_t)> handler) {
        
        if (debugMode_ && debugLogger_) {
            debugLogger_->log("Subscribing to message: " + messageType);
        }
        
        std::lock_guard<std::mutex> lock(handlersMutex_);
        handlers_[messageType] = handler;
        
        return true;
    }
    
    void startProcessing() {
        if (running_.exchange(true)) {
            return; // 既に実行中
        }
        
        processingThread_ = std::thread(&Impl::messageProcessingLoop, this);
        
        if (debugLogger_) {
            debugLogger_->log("Message processing started for: " + nodeId_);
        }
    }
    
    void stopProcessing() {
        if (!running_.exchange(false)) {
            return; // 既に停止中
        }
        
        // 処理スレッドに停止を通知
        queueCondition_.notify_all();
        
        if (processingThread_.joinable()) {
            processingThread_.join();
        }
        
        if (debugLogger_) {
            debugLogger_->log("Message processing stopped for: " + nodeId_);
        }
    }
    
    std::string getNodeId() const {
        return nodeId_;
    }
    
    LightMessagingBus::Statistics getStatistics() const {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        return statistics_;
    }
    
    void setDebugMode(bool enable) {
        debugMode_ = enable;
        if (debugLogger_) {
            debugLogger_->log("Debug mode " + std::string(enable ? "enabled" : "disabled") + " for: " + nodeId_);
        }
    }

private:
    std::string nodeId_;
    std::atomic<bool> running_;
    std::atomic<bool> debugMode_;
    
    // 統計情報（スレッドセーフ）
    mutable std::mutex statisticsMutex_;
    LightMessagingBus::Statistics statistics_;
    
    // メッセージハンドラー（スレッドセーフ）
    std::mutex handlersMutex_;
    std::unordered_map<std::string, std::function<void(const void*, size_t)>> handlers_;
    
    // メッセージキュー（スレッドセーフ・純粋バイナリ）
    std::mutex messageQueueMutex_;
    std::condition_variable queueCondition_;
    std::queue<InternalMessage> messageQueue_;
    
    // バックグラウンド処理スレッド
    std::thread processingThread_;
    
    // 重い依存関係（ヘッダーに露出させない）
    std::unique_ptr<NyaMeshDebugLogger> debugLogger_;
    
    /**
     * @brief メッセージ処理ループ
     * 
     * バックグラウンドスレッドで実行される
     */
    void messageProcessingLoop() {
        while (running_) {
            std::unique_lock<std::mutex> lock(messageQueueMutex_);
            
            // メッセージが来るまで待機
            queueCondition_.wait(lock, [this] { 
                return !messageQueue_.empty() || !running_; 
            });
            
            if (!running_) {
                break;
            }
            
            // キューからメッセージを取得（純粋バイナリ）
            while (!messageQueue_.empty()) {
                InternalMessage msg = std::move(messageQueue_.front());
                messageQueue_.pop();
                lock.unlock();
                
                // メッセージを処理（JSON不要）
                processMessage(msg);
                
                lock.lock();
            }
        }
    }
    
    /**
     * @brief 個別メッセージ処理（純粋バイナリ）
     */
    void processMessage(const InternalMessage& msg) {
        try {
            auto startTime = std::chrono::high_resolution_clock::now();
            
            // 純粋バイナリ処理（JSON不要）
            if (msg.debug && debugLogger_) {
                debugLogger_->log("Processing message: " + msg.messageType + 
                                " from " + msg.nodeId + " (size: " + std::to_string(msg.data.size()) + ")");
            }
            
            // 対応するハンドラーを探して実行（直接バイナリデータ）
            {
                std::lock_guard<std::mutex> lock(handlersMutex_);
                auto it = handlers_.find(msg.messageType);
                if (it != handlers_.end()) {
                    it->second(msg.data.data(), msg.data.size());
                }
            }
            
            // 統計更新
            auto endTime = std::chrono::high_resolution_clock::now();
            auto processingTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            
            {
                std::lock_guard<std::mutex> lock(statisticsMutex_);
                statistics_.messagesReceived++;
                statistics_.avgProcessingTimeMs = 
                    (statistics_.avgProcessingTimeMs + processingTime.count()) / 2;
            }
            
        } catch (const std::exception& e) {
            if (debugLogger_) {
                debugLogger_->log("Message processing error: " + std::string(e.what()));
            }
            
            std::lock_guard<std::mutex> lock(statisticsMutex_);
            statistics_.errorsCount++;
        }
    }
    
    /**
     * @brief バイナリデータを16進文字列に変換
     */
    std::string binaryToHex(const void* data, size_t size) const {
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        std::string result;
        result.reserve(size * 2);
        
        for (size_t i = 0; i < size; ++i) {
            char hex[3];
            std::snprintf(hex, sizeof(hex), "%02x", bytes[i]);
            result += hex;
        }
        
        return result;
    }
    
    /**
     * @brief 16進文字列をバイナリデータに変換
     */
    std::vector<uint8_t> hexToBinary(const std::string& hex) const {
        if (hex.length() % 2 != 0) {
            throw std::runtime_error("Invalid hex string length");
        }
        
        std::vector<uint8_t> result;
        result.reserve(hex.length() / 2);
        
        for (size_t i = 0; i < hex.length(); i += 2) {
            std::string byteStr = hex.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
            result.push_back(byte);
        }
        
        return result;
    }
};

// ===============================================
// LightMessagingBus - 公開インターフェース実装
// ===============================================

LightMessagingBus::LightMessagingBus(const std::string& nodeId)
    : pImpl_(std::make_unique<Impl>(nodeId)) {
}

LightMessagingBus::~LightMessagingBus() = default;

bool LightMessagingBus::sendMessageImpl(const std::string& messageType,
                                       const void* data, 
                                       size_t size,
                                       const LightOptions& options) {
    return pImpl_->sendMessageImpl(messageType, data, size, options);
}

bool LightMessagingBus::sendBinaryMessage(const std::string& messageType,
                                         const void* data,
                                         size_t size,
                                         const LightOptions& options) {
    return pImpl_->sendMessageImpl(messageType, data, size, options);
}

bool LightMessagingBus::subscribeMessageImpl(const std::string& messageType,
                                            std::function<void(const void*, size_t)> handler) {
    return pImpl_->subscribeMessageImpl(messageType, handler);
}

bool LightMessagingBus::subscribeBinaryMessage(const std::string& messageType,
                                              std::function<void(const void*, size_t)> handler) {
    return pImpl_->subscribeMessageImpl(messageType, handler);
}

void LightMessagingBus::startProcessing() {
    pImpl_->startProcessing();
}

void LightMessagingBus::stopProcessing() {
    pImpl_->stopProcessing();
}

std::string LightMessagingBus::getNodeId() const {
    return pImpl_->getNodeId();
}

LightMessagingBus::Statistics LightMessagingBus::getStatistics() const {
    return pImpl_->getStatistics();
}

void LightMessagingBus::setDebugMode(bool enable) {
    pImpl_->setDebugMode(enable);
}

// ===============================================
// LightMessagingFactory - ファクトリー実装
// ===============================================

LightMessagingFactory::GlobalConfig LightMessagingFactory::globalConfig_;

std::unique_ptr<LightMessagingBus> LightMessagingFactory::create(const std::string& nodeId) {
    auto bus = std::make_unique<LightMessagingBus>(nodeId);
    bus->setDebugMode(globalConfig_.debugMode);
    return bus;
}

void LightMessagingFactory::setGlobalConfig(const GlobalConfig& config) {
    globalConfig_ = config;
}

LightMessagingFactory::GlobalConfig LightMessagingFactory::getGlobalConfig() {
    return globalConfig_;
}

// ===============================================
// CoreMessagingHelper - 便利ラッパー実装
// ===============================================

CoreMessagingHelper::CoreMessagingHelper(const std::string& coreName)
    : coreName_(coreName) {
    
    messagingBus_ = LightMessagingFactory::create(coreName);
}

CoreMessagingHelper::~CoreMessagingHelper() {
    stopProcessing();
}

void CoreMessagingHelper::sendError(const std::string& error) {
    if (messagingBus_) {
        messages::SystemError msg(error, coreName_);
        messagingBus_->sendMessage<messages::SystemError>(
            message_types::SYSTEM_ERROR, msg, LightOptions(true));
    }
}

void CoreMessagingHelper::sendWarning(const std::string& warning) {
    if (messagingBus_) {
        messages::SystemWarning msg(warning, coreName_);
        messagingBus_->sendMessage<messages::SystemWarning>(
            message_types::SYSTEM_WARNING, msg, LightOptions(true));
    }
}

void CoreMessagingHelper::sendInfo(const std::string& info) {
    if (messagingBus_) {
        messages::SystemInfo msg(info, coreName_);
        messagingBus_->sendMessage<messages::SystemInfo>(
            message_types::SYSTEM_INFO, msg, LightOptions(true));
    }
}

void CoreMessagingHelper::sendInitialized(const std::string& version) {
    if (messagingBus_) {
        messages::CoreInitialized msg(coreName_, version);
        messagingBus_->sendMessage<messages::CoreInitialized>(
            message_types::CORE_INITIALIZED, msg, LightOptions(true));
    }
}

void CoreMessagingHelper::sendSettingsChanged(const std::string& path, 
                                             const std::string& oldValue,
                                             const std::string& newValue,
                                             const std::string& type) {
    if (messagingBus_) {
        messages::SettingsChanged msg(path, oldValue, newValue, type);
        messagingBus_->sendMessage<messages::SettingsChanged>(
            message_types::SETTINGS_CHANGED, msg, LightOptions(true));
    }
}

void CoreMessagingHelper::startProcessing() {
    if (messagingBus_) {
        messagingBus_->startProcessing();
    }
}

void CoreMessagingHelper::stopProcessing() {
    if (messagingBus_) {
        messagingBus_->stopProcessing();
    }
}

} // namespace nyamesh2