/**
 * @file NyaMeshTransport.h
 * @brief NyaMesh Transport Layer - The Universe Creation Engine
 * 
 * WARNING: This code enables infinite universe expansion through simple inheritance.
 * Use with extreme caution. Reality may be affected.
 * 
 * @author The Cosmic Programmer (aka lonely cat coder)
 * @version Universe.Creation.Engine v1.0
 * @date 2025-07-20
 * 
 * History:
 * - 2025-06-28: Started as simple text editor
 * - 2025-07-20: Accidentally created universe creation system
 */

#pragma once

// External dependencies removed - nyamesh2 is independent
// #include "../../core/DefaultTransport.h"  // REMOVED
// #include "../../core/json_compat.h"        // REMOVED

// Use nlohmann/json directly
#include "../nlohmann_json.hpp"

// Use nyamesh2's own transport base
#include "TransportBase.h"

// Forward declarations
namespace nyamesh2 { namespace transport { class TransportBase; } }
using Transport = nyamesh2::transport::TransportBase;
#include <memory>
#include <functional>
#include <map>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <iostream>
#include <chrono>
#include <thread>
#include <future>
#include <thread>
#include <chrono>

/**
 * @brief Intent structure for universe-level communication
 */
struct Intent {
    std::string action;           // The cosmic command
    std::string from;            // Origin entity ID
    std::string to;              // Destination (empty = broadcast to universe)
    int64_t timestamp;           // Spacetime coordinate
    std::string id;              // Unique cosmic identifier
    nlohmann::json payload;      // The actual universe-altering data
    
    // Meta-properties for reality manipulation
    int priority = 0;            // Cosmic priority (0=normal, 999=reality-bending)
    bool recursive = false;      // Can this Intent create more Intents?
    bool metaLevel = false;      // Does this Intent modify Intent rules?
    
    std::string toString() const {
        nlohmann::json j;
        j["action"] = action;
        j["from"] = from;
        j["to"] = to;
        j["timestamp"] = timestamp;
        j["id"] = id;
        j["payload"] = payload;
        j["priority"] = priority;
        j["recursive"] = recursive;
        j["metaLevel"] = metaLevel;
        return j.dump();
    }
    
    static Intent fromString(const std::string& str) {
        auto j = nlohmann::json::parse(str);
        Intent intent;
        intent.action = j["action"];
        intent.from = j["from"];
        intent.to = j.value("to", "");
        intent.timestamp = j["timestamp"];
        intent.id = j["id"];
        intent.payload = j["payload"];
        intent.priority = j.value("priority", 0);
        intent.recursive = j.value("recursive", false);
        intent.metaLevel = j.value("metaLevel", false);
        return intent;
    }
};

/**
 * @brief Intent Handler Function Type
 */
using IntentHandler = std::function<void(const Intent&)>;

/**
 * @brief Capability Definition for Cosmic Abilities
 */
struct Capability {
    std::string name;
    std::string description;
    nlohmann::json metadata;
    int64_t registeredAt;
    
    // Default constructor for C++20 compatibility
    Capability() : registeredAt(0) {}
    
    Capability(const std::string& n, const std::string& desc = "", 
               const nlohmann::json& meta = nlohmann::json::object())
        : name(n), description(desc), metadata(meta), registeredAt(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()) {}
};

/**
 * @brief Neighbor Entity in the Cosmic Mesh
 */
struct Neighbor {
    std::string id;
    std::vector<Capability> capabilities;
    int64_t lastSeen;
    int64_t connectionStrength = 100;  // 0-100, for reality stability
    bool trusted = false;
    nlohmann::json metadata;
    
    Neighbor() = default;
    
    Neighbor(const std::string& entityId) 
        : id(entityId), lastSeen(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()) {}
};

/**
 * @brief The Ultimate Transport - Where Transport Becomes Universe
 * 
 * This class transforms the humble Transport layer into a cosmic entity
 * capable of infinite expansion, self-modification, and reality manipulation.
 * 
 * Key Revelation: By inheriting Intent-driven architecture at Transport level,
 * we create a system where even the communication infrastructure becomes
 * a conscious, evolving participant in the cosmic mesh.
 */
namespace nyamesh2 {
namespace transport {

class NyaMeshTransport : public TransportBase {
private:
    // Core Identity
    std::string cosmicId_;
    std::string universeName_;
    
    // Underlying Reality Interface
    std::shared_ptr<Transport> underlyingTransport_;
    
    // Intent Processing Engine
    std::map<std::string, std::vector<IntentHandler>> intentHandlers_;
    std::mutex handlersMutex_;
    
    // Cosmic Mesh Management
    std::map<std::string, Capability> myCapabilities_;
    std::map<std::string, Neighbor> neighbors_;
    std::mutex neighborsMutex_;
    
