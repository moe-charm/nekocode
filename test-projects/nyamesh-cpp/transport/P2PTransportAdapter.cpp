/**
 * @file P2PTransportAdapter.cpp
 * @brief Revolutionary P2P Transport Adapter Implementation
 * 
 * @author nyamesh2 team
 * @version Migration from nyacore v14.0
 * @date 2025-01-22
 */

#include "P2PTransportAdapter.h"
#include "../messaging/P2PMessage.h"
#include <algorithm>
#include <random>
#include <thread>
#include <iostream>

namespace nyamesh2 {
namespace transport {

// ==========================================
// NyaMesh2Adapter Implementation
// ==========================================

NyaMesh2Adapter::NyaMesh2Adapter(std::shared_ptr<NyaMeshTransport> transport)
    : transport_(transport) {
    stats_.activeTransport = TransportType::NYAMESH2;
}

std::future<bool> NyaMesh2Adapter::joinNetwork(const std::vector<std::string>& bootstrapNodes) {
    return std::async(std::launch::async, [this, bootstrapNodes]() -> bool {
        try {
            // NyaMesh2 specific join logic
            transport_->initialize("nyamesh-hybrid-channel");
            
            std::lock_guard<std::mutex> lock(statsMutex_);
            stats_.lastSwitchTime = std::chrono::system_clock::now();
            return true;
        } catch (const std::exception& e) {
            std::cerr << "[NyaMesh2Adapter] Join error: " << e.what() << std::endl;
            return false;
        }
    });
}

std::future<bool> NyaMesh2Adapter::leaveNetwork() {
    return std::async(std::launch::async, [this]() -> bool {
        try {
            // NyaMeshTransport graceful shutdown is handled in destructor
            return true;
        } catch (const std::exception& e) {
            std::cerr << "[NyaMesh2Adapter] Leave error: " << e.what() << std::endl;
            return false;
        }
    });
}

std::future<size_t> NyaMesh2Adapter::broadcastMessage(const nyacore::V14::P2P::P2PMessage& message) {
    return std::async(std::launch::async, [this, message]() -> size_t {
        try {
            // Convert Base::Message to Intent for NyaMeshTransport
            Intent intent;
            intent.action = "adapter.broadcast";
            intent.payload = nlohmann::json{
                {"type", message.type},
                {"action", message.action},
                {"data", message.data}
            };
            
            transport_->sendIntent(intent);
            
            std::lock_guard<std::mutex> lock(statsMutex_);
            stats_.messagesSent++;
            return 1; // NyaMesh2 doesn't return peer count directly
        } catch (const std::exception& e) {
            std::lock_guard<std::mutex> lock(statsMutex_);
            stats_.messageDrops++;
            return 0;
        }
    });
}

std::future<bool> NyaMesh2Adapter::sendMessageToPeer(const std::string& peerId, const nyacore::V14::P2P::P2PMessage& message) {
    return std::async(std::launch::async, [this, peerId, message]() -> bool {
        try {
            // Convert Base::Message to Intent for NyaMeshTransport
            Intent intent;
            intent.action = "adapter.peer_message";
            intent.to = peerId;
            intent.payload = nlohmann::json{
                {"type", message.type},
                {"action", message.action},
                {"data", message.data}
            };
            
            transport_->sendIntent(intent);
            
            std::lock_guard<std::mutex> lock(statsMutex_);
            stats_.messagesSent++;
            return true;
        } catch (const std::exception& e) {
            std::lock_guard<std::mutex> lock(statsMutex_);
            stats_.messageDrops++;
            return false;
        }
    });
}

size_t NyaMesh2Adapter::getPeerCount() const {
    // NyaMesh2 implementation for peer count
    return stats_.currentPeerCount;
}

nlohmann::json NyaMesh2Adapter::getNetworkStats() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    nlohmann::json stats = stats_.toJson();
    
