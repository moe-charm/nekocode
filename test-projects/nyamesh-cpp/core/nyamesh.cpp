/**
 * @file nyamesh.cpp
 * @brief NyaMesh implementation - Thread-safe self-contained core
 */

#include "nyamesh.h"
#include "../transport/InProcessTransport.h"
#include "../transport/NyaMesh2TransportAdapter.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <shared_mutex>

namespace nyamesh2 {

NyaMesh::NyaMesh(const std::string& id, bool debug) 
    : debug_(debug), startTime_(std::chrono::system_clock::now()) {
    
    // Generate unique ID if not provided
    if (id.empty()) {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        nodeId_ = "mesh-" + std::to_string(timestamp) + "-" + std::to_string(dis(gen));
    } else {
        nodeId_ = id;
    }
    
    log("NyaMesh created with ID: " + nodeId_);
}

NyaMesh::~NyaMesh() {
    log("NyaMesh destructor called for: " + nodeId_);
    
    if (initialized_.load()) {
        // Force synchronous shutdown for destructor
        shuttingDown_.store(true);
        
        // Wake up processing thread
        queueCondition_.notify_all();
        
        // Wait for processing thread to finish
        if (processingThread_.joinable()) {
            processingThread_.join();
        }
        
        log("NyaMesh safely destroyed: " + nodeId_);
    }
}

std::future<void> NyaMesh::initialize() {
    return std::async(std::launch::async, [this]() {
        initializeImpl();
    });
}

void NyaMesh::initializeImpl() {
    if (initialized_.load()) {
        log("Already initialized, skipping");
        return;
    }
    
    log("Initializing NyaMesh: " + nodeId_);
    
    // Start message processing thread
    processingThread_ = std::thread(&NyaMesh::messageProcessingLoop, this);
    
    // Mark as initialized
    initialized_.store(true);
    
    log("NyaMesh initialization completed: " + nodeId_);
    
    // Call virtual hook for derived classes
    onInitialized();
}

std::future<void> NyaMesh::shutdown() {
    return std::async(std::launch::async, [this]() {
        shutdownImpl();
    });
}

void NyaMesh::shutdownImpl() {
    if (!initialized_.load() || shuttingDown_.load()) {
        return;
    }
    
    // Call virtual hook for derived classes before shutdown
    onShutdown();
    
    log("Shutting down NyaMesh: " + nodeId_);
    
    // Mark as shutting down
    shuttingDown_.store(true);
    
    // Wake up processing thread
    queueCondition_.notify_all();
    
    // Wait for processing thread to finish
    if (processingThread_.joinable()) {
        processingThread_.join();
    }
    
    // Clear handlers
    {
        std::unique_lock<std::shared_mutex> lock(handlersMutex_);
        handlers_.clear();
    }
    
    // Clear message queue
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        while (!messageQueue_.empty()) {
            messageQueue_.pop();
        }
    }
    
    initialized_.store(false);
    log("NyaMesh shutdown completed: " + nodeId_);
}

void NyaMesh::on(const std::string& messageType, MessageHandler handler) {
    if (!initialized_.load()) {
        log("WARNING: on() called before initialization for type: " + messageType);
    }
    
    if (shuttingDown_.load()) {
        log("WARNING: on() called during shutdown for type: " + messageType);
        return;
    }
    
    std::unique_lock<std::shared_mutex> lock(handlersMutex_);
    handlers_[messageType].emplace_back(std::move(handler));
    handlersRegistered_++;
    
    log("Handler registered for message type: " + messageType + " (total: " + std::to_string(handlersRegistered_.load()) + ")");
}

void NyaMesh::off(const std::string& messageType, MessageHandler handler) {
    std::unique_lock<std::shared_mutex> lock(handlersMutex_);
    
    auto it = handlers_.find(messageType);
    if (it != handlers_.end()) {
        if (handler == nullptr) {
            // Remove all handlers for this message type
            it->second.clear();
            log("All handlers removed for message type: " + messageType);
        } else {
            // Remove specific handler (simplified - would need more complex comparison in real implementation)
            it->second.clear();  // For simplicity, remove all
            log("Handler removed for message type: " + messageType);
        }
        
        if (it->second.empty()) {
            handlers_.erase(it);
        }
    }
}