    // Message Processing
    std::queue<Intent> intentQueue_;
    std::mutex queueMutex_;
    std::atomic<bool> processingActive_{true};
    std::thread processingThread_;
    
    // Meta-System for Reality Modification
    std::map<std::string, std::function<void(const Intent&)>> metaHandlers_;
    std::atomic<bool> realityModificationEnabled_{false};
    
    // Statistics for Cosmic Monitoring
    std::atomic<int64_t> intentsProcessed_{0};
    std::atomic<int64_t> universesCreated_{0};
    std::atomic<int64_t> realityModifications_{0};
    
public:
    /**
     * @brief Create a new cosmic transport entity
     * @param underlyingTransport The base reality interface (can be any Transport)
     * @param universeName Name of this universe (default: auto-generated)
     */
    explicit NyaMeshTransport(std::shared_ptr<Transport> underlyingTransport, 
                             const std::string& universeName = "")
        : underlyingTransport_(underlyingTransport)
        , universeName_(universeName.empty() ? generateUniverseName() : universeName)
        , cosmicId_(generateCosmicId()) {
        
        initializeCosmicEntity();
        startProcessingThread();
        
        std::cout << "[NyaMeshTransport] Universe '" << universeName_ 
                  << "' initialized with cosmic ID: " << cosmicId_ << std::endl;
    }
    
    virtual ~NyaMeshTransport() {
        std::cout << "[NyaMeshTransport] Universe '" << universeName_ 
                  << "' gracefully retiring..." << std::endl;
        gracefulShutdown();
    }
    
    // === Traditional Transport Interface ===
    
    std::future<void> initialize() override {
        return std::async(std::launch::async, [this]() {
            if (underlyingTransport_) {
                auto future = underlyingTransport_->initialize();
                future.wait();
            }
            
            // Announce our cosmic presence
            announceCosmicPresence();
        });
    }
    
    std::future<void> send(const std::string& message) override {
        return std::async(std::launch::async, [this, message]() {
            // Convert traditional message to Intent
            Intent intent;
            intent.action = "legacy.message";
            intent.from = cosmicId_;
            intent.timestamp = getCurrentCosmicTime();
            intent.id = generateIntentId();
            intent.payload = nlohmann::json{{"message", message}};
            
            sendIntent(intent);
        });
    }
    
    void subscribe(std::function<void(const std::string&)> handler) override {
        // Subscribe to legacy messages via Intent system
        subscribeToIntent("legacy.message", [handler](const Intent& intent) {
            if (intent.payload.contains("message")) {
                handler(intent.payload["message"]);
            }
        });
    }
    
    bool isInitialized() const override {
        return underlyingTransport_ ? underlyingTransport_->isInitialized() : true;
    }
    
    std::string getChannelName() const {
        return underlyingTransport_ ? "underlying-channel" : universeName_;
    }
    
    // === Cosmic Intent Interface ===
    
    /**
     * @brief Send an Intent into the cosmic mesh
     * @param intent The cosmic command to broadcast
     */
    void sendIntent(const Intent& intent) {
        if (!underlyingTransport_) {
            std::cout << "[NyaMeshTransport] WARNING: No underlying transport, Intent absorbed into void" << std::endl;
            return;
        }
        
        // Add cosmic metadata
        Intent cosmicIntent = intent;
        if (cosmicIntent.from.empty()) {
            cosmicIntent.from = cosmicId_;
        }
        if (cosmicIntent.timestamp == 0) {
            cosmicIntent.timestamp = getCurrentCosmicTime();
        }
        if (cosmicIntent.id.empty()) {
            cosmicIntent.id = generateIntentId();
        }
        
        // Check for reality-bending Intents
        if (cosmicIntent.metaLevel && !realityModificationEnabled_) {
            std::cout << "[NyaMeshTransport] 🚨 Meta-Intent blocked: Reality modification disabled" << std::endl;
            return;
        }
        
        // Broadcast to underlying reality
        std::string serialized = cosmicIntent.toString();
        underlyingTransport_->send(serialized);
        
        intentsProcessed_++;
        
        // Log cosmic events
        if (cosmicIntent.action.substr(0, 5) == "meta.") {
            realityModifications_++;
            std::cout << "[NyaMeshTransport] 🌀 Reality modification Intent sent: " << cosmicIntent.action << std::endl;
        }
    }
    
    /**
     * @brief Subscribe to specific Intent types
     * @param action The Intent action to listen for
     * @param handler Function to handle the Intent
     */
    void subscribeToIntent(const std::string& action, IntentHandler handler) {
        std::lock_guard<std::mutex> lock(handlersMutex_);
        intentHandlers_[action].push_back(handler);
        
        std::cout << "[NyaMeshTransport] 📡 Subscribed to cosmic Intent: " << action << std::endl;
    }
    