    // Get cosmic stats from NyaMeshTransport
    auto cosmicStats = transport_->getCosmicStats();
    stats["transport_specific"] = cosmicStats;
    stats["transport_specific"]["core_performance"] = "15.6x optimized";
    stats["transport_specific"]["intent_processing"] = "async_enabled";
    stats["transport_specific"]["tree_structure"] = "autonomous_growth";
    
    return stats;
}

double NyaMesh2Adapter::getAverageLatency() const {
    return stats_.averageLatencyMs;
}

size_t NyaMesh2Adapter::getMessageThroughput() const {
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - stats_.lastSwitchTime);
    if (duration.count() > 0) {
        return stats_.messagesSent / duration.count();
    }
    return 0;
}

double NyaMesh2Adapter::getReliabilityScore() const {
    if (stats_.messagesSent == 0) return 1.0;
    return 1.0 - (static_cast<double>(stats_.messageDrops) / stats_.messagesSent);
}

bool NyaMesh2Adapter::supportsFeature(const std::string& feature) const {
    static const std::set<std::string> supportedFeatures = {
        "high_performance", "low_latency", "intent_driven", 
        "autonomous_growth", "tree_structure", "async_processing"
    };
    return supportedFeatures.count(feature) > 0;
}

nlohmann::json NyaMesh2Adapter::getFeatureInfo(const std::string& feature) const {
    nlohmann::json info;
    
    if (feature == "high_performance") {
        info = {
            {"description", "15.6x performance improvement over baseline"},
            {"benchmark_nodes_per_sec", 3316},
            {"optimization_level", "maximum"}
        };
    } else if (feature == "low_latency") {
        info = {
            {"description", "Optimized for real-time applications"},
            {"typical_latency_ms", 5.0},
            {"best_for", "text_editing, real_time_collaboration"}
        };
    }
    
    return info;
}

// ==========================================
// MillionPeerAdapter Implementation
// ==========================================

MillionPeerAdapter::MillionPeerAdapter(std::shared_ptr<MillionPeerP2PTransport> transport)
    : transport_(transport) {
    stats_.activeTransport = TransportType::MILLION_PEER;
}

std::future<bool> MillionPeerAdapter::joinNetwork(const std::vector<std::string>& bootstrapNodes) {
    return std::async(std::launch::async, [this, bootstrapNodes]() -> bool {
        try {
            auto result = transport_->joinMillionPeerMesh(bootstrapNodes);
            return result.get();
        } catch (const std::exception& e) {
            std::cerr << "[MillionPeerAdapter] Join error: " << e.what() << std::endl;
            return false;
        }
    });
}

std::future<bool> MillionPeerAdapter::leaveNetwork() {
    return std::async(std::launch::async, [this]() -> bool {
        try {
            auto result = transport_->leaveMillionPeerMesh();
            return result.get();
        } catch (const std::exception& e) {
            std::cerr << "[MillionPeerAdapter] Leave error: " << e.what() << std::endl;
            return false;
        }
    });
}

std::future<size_t> MillionPeerAdapter::broadcastMessage(const nyacore::V14::P2P::P2PMessage& message) {
    return std::async(std::launch::async, [this, message]() -> size_t {
        try {
            // Convert P2PMessage to Base::Message for MillionPeer
            nyacore::V14::Base::Message baseMsg;
            baseMsg.type = message.type;
            baseMsg.action = message.action;
            baseMsg.payload = message.data;
            baseMsg.source = message.source;
            baseMsg.target = message.target;
            
            auto result = transport_->broadcastToMillionPeers(baseMsg);
            size_t peerCount = result.get();
            
            std::lock_guard<std::mutex> lock(statsMutex_);
            stats_.messagesSent++;
            return peerCount;
        } catch (const std::exception& e) {
            std::lock_guard<std::mutex> lock(statsMutex_);
            stats_.messageDrops++;
            return 0;
        }
    });
}