void NyaMesh::send(const std::string& messageType, const nlohmann::json& data, const std::string& to) {
    // v23: Thin wrapper - delegate to sendCore
    sendCore(messageType, data, to, nullptr);
}

void NyaMesh::send(const std::string& messageType, 
                   const nlohmann::json& data, 
                   const IntentCommonOptions& options) {
    // v23: Thin wrapper - delegate to sendCore
    std::string target = options.broadcast ? "" : "";
    sendCore(messageType, data, target, &options);
}

void NyaMesh::send(const std::string& messageType,
                   const nlohmann::json& data,
                   const std::string& to,
                   const IntentCommonOptions& options) {
    // v23: Thin wrapper - delegate to sendCore
    std::string actualTarget = to.empty() && options.broadcast ? "" : to;
    sendCore(messageType, data, actualTarget, &options);
}

// ===================================================================
// v23: Single Responsibility Core Send Implementation
// ===================================================================

void NyaMesh::sendCore(const std::string& messageType, 
                       const nlohmann::json& data, 
                       const std::string& to,
                       const IntentCommonOptions* options) {
    // State validation
    if (!initialized_.load()) {
        log("WARNING: send() called before initialization");
        return;
    }
    
    if (shuttingDown_.load()) {
        log("WARNING: send() called during shutdown");
        return;
    }
    
    // Debug logging (unified)
    if (globalDebugLogger.isEnabled()) {
        globalDebugLogger.logIntent(messageType, data, to, options);
    }
    
    // Create message
    SafeMessage message(messageType, data);
    message.from = nodeId_;
    message.to = to;
    
    // Validate message
    if (!validateMessage(message)) {
        log("ERROR: Invalid message, not sending: " + messageType);
        return;
    }
    
    messagesSent_++;
    
    // Message delivery
    if (to == nodeId_ || to.empty()) {
        publishLocal(message);
    }
    
    if (to != nodeId_) {
        publishToTransport(message);
    }
    
    // Hook for derived classes
    onMessageSent(message);
    
    // Simplified logging
    std::string targetInfo = to.empty() ? "broadcast" : to;
    log("Message sent: " + messageType + " to " + targetInfo);
    
    // Intent analysis (if options provided)
    if (options && (debug_.load() || options->debug)) {
        auto recommendation = IntentInterpreter::analyzeIntent(messageType, *options);
        if (recommendation.logWarning) {
            log("WARNING: " + recommendation.warningMessage);
        }
    }
}

bool NyaMesh::isInitialized() const {
    return initialized_.load();
}

std::string NyaMesh::getId() const {
    return nodeId_;
}

nlohmann::json NyaMesh::getStats() const {
    nlohmann::json stats;
    
    auto now = std::chrono::system_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - startTime_).count();
    
    // Core statistics with proper types
    stats["nodeId"] = nodeId_;
    stats["initialized"] = initialized_.load();
    stats["shuttingDown"] = shuttingDown_.load();
    stats["debug"] = debug_.load();
    stats["messagesSent"] = messagesSent_.load();
    stats["messagesReceived"] = messagesReceived_.load();
    stats["messagesProcessed"] = messagesProcessed_.load();
    stats["handlersRegistered"] = handlersRegistered_.load();
    stats["uptime"] = uptime;
    
    // Handler statistics
    {
        std::shared_lock<std::shared_mutex> lock(handlersMutex_);
        stats["handlerTypes"] = handlers_.size();
        
        size_t totalHandlers = 0;
        nlohmann::json handlerDetails = nlohmann::json::object();
        
        for (const auto& [type, handlerList] : handlers_) {
            totalHandlers += handlerList.size();
            handlerDetails[type] = handlerList.size();
        }
        
        stats["activeHandlers"] = totalHandlers;
        stats["handlerDetails"] = handlerDetails;
    }
    
    // Queue statistics
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        stats["queueSize"] = messageQueue_.size();
    }
    
    // Performance metrics
    stats["performance"] = {
        {"messagesPerSecond", uptime > 0 ? static_cast<double>(messagesProcessed_.load()) / uptime : 0.0},
        {"averageQueueTime", "N/A"} // TODO: Implement queue time tracking
    };
    
    return stats;
}

