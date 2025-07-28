#pragma once

/**
 * @file P2PMessage.h
 * @brief Simplified P2P Message Structure for Transport Adapter
 * 
 * A lightweight message structure specifically for P2P transport operations,
 * avoiding the complexity of the full Message class during initial integration.
 * 
 * @author Hybrid P2P Architecture Team + Claude Code
 * @version Phase 2.2 - Compilation Fix
 * @date 2025-07-21
 */

#include <string>
#include "../nlohmann_json.hpp"

namespace nyamesh2 {
namespace messaging {

/**
 * @brief Simplified P2P Message for transport operations
 * 
 * This is a minimal message structure designed for P2P transport adapter
 * to avoid compilation issues with the full Message class.
 */
struct P2PMessage {
    std::string type;           ///< Message type
    std::string action;         ///< Action to perform
    nlohmann::json data;        ///< Message payload
    std::string source;         ///< Source identifier
    std::string target;         ///< Target identifier (optional)
    
    /**
     * @brief Default constructor
     */
    P2PMessage() = default;
    
    /**
     * @brief Basic constructor
     */
    P2PMessage(const std::string& t, const std::string& a, const nlohmann::json& d = {})
        : type(t), action(a), data(d) {}
    
    /**
     * @brief Full constructor
     */
    P2PMessage(const std::string& t, const std::string& a, const nlohmann::json& d,
               const std::string& s, const std::string& tgt = "")
        : type(t), action(a), data(d), source(s), target(tgt) {}
    
    /**
     * @brief Convert to JSON
     */
    nlohmann::json toJson() const {
        return {
            {"type", type},
            {"action", action},
            {"data", data},
            {"source", source},
            {"target", target}
        };
    }
    
    /**
     * @brief Create from JSON
     */
    static P2PMessage fromJson(const nlohmann::json& j) {
        P2PMessage msg;
        msg.type = j.value("type", "");
        msg.action = j.value("action", "");
        msg.data = j.value("data", nlohmann::json{});
        msg.source = j.value("source", "");
        msg.target = j.value("target", "");
        return msg;
    }
    
    // toIntent() method will be implemented after including NyaMeshTransport.h
};

} // namespace messaging
} // namespace nyamesh2