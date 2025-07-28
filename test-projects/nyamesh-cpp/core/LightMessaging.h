/**
 * @file LightMessaging.h
 * @brief nyamesh_v22 革命的軽量メッセージバス
 * 
 * JSON地獄完全排除 - 型安全・高速・軽量な通信システム
 * 
 * 設計理念：
 * - ヘッダーからJSON完全排除（Gemini先生指摘）
 * - template使用で型安全性確保（ChatGPT先生提案）
 * - 内部シリアライズで通信効率最大化
 * - Interface+Pimplによる完璧な実装隠蔽
 * 
 * @version 1.0 - Revolution Core
 * @date 2025-07-25
 */

#pragma once

#include <string>
#include <functional>
#include <memory>
#include <future>
#include <vector>
#include <cstdint>

// 軽量メッセージ構造体のみインクルード（JSON不要！）
#include "../messages/CoreMessages.h"

namespace nyamesh2 {

/**
 * @brief 軽量メッセージ配信オプション
 * 
 * 旧IntentCommonOptionsの軽量版
 * 複雑なフィールドを排除し、本当に必要なもののみ
 */
struct LightOptions {
    bool broadcast = false;    // ブロードキャスト配信
    bool debug = false;        // デバッグログ出力
    int timeoutMs = 5000;      // タイムアウト（簡素化）
    
    LightOptions() = default;
    LightOptions(bool bcast, bool dbg = false) 
        : broadcast(bcast), debug(dbg) {}
};

/**
 * @brief 革命的軽量メッセージバス
 * 
 * 特徴：
 * - template使用でJSON排除
 * - 型安全なメッセージ通信
 * - Pimplパターンで実装完全隠蔽
 * - ヘッダーに重い依存関係なし
 */
class LightMessagingBus {
public:
    /**
     * @brief コンストラクタ
     * @param nodeId ノード識別ID
     */
    explicit LightMessagingBus(const std::string& nodeId);
    
    /**
     * @brief デストラクタ
     */
    ~LightMessagingBus();
    
    // コピー・ムーブ禁止（RAII）
    LightMessagingBus(const LightMessagingBus&) = delete;
    LightMessagingBus& operator=(const LightMessagingBus&) = delete;
    LightMessagingBus(LightMessagingBus&&) = delete;
    LightMessagingBus& operator=(LightMessagingBus&&) = delete;
    
    /**
     * @brief 型安全メッセージ送信
     * 
     * 旧版: send(intents::SYSTEM_ERROR, {{"error", error}});
     * 新版: sendMessage<SystemError>(message_types::SYSTEM_ERROR, {error, core});
     * 
     * @tparam MessageType メッセージ構造体型
     * @param messageType メッセージタイプ識別子
     * @param message メッセージデータ
     * @param options 配信オプション
     * @return 送信成功時true
     */
    template<typename MessageType>
    bool sendMessage(const std::string& messageType, 
                    const MessageType& message,
                    const LightOptions& options = LightOptions()) {
        return sendMessageImpl(messageType, &message, sizeof(MessageType), options);
    }
    
    /**
     * @brief 型安全メッセージ購読
     * 
     * 旧版: on(intents::SYSTEM_ERROR, [](const nlohmann::json& data) { ... });
     * 新版: subscribeMessage<SystemError>(message_types::SYSTEM_ERROR, 
     *                                    [](const SystemError& msg) { ... }); 
     * 
     * @tparam MessageType メッセージ構造体型
     * @param messageType メッセージタイプ識別子
     * @param handler メッセージハンドラー
     * @return 購読成功時true
     */
    template<typename MessageType>
    bool subscribeMessage(const std::string& messageType,
                         std::function<void(const MessageType&)> handler) {
        auto wrapper = [handler](const void* data, size_t size) {
            if (size == sizeof(MessageType)) {
                const MessageType* msg = static_cast<const MessageType*>(data);
                handler(*msg);
            }
        };
        return subscribeMessageImpl(messageType, wrapper);
    }
    
    /**
     * @brief 非同期メッセージ送信
     * 
     * 将来拡張用 - 現在は同期版のラッパー
     */
    template<typename MessageType>
    std::future<bool> sendMessageAsync(const std::string& messageType,
                                      const MessageType& message,
                                      const LightOptions& options = LightOptions()) {
        return std::async(std::launch::deferred, [this, messageType, message, options]() {
            return sendMessage<MessageType>(messageType, message, options);
        });
    }
    
    /**
     * @brief メッセージ処理開始
     * 
     * バックグラウンドでメッセージ処理を開始
     */
    void startProcessing();
    
    /**
     * @brief メッセージ処理停止
     * 
     * 安全にメッセージ処理を停止
     */
    void stopProcessing();
    
