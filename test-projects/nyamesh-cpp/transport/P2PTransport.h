#pragma once

/**
 * @file P2PTransport.h
 * @brief nyacore v14.0 P2PTransport - P2P mesh network transport
 * 
 * Transport injection pattern implementation for P2P communication
 * Based on JavaScript nyamesh transport.js architecture
 * 
 * @author CharmCode Architecture Team + Claude Code
 * @version Phase 6.2 - JavaScript Transport complete port
 * @date 2025-07-18
 */

// #include "ITransport.h"  // REMOVED - nyacore dependency
#include "TransportBase.h"
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>
#include <set>
#include <thread>
#include <condition_variable>
#include <optional>
#include <shared_mutex>

namespace nyamesh2 {
namespace transport {

/**
 * @brief P2P Transport configuration
 * P2P mesh network specific settings
 */
struct P2PTransportConfig {
    std::string nodeId;                      ///< Unique node ID
    std::string role = "peer";               ///< Node role (hub, peer, etc.)
    bool autoJoinMesh = true;                ///< Auto join mesh network
    int maxConnections = 100;                ///< Maximum peer connections
    int discoveryInterval = 30000;           ///< Capability discovery interval (ms)
    bool enableCapabilityBroadcast = true;   ///< Enable capability broadcasting
    std::vector<std::string> capabilities;   ///< Node capabilities
    
    // Transport base config fields
    std::string name = "P2PTransport";
    bool autoReconnect = true;
    int reconnectInterval = 5000;
    int messageTimeout = 30000;
    bool enableLogging = true;
    
    P2PTransportConfig() = default;
};

/**
 * @brief P2P Node representation
 * Represents a peer in the mesh network
 */
struct P2PNode {
    std::string nodeId;
    std::string role;
    std::set<std::string> capabilities;
    std::chrono::system_clock::time_point lastSeen;
    bool isConnected = false;
    
    P2PNode() = default;
    P2PNode(const std::string& id, const std::string& nodeRole) 
        : nodeId(id), role(nodeRole), lastSeen(std::chrono::system_clock::now()) {}
    
    nlohmann::json toJson() const {
        nlohmann::json capArray = nlohmann::json::array();
        for (const auto& cap : capabilities) {
            capArray.push_back(cap);
        }
        
        return {
            {"nodeId", nodeId},
            {"role", role},
            {"capabilities", capArray},
            {"lastSeen", std::chrono::duration_cast<std::chrono::milliseconds>(
                lastSeen.time_since_epoch()).count()},
            {"isConnected", isConnected}
        };
    }
};

/**
 * @brief P2P Message structure
 * Enhanced message for P2P routing
 */
struct P2PMessage {
    std::string id;
    std::string type;
    std::string sourceNodeId;
    std::string targetNodeId;
    std::string capability;
    nlohmann::json payload;  // TODO: Define nyamesh2::Message
    std::chrono::system_clock::time_point timestamp;
    int hopCount = 0;
    
    P2PMessage() = default;
    P2PMessage(const std::string& msgType, const std::string& source, 
               const std::string& target, const nlohmann::json& msg)
        : type(msgType), sourceNodeId(source), targetNodeId(target), // payload(msg),  // TODO: Fix Message type
          timestamp(std::chrono::system_clock::now()) {
        id = source + "_" + std::to_string(timestamp.time_since_epoch().count());
    }
    
    nlohmann::json toJson() const {
        return {
            {"id", id},
            {"type", type},
            {"sourceNodeId", sourceNodeId},
            {"targetNodeId", targetNodeId},
            {"capability", capability},
            {"payload", nlohmann::json{}},
            {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                timestamp.time_since_epoch()).count()},
            {"hopCount", hopCount}
        };
    }
};

/**
 * @brief P2PTransport implementation
 * P2P mesh network transport based on JavaScript nyamesh architecture
 * 
 * Features:
 * - Capability-based peer discovery
 * - Mesh network routing
 * - Auto-reconnection
 * - Message broadcasting
 * - Transport injection pattern
 */
class P2PTransport : public TransportBase {
private:
    // Core configuration
    P2PTransportConfig config_;
    std::unordered_map<std::string, nlohmann::json> options_;
    mutable std::mutex optionsMutex_;
    
    // P2P network state
    // Transport state enum
    enum class TransportState {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        ERROR
    };
    
    std::atomic<TransportState> state_{TransportState::DISCONNECTED};
    std::atomic<bool> initialized_{false};
    std::atomic<bool> joinedMesh_{false};
    
    // Peer management
    std::unordered_map<std::string, P2PNode> connectedPeers_;
    mutable std::shared_mutex peersMutex_;
    
    // Message handling
    std::unordered_map<std::string, std::vector<MessageHandler>> channelHandlers_;
    mutable std::mutex handlersMutex_;
    
    // Statistics are handled via getStats() method
    std::atomic<uint64_t> messagesSent_{0};
    std::atomic<uint64_t> messagesReceived_{0};
    std::atomic<uint64_t> messagesRouted_{0};
    std::chrono::steady_clock::time_point startTime_;
    
