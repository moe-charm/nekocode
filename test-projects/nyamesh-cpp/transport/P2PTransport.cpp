
#include "P2PTransport.h"
#include <iostream>

namespace nyamesh2 { namespace transport {

P2PTransport::P2PTransport() {
    config_.nodeId = "test-node-" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
}

P2PTransport::P2PTransport(const P2PTransportConfig& config) : config_(config) {
    if (config_.nodeId.empty()) {
        config_.nodeId = "test-node-" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    }
}

P2PTransport::~P2PTransport() {
    if (initialized_.load()) {
        stopBackgroundThreads();
    }
}

std::future<void> P2PTransport::initialize() {
    return std::async(std::launch::async, [this]() {
        if (initialized_.load()) return;
        
        state_.store(TransportState::CONNECTING);
        initialized_.store(true);
        state_.store(TransportState::CONNECTED);
        
        startTime_ = std::chrono::steady_clock::now();
        
        std::cout << "[P2PTransport] Initialized node: " << config_.nodeId << std::endl;
    });
}

std::future<void> P2PTransport::send(const std::string& message) {
    return std::async(std::launch::async, [this, message]() {
        if (!initialized_.load()) {
            std::cout << "[P2PTransport] ERROR: Not initialized" << std::endl;
            return;
        }
        
        messagesSent_++;
        std::cout << "[P2PTransport] Mock send: " << message.substr(0, 50) << "..." << std::endl;
        
        // For testing: trigger handlers
        std::lock_guard<std::mutex> lock(handlersMutex_);
        for (const auto& [channel, handlers] : channelHandlers_) {
            for (const auto& handler : handlers) {
                try {
                    handler(message);
                } catch (const std::exception& e) {
                    std::cout << "[P2PTransport] Handler error: " << e.what() << std::endl;
                }
            }
        }
    });
}

void P2PTransport::subscribe(MessageHandler handler) {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    channelHandlers_["default"].push_back(handler);
    std::cout << "[P2PTransport] Added message handler" << std::endl;
}

bool P2PTransport::isInitialized() const {
    return initialized_.load();
}

std::string P2PTransport::getTransportType() const {
    return "P2PTransport";
}

nlohmann::json P2PTransport::getStats() const {
    return nlohmann::json{};  // Simple mock
}

std::future<void> P2PTransport::destroy() {
    return std::async(std::launch::async, [this]() {
        if (initialized_.load()) {
            stopBackgroundThreads();
            initialized_.store(false);
            state_.store(TransportState::DISCONNECTED);
            std::cout << "[P2PTransport] Destroyed" << std::endl;
        }
    });
}

void P2PTransport::startBackgroundThreads() {
    // Mock implementation
}

void P2PTransport::stopBackgroundThreads() {
    shouldStop_.store(true);
}

std::future<bool> P2PTransport::joinMesh() {
    return std::async(std::launch::async, [this]() {
        joinedMesh_.store(true);
        return true;
    });
}

size_t P2PTransport::getConnectionCount() const {
    return 0;  // Mock
}

}} // namespace nyamesh2::transport