void NyaMesh::setDebug(bool enable) {
    debug_.store(enable);
    log("Debug mode " + std::string(enable ? "enabled" : "disabled"));
}

void NyaMesh::connectTransport(std::shared_ptr<transport::InProcessTransport> transport) {
    std::lock_guard<std::mutex> lock(transportMutex_);
    
    if (transport_) {
        log("WARNING: Replacing existing transport connection");
    }
    
    transport_ = transport;
    
    if (transport_) {
        // Set up message handler to receive messages from other nodes
        transport_->subscribe([this](const std::string& transportMessage) {
            try {
                // Parse transport message back to SafeMessage format
                auto messageJson = nlohmann::json::parse(transportMessage);
                
                // Extract the original message or the routed message
                nlohmann::json actualMessage;
                if (messageJson.contains("payload")) {
                    // This is a routed message from InProcessMessageBus
                    actualMessage = nlohmann::json::parse(messageJson["payload"].get<std::string>());
                } else {
                    // Direct message
                    actualMessage = messageJson;
                }
                
                // Convert back to SafeMessage
                SafeMessage safeMsg;
                safeMsg.type = actualMessage["messageType"].get<std::string>();
                safeMsg.category = actualMessage.value("category", "Notice");
                safeMsg.data = actualMessage["data"];
                safeMsg.from = actualMessage["from"].get<std::string>();
                safeMsg.to = actualMessage.value("to", "");
                
                if (actualMessage.contains("timestamp")) {
                    auto timestamp_ms = actualMessage["timestamp"].get<int64_t>();
                    safeMsg.timestamp = std::chrono::system_clock::time_point(
                        std::chrono::milliseconds(timestamp_ms)
                    );
                } else {
                    safeMsg.timestamp = std::chrono::system_clock::now();
                }
                
                // Only process if it's for us (avoid loops)
                if (safeMsg.to.empty() || safeMsg.to == nodeId_) {
                    // Queue the message for local processing
                    {
                        std::lock_guard<std::mutex> queueLock(queueMutex_);
                        messageQueue_.push(safeMsg);
                    }
                    queueCondition_.notify_one();
                    
                    log("Received transport message: " + safeMsg.type + " from " + safeMsg.from);
                }
                
            } catch (const std::exception& e) {
                log("ERROR: Failed to process transport message: " + std::string(e.what()));
            }
        });
        
        log("Transport connected: " + transport_->getTransportType() + " (" + transport_->getNodeId() + ")");
    }
}

void NyaMesh::disconnectTransport() {
    std::lock_guard<std::mutex> lock(transportMutex_);
    
    if (transport_) {
        log("Disconnecting transport: " + transport_->getTransportType());
        transport_.reset();
    }
}

void NyaMesh::connectP2PAdapter(std::shared_ptr<transport::NyaMesh2TransportAdapter> adapter) {
    std::lock_guard<std::mutex> lock(p2pAdapterMutex_);
    
    if (p2pAdapter_) {
        log("WARNING: Replacing existing P2P adapter connection");
    }
    
    p2pAdapter_ = adapter;
    
    if (p2pAdapter_) {
        // Set up P2P message handler to integrate with NyaMesh core
        p2pAdapter_->setMessageHandler([this](const SafeMessage& msg) {
            try {
                // Messages received via P2P adapter are processed through normal message queue
                {
                    std::lock_guard<std::mutex> queueLock(queueMutex_);
                    messageQueue_.push(msg);
                }
                queueCondition_.notify_one();
                
                log("Received P2P message: " + msg.type + " from " + msg.from);
                
            } catch (const std::exception& e) {
                log("ERROR: Failed to process P2P adapter message: " + std::string(e.what()));
            }
        });
        
        log("P2P adapter connected: " + p2pAdapter_->getTransportTypeString() + 
            " (" + p2pAdapter_->getNodeId() + ")");
    }
}

void NyaMesh::disconnectP2PAdapter() {
    std::lock_guard<std::mutex> lock(p2pAdapterMutex_);
    
    if (p2pAdapter_) {
        log("Disconnecting P2P adapter: " + p2pAdapter_->getTransportTypeString());
        p2pAdapter_.reset();
    }
}

bool NyaMesh::hasP2PAdapter() const {
    std::lock_guard<std::mutex> lock(p2pAdapterMutex_);
    return p2pAdapter_ != nullptr;
}