    /**
     * @brief ノードID取得
     */
    std::string getNodeId() const;
    
    /**
     * @brief 統計情報取得
     */
    struct Statistics {
        uint64_t messagesSent = 0;
        uint64_t messagesReceived = 0;
        uint64_t errorsCount = 0;
        uint64_t avgProcessingTimeMs = 0;
    };
    
    Statistics getStatistics() const;
    
    /**
     * @brief デバッグ情報出力
     * @param enable デバッグ有効化
     */
    void setDebugMode(bool enable);
    
    /**
     * @brief 簡易メッセージ送信（バイナリデータ版）
     * @param messageType メッセージタイプ
     * @param data バイナリデータ
     * @param size データサイズ
     * @param options 配信オプション
     * @return 送信成功時true
     */
    bool sendBinaryMessage(const std::string& messageType,
                          const void* data,
                          size_t size,
                          const LightOptions& options = LightOptions());
    
    /**
     * @brief 簡易メッセージ購読（バイナリデータ版）
     * @param messageType メッセージタイプ
     * @param handler バイナリデータハンドラー
     * @return 購読成功時true
     */
    bool subscribeBinaryMessage(const std::string& messageType,
                               std::function<void(const void*, size_t)> handler);

private:
    /**
     * @brief 内部実装（Pimplパターン）
     * 
     * 重い依存関係（JSON、ネットワーク等）を完全隠蔽
     * ヘッダーファイルにはいっさい露出させない
     */
    class Impl;
    std::unique_ptr<Impl> pImpl_;
    
    /**
     * @brief メッセージ送信内部実装
     * 
     * templateから呼び出される共通処理
     * バイナリデータとして扱うことで型に依存しない
     */
    bool sendMessageImpl(const std::string& messageType,
                        const void* data, 
                        size_t size,
                        const LightOptions& options);
    
    /**
     * @brief メッセージ購読内部実装
     * 
     * templateから呼び出される共通処理
     */
    bool subscribeMessageImpl(const std::string& messageType,
                             std::function<void(const void*, size_t)> handler);
    
    friend class LightMessagingFactory;
};

/**
 * @brief 軽量メッセージバス用ファクトリー
 * 
 * オブジェクト作成を隠蔽し、設定の一元管理を実現
 */
class LightMessagingFactory {
public:
    /**
     * @brief 軽量メッセージバス作成
     * 
     * @param nodeId ノード識別ID
     * @param config 設定（将来拡張用）
     * @return メッセージバスインスタンス
     */
    static std::unique_ptr<LightMessagingBus> create(const std::string& nodeId);
    
    /**
     * @brief グローバル設定
     * 
     * 全メッセージバスに適用される設定
     */
    struct GlobalConfig {
        bool debugMode = false;
        int defaultTimeoutMs = 5000;
        size_t maxMessageSize = 1024 * 1024; // 1MB
        bool enableStatistics = true;
    };
    
    static void setGlobalConfig(const GlobalConfig& config);
    static GlobalConfig getGlobalConfig();

private:
    static GlobalConfig globalConfig_;
};

/**
 * @brief Core向け便利ラッパー
 * 
 * 各Coreで簡単に使える軽量インターフェース
 */
class CoreMessagingHelper {
public:
    explicit CoreMessagingHelper(const std::string& coreName);
    ~CoreMessagingHelper();
    
    // システムメッセージ送信ヘルパー
    void sendError(const std::string& error);
    void sendWarning(const std::string& warning);
    void sendInfo(const std::string& info);
    void sendInitialized(const std::string& version);
    
    // 設定メッセージ送信ヘルパー
    void sendSettingsChanged(const std::string& path, 
                           const std::string& oldValue,
                           const std::string& newValue,
                           const std::string& type);
    
    // メッセージ購読ヘルパー
    template<typename MessageType>
    void subscribe(const std::string& messageType,
                  std::function<void(const MessageType&)> handler) {
        if (messagingBus_) {
            messagingBus_->subscribeMessage<MessageType>(messageType, handler);
        }
    }
    
    void startProcessing();
    void stopProcessing();

private:
    std::string coreName_;
    std::unique_ptr<LightMessagingBus> messagingBus_;
};

} // namespace nyamesh2

/**
 * @brief 便利マクロ定義
 * 
 * Core実装で使いやすくするためのマクロ
 */
#define NYAMESH_SEND_ERROR(core, error) \
    core.sendError(error)

#define NYAMESH_SEND_WARNING(core, warning) \
    core.sendWarning(warning)

#define NYAMESH_SEND_INFO(core, info) \
    core.sendInfo(info)

#define NYAMESH_SUBSCRIBE(core, MessageType, messageType, handler) \
    core.subscribe<MessageType>(messageType, handler)