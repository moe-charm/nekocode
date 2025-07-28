/**
 * @file nyamesh.h
 * @brief NyaMesh - Thread-Safe Self-Contained Core
 * 
 * 設計者指摘による完全修正版:
 * - スレッドセーフティ完全対応
 * - ライフサイクル管理完備
 * - メモリ安全性確保
 * - JavaScript版との完全互換
 * - 外部から安全に使えるコア設計
 * 
 * @author nyamesh2 team + 設計者
 * @date 2025-07-22
 */

#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <memory>
#include <future>
#include <chrono>
#include <thread>
#include <queue>
#include <condition_variable>
#include <set>

// SafeMessage definition for this header only

// Include full-featured JSON library for JavaScript-compatible messaging
#include "../nlohmann_json.hpp"

// v2.1 additions
#include "../intents/IntentCommonOptions.h"
#include "../debug/NyaMeshDebugLogger.h"

// Forward declarations to avoid circular dependencies
namespace nyamesh2 {
namespace transport {
    class InProcessTransport;
    class NyaMesh2TransportAdapter;
}
}

namespace nyamesh2 {

/**
 * @brief Thread-safe message structure with nlohmann::json
 */
struct SafeMessage {
    std::string type;
    std::string category;
    nlohmann::json data;
    std::chrono::system_clock::time_point timestamp;
    std::string from;
    std::string to;
    
    SafeMessage() = default;
    SafeMessage(const std::string& t, const nlohmann::json& d, const std::string& c = "Notice") 
        : type(t), category(c), data(d), timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief Thread-safe self-contained NyaMesh core
 * 
 * JavaScript版との完全互換を保ちつつ、C++環境でのスレッドセーフティとメモリ安全性を確保
 */
class NyaMesh {
public:
    // JavaScript版互換の公開インターフェース
    using MessageHandler = std::function<void(const SafeMessage&)>;
    
    /**
     * @brief Constructor
     * @param id Node ID (JavaScript版と同様)
     * @param options Configuration options
     */
    explicit NyaMesh(const std::string& id = "", bool debug = false);
    
    /**
     * @brief Destructor - ensures safe cleanup
     */
    virtual ~NyaMesh();
    
    // === JavaScript版互換メソッド ===
    
    /**
     * @brief Subscribe to message type (JavaScript版のon()と同等)
     * @param messageType Message type to listen for
     * @param handler Handler function
     */
    void on(const std::string& messageType, MessageHandler handler);
    
    /**
     * @brief Unsubscribe from message type (JavaScript版のoff()と同等)
     * @param messageType Message type
     * @param handler Handler to remove (nullptr = remove all)
     */
    void off(const std::string& messageType, MessageHandler handler = nullptr);
    
    /**
     * @brief Send message (JavaScript版のsend()完全互換)
     * @param messageType Message type
     * @param data Message data (supports full JSON structures for P2P)
     * @param to Target node (empty = broadcast)
     * @deprecated Use send() with IntentCommonOptions parameter instead (v2.1)
     */
    [[deprecated("Use send() with IntentCommonOptions parameter instead (v2.1)")]]
    void send(const std::string& messageType, const nlohmann::json& data, const std::string& to = "");
    
    /**
     * @brief Send message with v2.1 common options
     * @param messageType Message type (intent type)
     * @param data Message data (payload)
     * @param options Common options (broadcast, parallelSafe, debug, etc.)
     * @since v2.1
     */
    void send(const std::string& messageType, 
              const nlohmann::json& data, 
              const IntentCommonOptions& options);
    
    /**
     * @brief Send message with v2.1 common options and explicit target
     * @param messageType Message type (intent type)
     * @param data Message data (payload)
     * @param to Target node ID (empty = use options.broadcast)
     * @param options Common options
     * @since v2.1
     */
    void send(const std::string& messageType, 
              const nlohmann::json& data,
              const std::string& to,
              const IntentCommonOptions& options);
    
    /**
     * @brief Initialize the mesh node (JavaScript版のinitialize()と同等)
     * @return Future that completes when initialization is done
     */
    std::future<void> initialize();
    
    /**
     * @brief Shutdown the mesh node safely
     * @return Future that completes when shutdown is done
     */
    std::future<void> shutdown();
    
    // === Status and diagnostics ===
    
    /**
     * @brief Check if node is initialized
     */
    bool isInitialized() const;
    
    /**
     * @brief Get node ID
     */
    std::string getId() const;
    
    /**
     * @brief Get statistics (JavaScript版互換の構造化データ)
     */
    nlohmann::json getStats() const;
    
    /**
     * @brief Enable/disable debug mode
     */
    void setDebug(bool enable);
    
    /**
     * @brief Connect transport layer for P2P communication
     * @param transport Transport instance for routing messages
     */
    void connectTransport(std::shared_ptr<nyamesh2::transport::InProcessTransport> transport);
    