void NyaMesh::messageProcessingLoop() {
    log("Message processing thread started");
    
    while (!shuttingDown_.load()) {
        SafeMessage message;
        bool hasMessage = false;
        
        // Wait for message or shutdown
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            
            queueCondition_.wait(lock, [this]() {
                return !messageQueue_.empty() || shuttingDown_.load();
            });
            
            if (!messageQueue_.empty()) {
                message = messageQueue_.front();
                messageQueue_.pop();
                hasMessage = true;
            }
        }
        
        // Process message if available
        if (hasMessage && !shuttingDown_.load()) {
            processMessage(message);
        }
        
        // Periodic cleanup
        static int cleanupCounter = 0;
        if (++cleanupCounter % 100 == 0) {
            cleanupExpiredHandlers();
        }
    }
    
    log("Message processing thread ended");
}

void NyaMesh::processMessage(const SafeMessage& message) {
    messagesReceived_++;
    
    // Call virtual hook for derived classes
    onMessageReceived(message);
    
    log("Processing message: " + message.type + " from " + message.from);
    
    try {
        // Local delivery only (受信専用処理)
        publishLocal(message);
        
        messagesProcessed_++;
        
    } catch (const std::exception& e) {
        log("ERROR: Exception processing received message " + message.type + ": " + e.what());
    }
}

void NyaMesh::publishLocal(const SafeMessage& message) {
    std::vector<HandlerInfo> handlersToCall;
    
    // Copy handlers under lock
    {
        std::shared_lock<std::shared_mutex> lock(handlersMutex_);
        
        auto it = handlers_.find(message.type);
        if (it != handlers_.end()) {
            handlersToCall = it->second;  // Copy the vector
        }
    }
    
    // Call handlers outside of lock
    for (const auto& handlerInfo : handlersToCall) {
        try {
            // Check if handler owner is still alive (if we had proper lifetime tracking)
            handlerInfo.handler(message);
        } catch (const std::exception& e) {
            log("ERROR: Exception in message handler for " + message.type + ": " + e.what());
        }
    }
    
    if (!handlersToCall.empty()) {
        log("Message delivered to " + std::to_string(handlersToCall.size()) + " handlers: " + message.type);
    }
}

void NyaMesh::publishToTransport(const SafeMessage& message) {
    // PRIORITY-BASED TRANSPORT SELECTION (LOOP PREVENTION)
    // Priority 1: P2P Adapter (if available)
    // Priority 2: InProcess Transport (fallback only)
    // IMPORTANT: Only ONE transport per message to prevent loops
    
    bool messageSent = false;
    std::string transportUsed = "";
    
    // Priority 1: Try P2P Adapter first
    {
        std::lock_guard<std::mutex> lock(p2pAdapterMutex_);
        
        if (p2pAdapter_ && p2pAdapter_->isInitialized()) {
            // Convert SafeMessage to JSON for P2P adapter
            nlohmann::json p2pMessage = {
                {"type", message.type},
                {"category", message.category},
                {"data", message.data}
            };
            
            try {
                if (message.to.empty()) {
                    // Broadcast message
                    auto broadcastFuture = p2pAdapter_->broadcast(p2pMessage);
                    // Don't wait to avoid blocking
                } else {
                    // Direct message to specific peer
                    auto sendFuture = p2pAdapter_->sendToPeer(message.to, p2pMessage);
                    // Don't wait to avoid blocking
                }
                
                messageSent = true;
                transportUsed = "P2PAdapter";
                log("Message routed via P2PAdapter: " + message.type + " to " + 
                    (message.to.empty() ? "broadcast" : message.to));
                    
            } catch (const std::exception& e) {
                log("ERROR: Failed to route message via P2PAdapter: " + std::string(e.what()));
            }
        }
    }
    
    // Priority 2: Fallback to InProcessTransport ONLY if P2P failed
    if (!messageSent) {
        std::lock_guard<std::mutex> lock(transportMutex_);
        
        if (transport_ && transport_->isInitialized()) {
            // Convert SafeMessage to JSON format expected by transport
            nlohmann::json transportMessage = {
                {"messageType", message.type},
                {"category", message.category},
                {"data", message.data},
                {"from", message.from},
                {"to", message.to},
                {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                    message.timestamp.time_since_epoch()).count()}
            };
            
            try {
                // Send via transport (async)
                auto sendFuture = transport_->send(transportMessage.dump());
                // Don't wait for completion to avoid blocking message processing
                
                messageSent = true;
                transportUsed = "InProcessTransport";
                log("Message routed via InProcessTransport (fallback): " + message.type + " to " + 
                    (message.to.empty() ? "broadcast" : message.to));
                    
            } catch (const std::exception& e) {
                log("ERROR: Failed to route message via InProcessTransport: " + std::string(e.what()));
            }
        }
    }
    
    // Log result
    if (messageSent) {
        log("TRANSPORT: Message sent via " + transportUsed + " (loop prevention active)");
    } else {
        log("WARNING: No transport available, cannot route message: " + message.type + " to " + 
            (message.to.empty() ? "broadcast" : message.to));
    }
}

