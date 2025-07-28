/**
 * @file TransportBase.h
 * @brief Base Transport class for nyamesh2 - Independent implementation
 * 
 * This is nyamesh2's own transport base class, free from nyacore dependencies
 * 
 * @author nyamesh2 team
 * @date 2025-01-22
 */

#pragma once

#include <string>
#include <functional>
#include <memory>
#include <future>
#include "../nlohmann_json.hpp"

namespace nyamesh2 {
namespace transport {

/**
 * @brief Base class for all nyamesh2 transports
 * 
 * Provides the basic interface that all transport implementations must follow.
 * This is a simplified version focused on P2P messaging.
 */
class TransportBase {
public:
    // Message handler callback type
    using MessageHandler = std::function<void(const std::string&)>;
    
    /**
     * @brief Constructor
     */
    TransportBase() = default;
    
    /**
     * @brief Virtual destructor
     */
    virtual ~TransportBase() = default;
    
    /**
     * @brief Initialize the transport
     * @return Future that completes when initialization is done
     */
    virtual std::future<void> initialize() = 0;
    
    /**
     * @brief Send a message through the transport
     * @param message The message to send
     * @return Future that completes when send is done
     */
    virtual std::future<void> send(const std::string& message) = 0;
    
    /**
     * @brief Subscribe to incoming messages
     * @param handler Function to call when message is received
     */
    virtual void subscribe(MessageHandler handler) = 0;
    
    /**
     * @brief Check if transport is initialized
     * @return true if initialized
     */
    virtual bool isInitialized() const = 0;
    
    /**
     * @brief Get transport statistics
     * @return JSON object with stats
     */
    virtual nlohmann::json getStats() const {
        return {
            {"type", getTransportType()},
            {"initialized", isInitialized()}
        };
    }
    
    /**
     * @brief Get transport type name
     * @return Transport type as string
     */
    virtual std::string getTransportType() const = 0;
    
    /**
     * @brief Destroy/cleanup the transport
     * @return Future that completes when cleanup is done
     */
    virtual std::future<void> destroy() {
        // Default implementation
        return std::async(std::launch::async, []() {
            // Nothing to do by default
        });
    }
};

// For backward compatibility during migration
using Transport = TransportBase;

} // namespace transport
} // namespace nyamesh2