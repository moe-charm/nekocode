/**
 * @file InProcessTransport.h
 * @brief Simple in-process transport for nyamesh2 testing and local communication
 * 
 * Provides message routing between multiple nyamesh2 instances within the same process.
 * Perfect for testing hierarchical systems like child creation.
 * 
 * @author nyamesh2 team
 * @date 2025-07-22
 */

#pragma once

#include "TransportBase.h"
#include <memory>
#include <future>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include "../nlohmann_json.hpp"

namespace nyamesh2 {
namespace transport {

/**
 * @brief Simple in-process message bus for routing messages between nyamesh2 nodes
 * 
 * This is a singleton that maintains a registry of all transport instances
 * and routes messages between them based on node IDs.
 */
class InProcessMessageBus {
private:
    static std::shared_ptr<InProcessMessageBus> instance_;
    static std::mutex instanceMutex_;
    
    // Registry of active transports
    std::unordered_map<std::string, class InProcessTransport*> transports_;
    mutable std::mutex transportsMutex_;
    
    // Statistics
    std::atomic<uint64_t> messagesRouted_{0};
    std::atomic<uint64_t> routingErrors_{0};
    
public:
    static std::shared_ptr<InProcessMessageBus> getInstance();
    
    void registerTransport(const std::string& nodeId, InProcessTransport* transport);
    void unregisterTransport(const std::string& nodeId);
    
    void routeMessage(const std::string& fromNodeId, const std::string& toNodeId, const std::string& message);
    void broadcastMessage(const std::string& fromNodeId, const std::string& message);
    
    std::vector<std::string> getRegisteredNodes() const;
    nlohmann::json getStats() const;
    
private:
    InProcessMessageBus() = default;
};

/**
 * @brief Simple in-process transport implementation
 * 
 * Routes messages through the InProcessMessageBus to other transport instances
 * within the same process. Perfect for testing and local communication.
 */
class InProcessTransport : public TransportBase {
private:
    std::string nodeId_;
    std::atomic<bool> initialized_{false};
    
    // Message handling
    std::vector<MessageHandler> handlers_;
    mutable std::mutex handlersMutex_;
    
    // Statistics
    std::atomic<uint64_t> messagesSent_{0};
    std::atomic<uint64_t> messagesReceived_{0};
    
public:
    explicit InProcessTransport(const std::string& nodeId = "");
    ~InProcessTransport() override;
    
    // TransportBase interface
    std::future<void> initialize() override;
    std::future<void> send(const std::string& message) override;
    void subscribe(MessageHandler handler) override;
    bool isInitialized() const override;
    std::string getTransportType() const override;
    nlohmann::json getStats() const override;
    std::future<void> destroy() override;
    
    // InProcess-specific methods
    std::string getNodeId() const { return nodeId_; }
    void setNodeId(const std::string& nodeId);
    
    // Called by InProcessMessageBus to deliver messages
    void deliverMessage(const std::string& message);
    
    // Direct messaging (bypasses routing)
    void sendDirect(const std::string& targetNodeId, const std::string& message);
    void broadcastDirect(const std::string& message);
    
private:
    void generateNodeId();
};

// ==========================================
// Factory Functions
// ==========================================

/**
 * @brief Create an InProcessTransport with auto-generated node ID
 */
std::shared_ptr<InProcessTransport> createInProcessTransport();

/**
 * @brief Create an InProcessTransport with specific node ID
 */
std::shared_ptr<InProcessTransport> createInProcessTransport(const std::string& nodeId);

} // namespace transport
} // namespace nyamesh2