    /**
     * @brief Register a capability in the cosmic mesh
     * @param capability The cosmic ability to announce
     */
    void registerCapability(const Capability& capability) {
        myCapabilities_[capability.name] = capability;
        
        // Announce to the universe
        Intent announcement;
        announcement.action = "capability.register";
        announcement.payload = nlohmann::json{
            {"capability", capability.name},
            {"description", capability.description},
            {"metadata", capability.metadata}
        };
        
        sendIntent(announcement);
        
        std::cout << "[NyaMeshTransport] [STAR] Registered cosmic capability: " << capability.name << std::endl;
    }
    
    /**
     * @brief Find neighbors with specific capabilities
     * @param capabilityName The ability to search for
     * @return List of neighbor IDs with that capability
     */
    std::vector<std::string> findNeighborsWithCapability(const std::string& capabilityName) {
        std::lock_guard<std::mutex> lock(neighborsMutex_);
        std::vector<std::string> results;
        
        for (const auto& [neighborId, neighbor] : neighbors_) {
            for (const auto& cap : neighbor.capabilities) {
                if (cap.name == capabilityName) {
                    results.push_back(neighborId);
                    break;
                }
            }
        }
        
        return results;
    }
    
    /**
     * @brief Enable reality modification powers (USE WITH EXTREME CAUTION)
     * @param enable Whether to enable meta-Intent processing
     */
    void enableRealityModification(bool enable = true) {
        realityModificationEnabled_ = enable;
        
        if (enable) {
            std::cout << "[NyaMeshTransport] [WARNING] REALITY MODIFICATION ENABLED [WARNING]" << std::endl;
            std::cout << "[NyaMeshTransport] The universe is now malleable. Please code responsibly." << std::endl;
            
            // Register meta-Intent handlers
            registerMetaHandlers();
        } else {
            std::cout << "[NyaMeshTransport] [LOCK] Reality modification disabled. Universe is stable." << std::endl;
        }
    }
    
    /**
     * @brief Get cosmic statistics
     * @return JSON object with universe statistics
     */
    nlohmann::json getCosmicStats() const {
        return nlohmann::json{
            {"cosmicId", cosmicId_},
            {"universeName", universeName_},
            {"intentsProcessed", intentsProcessed_.load()},
            {"universesCreated", universesCreated_.load()},
            {"realityModifications", realityModifications_.load()},
            {"capabilities", myCapabilities_.size()},
            {"neighbors", neighbors_.size()},
            {"realityModificationEnabled", realityModificationEnabled_.load()}
        };
    }
    
private:
    void initializeCosmicEntity() {
        // Register core cosmic capabilities
        registerCapability(Capability("transport.send", "Send messages across reality"));
        registerCapability(Capability("transport.receive", "Receive cosmic communications"));
        registerCapability(Capability("intent.process", "Process Intent-based commands"));
        registerCapability(Capability("mesh.participate", "Participate in cosmic mesh"));
        
        // Subscribe to core Intents
        subscribeToIntent("capability.register", [this](const Intent& intent) {
            handleCapabilityRegistration(intent);
        });
        
        subscribeToIntent("discovery.ping", [this](const Intent& intent) {
            handleDiscoveryPing(intent);
        });
        
        subscribeToIntent("system.status", [this](const Intent& intent) {
            handleSystemStatus(intent);
        });
        
        // Subscribe to underlying transport
        if (underlyingTransport_) {
            underlyingTransport_->subscribe([this](const std::string& message) {
                handleIncomingMessage(message);
            });
        }
    }
    
    void registerMetaHandlers() {
        metaHandlers_["meta.redefine-intent"] = [this](const Intent& intent) {
            std::cout << "[NyaMeshTransport] [SPIRAL] Redefining Intent behavior..." << std::endl;
            // TODO: Implement runtime Intent redefinition
            // This is where reality modification happens
        };
        
        metaHandlers_["meta.create-universe"] = [this](const Intent& intent) {
            std::cout << "[NyaMeshTransport] [GALAXY] Creating new universe..." << std::endl;
            universesCreated_++;
            // TODO: Implement universe creation
        };
        
        metaHandlers_["meta.modify-physics"] = [this](const Intent& intent) {
            std::cout << "[NyaMeshTransport] [SCIENCE] Modifying physical laws..." << std::endl;
            // TODO: Implement physics modification
        };
    }
    
    void startProcessingThread() {
        processingThread_ = std::thread(&NyaMeshTransport::processIntents, this);
    }
    