    /**
     * @brief Disconnect transport layer
     */
    void disconnectTransport();
    
    /**
     * @brief Connect P2P transport adapter for unified P2P operations
     * @param adapter P2P transport adapter instance
     */
    void connectP2PAdapter(std::shared_ptr<transport::NyaMesh2TransportAdapter> adapter);
    
    /**
     * @brief Disconnect P2P transport adapter
     */
    void disconnectP2PAdapter();
    
    /**
     * @brief Check if P2P adapter is connected
     */
    bool hasP2PAdapter() const;

protected:
    // === Protected members for inheritance ===
    
    /**
     * @brief Node ID - accessible to derived classes
     */
    std::string nodeId_;
    
    /**
     * @brief Debug state - accessible to derived classes
     */
    std::atomic<bool> debug_{false};
    
    /**
     * @brief Statistics - accessible to derived classes
     */
    std::atomic<uint64_t> messagesSent_{0};
    std::atomic<uint64_t> messagesReceived_{0};
    std::atomic<uint64_t> messagesProcessed_{0};
    std::atomic<uint64_t> handlersRegistered_{0};
    std::chrono::system_clock::time_point startTime_;
    
    /**
     * @brief Log message (thread-safe) - available to derived classes
     */
    void log(const std::string& message) const;
    
    /**
     * @brief Generate unique message ID - available to derived classes
     */
    std::string generateMessageId() const;
    
    /**
     * @brief Core send implementation - single responsibility
     * @param messageType Message type (intent type)
     * @param data Message data (payload)  
     * @param to Target node ID (empty = broadcast)
     * @param options Common options (nullptr = default)
     * @note Internal method - all public send() methods delegate to this
     */
    void sendCore(const std::string& messageType, 
                  const nlohmann::json& data, 
                  const std::string& to,
                  const IntentCommonOptions* options);
    
    /**
     * @brief Hook for derived classes - called after initialization
     */
    virtual void onInitialized() {}
    
    /**
     * @brief Hook for derived classes - called before shutdown
     */
    virtual void onShutdown() {}
    
    /**
     * @brief Hook for derived classes - called before message processing
     */
    virtual void onMessageReceived(const SafeMessage& message) {}
    
    /**
     * @brief Hook for derived classes - called after message sent
     */
    virtual void onMessageSent(const SafeMessage& message) {}

private:
    // === Core state management (private) ===
    
    std::atomic<bool> initialized_{false};
    std::atomic<bool> shuttingDown_{false};
    
    // === Thread-safe message handling ===
    
    struct HandlerInfo {
        MessageHandler handler;
        std::weak_ptr<void> ownerToken;  // For lifetime tracking
        std::chrono::system_clock::time_point registeredAt;
        
        HandlerInfo(MessageHandler h) : handler(std::move(h)), registeredAt(std::chrono::system_clock::now()) {}
    };
    
    std::unordered_map<std::string, std::vector<HandlerInfo>> handlers_;
    mutable std::shared_mutex handlersMutex_;
    
    // === Message processing queue ===
    
    std::queue<SafeMessage> messageQueue_;
    mutable std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    std::thread processingThread_;
    
    // === Transport integration ===
    
    std::shared_ptr<nyamesh2::transport::InProcessTransport> transport_;
    mutable std::mutex transportMutex_;
    
    // === P2P Adapter integration ===
    
    std::shared_ptr<transport::NyaMesh2TransportAdapter> p2pAdapter_;
    mutable std::mutex p2pAdapterMutex_;
    
    // === Internal methods ===
    
    /**
     * @brief Message processing thread main loop
     */
    void messageProcessingLoop();
    
    /**
     * @brief Process a single message safely
     */
    void processMessage(const SafeMessage& message);
    
    /**
     * @brief Publish message locally to handlers
     */
    void publishLocal(const SafeMessage& message);
    
    /**
     * @brief Send message to transport layer
     */
    void publishToTransport(const SafeMessage& message);
    
    /**
     * @brief Clean up expired handlers
     */
    void cleanupExpiredHandlers();
    
    /**
     * @brief Validate message before processing
     */
    bool validateMessage(const SafeMessage& message) const;
    
    /**
     * @brief Safe initialization implementation
     */
    void initializeImpl();
    
    /**
     * @brief Safe shutdown implementation
     */
    void shutdownImpl();
};

// === Factory functions ===

/**
 * @brief Create a safe NyaMesh instance
 * @param id Node ID
 * @param debug Enable debug mode
 * @return Shared pointer to NyaMesh instance
 */
std::shared_ptr<NyaMesh> createNyaMesh(const std::string& id = "", bool debug = false);

/**
 * @brief Create multiple interconnected safe NyaMesh instances
 * @param count Number of instances to create
 * @param debug Enable debug mode
 * @return Vector of NyaMesh instances
 */
std::vector<std::shared_ptr<NyaMesh>> createNyaMeshNetwork(int count, bool debug = false);

} // namespace nyamesh2