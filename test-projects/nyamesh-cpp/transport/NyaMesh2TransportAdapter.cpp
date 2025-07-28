/**
 * @file NyaMesh2TransportAdapter.cpp
 * @brief Implementation of lightweight P2P Transport Adapter for nyamesh2
 * 
 * @author nyamesh2 team
 * @date 2025-07-22
 */

#include "NyaMesh2TransportAdapter.h"
#include "../core/nyamesh.h"  // For SafeMessage definition
#include <iostream>
#include <random>
#include <algorithm>

namespace nyamesh2 {
namespace transport {

// ==========================================
// NyaMesh2TransportAdapter Implementation
// ==========================================

NyaMesh2TransportAdapter::NyaMesh2TransportAdapter(const std::string& nodeId, bool debug) 
    : nodeId_(nodeId), debug_(debug), startTime_(std::chrono::system_clock::now()) {
    
    // Generate unique node ID if not provided
    if (nodeId_.empty()) {
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        nodeId_ = "adapter-" + std::to_string(timestamp) + "-" + std::to_string(dis(gen));
    }
    
    log("NyaMesh2TransportAdapter created with ID: " + nodeId_);
}

NyaMesh2TransportAdapter::~NyaMesh2TransportAdapter() {
    log("NyaMesh2TransportAdapter destructor called for: " + nodeId_);
    
    // Cleanup will be handled by smart pointers and RAII
}

void NyaMesh2TransportAdapter::configure(const AdapterRequirements& requirements) {
    std::lock_guard<std::mutex> lock(statsMutex_);
    requirements_ = requirements;
    
    log("Configured adapter for use case: " + requirements.useCase + 
        ", expected peers: " + std::to_string(requirements.expectedPeerCount));
    
    // Future: Auto-select transport based on requirements
    if (requirements.needsLowLatency && requirements.expectedPeerCount < 100) {
        // Would select NyaMesh2 in future
        log("Recommended transport: NyaMesh2 (low latency, small scale)");
    } else if (requirements.needsLargeScale && requirements.expectedPeerCount > 1000) {
        // Would select MillionPeer in future
        log("Recommended transport: MillionPeer (large scale)");
    }
}

void NyaMesh2TransportAdapter::setTransportType(AdapterTransportType type) {
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    if (type != AdapterTransportType::IN_PROCESS) {
        log("WARNING: Only IN_PROCESS transport currently supported, requested: " + 
            std::to_string(static_cast<int>(type)));
        return;
    }
    
    currentTransport_ = type;
    log("Transport type set to: " + getTransportTypeString());
}

AdapterTransportType NyaMesh2TransportAdapter::getCurrentTransport() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return currentTransport_;
}

std::future<void> NyaMesh2TransportAdapter::initialize() {
    return std::async(std::launch::async, [this]() {
        log("Initializing NyaMesh2TransportAdapter");
        
        switch (currentTransport_) {
            case AdapterTransportType::IN_PROCESS:
                initializeInProcessTransport();
                break;
            default:
                log("ERROR: Unsupported transport type");
                throw std::runtime_error("Unsupported transport type");
        }
        
        log("NyaMesh2TransportAdapter initialization completed");
    });
}

std::future<bool> NyaMesh2TransportAdapter::joinNetwork(const std::vector<std::string>& bootstrapNodes) {
    return std::async(std::launch::async, [this, bootstrapNodes]() -> bool {
        log("Joining P2P network with " + std::to_string(bootstrapNodes.size()) + " bootstrap nodes");
        
        try {
            if (inProcessTransport_ && !inProcessTransport_->isInitialized()) {
                auto initFuture = inProcessTransport_->initialize();
                initFuture.wait();
            }
            
            // For InProcessTransport, joining is automatic when initialized
            log("Successfully joined P2P network");
            return true;
            
        } catch (const std::exception& e) {
            log("ERROR: Failed to join network: " + std::string(e.what()));
            return false;
        }
    });
}

std::future<bool> NyaMesh2TransportAdapter::leaveNetwork() {
    return std::async(std::launch::async, [this]() -> bool {
        log("Leaving P2P network");
        
        try {
            if (inProcessTransport_) {
                auto destroyFuture = inProcessTransport_->destroy();
                destroyFuture.wait();
            }
            
            log("Successfully left P2P network");
            return true;
            
        } catch (const std::exception& e) {
            log("ERROR: Failed to leave network: " + std::string(e.what()));
            return false;
        }
    });
}

std::future<bool> NyaMesh2TransportAdapter::sendToPeer(const std::string& peerId, const nlohmann::json& message) {
    return std::async(std::launch::async, [this, peerId, message]() -> bool {
        log("Sending message to peer: " + peerId);
        
        try {
            if (!inProcessTransport_ || !inProcessTransport_->isInitialized()) {
                log("ERROR: Transport not initialized");
                return false;
            }
            
            // Create SafeMessage format
            SafeMessage safeMsg;
            safeMsg.type = message.value("type", "adapter.message");
            safeMsg.data = message;
            safeMsg.from = nodeId_;
            safeMsg.to = peerId;
            safeMsg.timestamp = std::chrono::system_clock::now();
            
            // Convert to transport format
            std::string transportMessage = safeMessageToTransport(safeMsg);
            
            // Send via transport
            auto sendFuture = inProcessTransport_->send(transportMessage);
            sendFuture.wait();
            
            updateStats(true);
            log("Message sent successfully to: " + peerId);
            return true;
            
        } catch (const std::exception& e) {
            log("ERROR: Failed to send message: " + std::string(e.what()));
            return false;
        }
    });
}

std::future<size_t> NyaMesh2TransportAdapter::broadcast(const nlohmann::json& message) {
    return std::async(std::launch::async, [this, message]() -> size_t {
        log("Broadcasting message to all peers");
        
        try {
            if (!inProcessTransport_ || !inProcessTransport_->isInitialized()) {
                log("ERROR: Transport not initialized");
                return 0;
            }
            
            // Create SafeMessage format for broadcast
            SafeMessage safeMsg;
            safeMsg.type = message.value("type", "adapter.broadcast");
            safeMsg.data = message;
            safeMsg.from = nodeId_;
            safeMsg.to = "";  // Empty = broadcast
            safeMsg.timestamp = std::chrono::system_clock::now();
            
            // Convert to transport format
            std::string transportMessage = safeMessageToTransport(safeMsg);
            
            // Broadcast via transport
            auto sendFuture = inProcessTransport_->send(transportMessage);
            sendFuture.wait();
            
            updateStats(true);
            
            // For InProcessTransport, return connected peer count
            size_t peerCount = peerCount_.load();
            log("Broadcast completed to " + std::to_string(peerCount) + " peers");
            return peerCount;
            
        } catch (const std::exception& e) {
            log("ERROR: Failed to broadcast message: " + std::string(e.what()));
            return 0;
        }
    });
}

void NyaMesh2TransportAdapter::setMessageHandler(std::function<void(const SafeMessage&)> handler) {
    messageHandler_ = handler;
    log("Message handler set");
}

size_t NyaMesh2TransportAdapter::getPeerCount() const {
    return peerCount_.load();
}

nlohmann::json NyaMesh2TransportAdapter::getStats() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    auto now = std::chrono::system_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - startTime_).count();
    
    nlohmann::json stats = {
        {"nodeId", nodeId_},
        {"transportType", getTransportTypeString()},
        {"initialized", isInitialized()},
        {"messagesSent", messagesSent_.load()},
        {"messagesReceived", messagesReceived_.load()},
        {"peerCount", peerCount_.load()},
        {"uptime", uptime},
        {"requirements", requirements_.toJson()}
    };
    
    // Add transport-specific stats
    if (inProcessTransport_) {
        stats["transportStats"] = inProcessTransport_->getStats();
    }
    
    return stats;
}