std::future<bool> MillionPeerAdapter::sendMessageToPeer(const std::string& peerId, const nyacore::V14::P2P::P2PMessage& message) {
    return std::async(std::launch::async, [this, peerId, message]() -> bool {
        try {
            // Convert P2PMessage to Base::Message for MillionPeer
            nyacore::V14::Base::Message baseMsg;
            baseMsg.type = message.type;
            baseMsg.action = message.action;
            baseMsg.payload = message.data;
            baseMsg.source = message.source;
            baseMsg.target = peerId;
            
            auto result = transport_->send(baseMsg);
            result.wait();
            
            std::lock_guard<std::mutex> lock(statsMutex_);
            stats_.messagesSent++;
            return true;
        } catch (const std::exception& e) {
            std::lock_guard<std::mutex> lock(statsMutex_);
            stats_.messageDrops++;
            return false;
        }
    });
}

size_t MillionPeerAdapter::getPeerCount() const {
    return transport_->getTotalPeersDiscovered();
}

nlohmann::json MillionPeerAdapter::getNetworkStats() const {
    auto baseStats = transport_->getMillionScaleStats();
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    nlohmann::json stats = stats_.toJson();
    stats["transport_specific"] = baseStats;
    return stats;
}

double MillionPeerAdapter::getAverageLatency() const {
    return transport_->getAverageHopCount() * 10.0; // Estimate: 10ms per hop
}

size_t MillionPeerAdapter::getMessageThroughput() const {
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - stats_.lastSwitchTime);
    if (duration.count() > 0) {
        return stats_.messagesSent / duration.count();
    }
    return 0;
}

double MillionPeerAdapter::getReliabilityScore() const {
    if (stats_.messagesSent == 0) return 1.0;
    return 1.0 - (static_cast<double>(stats_.messageDrops) / stats_.messagesSent);
}

bool MillionPeerAdapter::supportsFeature(const std::string& feature) const {
    static const std::set<std::string> supportedFeatures = {
        "large_scale", "high_reliability", "distributed_hash_table", 
        "super_nodes", "load_balancing", "compression"
    };
    return supportedFeatures.count(feature) > 0;
}

nlohmann::json MillionPeerAdapter::getFeatureInfo(const std::string& feature) const {
    nlohmann::json info;
    
    if (feature == "large_scale") {
        info = {
            {"description", "Supports up to 1 million peers"},
            {"max_peers", 1000000},
            {"routing_hops", transport_->getAverageHopCount()}
        };
    } else if (feature == "high_reliability") {
        info = {
            {"description", "Enterprise-grade reliability and stability"},
            {"uptime_target", "99.9%"},
            {"best_for", "file_sharing, search, large_networks"}
        };
    }
    
    return info;
}

// ==========================================
// P2PTransportAdapter Implementation
// ==========================================

P2PTransportAdapter::P2PTransportAdapter() {
    initializeTransports();
}

P2PTransportAdapter::~P2PTransportAdapter() {
    isRunning_.store(false);
    if (monitoringThread_.joinable()) {
        monitoringThread_.join();
    }
}

void P2PTransportAdapter::initializeTransports() {
    try {
        // Initialize NyaMesh2 with underlying DefaultTransport
        auto underlyingTransport = std::make_shared<DefaultTransport>();
        auto nyameshTransport = std::make_shared<NyaMeshTransport>(underlyingTransport, "HybridMesh-NyaMesh");
        nyamesh2Adapter_ = std::make_unique<NyaMesh2Adapter>(nyameshTransport);
        
        // Initialize MillionPeer
        auto millionPeerTransport = std::make_shared<MillionPeerP2PTransport>();
        millionPeerAdapter_ = std::make_unique<MillionPeerAdapter>(millionPeerTransport);
        
        // Default to AUTO mode
        currentTransport_ = TransportType::AUTO;
        
        std::cout << "[P2PTransportAdapter] Hybrid P2P system initialized successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[P2PTransportAdapter] Initialization error: " << e.what() << std::endl;
        throw;
    }
}

