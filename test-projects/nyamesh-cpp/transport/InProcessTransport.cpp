/**
 * @file InProcessTransport.cpp
 * @brief Implementation of InProcessTransport for nyamesh2 testing
 */

#include "InProcessTransport.h"
#include <iostream>
#include <chrono>
#include <random>
#include <algorithm>

namespace nyamesh2 {
namespace transport {

// ==========================================
// InProcessMessageBus Implementation
// ==========================================

std::shared_ptr<InProcessMessageBus> InProcessMessageBus::instance_;
std::mutex InProcessMessageBus::instanceMutex_;

std::shared_ptr<InProcessMessageBus> InProcessMessageBus::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex_);
    if (!instance_) {
        instance_.reset(new InProcessMessageBus());
    }
    return instance_;
}

void InProcessMessageBus::registerTransport(const std::string& nodeId, InProcessTransport* transport) {
    std::lock_guard<std::mutex> lock(transportsMutex_);
    transports_[nodeId] = transport;
    std::cout << "[InProcessMessageBus] Registered transport for node: " << nodeId << std::endl;
}

void InProcessMessageBus::unregisterTransport(const std::string& nodeId) {
    std::lock_guard<std::mutex> lock(transportsMutex_);
    auto it = transports_.find(nodeId);
    if (it != transports_.end()) {
        transports_.erase(it);
        std::cout << "[InProcessMessageBus] Unregistered transport for node: " << nodeId << std::endl;
    }
}

void InProcessMessageBus::routeMessage(const std::string& fromNodeId, const std::string& toNodeId, const std::string& message) {
    std::lock_guard<std::mutex> lock(transportsMutex_);
    
    auto it = transports_.find(toNodeId);
    if (it != transports_.end()) {
        // Create message with routing metadata
        nlohmann::json messageWithMeta = {
            {"from", fromNodeId},
            {"to", toNodeId},
            {"payload", message},
            {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()}
        };
        
        it->second->deliverMessage(messageWithMeta.dump());
        messagesRouted_++;
        
        std::cout << "[InProcessMessageBus] Routed message from " << fromNodeId << " to " << toNodeId << std::endl;
    } else {
        routingErrors_++;
        std::cout << "[InProcessMessageBus] ERROR: Could not route message from " << fromNodeId 
                  << " to " << toNodeId << " (target not found)" << std::endl;
    }
}

void InProcessMessageBus::broadcastMessage(const std::string& fromNodeId, const std::string& message) {
    std::lock_guard<std::mutex> lock(transportsMutex_);
    
    for (const auto& [nodeId, transport] : transports_) {
        if (nodeId != fromNodeId) {  // Don't send to self
            nlohmann::json messageWithMeta = {
                {"from", fromNodeId},
                {"to", "broadcast"},
                {"payload", message},
                {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count()}
            };
            
            transport->deliverMessage(messageWithMeta.dump());
            messagesRouted_++;
        }
    }
    
    std::cout << "[InProcessMessageBus] Broadcast message from " << fromNodeId 
              << " to " << (transports_.size() - 1) << " nodes" << std::endl;
}

std::vector<std::string> InProcessMessageBus::getRegisteredNodes() const {
    std::lock_guard<std::mutex> lock(transportsMutex_);
    std::vector<std::string> nodes;
    for (const auto& [nodeId, transport] : transports_) {
        nodes.push_back(nodeId);
    }
    return nodes;
}

nlohmann::json InProcessMessageBus::getStats() const {
    return {
        {"messagesRouted", messagesRouted_.load()},
        {"routingErrors", routingErrors_.load()},
        {"registeredNodes", getRegisteredNodes().size()}
    };
}

// ==========================================
// InProcessTransport Implementation
// ==========================================

InProcessTransport::InProcessTransport(const std::string& nodeId) : nodeId_(nodeId) {
    if (nodeId_.empty()) {
        generateNodeId();
    }
}

InProcessTransport::~InProcessTransport() {
    if (initialized_.load()) {
        InProcessMessageBus::getInstance()->unregisterTransport(nodeId_);
    }
}

