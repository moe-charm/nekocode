#pragma once

/**
 * @file P2PTransportAdapter.h
 * @brief Revolutionary P2P Transport Adapter - Hybrid Integration
 * 
 * Seamlessly combines NyaMesh2 (15.6x performance) with MillionPeerP2PTransport (large-scale stability)
 * Automatic optimal transport selection based on use case requirements
 * 
 * @author nyamesh2 team
 * @version Migration from nyacore v14.0
 * @date 2025-01-22
 */

#include "NyaMeshTransport.h"
#include "MillionPeerP2PTransport.h"
#include "TransportBase.h"
#include "../messaging/P2PMessage.h"
#include <memory>
#include <future>
#include <atomic>
#include <chrono>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include "../nlohmann_json.hpp"

namespace nyamesh2 {
namespace transport {

/**
 * @brief Transport type selection for optimal performance
 */
enum class TransportType {
    AUTO,              ///< Automatic selection (recommended)
    NYAMESH2,         ///< High-performance, small-medium scale
    MILLION_PEER,     ///< Large-scale, stability focused
    HYBRID            ///< Both transports simultaneously
};

/**
 * @brief Transport requirements for automatic selection
 */
struct TransportRequirements {
    size_t expectedPeerCount = 0;           ///< Expected number of peers
    bool needsHighPerformance = false;      ///< High performance requirement
    bool needsLowLatency = false;           ///< Low latency requirement  
    bool needsLargeScale = false;           ///< Large scale requirement
    bool needsReliability = false;          ///< High reliability requirement
    std::string useCase;                    ///< "text-editing", "file-sync", "search", etc.
    
    // Performance preferences
    double latencyToleranceMs = 100.0;      ///< Latency tolerance in milliseconds
    size_t expectedMessageRate = 100;       ///< Messages per second
    size_t maxMessageSize = 1024;           ///< Maximum message size in bytes
    
    nlohmann::json toJson() const {
        return {
            {"expectedPeerCount", expectedPeerCount},
            {"needsHighPerformance", needsHighPerformance},
            {"needsLowLatency", needsLowLatency},
            {"needsLargeScale", needsLargeScale},
            {"needsReliability", needsReliability},
            {"useCase", useCase},
            {"latencyToleranceMs", latencyToleranceMs},
            {"expectedMessageRate", expectedMessageRate},
            {"maxMessageSize", maxMessageSize}
        };
    }
};

/**
 * @brief Transport performance statistics
 */
struct TransportStats {
    TransportType activeTransport = TransportType::AUTO;
    size_t currentPeerCount = 0;
    double averageLatencyMs = 0.0;
    size_t messagesSent = 0;
    size_t messagesReceived = 0;
    size_t messageDrops = 0;
    double reliabilityScore = 1.0;
    std::chrono::system_clock::time_point lastSwitchTime;
    
    nlohmann::json toJson() const {
        return {
            {"activeTransport", static_cast<int>(activeTransport)},
            {"currentPeerCount", currentPeerCount},
            {"averageLatencyMs", averageLatencyMs},
            {"messagesSent", messagesSent},
            {"messagesReceived", messagesReceived},
            {"messageDrops", messageDrops},
            {"reliabilityScore", reliabilityScore},
            {"lastSwitchTime", std::chrono::duration_cast<std::chrono::milliseconds>(
                lastSwitchTime.time_since_epoch()).count()}
        };
    }
};

/**
 * @brief Unified P2P Interface for transport abstraction
 */
class UnifiedP2PInterface {
public:
    virtual ~UnifiedP2PInterface() = default;
    
    // Basic operations
    virtual std::future<bool> joinNetwork(const std::vector<std::string>& bootstrapNodes) = 0;
    virtual std::future<bool> leaveNetwork() = 0;
    virtual std::future<size_t> broadcastMessage(const nyacore::V14::P2P::P2PMessage& message) = 0;
    virtual std::future<bool> sendMessageToPeer(const std::string& peerId, const nyacore::V14::P2P::P2PMessage& message) = 0;
    
    // Statistics and monitoring
    virtual size_t getPeerCount() const = 0;
    virtual nlohmann::json getNetworkStats() const = 0;
    virtual std::string getTransportType() const = 0;
    
    // Performance information
    virtual double getAverageLatency() const = 0;
    virtual size_t getMessageThroughput() const = 0;
    virtual double getReliabilityScore() const = 0;
    
    // Transport-specific features
    virtual bool supportsFeature(const std::string& feature) const = 0;
    virtual nlohmann::json getFeatureInfo(const std::string& feature) const = 0;
};

/**
 * @brief NyaMesh2 adapter implementation
 */
class NyaMesh2Adapter : public UnifiedP2PInterface {
private:
    std::shared_ptr<NyaMeshTransport> transport_;
    mutable std::mutex statsMutex_;
    TransportStats stats_;
    
public:
    explicit NyaMesh2Adapter(std::shared_ptr<NyaMeshTransport> transport);
    
