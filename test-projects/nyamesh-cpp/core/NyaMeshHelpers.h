/**
 * @file NyaMeshHelpers.h
 * @brief NyaMesh2 ヘルパークラス群 - 実用的な使い方をサポート
 * 
 * にゃーの洞察：
 * - 普通はこんな過激な使い方しない
 * - でも、エッジケースに対応できる設計は重要
 * - ヘルパークラスで複雑さを隠蔽
 * 
 * @author にゃー + Claude + ChatGPT Wisdom
 * @date 2025-07-21
 */

#pragma once

#include <memory>
#include <functional>
#include <chrono>
#include <optional>
#include <mutex>

namespace nyamesh {

// IntentResult定義
enum class IntentResult {
    SUCCESS,
    FAILED,
    TIMEOUT
};

/**
 * @brief Intent送信ポリシー
 */
enum class IntentPolicy {
    FIRE_AND_FORGET,      // 送ったら忘れる（最速）
    WAIT_FOR_ACK,         // 確認応答待ち
    GUARANTEED_DELIVERY,  // 配達保証（再送あり）
    PARENT_AWARE         // 親変更対応（V2モード）
};

/**
 * @brief ノード操作の安全レベル
 */
enum class SafetyLevel {
    UNSAFE,      // ロックなし（単一スレッド環境）
    BASIC,       // 基本的なmutex
    FULL,        // Read-Write Lock
    PARANOID     // 完全整合性チェック付き
};

/**
 * @brief SafeNodeHandle - ノードへの安全なアクセス
 * 
 * 普通の使い方をサポートするRAIIラッパー
 */
template<typename NodeType>
class SafeNodeHandle {
private:
    NodeType* node_;
    std::unique_lock<std::mutex> lock_;
    
public:
    SafeNodeHandle(NodeType* node) 
        : node_(node), lock_(node->nodeMutex) {}
    
    NodeType* operator->() { return node_; }
    const NodeType* operator->() const { return node_; }
    
    // 親変更を安全に実行
    void changeParent(int newParentId, std::function<void(int, int)> callback = nullptr) {
        int oldParent = node_->parentId.load();
        node_->isReparenting.store(true);
        
        if (callback) {
            callback(oldParent, newParentId);
        }
        
        node_->parentId.store(newParentId);
        node_->isReparenting.store(false);
    }
};

/**
 * @brief IntentBuilder - Intent構築ヘルパー
 * 
 * 普通の使い方：
 * auto intent = IntentBuilder(fromNode, toNode)
 *     .withPayload("hello")
 *     .withPolicy(IntentPolicy::FIRE_AND_FORGET)
 *     .build();
 */
class IntentBuilder {
private:
    int fromNode_;
    int toNode_;
    std::string payload_;
    IntentPolicy policy_ = IntentPolicy::FIRE_AND_FORGET;
    std::chrono::milliseconds timeout_{100};
    std::optional<int> parentSnapshot_;
    
public:
    IntentBuilder(int from, int to) : fromNode_(from), toNode_(to) {}
    
    IntentBuilder& withPayload(const std::string& payload) {
        payload_ = payload;
        return *this;
    }
    
    IntentBuilder& withPolicy(IntentPolicy policy) {
        policy_ = policy;
        return *this;
    }
    
    IntentBuilder& withTimeout(std::chrono::milliseconds timeout) {
        timeout_ = timeout;
        return *this;
    }
    
    IntentBuilder& withParentSnapshot(int parentId) {
        parentSnapshot_ = parentId;
        return *this;
    }
    
    auto build() {
        struct Intent {
            int from, to;
            std::string payload;
            IntentPolicy policy;
            std::chrono::milliseconds timeout;
            std::optional<int> parentSnapshot;
        };
        
        return Intent{fromNode_, toNode_, payload_, policy_, timeout_, parentSnapshot_};
    }
};

/**
 * @brief BatchOperationGuard - バッチ操作の安全実行
 * 
 * 大量のノード操作を安全に実行
 */
class BatchOperationGuard {
private:
    std::function<void()> onComplete_;
    bool committed_ = false;
    
public:
    explicit BatchOperationGuard(std::function<void()> onComplete = nullptr)
        : onComplete_(onComplete) {}
    
    ~BatchOperationGuard() {
        if (!committed_ && onComplete_) {
            onComplete_();  // ロールバック等
        }
    }
    