void P2PTransportAdapter::configure(const TransportRequirements& requirements) {
    std::unique_lock<std::shared_mutex> lock(adapterMutex_);
    requirements_ = requirements;
    
    if (currentTransport_ == TransportType::AUTO) {
        auto optimalType = selectOptimalTransport(requirements);
        setActiveTransport(optimalType);
    }
    
    std::cout << "[P2PTransportAdapter] Configured for use case: " << requirements.useCase << std::endl;
}

TransportType P2PTransportAdapter::selectOptimalTransport(const TransportRequirements& requirements) const {
    // High performance + low latency + small scale → NyaMesh2
    if (requirements.needsHighPerformance && requirements.needsLowLatency 
        && requirements.expectedPeerCount <= 1000) {
        return TransportType::NYAMESH2;
    }
    
    // Large scale or high peer count → MillionPeer
    if (requirements.needsLargeScale || requirements.expectedPeerCount > 10000) {
        return TransportType::MILLION_PEER;
    }
    
    // Use case specific selection
    if (requirements.useCase == "text-editing" || requirements.useCase == "real-time-collaboration") {
        return TransportType::NYAMESH2; // Real-time needs high performance
    }
    
    if (requirements.useCase == "file-sharing" || requirements.useCase == "search" 
        || requirements.useCase == "large-scale-distribution") {
        return TransportType::MILLION_PEER; // Stability and scale
    }
    
    // Performance history based selection
    auto historyChoice = selectByPerformanceHistory();
    if (historyChoice != TransportType::AUTO) {
        return historyChoice;
    }
    
    // Default: NyaMesh2 for its 15.6x performance advantage
    return TransportType::NYAMESH2;
}

TransportType P2PTransportAdapter::selectByPerformanceHistory() const {
    std::shared_lock<std::shared_mutex> lock(adapterMutex_);
    
    auto nyameshStats = transportStats_.find(TransportType::NYAMESH2);
    auto millionStats = transportStats_.find(TransportType::MILLION_PEER);
    
    if (nyameshStats == transportStats_.end() || millionStats == transportStats_.end()) {
        return TransportType::AUTO; // No history available
    }
    
    // Compare reliability scores
    if (nyameshStats->second.reliabilityScore > millionStats->second.reliabilityScore + 0.1) {
        return TransportType::NYAMESH2;
    } else if (millionStats->second.reliabilityScore > nyameshStats->second.reliabilityScore + 0.1) {
        return TransportType::MILLION_PEER;
    }
    
    // Compare latency (lower is better)
    if (nyameshStats->second.averageLatencyMs < millionStats->second.averageLatencyMs * 0.8) {
        return TransportType::NYAMESH2;
    } else if (millionStats->second.averageLatencyMs < nyameshStats->second.averageLatencyMs * 0.8) {
        return TransportType::MILLION_PEER;
    }
    
    return TransportType::AUTO; // No clear winner
}

void P2PTransportAdapter::setActiveTransport(TransportType type) {
    if (type == TransportType::AUTO) {
        type = selectOptimalTransport(requirements_);
    }
    
    switch (type) {
        case TransportType::NYAMESH2:
            activeTransport_ = nyamesh2Adapter_.get();
            break;
        case TransportType::MILLION_PEER:
            activeTransport_ = millionPeerAdapter_.get();
            break;
        default:
            activeTransport_ = nyamesh2Adapter_.get(); // Default fallback
            break;
    }
    
    currentTransport_ = type;
    globalStats_.activeTransport = type;
    globalStats_.lastSwitchTime = std::chrono::system_clock::now();
    
    if (switchCallback_) {
        switchCallback_(globalStats_.activeTransport, type);
    }
}

UnifiedP2PInterface* P2PTransportAdapter::getActiveTransport() const {
    std::shared_lock<std::shared_mutex> lock(adapterMutex_);
    return activeTransport_;
}

std::future<bool> P2PTransportAdapter::joinNetwork(const std::vector<std::string>& bootstrapNodes) {
    auto transport = getActiveTransport();
    if (!transport) {
        return std::async(std::launch::async, []() { return false; });
    }
    
    isRunning_.store(true);
    startPerformanceMonitoring();
    
    return transport->joinNetwork(bootstrapNodes);
}