std::future<void> InProcessTransport::initialize() {
    return std::async(std::launch::async, [this]() {
        if (!initialized_.load()) {
            InProcessMessageBus::getInstance()->registerTransport(nodeId_, this);
            initialized_.store(true);
            std::cout << "[InProcessTransport] Initialized node: " << nodeId_ << std::endl;
        }
    });
}

std::future<void> InProcessTransport::send(const std::string& message) {
    return std::async(std::launch::async, [this, message]() {
        if (!initialized_.load()) {
            std::cout << "[InProcessTransport] ERROR: Cannot send, transport not initialized" << std::endl;
            return;
        }
        
        try {
            // Parse message to extract routing information
            auto messageJson = nlohmann::json::parse(message);
            
            if (messageJson.contains("to")) {
                std::string targetNode = messageJson["to"];
                if (targetNode == "broadcast" || targetNode.empty()) {
                    InProcessMessageBus::getInstance()->broadcastMessage(nodeId_, message);
                } else {
                    InProcessMessageBus::getInstance()->routeMessage(nodeId_, targetNode, message);
                }
            } else {
                // No routing info, broadcast
                InProcessMessageBus::getInstance()->broadcastMessage(nodeId_, message);
            }
            
            messagesSent_++;
            
        } catch (const nlohmann::json::parse_error&) {
            // Message is not JSON, treat as simple broadcast
            InProcessMessageBus::getInstance()->broadcastMessage(nodeId_, message);
            messagesSent_++;
        }
    });
}

void InProcessTransport::subscribe(MessageHandler handler) {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    handlers_.push_back(handler);
    std::cout << "[InProcessTransport] Added message handler for node: " << nodeId_ << std::endl;
}

bool InProcessTransport::isInitialized() const {
    return initialized_.load();
}

std::string InProcessTransport::getTransportType() const {
    return "InProcess";
}

nlohmann::json InProcessTransport::getStats() const {
    return {
        {"transportType", getTransportType()},
        {"nodeId", nodeId_},
        {"initialized", initialized_.load()},
        {"messagesSent", messagesSent_.load()},
        {"messagesReceived", messagesReceived_.load()},
        {"subscribedHandlers", handlers_.size()}
    };
}

std::future<void> InProcessTransport::destroy() {
    return std::async(std::launch::async, [this]() {
        if (initialized_.load()) {
            InProcessMessageBus::getInstance()->unregisterTransport(nodeId_);
            initialized_.store(false);
            std::cout << "[InProcessTransport] Destroyed node: " << nodeId_ << std::endl;
        }
    });
}

void InProcessTransport::setNodeId(const std::string& nodeId) {
    if (initialized_.load()) {
        std::cout << "[InProcessTransport] ERROR: Cannot change nodeId after initialization" << std::endl;
        return;
    }
    nodeId_ = nodeId;
}

void InProcessTransport::deliverMessage(const std::string& message) {
    messagesReceived_++;
    
    std::lock_guard<std::mutex> lock(handlersMutex_);
    for (auto& handler : handlers_) {
        try {
            handler(message);
        } catch (const std::exception& e) {
            std::cout << "[InProcessTransport] ERROR in message handler: " << e.what() << std::endl;
        }
    }
}

void InProcessTransport::sendDirect(const std::string& targetNodeId, const std::string& message) {
    if (initialized_.load()) {
        InProcessMessageBus::getInstance()->routeMessage(nodeId_, targetNodeId, message);
        messagesSent_++;
    }
}

void InProcessTransport::broadcastDirect(const std::string& message) {
    if (initialized_.load()) {
        InProcessMessageBus::getInstance()->broadcastMessage(nodeId_, message);
        messagesSent_++;
    }
}

void InProcessTransport::generateNodeId() {
    // Generate a unique node ID
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    
    nodeId_ = "node-" + std::to_string(timestamp) + "-" + std::to_string(dis(gen));
}

// ==========================================
// Factory Functions
// ==========================================

std::shared_ptr<InProcessTransport> createInProcessTransport() {
    return std::make_shared<InProcessTransport>();
}

std::shared_ptr<InProcessTransport> createInProcessTransport(const std::string& nodeId) {
    return std::make_shared<InProcessTransport>(nodeId);
}

} // namespace transport
} // namespace nyamesh2