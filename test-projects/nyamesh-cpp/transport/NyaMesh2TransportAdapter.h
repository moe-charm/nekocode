/**
 * @file NyaMesh2TransportAdapter.h
 * @brief Lightweight P2P Transport Adapter for nyamesh2 core integration
 * 
 * 設計目標:
 * - SafeMessage完全統合
 * - nlohmann::json直接使用
 * - 依存関係完全整理
 * - NyaMeshコア直接統合
 * - 段階的拡張可能設計
 * 
 * @author nyamesh2 team
 * @date 2025-07-22
 */

#pragma once

#include "InProcessTransport.h"
#include "TransportBase.h"
#include "../nlohmann_json.hpp"

// Forward declaration for SafeMessage
namespace nyamesh2 {
    struct SafeMessage;
}
#include <memory>
#include <future>
#include <atomic>
#include <chrono>
#include <string>
#include <vector>
#include <mutex>
#include <functional>

namespace nyamesh2 {
namespace transport {

/**
 * @brief Transport selection for optimal performance
 */
enum class AdapterTransportType {
    AUTO,           ///< Automatic selection
    IN_PROCESS,     ///< InProcessTransport (testing/local)
    NYAMESH2,       ///< NyaMesh2 high-performance (future)
    MILLION_PEER    ///< MillionPeer large-scale (future)  
};

/**
 * @brief Transport requirements for automatic selection
 */
struct AdapterRequirements {
    size_t expectedPeerCount = 0;
    bool needsLowLatency = false;
    bool needsHighPerformance = false;
    bool needsLargeScale = false;
    std::string useCase;
    
    nlohmann::json toJson() const {
        return {
            {"expectedPeerCount", expectedPeerCount},
            {"needsLowLatency", needsLowLatency},
            {"needsHighPerformance", needsHighPerformance},
            {"needsLargeScale", needsLargeScale},
            {"useCase", useCase}
        };
    }
};

/**
 * @brief Lightweight P2P Transport Adapter for nyamesh2
 * 
 * 現在の機能:
 * - InProcessTransport統合
 * - SafeMessage完全対応
 * - 基本P2P操作
 * 
 * 将来拡張予定:
 * - NyaMeshTransport統合
 * - MillionPeerP2PTransport統合
 * - 自動最適選択
 * - パフォーマンス監視
 */
class NyaMesh2TransportAdapter {
private:
    // Current transport implementation
    std::shared_ptr<InProcessTransport> inProcessTransport_;
    AdapterTransportType currentTransport_ = AdapterTransportType::IN_PROCESS;
    AdapterRequirements requirements_;
    
    // Statistics
    mutable std::mutex statsMutex_;
    std::atomic<size_t> messagesSent_{0};
    std::atomic<size_t> messagesReceived_{0};
    std::atomic<size_t> peerCount_{0};
    std::chrono::system_clock::time_point startTime_;
    
    // Configuration
    std::string nodeId_;
    bool debug_ = false;
    
    // Message handling
    std::function<void(const SafeMessage&)> messageHandler_;

public:
    /**
     * @brief Constructor
     * @param nodeId Node identifier
     * @param debug Enable debug mode
     */
    explicit NyaMesh2TransportAdapter(const std::string& nodeId = "", bool debug = false);
    
    /**
     * @brief Destructor
     */
    ~NyaMesh2TransportAdapter();
    
    // === Configuration ===
    
    /**
     * @brief Configure transport requirements
     * @param requirements Transport requirements
     */
    void configure(const AdapterRequirements& requirements);
    
    /**
     * @brief Set transport type (currently only IN_PROCESS supported)
     * @param type Transport type
     */
    void setTransportType(AdapterTransportType type);
    
    /**
     * @brief Get current transport type
     */
    AdapterTransportType getCurrentTransport() const;
    
    // === Network Operations ===
    
    /**
     * @brief Initialize transport
     * @return Future that completes when initialization is done
     */
    std::future<void> initialize();
    
    /**
     * @brief Join P2P network (placeholder for future implementation)
     * @param bootstrapNodes Bootstrap node addresses  
     * @return Future that indicates success/failure
     */
    std::future<bool> joinNetwork(const std::vector<std::string>& bootstrapNodes = {});
    
    /**
     * @brief Leave P2P network
     * @return Future that indicates success/failure
     */
    std::future<bool> leaveNetwork();
    
    /**
     * @brief Send message to specific peer
     * @param peerId Target peer ID
     * @param message Message data (SafeMessage compatible)
     * @return Future that indicates success/failure
     */
    std::future<bool> sendToPeer(const std::string& peerId, const nlohmann::json& message);
    
