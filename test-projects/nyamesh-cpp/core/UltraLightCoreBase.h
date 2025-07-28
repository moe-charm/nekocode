/**
 * @file UltraLightCoreBase.h
 * @brief 究極軽量Coreベースクラス - ヘッダー依存完全排除
 * 
 * 革命的設計理念：
 * - JSON依存完全排除（Gemini先生指摘対応）
 * - 重いヘッダー一切なし（コンパイル時間革命）
 * - template活用で型安全性確保（ChatGPT先生提案）
 * - 完全Pimplで実装隠蔽（真の独立性実現）
 * - Phase C（C++20モジュール）準備完了
 * 
 * 対比：
 * 旧CoreBase.h: #include "nyamesh.h" → JSON地獄
 * 新版: <string>, <memory>のみ → 完全軽量
 * 
 * @version 1.0 - Header Independence Revolution
 * @date 2025-07-25
 */

#pragma once

#include <string>
#include <memory>
#include <functional>
#include <future>
#include <vector>

// ✅ 軽量メッセージ構造体のみインクルード（JSON不要）
#include "../messages/CoreMessages.h"

// ✅ 前方宣言のみ - 重い実装は完全隠蔽
namespace nyamesh2 {
    class CoreMessagingHelper;
    class LightMessagingBus;
    struct LightOptions;
}

namespace charmcode {

/**
 * @brief 究極軽量Coreベースクラス
 * 
 * 革命的特徴：
 * - ヘッダーレベルでJSON完全排除
 * - template使用で型安全メッセージング
 * - 完全Pimpl実装で真の独立性
 * - Phase C（C++20モジュール）対応準備済み
 * 
 * 使用例：
 * ```cpp
 * class MyCore : public UltraLightCoreBase {
 * public:
 *     MyCore() : UltraLightCoreBase("MyCore") {}
 *     
 *     void someMethod() {
 *         // ✅ 型安全メッセージ送信
 *         sendMessage<messages::SystemInfo>(
 *             message_types::SYSTEM_INFO,
 *             messages::SystemInfo("処理完了", getCoreName())
 *         );
 *     }
 * };
 * ```
 */
class UltraLightCoreBase {
public:
    /**
     * @brief コンストラクタ
     * @param coreName Core識別名
     */
    explicit UltraLightCoreBase(const std::string& coreName);
    
    /**
     * @brief デストラクタ
     */
    virtual ~UltraLightCoreBase();
    
    // コピー・ムーブ禁止（RAII）
    UltraLightCoreBase(const UltraLightCoreBase&) = delete;
    UltraLightCoreBase& operator=(const UltraLightCoreBase&) = delete;
    UltraLightCoreBase(UltraLightCoreBase&&) = delete;
    UltraLightCoreBase& operator=(UltraLightCoreBase&&) = delete;
    
    // ===============================================
    // 🔥 革命的型安全メッセージング
    // ===============================================
    
    /**
     * @brief 型安全メッセージ送信
     * 
     * 旧版: send(intents::SYSTEM_ERROR, {{"error", error}});  ← JSON地獄
     * 新版: sendMessage<SystemError>(SYSTEM_ERROR, {error, core});  ← 型安全
     * 
     * @tparam MessageType メッセージ構造体型
     * @param messageType メッセージタイプ識別子
     * @param message メッセージデータ
     * @param broadcast ブロードキャスト送信フラグ
     * @return 送信成功時true
     */
    template<typename MessageType>
    bool sendMessage(const std::string& messageType, 
                    const MessageType& message,
                    bool broadcast = false) {
        return sendMessageImpl(messageType, &message, sizeof(MessageType), broadcast);
    }
    
    /**
     * @brief 型安全メッセージ購読
     * 
     * 旧版: on(intents::SYSTEM_ERROR, [](const nlohmann::json& data) { ... });
     * 新版: subscribeMessage<SystemError>(SYSTEM_ERROR, [](const SystemError& msg) { ... });
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
     * 将来拡張・Phase C対応
     */
    template<typename MessageType>
    std::future<bool> sendMessageAsync(const std::string& messageType,
                                      const MessageType& message,
                                      bool broadcast = false) {
        return std::async(std::launch::deferred, [this, messageType, message, broadcast]() {
            return sendMessage<MessageType>(messageType, message, broadcast);
        });
    }
    
    // ===============================================
    // 🎯 便利な通知メソッド群
    // ===============================================
    
    /**
     * @brief エラー通知送信
     * @param error エラー内容
     */
    void sendError(const std::string& error);
    
    /**
     * @brief 警告通知送信
     * @param warning 警告内容
     */
    void sendWarning(const std::string& warning);
    
    /**
     * @brief 情報通知送信
     * @param info 情報内容
     */
    void sendInfo(const std::string& info);
    
    /**
     * @brief 初期化完了通知
     * @param version Coreバージョン
     */
    void sendInitialized(const std::string& version = "1.0");
    
    // ===============================================
    // 🛠️ Core管理インターフェース
    // ===============================================
    
    /**
     * @brief Core名取得
     */
    std::string getCoreName() const;
    