bool NyaMesh2TransportAdapter::isInitialized() const {
    return inProcessTransport_ && inProcessTransport_->isInitialized();
}

std::string NyaMesh2TransportAdapter::getTransportTypeString() const {
    switch (currentTransport_) {
        case AdapterTransportType::AUTO: return "AUTO";
        case AdapterTransportType::IN_PROCESS: return "IN_PROCESS";
        case AdapterTransportType::NYAMESH2: return "NYAMESH2";
        case AdapterTransportType::MILLION_PEER: return "MILLION_PEER";
        default: return "UNKNOWN";
    }
}

void NyaMesh2TransportAdapter::optimizeForUseCase(const std::string& useCase) {
    AdapterRequirements req;
    
    if (useCase == "text-editing") {
        req = createRequirementsForTextEditing();
    } else if (useCase == "file-sharing") {
        req = createRequirementsForFileSharing();
    } else if (useCase == "search") {
        req = createRequirementsForSearch();
    } else {
        req.useCase = useCase;
        log("Custom use case optimization: " + useCase);
    }
    
    configure(req);
}

nlohmann::json NyaMesh2TransportAdapter::getRecommendations() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    nlohmann::json recommendations = {
        {"currentTransport", getTransportTypeString()},
        {"optimal", true},  // For now, always optimal
        {"suggestions", nlohmann::json::array()}
    };
    
    // Future: Add intelligent recommendations based on performance
    if (requirements_.expectedPeerCount > 100) {
        recommendations["suggestions"].push_back("Consider MILLION_PEER for large scale");
    }
    
    if (requirements_.needsLowLatency) {
        recommendations["suggestions"].push_back("NYAMESH2 recommended for low latency");
    }
    
    return recommendations;
}

