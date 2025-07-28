#pragma once

//=============================================================================
// 🎮 Unity パターン定義 - Unity 特有の解析パターン集
//=============================================================================

#include <string>
#include <vector>
#include <regex>

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
// 🏷️ Unity 属性パターン
//=============================================================================

// Unity 属性の正規表現パターン
const std::regex UNITY_ATTRIBUTE_PATTERN(
    R"(\[\s*(SerializeField|Header|Range|Tooltip|Space|TextArea|Multiline|RequireComponent|ExecuteInEditMode|ExecuteAlways|AddComponentMenu|ContextMenu|MenuItem|CustomEditor|CanEditMultipleObjects|CreateAssetMenu)\s*(?:\([^)]*\))?\s*\])"
);

// SerializeField 詳細パターン
const std::regex SERIALIZE_FIELD_PATTERN(
    R"(\[SerializeField\]\s*(?:private\s+)?(\S+)\s+(\w+))"
);

// Range 属性詳細パターン
const std::regex RANGE_ATTRIBUTE_PATTERN(
    R"(\[Range\s*\(\s*([^,]+)\s*,\s*([^)]+)\s*\)\s*\])"
);

//=============================================================================
// 🎯 コルーチンパターン
//=============================================================================

// IEnumerator メソッド定義
const std::regex COROUTINE_PATTERN(
    R"((?:public|private|protected)?\s*IEnumerator\s+(\w+)\s*\([^)]*\))"
);

// yield return ステートメント
const std::regex YIELD_PATTERN(
    R"(yield\s+return\s+(?:null|new\s+WaitFor\w+\([^)]*\)|[^;]+);)"
);

// StartCoroutine 呼び出し
const std::regex START_COROUTINE_PATTERN(
    R"(StartCoroutine\s*\(\s*(?:\"(\w+)\"|(\w+)\s*\([^)]*\))\s*\))"
);

//=============================================================================
// ⚠️ パフォーマンス警告パターン
//=============================================================================

struct PerformancePattern {
    std::string name;
    std::regex pattern;
    std::string warning_message;
    std::string suggestion;
};

const std::vector<PerformancePattern> PERFORMANCE_PATTERNS = {
    {
        "update_allocation",
        std::regex(R"((?:Update|FixedUpdate|LateUpdate)\s*\([^)]*\)\s*\{[^}]*new\s+\w+)"),
        "Update 内でのメモリアロケーション検出",
        "Start() でオブジェクトをキャッシュすることを推奨"
    },
    {
        "update_find",
        std::regex(R"((?:Update|FixedUpdate|LateUpdate)\s*\([^)]*\)\s*\{[^}]*(?:GameObject\.Find|transform\.Find))"),
        "Update 内での Find 使用検出",
        "Start() で参照をキャッシュすることを推奨"
    },
    {
        "update_getcomponent",
        std::regex(R"((?:Update|FixedUpdate|LateUpdate)\s*\([^)]*\)\s*\{[^}]*GetComponent)"),
        "Update 内での GetComponent 使用検出",
        "Start() でコンポーネントをキャッシュすることを推奨"
    },
    {
        "string_concatenation_loop",
        std::regex(R"(for\s*\([^)]*\)\s*\{[^}]*\+=\s*\"[^\"]*\")"),
        "ループ内での文字列結合検出",
        "StringBuilder の使用を推奨"
    }
};

//=============================================================================
// 🔍 Unity クラス判定
//=============================================================================

// MonoBehaviour 継承パターン
const std::regex MONOBEHAVIOUR_CLASS_PATTERN(
    R"(class\s+(\w+)\s*:\s*(?:\w+\s*,\s*)*MonoBehaviour)"
);

// ScriptableObject 継承パターン
const std::regex SCRIPTABLEOBJECT_CLASS_PATTERN(
    R"(class\s+(\w+)\s*:\s*(?:\w+\s*,\s*)*ScriptableObject)"
);

// Editor クラスパターン
const std::regex EDITOR_CLASS_PATTERN(
    R"(class\s+(\w+)\s*:\s*(?:\w+\s*,\s*)*(?:Editor|EditorWindow|PropertyDrawer))"
);

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