    /**
     * @brief Core ID取得（一意識別子）
     */
    std::string getCoreId() const;
    
    /**
     * @brief メッセージ処理開始
     */
    void startProcessing();
    
    /**
     * @brief メッセージ処理停止
     */
    void stopProcessing();
    
    /**
     * @brief デバッグモード設定
     * @param enable デバッグ有効化
     */
    void setDebugMode(bool enable);
    
    /**
     * @brief 統計情報取得
     */
    struct CoreStatistics {
        uint64_t messagesSent = 0;
        uint64_t messagesReceived = 0;
        uint64_t errorsCount = 0;
        uint64_t uptimeMs = 0;
    };
    
    CoreStatistics getStatistics() const;
    
    // ===============================================
    // 🎨 継承先実装インターフェース
    // ===============================================
    
protected:
    /**
     * @brief Core初期化後フック
     * 
     * Core固有の初期化処理を実装
     * メッセージ購読設定等を行う
     */
    virtual void onInitialized() = 0;
    
    /**
     * @brief Core終了前フック
     * 
     * Core固有のクリーンアップ処理を実装
     */
    virtual void onShutdown() = 0;
    
    /**
     * @brief Intent購読ヘルパー
     * 
     * 継承先で簡単に購読設定できるヘルパー
     * 
     * @tparam MessageType メッセージ型
     * @param messageType メッセージタイプ
     * @param handler ハンドラー関数
     */
    template<typename MessageType>
    void subscribeIntent(const std::string& messageType,
                        std::function<void(const MessageType&)> handler) {
        subscribeMessage<MessageType>(messageType, handler);
        logSubscription(messageType);
    }
    
    /**
     * @brief ログ出力（デバッグ用）
     * @param message ログメッセージ
     */
    void log(const std::string& message);
    
    /**
     * @brief ユニークID生成
     * @return ユニークID文字列
     */
    std::string generateUniqueId();

private:
    // ===============================================
    // 🔒 完全Pimpl実装 - 真の独立性
    // ===============================================
    
    /**
     * @brief 内部実装クラス（完全隠蔽）
     * 
     * 重い依存関係（nyamesh、JSON等）をすべて隠蔽
     * ヘッダーレベルでは一切見えない
     */
    class Impl;
    std::unique_ptr<Impl> pImpl_;
    
    /**
     * @brief メッセージ送信内部実装
     * 
     * templateから呼び出される共通処理
     */
    bool sendMessageImpl(const std::string& messageType,
                        const void* data, 
                        size_t size,
                        bool broadcast);
    
    /**
     * @brief メッセージ購読内部実装
     * 
     * templateから呼び出される共通処理
     */
    bool subscribeMessageImpl(const std::string& messageType,
                             std::function<void(const void*, size_t)> handler);
    
    /**
     * @brief 購読ログ出力
     */
    void logSubscription(const std::string& messageType);
    
    friend class CoreFactory;  // Factory経由での作成を許可
};

// ===============================================
// 🏭 Core作成用ファクトリー
// ===============================================

/**
 * @brief Coreファクトリークラス
 * 
 * Core作成の統一インターフェース
 * Phase C（C++20モジュール）対応準備
 */
class CoreFactory {
public:
    /**
     * @brief Core作成
     * @tparam CoreType 作成するCore型
     * @param coreName Core名
     * @param args Core固有引数
     * @return Coreインスタンス
     */
    template<typename CoreType, typename... Args>
    static std::unique_ptr<CoreType> create(const std::string& coreName, Args&&... args) {
        auto core = std::make_unique<CoreType>(std::forward<Args>(args)...);
        
        // 共通初期化処理
        core->startProcessing();
        core->onInitialized();
        
        return core;
    }
    
    /**
     * @brief グローバルCore設定
     */
    struct GlobalConfig {
        bool debugMode = false;
        int messageTimeoutMs = 5000;
        bool enableStatistics = true;
        size_t maxMessageSize = 1024 * 1024;
    };
    
    static void setGlobalConfig(const GlobalConfig& config);
    static GlobalConfig getGlobalConfig();

private:
    static GlobalConfig globalConfig_;
};

} // namespace charmcode

/**
 * @brief 便利マクロ定義（Phase C準備）
 * 
 * C++20モジュール移行時に簡単に置換可能
 */
#define NYAMESH_V22_SEND_ERROR(core, error) \
    (core).sendError(error)

#define NYAMESH_V22_SEND_WARNING(core, warning) \
    (core).sendWarning(warning)

#define NYAMESH_V22_SEND_INFO(core, info) \
    (core).sendInfo(info)

#define NYAMESH_V22_SUBSCRIBE(core, MessageType, messageType, handler) \
    (core).subscribeIntent<MessageType>(messageType, handler)

#define NYAMESH_V22_SEND_MESSAGE(core, MessageType, messageType, message) \
    (core).sendMessage<MessageType>(messageType, message)

/**
 * @brief Phase C（C++20モジュール）対応準備
 * 
 * 将来のモジュール移行時に使用
 */
#ifdef __cpp_modules
// export module nyamesh.core_base;
// export import nyamesh.messages;
#endif