    // Background threads
    std::thread discoveryThread_;
    std::thread maintenanceThread_;
    std::atomic<bool> shouldStop_{false};
    std::condition_variable stopCondition_;
    std::mutex stopMutex_;
    
    // Callbacks
    std::function<void(TransportState)> stateChangeCallback_;
    std::function<void(const std::string&)> errorCallback_;
    
protected:
    // Internal methods
    void updateState(TransportState newState);
    void recordError(const std::string& error);
    void logDebug(const std::string& message) const;
    
    // P2P specific methods
    void startBackgroundThreads();
    void stopBackgroundThreads();
    void discoveryLoop();
    void maintenanceLoop();
    void broadcastCapabilities();
    void handleP2PMessage(const P2PMessage& p2pMsg);
    void routeMessage(const P2PMessage& p2pMsg);
    std::vector<std::string> findPeersWithCapability(const std::string& capability);
    void cleanupDisconnectedPeers();

public:
    P2PTransport();
    explicit P2PTransport(const P2PTransportConfig& config);
    ~P2PTransport() override;
    
    // Transport interface implementation
    std::future<void> initialize() override;
    std::future<void> send(const std::string& message) override;
    void subscribe(MessageHandler handler) override;
    nlohmann::json getStats() const override;
    std::future<void> destroy() override;
    
    // Configuration and options
    void configure(const P2PTransportConfig& config);
    void setOption(const std::string& key, const nlohmann::json& value);
    std::optional<nlohmann::json> getOption(const std::string& key) const;
    
    // State management
    TransportState getState() const;
    std::string getTransportType() const override;
    std::string getDescription() const;
    bool isInitialized() const override;
    
    // Event callbacks
    void onStateChange(std::function<void(TransportState)> callback);
    void onError(std::function<void(const std::string&)> callback);
    
    // P2P specific API
    
    /**
     * @brief Join P2P mesh network
     * @return Success status
     */
    std::future<bool> joinMesh();
    
    /**
     * @brief Leave P2P mesh network
     * @return Success status
     */
    std::future<bool> leaveMesh();
    
    /**
     * @brief Connect to a specific peer
     * @param peerId Target peer ID
     * @return Success status
     */
    std::future<bool> connectToPeer(const std::string& peerId);
    
    /**
     * @brief Disconnect from a specific peer
     * @param peerId Target peer ID
     * @return Success status
     */
    std::future<bool> disconnectFromPeer(const std::string& peerId);
    
    /**
     * @brief Add capability to this node
     * @param capability Capability name
     */
    void addCapability(const std::string& capability);
    
    /**
     * @brief Remove capability from this node
     * @param capability Capability name
     */
    void removeCapability(const std::string& capability);
    
    /**
     * @brief Get current node capabilities
     * @return Set of capabilities
     */
    std::set<std::string> getCapabilities() const;
    
    /**
     * @brief Broadcast message to all connected peers
     * @param message Message to broadcast
     * @return Number of peers reached
     */
    std::future<int> broadcastMessage(const nlohmann::json& message);
    
    /**
     * @brief Send direct message to specific peer
     * @param targetPeerId Target peer ID
     * @param message Message to send
     * @return Success status
     */
    std::future<bool> sendDirectMessage(const std::string& targetPeerId, const nlohmann::json& message);
    
    /**
     * @brief Get connected peers information
     * @return Map of peer ID to P2PNode
     */
    std::unordered_map<std::string, P2PNode> getConnectedPeers() const;
    
    /**
     * @brief Get network statistics
     * @return P2P network statistics
     */
    nlohmann::json getNetworkStats() const;
    
    /**
     * @brief Check if peer has specific capability
     * @param peerId Peer ID
     * @param capability Capability name
     * @return True if peer has capability
     */
    bool peerHasCapability(const std::string& peerId, const std::string& capability) const;
    
    /**
     * @brief Get node ID
     * @return This node's ID
     */
    std::string getNodeId() const { return config_.nodeId; }
    
    /**
     * @brief Get node role
     * @return This node's role
     */
    std::string getNodeRole() const { return config_.role; }
    
    /**
     * @brief Check if joined to mesh
     * @return True if joined to mesh network
     */
    bool isJoinedToMesh() const { return joinedMesh_.load(); }
    
    /**
     * @brief Get connection count
     * @return Number of connected peers
     */
    size_t getConnectionCount() const;
};

} // namespace transport
} // namespace nyamesh2

// Usage example:
/*
// Create P2P transport for nyamesh2
auto p2pTransport = std::make_shared<nyamesh2::transport::P2PTransport>();
nyamesh2::transport::P2PTransportConfig config;
config.nodeId = "node-" + std::to_string(std::time(nullptr));
config.role = "peer";
config.capabilities = {"file.processing", "data.storage"};
p2pTransport->configure(config);

// Initialize and use
p2pTransport->initialize();
p2pTransport->send("message data");
*/