std::future<bool> P2PTransportAdapter::leaveNetwork() {
    auto transport = getActiveTransport();
    if (!transport) {
        return std::async(std::launch::async, []() { return false; });
    }
    
    isRunning_.store(false);
    stopPerformanceMonitoring();
    
    return transport->leaveNetwork();
}

std::future<size_t> P2PTransportAdapter::broadcastMessage(const nyacore::V14::P2P::P2PMessage& message) {
    auto transport = getActiveTransport();
    if (!transport) {
        return std::async(std::launch::async, []() { return size_t(0); });
    }
    
    return transport->broadcastMessage(message);
}

std::future<bool> P2PTransportAdapter::sendMessageToPeer(const std::string& peerId, const nyacore::V14::P2P::P2PMessage& message) {
    auto transport = getActiveTransport();
    if (!transport) {
        return std::async(std::launch::async, []() { return false; });
    }
    
    return transport->sendMessageToPeer(peerId, message);
}

size_t P2PTransportAdapter::getPeerCount() const {
    auto transport = getActiveTransport();
    return transport ? transport->getPeerCount() : 0;
}

nlohmann::json P2PTransportAdapter::getNetworkStats() const {
    auto transport = getActiveTransport();
    if (!transport) {
        return nlohmann::json{};
    }
    
    auto stats = transport->getNetworkStats();
    stats["adapter_info"] = {
        {"current_transport", transportTypeToString(currentTransport_)},
        {"auto_switch_enabled", autoSwitchEnabled_.load()},
        {"requirements", requirements_.toJson()},
        {"global_stats", globalStats_.toJson()}
    };
    
    return stats;
}

std::string P2PTransportAdapter::getTransportType() const {
    return "HybridAdapter(" + transportTypeToString(currentTransport_) + ")";
}

double P2PTransportAdapter::getAverageLatency() const {
    auto transport = getActiveTransport();
    return transport ? transport->getAverageLatency() : 0.0;
}

size_t P2PTransportAdapter::getMessageThroughput() const {
    auto transport = getActiveTransport();
    return transport ? transport->getMessageThroughput() : 0;
}

double P2PTransportAdapter::getReliabilityScore() const {
    auto transport = getActiveTransport();
    return transport ? transport->getReliabilityScore() : 0.0;
}

bool P2PTransportAdapter::supportsFeature(const std::string& feature) const {
    // Adapter supports features from both transports
    return (nyamesh2Adapter_->supportsFeature(feature) || 
            millionPeerAdapter_->supportsFeature(feature));
}

nlohmann::json P2PTransportAdapter::getFeatureInfo(const std::string& feature) const {
    nlohmann::json info;
    
    if (nyamesh2Adapter_->supportsFeature(feature)) {
        info["nyamesh2"] = nyamesh2Adapter_->getFeatureInfo(feature);
    }
    
    if (millionPeerAdapter_->supportsFeature(feature)) {
        info["million_peer"] = millionPeerAdapter_->getFeatureInfo(feature);
    }
    
    return info;
}

void P2PTransportAdapter::startPerformanceMonitoring() {
    if (!monitoringThread_.joinable()) {
        monitoringThread_ = std::thread(&P2PTransportAdapter::monitoringLoop, this);
    }
}

void P2PTransportAdapter::stopPerformanceMonitoring() {
    isRunning_.store(false);
}

void P2PTransportAdapter::monitoringLoop() {
    while (isRunning_.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        updateGlobalStats();
        
        if (autoSwitchEnabled_.load()) {
            checkAutoSwitch();
        }
    }
}

void P2PTransportAdapter::updateGlobalStats() {
    auto transport = getActiveTransport();
    if (!transport) return;
    
    std::unique_lock<std::shared_mutex> lock(adapterMutex_);
    globalStats_.currentPeerCount = transport->getPeerCount();
    globalStats_.averageLatencyMs = transport->getAverageLatency();
    globalStats_.reliabilityScore = transport->getReliabilityScore();
}