    /**
     * @brief Broadcast message to all peers
     * @param message Message data
     * @return Future with number of peers reached
     */
    std::future<size_t> broadcast(const nlohmann::json& message);
    
    // === Message Handling ===
    
    /**
     * @brief Set message handler for received messages
     * @param handler Message handler function
     */
    void setMessageHandler(std::function<void(const SafeMessage&)> handler);
    
    // === Statistics and Monitoring ===
    
    /**
     * @brief Get current peer count
     */
    size_t getPeerCount() const;
    
    /**
     * @brief Get transport statistics
     */
    nlohmann::json getStats() const;
    
    /**
     * @brief Check if transport is initialized
     */
    bool isInitialized() const;
    
    /**
     * @brief Get transport type as string
     */
    std::string getTransportTypeString() const;
    
    // === Utility Functions ===
    
    /**
     * @brief Get node ID
     */
    std::string getNodeId() const { return nodeId_; }
    
    /**
     * @brief Enable/disable debug mode
     */
    void setDebug(bool enable) { debug_ = enable; }
    
    // === Future Extensions (Placeholders) ===
    
    /**
     * @brief Optimize for specific use case (future implementation)
     * @param useCase Use case identifier
     */
    void optimizeForUseCase(const std::string& useCase);
    
    /**
     * @brief Get transport recommendations (future implementation)
     */
    nlohmann::json getRecommendations() const;

private:
    // === Internal Implementation ===
    
    /**
     * @brief Initialize InProcess transport
     */
    void initializeInProcessTransport();
    
    /**
     * @brief Handle received transport message
     */
    void handleTransportMessage(const std::string& transportMessage);
    
    /**
     * @brief Convert SafeMessage to transport format
     */
    std::string safeMessageToTransport(const SafeMessage& message);
    
    /**
     * @brief Convert transport message to SafeMessage
     */
    SafeMessage transportToSafeMessage(const std::string& transportMessage);
    
    /**
     * @brief Log debug message
     */
    void log(const std::string& message) const;
    
    /**
     * @brief Update statistics
     */
    void updateStats(bool sent, bool received = false);
};

// ==========================================
// Factory Functions
// ==========================================

/**
 * @brief Create NyaMesh2 Transport Adapter
 * @param nodeId Node identifier
 * @param debug Enable debug mode
 */
std::shared_ptr<NyaMesh2TransportAdapter> createNyaMesh2TransportAdapter(
    const std::string& nodeId = "", bool debug = false);

/**
 * @brief Create NyaMesh2 Transport Adapter with requirements
 * @param requirements Transport requirements
 * @param nodeId Node identifier
 * @param debug Enable debug mode
 */
std::shared_ptr<NyaMesh2TransportAdapter> createNyaMesh2TransportAdapter(
    const AdapterRequirements& requirements, const std::string& nodeId = "", bool debug = false);

// ==========================================
// Utility Functions
// ==========================================

/**
 * @brief Create requirements for text editing use case
 */
AdapterRequirements createRequirementsForTextEditing();

/**
 * @brief Create requirements for file sharing use case
 */
AdapterRequirements createRequirementsForFileSharing();

/**
 * @brief Create requirements for search use case
 */
AdapterRequirements createRequirementsForSearch();

/**
 * @brief Get recommended transport type for use case
 */
AdapterTransportType getRecommendedTransportType(const std::string& useCase, size_t expectedPeers = 0);

} // namespace transport
} // namespace nyamesh2

/*
 * === NyaMesh2 Transport Adapter Usage Examples ===
 * 
 * // Basic usage
 * auto adapter = createNyaMesh2TransportAdapter("node-1", true);
 * adapter->initialize().wait();
 * 
 * // Set message handler
 * adapter->setMessageHandler([](const SafeMessage& msg) {
 *     std::cout << "Received: " << msg.type << " from " << msg.from << std::endl;
 * });
 * 
 * // Send messages
 * nlohmann::json data = {{"message", "Hello P2P!"}};
 * adapter->sendToPeer("node-2", data);
 * adapter->broadcast(data);
 * 
 * // Use case optimization
 * AdapterRequirements req = createRequirementsForTextEditing();
 * auto textAdapter = createNyaMesh2TransportAdapter(req, "text-node");
 * 
 * // Statistics
 * auto stats = adapter->getStats();
 * std::cout << "Peers: " << adapter->getPeerCount() << std::endl;
 */