    void commit() { committed_ = true; }
};

/**
 * @brief ParentChildTransaction - 親子関係変更のトランザクション
 * 
 * 普通の使い方：親子関係を安全に変更
 */
class ParentChildTransaction {
private:
    struct Change {
        int nodeId;
        int oldParent;
        int newParent;
    };
    std::vector<Change> changes_;
    bool committed_ = false;
    
public:
    void addChange(int nodeId, int oldParent, int newParent) {
        changes_.push_back({nodeId, oldParent, newParent});
    }
    
    void commit() {
        // 全変更を一括適用
        for (const auto& change : changes_) {
            // 実際の変更処理
        }
        committed_ = true;
    }
    
    void rollback() {
        if (!committed_) {
            // 変更を元に戻す
            for (auto it = changes_.rbegin(); it != changes_.rend(); ++it) {
                // ロールバック処理
            }
        }
    }
    
    ~ParentChildTransaction() {
        if (!committed_) {
            rollback();
        }
    }
};

/**
 * @brief IntentRouter - 賢いIntent配送
 * 
 * 普通の使い方ではこれで十分
 */
class IntentRouter {
private:
    SafetyLevel safetyLevel_ = SafetyLevel::BASIC;
    
public:
    void setSafetyLevel(SafetyLevel level) { safetyLevel_ = level; }
    
    template<typename NodePool>
    bool routeIntent(NodePool& pool, int from, int to, const std::string& payload) {
        switch (safetyLevel_) {
            case SafetyLevel::UNSAFE:
                // 単純送信（最速）
                return pool.sendIntentUnsafe(from, to, payload);
                
            case SafetyLevel::BASIC:
                // 基本的な保護
                return pool.sendIntent(from, to, payload) == IntentResult::SUCCESS;
                
            case SafetyLevel::FULL:
                // Read-Write Lock使用
                return pool.sendIntentWithRWLock(from, to, payload);
                
            case SafetyLevel::PARANOID:
                // 完全整合性チェック
                return pool.sendIntentWithFullCheck(from, to, payload);
        }
        return false;
    }
};

/**
 * @brief TypicalUsageHelper - 典型的な使用パターン
 */
class TypicalUsageHelper {
public:
    // 普通のノード作成
    template<typename Pool>
    static int createChild(Pool& pool, int parentId, const std::string& name = "") {
        auto nodes = pool.batchCreateNodes(parentId, 1);
        return nodes.empty() ? -1 : nodes[0];
    }
    
    // 普通のIntent送信（Fire and Forget）
    template<typename Pool>
    static void sendMessage(Pool& pool, int from, int to, const std::string& msg) {
        IntentBuilder(from, to)
            .withPayload(msg)
            .withPolicy(IntentPolicy::FIRE_AND_FORGET)
            .build();
        
        pool.sendIntent(from, to, msg);
    }
    
    // 安全な親変更（普通はこれで十分）
    template<typename Pool>
    static bool safeReparent(Pool& pool, int nodeId, int newParentId) {
        // 一時的にIntent受信を停止
        pool.pauseIntentProcessing(nodeId);
        
        // 親変更
        pool.changeParent(nodeId, newParentId);
        
        // Intent処理再開
        pool.resumeIntentProcessing(nodeId);
        
        return true;
    }
};

/**
 * @brief PerformanceHint - パフォーマンスヒント
 */
struct PerformanceHint {
    static constexpr size_t TYPICAL_CHILDREN_COUNT = 10;      // 普通の子供数
    static constexpr size_t TYPICAL_INTENT_RATE = 100;        // 秒間Intent数
    static constexpr size_t TYPICAL_REPARENT_RATE = 1;        // 秒間親変更数
    
    // 推奨設定を取得
    static SafetyLevel recommendedSafetyLevel(size_t intentRate, size_t reparentRate) {
        if (reparentRate == 0) {
            return SafetyLevel::UNSAFE;  // 親変更なし
        } else if (reparentRate < 10 && intentRate < 1000) {
            return SafetyLevel::BASIC;   // 普通の使い方
        } else if (reparentRate < 100) {
            return SafetyLevel::FULL;    // やや激しい
        } else {
            return SafetyLevel::PARANOID; // 超過激
        }
    }
};

} // namespace nyamesh