    std::future<bool> joinNetwork(const std::vector<std::string>& bootstrapNodes) override;
    std::future<bool> leaveNetwork() override;
    std::future<size_t> broadcastMessage(const nyacore::V14::P2P::P2PMessage& message) override;
    std::future<bool> sendMessageToPeer(const std::string& peerId, const nyacore::V14::P2P::P2PMessage& message) override;
    
    size_t getPeerCount() const override;
    nlohmann::json getNetworkStats() const override;
    std::string getTransportType() const override { return "NyaMesh2"; }
    
    double getAverageLatency() const override;
    size_t getMessageThroughput() const override;
    double getReliabilityScore() const override;
    
    bool supportsFeature(const std::string& feature) const override;
    nlohmann::json getFeatureInfo(const std::string& feature) const override;
};

/**
 * @brief MillionPeer adapter implementation
 */
class MillionPeerAdapter : public UnifiedP2PInterface {
private:
    std::shared_ptr<MillionPeerP2PTransport> transport_;
    mutable std::mutex statsMutex_;
    TransportStats stats_;
    
public:
    explicit MillionPeerAdapter(std::shared_ptr<MillionPeerP2PTransport> transport);
    
    std::future<bool> joinNetwork(const std::vector<std::string>& bootstrapNodes) override;
    std::future<bool> leaveNetwork() override;
    std::future<size_t> broadcastMessage(const nyacore::V14::P2P::P2PMessage& message) override;
    std::future<bool> sendMessageToPeer(const std::string& peerId, const nyacore::V14::P2P::P2PMessage& message) override;
    
    size_t getPeerCount() const override;
    nlohmann::json getNetworkStats() const override;
    std::string getTransportType() const override { return "MillionPeer"; }
    
    double getAverageLatency() const override;
    size_t getMessageThroughput() const override;
    double getReliabilityScore() const override;
    
    bool supportsFeature(const std::string& feature) const override;
    nlohmann::json getFeatureInfo(const std::string& feature) const override;
};

/**
 * @brief Revolutionary P2P Transport Adapter
 * 
 * Key Features:
 * - Automatic optimal transport selection
 * - Seamless switching between NyaMesh2 and MillionPeer
 * - Use-case specific optimization
 * - Performance monitoring and statistics
 * - Hot-swapping capability for dynamic optimization
 */
class P2PTransportAdapter : public UnifiedP2PInterface {
private:
    // Transport instances
    std::unique_ptr<NyaMesh2Adapter> nyamesh2Adapter_;
    std::unique_ptr<MillionPeerAdapter> millionPeerAdapter_;
    
    // Current configuration
    TransportType currentTransport_ = TransportType::AUTO;
    TransportRequirements requirements_;
    UnifiedP2PInterface* activeTransport_ = nullptr;
    
    // Performance monitoring
    mutable std::shared_mutex adapterMutex_;
    TransportStats globalStats_;
    std::map<TransportType, TransportStats> transportStats_;
    
    // Automatic selection logic
    std::atomic<bool> autoSwitchEnabled_{true};
    std::chrono::seconds switchCooldown_{30};
    std::function<void(TransportType, TransportType)> switchCallback_;
    
    // Performance thresholds for auto-switching
    double maxLatencyThreshold_ = 500.0;     // ms
    double minReliabilityThreshold_ = 0.8;   // 80%
    size_t maxDropRateThreshold_ = 10;       // 10 drops per 100 messages
    
public:
    P2PTransportAdapter();
    ~P2PTransportAdapter();
    
    // Configuration
    void configure(const TransportRequirements& requirements);
    void setTransportType(TransportType type);
    void setAutoSwitchEnabled(bool enabled) { autoSwitchEnabled_.store(enabled); }
    void setSwitchCallback(std::function<void(TransportType, TransportType)> callback);
    
    // Transport selection logic
    TransportType selectOptimalTransport(const TransportRequirements& requirements) const;
    std::future<bool> switchToTransport(TransportType target);
    bool canSwitchToTransport(TransportType target) const;
    
    // UnifiedP2PInterface implementation
    std::future<bool> joinNetwork(const std::vector<std::string>& bootstrapNodes) override;
    std::future<bool> leaveNetwork() override;
    std::future<size_t> broadcastMessage(const nyacore::V14::P2P::P2PMessage& message) override;
    std::future<bool> sendMessageToPeer(const std::string& peerId, const nyacore::V14::P2P::P2PMessage& message) override;
    
