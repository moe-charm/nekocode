#pragma once

//=============================================================================
// 🎮 Unity パターン定義 - Unity 特有の解析パターン集
// 
// 🚫 std::regex は使用禁止！文字列ベース検索のみ使用
//=============================================================================

#include <string>
#include <vector>
#include <map>

namespace nekocode {
namespace unity {

//=============================================================================
// 🔄 Unity ライフサイクルメソッド定義
//=============================================================================

// MonoBehaviour ライフサイクル（実行順序付き）
const std::vector<std::string> MONOBEHAVIOUR_LIFECYCLE = {
    "Awake",           // 1. オブジェクト生成時
    "OnEnable",        // 2. 有効化時
    "Start",           // 3. 最初のフレーム前
    "FixedUpdate",     // 4. 物理演算更新（固定間隔）
    "Update",          // 5. 毎フレーム更新
    "LateUpdate",      // 6. Update 後の更新
    "OnDisable",       // 7. 無効化時
    "OnDestroy",       // 8. 破棄時
    
    // 追加のライフサイクル
    "OnApplicationPause",
    "OnApplicationFocus",
    "OnApplicationQuit",
    "Reset"            // エディタでリセット時
};

// コリジョン・トリガーイベント
const std::vector<std::string> PHYSICS_EVENTS = {
    "OnCollisionEnter", "OnCollisionStay", "OnCollisionExit",
    "OnCollisionEnter2D", "OnCollisionStay2D", "OnCollisionExit2D",
    "OnTriggerEnter", "OnTriggerStay", "OnTriggerExit",
    "OnTriggerEnter2D", "OnTriggerStay2D", "OnTriggerExit2D"
};

// レンダリングイベント
const std::vector<std::string> RENDER_EVENTS = {
    "OnPreRender", "OnRenderObject", "OnPostRender",
    "OnRenderImage", "OnWillRenderObject",
    "OnBecameVisible", "OnBecameInvisible"
};

//=============================================================================
// 🏷️ Unity 属性パターン（文字列ベース検索）
//=============================================================================

// Unity 属性の文字列リスト
const std::vector<std::string> UNITY_ATTRIBUTES = {
    "[SerializeField]", "[Header]", "[Range]", "[Tooltip]", "[Space]",
    "[TextArea]", "[Multiline]", "[RequireComponent]", "[ExecuteInEditMode]",
    "[ExecuteAlways]", "[AddComponentMenu]", "[ContextMenu]", "[MenuItem]",
    "[CustomEditor]", "[CanEditMultipleObjects]", "[CreateAssetMenu]"
};

//=============================================================================
// 🎯 コルーチンパターン（文字列ベース）
//=============================================================================

// コルーチン関連の文字列パターン
const std::vector<std::string> COROUTINE_PATTERNS = {
    "IEnumerator",      // コルーチンメソッドの戻り値型
    "yield return",     // yield ステートメント
    "StartCoroutine",   // コルーチン開始
    "StopCoroutine"     // コルーチン停止
};

//=============================================================================
// ⚠️ パフォーマンス警告パターン（文字列ベース）
//=============================================================================

struct PerformancePattern {
    std::string name;
    std::string search_pattern;      // 検索する文字列
    std::string warning_message;
    std::string suggestion;
};

const std::vector<PerformancePattern> PERFORMANCE_PATTERNS = {
    {
        "update_allocation",
        "new ",
        "Update 内でのメモリアロケーション検出",
        "Start() でオブジェクトをキャッシュすることを推奨"
    },
    {
        "update_find",
        "GameObject.Find",
        "Update 内での Find 使用検出",
        "Start() で参照をキャッシュすることを推奨"
    },
    {
        "update_getcomponent",
        "GetComponent",
        "Update 内での GetComponent 使用検出",
        "Start() でコンポーネントをキャッシュすることを推奨"
    },
    {
        "string_concatenation",
        "+= \"",
        "文字列結合検出",
        "StringBuilder の使用を推奨"
    }
};

//=============================================================================
// 🔍 Unity クラス判定（文字列ベース）
//=============================================================================

// Unity 基底クラスの文字列パターン
const std::vector<std::string> UNITY_BASE_CLASSES = {
    ": MonoBehaviour",
    ": ScriptableObject", 
    ": Editor",
    ": EditorWindow",
    ": PropertyDrawer",
    ": NetworkBehaviour"  // Networking 対応
};

//=============================================================================
// 📊 Unity 統計情報構造
//=============================================================================

struct UnityStatistics {
    // 基本統計
    int monobehaviour_count = 0;
    int scriptableobject_count = 0;
    int editor_script_count = 0;
    int coroutine_count = 0;
    
    // ライフサイクルメソッド使用統計
    std::map<std::string, int> lifecycle_usage;
    
    // 属性使用統計
    std::map<std::string, int> attribute_usage;
    
    // パフォーマンス警告
    std::vector<std::pair<std::string, int>> performance_warnings;
    
    // Unity 特有機能
    int serialized_fields = 0;
    int custom_editors = 0;
    int context_menus = 0;
};

} // namespace unity
} // namespace nekocode