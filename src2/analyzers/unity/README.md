# 🎮 Unity 専用解析エンジン

## 🎯 設計思想・配置理由

### **なぜ Unity が独立フォルダを必要とするか**
1. **C# の拡張**: 基本は C# だが、Unity 特有の要素が多数存在
2. **ゲームエンジン特化**: MonoBehaviour、コルーチン、属性システム
3. **パフォーマンス解析**: Update() 内の処理、毎フレーム new 警告
4. **エコシステム対応**: .meta、.prefab、.unity ファイルとの連携

### **ファイル構成と役割**
```
unity/
├── unity_analyzer.cpp              # Unity 特化アナライザー（C# 継承）
├── unity_performance_analyzer.cpp  # パフォーマンス警告システム
├── unity_patterns.hpp              # Unity パターン定義
└── README.md                       # この設計理由書
```

### **Unity 特有の解析対象**

#### **1. MonoBehaviour ライフサイクル**
```csharp
// 🔄 実行順序を理解した解析
void Awake() { }      // 1. 最初に一度だけ
void Start() { }      // 2. 初期化時に一度
void Update() { }     // 3. 毎フレーム実行
void LateUpdate() { } // 4. Update 後に実行
void FixedUpdate() {} // 5. 物理演算用（固定間隔）
void OnDestroy() { }  // 6. 破棄時
```

#### **2. コルーチン検出**
```csharp
// ✅ 検出すべきパターン
IEnumerator FadeIn(float duration) {
    yield return new WaitForSeconds(duration);
    yield return null;
}

StartCoroutine(FadeIn(2.0f));
StopCoroutine("FadeIn");
```

#### **3. Unity 属性システム**
```csharp
[SerializeField] private float speed = 5.0f;     // インスペクター表示
[Header("Movement Settings")]                     // セクション見出し
[Range(0, 100)] public int health = 100;        // スライダー表示
[Tooltip("プレイヤーの移動速度")]               // ツールチップ
[RequireComponent(typeof(Rigidbody))]            // 必須コンポーネント
[ExecuteInEditMode]                              // エディタ実行
```

#### **4. パフォーマンス警告対象**
```csharp
// ❌ 警告すべきパターン
void Update() {
    GameObject obj = new GameObject();     // 毎フレーム new！
    transform.Find("Child");               // 毎フレーム検索！
    GetComponent<Rigidbody>();            // 毎フレーム取得！
    
    for (int i = 0; i < 1000; i++) {     // Update 内重い処理！
        ComplexCalculation();
    }
}

// ✅ 推奨パターン
private Rigidbody rb;
void Start() {
    rb = GetComponent<Rigidbody>();      // キャッシュ
}
```

### **実装戦略**

#### **Phase 1: 基本検出（即実装可能）**
```cpp
class UnityAnalyzer : public CSharpPEGTLAnalyzer {
    // MonoBehaviour 継承チェック
    bool is_monobehaviour(const ClassInfo& cls);
    
    // ライフサイクル関数分類
    void classify_unity_methods(std::vector<FunctionInfo>& functions);
    
    // Unity 属性検出
    void detect_unity_attributes(const std::string& content);
};
```

#### **Phase 2: パフォーマンス解析**
```cpp
class UnityPerformanceAnalyzer {
    struct PerformanceWarning {
        std::string type;        // "allocation", "find", "getcomponent"
        std::string location;    // "Update:15"
        std::string message;     // "毎フレーム new 検出"
        std::string suggestion;  // "Start() でキャッシュ推奨"
    };
    
    std::vector<PerformanceWarning> analyze_performance(
        const AnalysisResult& result,
        const std::string& content
    );
};
```

#### **Phase 3: 統合解析**
```cpp
// Unity プロジェクト全体解析
struct UnityProjectAnalysis {
    // 基本統計
    int total_monobehaviours;
    int total_scriptable_objects;
    int total_coroutines;
    
    // パフォーマンス統計
    int performance_warnings;
    int update_allocations;
    int heavy_update_methods;
    
    // Unity 特有統計
    std::map<std::string, int> lifecycle_methods;
    std::map<std::string, int> unity_attributes;
    std::vector<std::string> editor_scripts;
};
```

### **C# アナライザーとの連携**
```cpp
// 継承による拡張
class UnityAnalyzer : public CSharpPEGTLAnalyzer {
public:
    AnalysisResult analyze(const std::string& content, 
                          const std::string& filename) override {
        // 1. 基本的な C# 解析を実行
        auto result = CSharpPEGTLAnalyzer::analyze(content, filename);
        
        // 2. Unity 特有の解析を追加
        enhance_with_unity_patterns(result, content);
        
        // 3. パフォーマンス警告を追加
        add_performance_warnings(result, content);
        
        return result;
    }
};
```

## 🏆 実装完了・成果実績 ✅

**🎉 完成機能（2025-07-31実装完了）**:
- ✅ **Unity プロジェクト自動検出**: `using UnityEngine`, `: MonoBehaviour` パターン認識完了
- ✅ **Unity特化メンバ変数検出**: `[SerializeField]` 属性付きフィールド完全対応
- ✅ **Unity型分類システム**: GameObject・Transform・UI・Physics・Audio系完全識別
- ✅ **ライフサイクルメソッド分類**: Awake→Start→Update→OnDestroy 実行順序認識
- ✅ **パフォーマンス警告システム**: Update内での非効率パターン自動検出
- ✅ **コンポジション設計**: C#解析ベース + Unity特化機能の組み合わせ完成

**🔧 検出精度実績**:
- **PlayerController**: 13個のメンバ変数検出（`[SerializeField]` → `serialized` アクセス修飾子）
- **Unity UI要素**: Text, Button, Slider, Canvas の完全検出
- **ScriptableObject**: WeaponData の全フィールド + Range・Header属性メタデータ保存
- **パフォーマンス警告**: `GameObject.Find`, `GetComponent` 頻繁使用自動検出
- **複雑型サポート**: `UnityEvent<T>`, `List<GameObject>` 等の高度型解析

### **達成された成果**

1. **開発効率向上** ✅
   - MonoBehaviour/ScriptableObject 使用状況の完全可視化
   - パフォーマンス問題の自動早期発見システム
   - Unity ベストプラクティスの自動チェック実装

2. **コード品質向上** ✅
   - Update() 内の処理最適化提案システム
   - メモリアロケーション・Find操作の自動検出
   - コンポーネント参照キャッシュ推奨システム

3. **Unity 特化機能** ✅
   - コルーチン・ライフサイクルメソッド使用統計
   - Unity属性（SerializeField, Header, Range等）使用パターン分析
   - エディタ拡張・コンパイル条件の検出

### **将来展望**
- .meta ファイル解析によるアセット依存関係
- .prefab/.unity ファイルとの統合
- Unity バージョン別対応
- パッケージマネージャー連携
- Timeline/Animation 解析

## 🎮 Unity 開発者のための最強ツールへ！