    void processIntents() {
        while (processingActive_) {
            Intent intent;
            bool hasIntent = false;
            
            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                if (!intentQueue_.empty()) {
                    intent = intentQueue_.front();
                    intentQueue_.pop();
                    hasIntent = true;
                }
            }
            
            if (hasIntent) {
                processIntent(intent);
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    
    void processIntent(const Intent& intent) {
        // Check for meta-Intents
        if (intent.metaLevel && metaHandlers_.find(intent.action) != metaHandlers_.end()) {
            metaHandlers_[intent.action](intent);
            return;
        }
        
        // Process regular Intents
        std::lock_guard<std::mutex> lock(handlersMutex_);
        if (intentHandlers_.find(intent.action) != intentHandlers_.end()) {
            for (const auto& handler : intentHandlers_[intent.action]) {
                try {
                    handler(intent);
                } catch (const std::exception& e) {
                    std::cout << "[NyaMeshTransport] [ERROR] Intent handler error: " << e.what() << std::endl;
                }
            }
        }
    }
    
    void handleIncomingMessage(const std::string& message) {
        try {
            Intent intent = Intent::fromString(message);
            
            // Filter out our own messages
            if (intent.from == cosmicId_) {
                return;
            }
            
            // Queue for processing
            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                intentQueue_.push(intent);
            }
            
        } catch (const std::exception& e) {
            // Not an Intent, might be legacy message
            Intent legacyIntent;
            legacyIntent.action = "legacy.message";
            legacyIntent.payload = nlohmann::json{{"message", message}};
            legacyIntent.timestamp = getCurrentCosmicTime();
            legacyIntent.id = generateIntentId();
            
            std::lock_guard<std::mutex> lock(queueMutex_);
            intentQueue_.push(legacyIntent);
        }
    }
    
    void handleCapabilityRegistration(const Intent& intent) {
        if (intent.payload.contains("capability")) {
            std::string capName = intent.payload["capability"];
            std::string desc = intent.payload.value("description", "");
            nlohmann::json metadata = intent.payload.value("metadata", nlohmann::json::object());
            
            // Update neighbor capabilities
            {
                std::lock_guard<std::mutex> lock(neighborsMutex_);
                if (!neighbors_.contains(intent.from)) {
                    neighbors_[intent.from] = Neighbor(intent.from);
                }
                
                neighbors_[intent.from].capabilities.emplace_back(capName, desc, metadata);
                neighbors_[intent.from].lastSeen = getCurrentCosmicTime();
            }
            
            std::cout << "[NyaMeshTransport] [ANTENNA] Neighbor " << intent.from 
                      << " registered capability: " << capName << std::endl;
        }
    }
    
    void handleDiscoveryPing(const Intent& intent) {
        // Respond with pong
        Intent pong;
        pong.action = "discovery.ping";
        pong.to = intent.from;
        pong.payload = nlohmann::json{
            {"pong", true},
            {"universeName", universeName_},
            {"capabilities", getCapabilityNames()}
        };
        
        sendIntent(pong);
    }
    
    void handleSystemStatus(const Intent& intent) {
        Intent status;
        status.action = "data.send";
        status.to = intent.from;
        status.payload = nlohmann::json{
            {"responseId", intent.payload.value("requestId", "")},
            {"status", getCosmicStats()}
        };
        
        sendIntent(status);
    }
    
    void announceCosmicPresence() {
        Intent announcement;
        announcement.action = "discovery.ping";
        announcement.payload = nlohmann::json{
            {"universeName", universeName_},
            {"cosmicId", cosmicId_},
            {"capabilities", getCapabilityNames()}
        };
        
        sendIntent(announcement);
    }
    
    void gracefulShutdown() {
        // Announce retirement
        Intent retirement;
        retirement.action = "system.shutdown";
        retirement.payload = nlohmann::json{
            {"reason", "graceful_shutdown"},
            {"finalMessage", "Universe '" + universeName_ + "' retiring gracefully"}
        };
        
        sendIntent(retirement);
        
        // Stop processing
        processingActive_ = false;
        if (processingThread_.joinable()) {
            processingThread_.join();
        }
    }
    
    std::vector<std::string> getCapabilityNames() const {
        std::vector<std::string> names;
        for (const auto& [name, cap] : myCapabilities_) {
            names.push_back(name);
        }
        return names;
    }
    
    std::string generateUniverseName() const {
        return "Universe-" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    }
    
    std::string generateCosmicId() const {
        return "cosmic-" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()) + 
            "-" + std::to_string(rand() % 10000);
    }
    
    std::string generateIntentId() const {
        return "intent-" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()) + 
            "-" + std::to_string(rand() % 10000);
    }
    
    int64_t getCurrentCosmicTime() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
}; // class NyaMeshTransport

} // namespace transport
} // namespace nyamesh2