void P2PTransportAdapter::checkAutoSwitch() {
    auto transport = getActiveTransport();
    if (!transport) return;
    
    // Check if current transport is unhealthy
    bool shouldSwitch = false;
    TransportType newTransport = currentTransport_;
    
    if (transport->getAverageLatency() > maxLatencyThreshold_) {
        shouldSwitch = true;
        newTransport = (currentTransport_ == TransportType::NYAMESH2) ? 
                      TransportType::MILLION_PEER : TransportType::NYAMESH2;
    }
    
    if (transport->getReliabilityScore() < minReliabilityThreshold_) {
        shouldSwitch = true;
        newTransport = (currentTransport_ == TransportType::NYAMESH2) ? 
                      TransportType::MILLION_PEER : TransportType::NYAMESH2;
    }
    
    // Check cooldown period
    auto now = std::chrono::system_clock::now();
    if (shouldSwitch && (now - globalStats_.lastSwitchTime) > switchCooldown_) {
        std::cout << "[P2PTransportAdapter] Auto-switching from " 
                  << transportTypeToString(currentTransport_) 
                  << " to " << transportTypeToString(newTransport) << std::endl;
        switchToTransport(newTransport);
    }
}

std::string P2PTransportAdapter::transportTypeToString(TransportType type) const {
    switch (type) {
        case TransportType::AUTO: return "AUTO";
        case TransportType::NYAMESH2: return "NyaMesh2";
        case TransportType::MILLION_PEER: return "MillionPeer";
        case TransportType::HYBRID: return "HYBRID";
        default: return "UNKNOWN";
    }
}

// ==========================================
// Factory Functions
// ==========================================

std::shared_ptr<P2PTransportAdapter> createP2PTransportAdapter() {
    return std::make_shared<P2PTransportAdapter>();
}

std::shared_ptr<P2PTransportAdapter> createP2PTransportAdapter(const TransportRequirements& requirements) {
    auto adapter = std::make_shared<P2PTransportAdapter>();
    adapter->configure(requirements);
    return adapter;
}

std::shared_ptr<P2PTransportAdapter> createP2PTransportAdapter(TransportType forceType) {
    auto adapter = std::make_shared<P2PTransportAdapter>();
    adapter->setTransportType(forceType);
    return adapter;
}

// ==========================================
// Utility Functions
// ==========================================

TransportRequirements createRequirementsForTextEditing() {
    TransportRequirements req;
    req.useCase = "text-editing";
    req.needsHighPerformance = true;
    req.needsLowLatency = true;
    req.expectedPeerCount = 20;
    req.latencyToleranceMs = 50.0;
    req.expectedMessageRate = 1000;
    req.maxMessageSize = 512;
    return req;
}

TransportRequirements createRequirementsForFileSharing() {
    TransportRequirements req;
    req.useCase = "file-sharing";
    req.needsReliability = true;
    req.needsLargeScale = true;
    req.expectedPeerCount = 500;
    req.latencyToleranceMs = 1000.0;
    req.expectedMessageRate = 100;
    req.maxMessageSize = 65536;
    return req;
}

TransportRequirements createRequirementsForSearch() {
    TransportRequirements req;
    req.useCase = "search";
    req.needsLargeScale = true;
    req.needsReliability = true;
    req.expectedPeerCount = 1000;
    req.latencyToleranceMs = 500.0;
    req.expectedMessageRate = 500;
    req.maxMessageSize = 4096;
    return req;
}

TransportRequirements createRequirementsForLargeScale() {
    TransportRequirements req;
    req.useCase = "large-scale-distribution";
    req.needsLargeScale = true;
    req.needsReliability = true;
    req.expectedPeerCount = 10000;
    req.latencyToleranceMs = 2000.0;
    req.expectedMessageRate = 50;
    req.maxMessageSize = 32768;
    return req;
}

} // namespace transport
} // namespace nyamesh2