    size_t getPeerCount() const override;
    nlohmann::json getNetworkStats() const override;
    std::string getTransportType() const override;
    
    double getAverageLatency() const override;
    size_t getMessageThroughput() const override;
    double getReliabilityScore() const override;
    
    bool supportsFeature(const std::string& feature) const override;
    nlohmann::json getFeatureInfo(const std::string& feature) const override;
    
    // Adapter-specific features
    TransportType getCurrentTransport() const;
    TransportRequirements getRequirements() const;
    TransportStats getGlobalStats() const;
    std::map<TransportType, TransportStats> getAllTransportStats() const;
    
    // Performance optimization
    void optimizeForUseCase(const std::string& useCase);
    void setPerformanceThresholds(double maxLatency, double minReliability, size_t maxDropRate);
    nlohmann::json getOptimizationRecommendations() const;
    
    // Hybrid mode operations (both transports simultaneously)
    std::future<bool> enableHybridMode();
    std::future<bool> disableHybridMode();
    bool isHybridModeActive() const;
    
private:
    // Internal implementation
    void initializeTransports();
    void startPerformanceMonitoring();
    void stopPerformanceMonitoring();
    void updateGlobalStats();
    void checkAutoSwitch();
    
    // Transport management
    UnifiedP2PInterface* getActiveTransport() const;
    void setActiveTransport(TransportType type);
    bool isTransportHealthy(TransportType type) const;
    
    // Selection algorithms
    TransportType selectByPeerCount(size_t peerCount) const;
    TransportType selectByLatencyRequirement(double latencyMs) const;
    TransportType selectByUseCase(const std::string& useCase) const;
    TransportType selectByPerformanceHistory() const;
    
    // Statistics and monitoring
    void recordMessage(bool sent, bool success, size_t size, double latencyMs);
    void updateTransportStats(TransportType type, const TransportStats& stats);
    
    // Utilities
    std::string transportTypeToString(TransportType type) const;
    TransportType stringToTransportType(const std::string& str) const;
    
    // Thread management
    std::atomic<bool> isRunning_{false};
    std::thread monitoringThread_;
    void monitoringLoop();
};

// ==========================================
// Factory Functions
// ==========================================

/**
 * @brief Create P2P Transport Adapter with automatic configuration
 */
std::shared_ptr<P2PTransportAdapter> createP2PTransportAdapter();

/**
 * @brief Create P2P Transport Adapter with specific requirements
 */
std::shared_ptr<P2PTransportAdapter> createP2PTransportAdapter(const TransportRequirements& requirements);

/**
 * @brief Create P2P Transport Adapter with forced transport type
 */
std::shared_ptr<P2PTransportAdapter> createP2PTransportAdapter(TransportType forceType);

// ==========================================
// Utility Functions
// ==========================================

/**
 * @brief Get recommended transport for specific use case
 */
TransportType getRecommendedTransport(const std::string& useCase, size_t expectedPeers = 0);

/**
 * @brief Create optimized requirements for common use cases
 */
TransportRequirements createRequirementsForTextEditing();
TransportRequirements createRequirementsForFileSharing();
TransportRequirements createRequirementsForSearch();
TransportRequirements createRequirementsForLargeScale();

} // namespace transport
} // namespace nyamesh2

/*
 * === Revolutionary P2P Transport Adapter Usage Examples ===
 * 
 * // Automatic transport selection (recommended)
 * auto adapter = createP2PTransportAdapter();
 * adapter->optimizeForUseCase("text-editing");
 * adapter->joinNetwork({"bootstrap1.example.com", "bootstrap2.example.com"});
 * 
 * // Manual transport selection
 * auto adapter = createP2PTransportAdapter(TransportType::NYAMESH2);
 * 
 * // Custom requirements
 * TransportRequirements req;
 * req.useCase = "real-time-collaboration";
 * req.needsLowLatency = true;
 * req.expectedPeerCount = 20;
 * auto adapter = createP2PTransportAdapter(req);
 * 
 * // Use case specific shortcuts
 * auto textEditingAdapter = createP2PTransportAdapter(createRequirementsForTextEditing());
 * auto fileAdapter = createP2PTransportAdapter(createRequirementsForFileSharing());
 * auto searchAdapter = createP2PTransportAdapter(createRequirementsForSearch());
 * 
 * // Performance monitoring
 * auto stats = adapter->getGlobalStats();
 * auto recommendations = adapter->getOptimizationRecommendations();
 * 
 * // Dynamic optimization
 * adapter->setSwitchCallback([](TransportType from, TransportType to) {
 *     std::cout << "Switched from " << static_cast<int>(from) 
 *               << " to " << static_cast<int>(to) << std::endl;
 * });
 */