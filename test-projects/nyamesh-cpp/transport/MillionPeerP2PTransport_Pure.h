#pragma once

/**
 * @file MillionPeerP2PTransport_Pure.h
 * @brief Pure send method extension for nyamesh2
 * 
 * nyamesh2を外部依存なしにするための拡張
 */

#include "MillionPeerP2PTransport.h"
#include "../core/nyamesh.h"

namespace nyamesh2 {

/**
 * @brief MillionPeerP2PTransportの拡張
 * nyamesh2::Message を直接送信できるようにする
 */
class MillionPeerP2PTransportPure : public MillionPeerP2PTransport {
public:
    using MillionPeerP2PTransport::MillionPeerP2PTransport;
    
    /**
     * @brief nyamesh2::Message を直接送信
     * @param msg nyamesh2のメッセージ
     * @return 送信成功数
     */
    std::future<size_t> sendPure(const Message& msg) {
        return std::async(std::launch::async, [this, msg]() {
            // Base::Message に変換
            nyacore::V14::Base::Message baseMsg;
            baseMsg.type = msg.type;
            baseMsg.from = getNodeId();
            baseMsg.data = msg.data;
            
            // カテゴリ変換
            if (msg.category == "IntentRequest") {
                baseMsg.category = nyacore::V14::Base::MessageCategory::IntentRequest;
            } else if (msg.category == "IntentResponse") {
                baseMsg.category = nyacore::V14::Base::MessageCategory::IntentResponse;
            } else if (msg.category == "Proposal") {
                baseMsg.category = nyacore::V14::Base::MessageCategory::Proposal;
            } else {
                baseMsg.category = nyacore::V14::Base::MessageCategory::Notice;
            }
            
            // 基底クラスのsendを呼び出し
            auto future = send(baseMsg, msg.type);
            future.wait();
            
            return 1; // 成功
        });
    }
    
    /**
     * @brief ブロードキャスト版
     * @param msg nyamesh2のメッセージ
     * @return 到達したピア数
     */
    std::future<size_t> broadcastPure(const Message& msg) {
        return std::async(std::launch::async, [this, msg]() {
            // Base::Message に変換
            nyacore::V14::Base::Message baseMsg;
            baseMsg.type = msg.type;
            baseMsg.from = getNodeId();
            baseMsg.data = msg.data;
            
            // カテゴリ変換
            if (msg.category == "IntentRequest") {
                baseMsg.category = nyacore::V14::Base::MessageCategory::IntentRequest;
            } else if (msg.category == "IntentResponse") {
                baseMsg.category = nyacore::V14::Base::MessageCategory::IntentResponse;
            } else if (msg.category == "Proposal") {
                baseMsg.category = nyacore::V14::Base::MessageCategory::Proposal;
            } else {
                baseMsg.category = nyacore::V14::Base::MessageCategory::Notice;
            }
            
            // ブロードキャスト
            auto future = broadcastToMillionPeers(baseMsg, msg.type);
            return future.get();
        });
    }
};

} // namespace nyamesh2