void NyaMesh::cleanupExpiredHandlers() {
    std::unique_lock<std::shared_mutex> lock(handlersMutex_);
    
    // Remove expired handlers (simplified - would use weak_ptr in real implementation)
    // For now, just log cleanup attempt
    log("Handler cleanup performed (placeholder)");
}

void NyaMesh::log(const std::string& message) const {
    if (debug_.load()) {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        std::cout << "[" << timestamp << "] [NyaMesh:" << nodeId_ << "] " << message << std::endl;
    }
}

std::string NyaMesh::generateMessageId() const {
    static std::atomic<uint64_t> counter{0};
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return nodeId_ + "-msg-" + std::to_string(timestamp) + "-" + std::to_string(counter.fetch_add(1));
}

bool NyaMesh::validateMessage(const SafeMessage& message) const {
    // Unified validation for all message types
    if (message.type.empty()) {
        log("ERROR: Message missing type");
        return false;
    }
    
    if (message.from.empty()) {
        log("ERROR: Message missing sender");
        return false;
    }
    
    // Data validation
    if (message.data.empty()) {
        log("WARNING: Message with empty data: " + message.type);
        return true; // Allow empty data for some message types
    }
    
    try {
        // P2P message validation
        if (message.type.length() >= 4 && message.type.substr(0, 4) == "p2p.") {
            // P2P messages should have route information
            if (!message.data.contains("route")) {
                log("WARNING: P2P message without route information: " + message.type);
            } else {
                const auto& route = message.data["route"];
                
                // Check priority
                if (!route.contains("priority") || !route["priority"].is_number()) {
                    log("WARNING: P2P message missing or invalid priority");
                }
                
                // Check TTL
                if (route.contains("ttl") && route["ttl"].is_number()) {
                    int ttl = route["ttl"].get<int>();
                    if (ttl <= 0) {
                        log("ERROR: P2P message with expired TTL");
                        return false;
                    }
                }
            }
        }
        
        // Metadata validation
        if (message.data.contains("metadata")) {
            const auto& metadata = message.data["metadata"];
            if (metadata.contains("messageId") && metadata["messageId"].is_string()) {
                std::string msgId = metadata["messageId"].get<std::string>();
                if (msgId.empty()) {
                    log("WARNING: Message with empty messageId");
                }
            }
        }
        
        // Large message warning
        std::string jsonStr = message.data.dump();
        if (jsonStr.size() > 10000) {  // > 10KB
            log("WARNING: Large message detected (" + std::to_string(jsonStr.size()) + " bytes): " + message.type);
        }
        
    } catch (const std::exception& e) {
        log("ERROR: Message validation failed: " + std::string(e.what()));
        return false;
    }
    
    return true;
}

// === Factory functions ===

std::shared_ptr<NyaMesh> createNyaMesh(const std::string& id, bool debug) {
    auto mesh = std::make_shared<NyaMesh>(id, debug);
    return mesh;
}

std::vector<std::shared_ptr<NyaMesh>> createNyaMeshNetwork(int count, bool debug) {
    std::vector<std::shared_ptr<NyaMesh>> network;
    network.reserve(count);
    
    for (int i = 0; i < count; i++) {
        std::string id = "mesh-node-" + std::to_string(i + 1);
        network.push_back(createNyaMesh(id, debug));
    }
    
    return network;
}

} // namespace nyamesh2