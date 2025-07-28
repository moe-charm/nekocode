/**
 * @file IntentCommonOptions.cpp
 * @brief IntentInterpreter静的メンバー定義
 * 
 * JavaScript版v2.1のIntentInterpretationGuideと同じ分類。
 * 
 * @author nyamesh_v21 team
 * @date 2025-01-24
 */

#include "IntentCommonOptions.h"

namespace nyamesh2 {

// ブロードキャストが意味を持つIntent
const std::set<std::string> IntentInterpreter::broadcastMeaningful_ = {
    "data.send",            // データ配信（元々の設計）
    "system.notify",        // 緊急通知（shutdown等）
    "core.retire",          // 引退宣言
    "capability.register",  // 能力宣言（接続時）
    "capability.unregister" // 能力削除通知
};

// ブロードキャストが無意味/危険なIntent
const std::set<std::string> IntentInterpreter::broadcastMeaningless_ = {
    "discovery.ping",       // 無限ループリスク
    "core.link",           // 1対1接続の性質
    "core.detach",         // 特定相手との切断
    "data.request",        // 特定データ要求
    "system.ping",         // システム応答要求
    "system.status",       // 状態問い合わせ
    "core.connections",    // 接続数問い合わせ
    "core.capabilities"    // 能力問い合わせ
};

// 並列処理が安全なIntent（一般的）
const std::set<std::string> IntentInterpreter::parallelSafeByNature_ = {
    "data.send",           // データ送信（状態変更なし）
    "system.notify",       // 通知（読み取り専用）
    "discovery.ping",      // ping（副作用なし）
    "system.ping"          // システムping（副作用なし）
};

// 並列処理に注意が必要なIntent
const std::set<std::string> IntentInterpreter::parallelCaution_ = {
    "capability.register",    // 状態変更あり
    "capability.unregister",  // 状態変更あり
    "core.link",             // 接続状態変更
    "core.detach",           // 接続状態変更
    "core.retire",           // ライフサイクル変更
    "data.request",          // 応答順序重要
    "system.status",         // 応答順序重要
    "core.connections",      // 応答順序重要
    "core.capabilities"      // 応答順序重要
};

} // namespace nyamesh2