// ==========================================
// Private Implementation
// ==========================================

void NyaMesh2TransportAdapter::initializeInProcessTransport() {
    log("Initializing InProcessTransport");
    
    inProcessTransport_ = createInProcessTransport(nodeId_);
    
    // Set up message handler
    inProcessTransport_->subscribe([this](const std::string& transportMessage) {
        handleTransportMessage(transportMessage);
    });
    
    auto initFuture = inProcessTransport_->initialize();
    initFuture.wait();
    
    log("InProcessTransport initialized successfully");
}

void NyaMesh2TransportAdapter::handleTransportMessage(const std::string& transportMessage) {
    try {
        updateStats(false, true);  // Received message
        
        log("Received transport message: " + transportMessage.substr(0, 100) + 
            (transportMessage.size() > 100 ? "..." : ""));
        
        // Convert transport message to SafeMessage
        SafeMessage safeMsg = transportToSafeMessage(transportMessage);
        
        // Call user message handler
        if (messageHandler_) {
            messageHandler_(safeMsg);
        }
        
    } catch (const std::exception& e) {
        log("ERROR: Failed to handle transport message: " + std::string(e.what()));
    }
}

std::string NyaMesh2TransportAdapter::safeMessageToTransport(const SafeMessage& message) {
    nlohmann::json transportMsg = {
        {"messageType", message.type},
        {"category", message.category},
        {"data", message.data},
        {"from", message.from},
        {"to", message.to},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            message.timestamp.time_since_epoch()).count()}
    };
    
    return transportMsg.dump();
}

SafeMessage NyaMesh2TransportAdapter::transportToSafeMessage(const std::string& transportMessage) {
    auto json = nlohmann::json::parse(transportMessage);
    
    SafeMessage safeMsg;
    
    // Handle both direct messages and routed messages from InProcessMessageBus
    nlohmann::json actualMessage;
    if (json.contains("payload")) {
        // This is a routed message from InProcessMessageBus
        actualMessage = nlohmann::json::parse(json["payload"].get<std::string>());
    } else {
        // Direct message
        actualMessage = json;
    }
    
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
    
    return safeMsg;
}

void NyaMesh2TransportAdapter::log(const std::string& message) const {
    if (debug_) {
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        std::cout << "[" << timestamp << "] [NyaMesh2TransportAdapter:" << nodeId_ << "] " 
                  << message << std::endl;
    }
}

void NyaMesh2TransportAdapter::updateStats(bool sent, bool received) {
    if (sent) {
        messagesSent_++;
    }
    if (received) {
        messagesReceived_++;
        
        // Update peer count estimation (simplified)
        // For InProcessTransport, could query actual peer count
        // peerCount_ = ...;
    }
}

// ==========================================
// Factory Functions
// ==========================================

std::shared_ptr<NyaMesh2TransportAdapter> createNyaMesh2TransportAdapter(
    const std::string& nodeId, bool debug) {
    return std::make_shared<NyaMesh2TransportAdapter>(nodeId, debug);
}

std::shared_ptr<NyaMesh2TransportAdapter> createNyaMesh2TransportAdapter(
    const AdapterRequirements& requirements, const std::string& nodeId, bool debug) {
    auto adapter = std::make_shared<NyaMesh2TransportAdapter>(nodeId, debug);
    adapter->configure(requirements);
    return adapter;
}

// ==========================================
// Utility Functions  
// ==========================================

AdapterRequirements createRequirementsForTextEditing() {
    AdapterRequirements req;
    req.useCase = "text-editing";
    req.needsLowLatency = true;
    req.needsHighPerformance = true;
    req.expectedPeerCount = 10;  // Small collaborative group
    return req;
}

AdapterRequirements createRequirementsForFileSharing() {
    AdapterRequirements req;
    req.useCase = "file-sharing";
    req.needsLargeScale = true;
    req.expectedPeerCount = 100;
    return req;
}

AdapterRequirements createRequirementsForSearch() {
    AdapterRequirements req;
    req.useCase = "search";
    req.needsHighPerformance = true;
    req.expectedPeerCount = 50;
    return req;
}

AdapterTransportType getRecommendedTransportType(const std::string& useCase, size_t expectedPeers) {
    // For now, always recommend IN_PROCESS (only implemented transport)
    // Future: Intelligent selection based on use case and peer count
    
    if (useCase == "text-editing" && expectedPeers < 50) {
        return AdapterTransportType::IN_PROCESS;  // Would be NYAMESH2 in future
    } else if (expectedPeers > 1000) {
        return AdapterTransportType::IN_PROCESS;  // Would be MILLION_PEER in future
    }
    
    return AdapterTransportType::IN_PROCESS;
}

} // namespace transport
} // namespace nyamesh2