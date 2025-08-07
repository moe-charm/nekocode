# 🚨 Current Task - 編集機能の根本的設計見直し

## 🔥 **重大発覚：設計根本ミス** (2025-08-07)

### **🚨 問題発覚**
- **現在の実装**: 全編集機能が`SessionData`依存（movelines、insert、replace）
- **設計の誤り**: 基本的なファイル操作にセッション解析が不要なのに必須になっている
- **ドキュメント矛盾**: "セッション不要"と宣伝しているが実装はセッション必須

### **💡 正しい理解**
```cpp
// ❌ 間違い：基本ファイル操作なのにSessionData必要
nlohmann::json cmd_movelines_preview(const SessionData& session, ...);

// ✅ 正しい：純粋なファイル操作はセッション不要
nlohmann::json movelines_preview(const std::string& srcfile, ...);
```

---

## 🎯 **新戦略：Direct vs Session 完全分離設計**

### **🔥 Direct Mode（セッション不要）**
**基本的なファイル操作 - 純粋なテキスト処理**

```cpp
namespace DirectEdit {
    // 行移動：純粋なファイル操作
    nlohmann::json movelines_preview(
        const std::string& srcfile, int start_line, int line_count,
        const std::string& dstfile, int insert_line);
    nlohmann::json movelines_confirm(const std::string& preview_id);
    
    // 挿入：位置指定での文字列挿入
    nlohmann::json insert_preview(
        const std::string& file_path,
        const std::string& position,  // "start", "end", "42"
        const std::string& content);
    nlohmann::json insert_confirm(const std::string& preview_id);
    
    // 置換：パターンマッチング置換
    nlohmann::json replace_preview(
        const std::string& file_path,
        const std::string& pattern,
        const std::string& replacement);
    nlohmann::json replace_confirm(const std::string& preview_id);
}
```

### **🧠 Session Mode（セッション必要）** - 🚧 **将来実装**
**構造的編集操作 - 解析結果が必要**

```cpp
namespace StructuralEdit {
    // クラス移動：クラス境界・依存関係解析が必要
    nlohmann::json move_class(
        const SessionData& session,
        const std::string& class_name,
        const std::string& target_file);
    
    // 関数移動：スコープ・依存関係解析が必要  
    nlohmann::json move_function(
        const SessionData& session,
        const std::string& function_name,
        const std::string& target_file);
        
    // メソッド抽出：コンテキスト解析が必要
    nlohmann::json extract_method(
        const SessionData& session,
        const std::string& file_path,
        int start_line, int end_line,
        const std::string& method_name);
}
```

### **📊 操作の本質的分類**

| **操作** | **本質** | **必要情報** | **正しいモード** |
|----------|---------|-------------|----------------|
| **movelines** | ファイル操作 | 行番号のみ | **Direct** |
| **insert** | ファイル操作 | 位置指定のみ | **Direct** |
| **replace** | ファイル操作 | パターンのみ | **Direct** |
| **move-class** | 構造操作 | クラス境界・依存関係 | **Session** |
| **move-function** | 構造操作 | 関数境界・スコープ | **Session** |
| **extract-method** | 構造操作 | コンテキスト・スコープ | **Session** |

---

## 🛠️ **実装計画：Phase 1 - Direct Mode実装**

### **🎯 Phase 1目標**
- 現在のSession依存実装からDirect Mode実装を作成
- セッション不要でファイル操作可能な軽量実装
- MCPからの"セッション不要"宣伝を実現

### **📁 実装構造設計**
```
src/core/commands/
├── direct_edit/                    # NEW! Direct Mode実装
│   ├── direct_movelines.cpp        # 行移動（セッション不要）
│   ├── direct_insert.cpp           # 挿入（セッション不要）
│   ├── direct_replace.cpp          # 置換（セッション不要）
│   └── direct_edit_common.hpp      # 共通ユーティリティ
│
└── edit/                           # 既存Session Mode実装
    ├── movelines_commands.cpp      # SessionData依存版（既存）
    ├── insert_commands.cpp         # SessionData依存版（既存）
    ├── replace_commands.cpp        # SessionData依存版（既存）
    └── edit_history_commands.cpp   # 履歴管理（共通）
```

### **🔧 実装方針**
1. **SessionData除去**: 現在の実装からセッション依存部分を削除
2. **ファイルパス解決**: 相対パス→絶対パス変換をcurrent_path()ベースに
3. **エラーハンドリング**: ファイル存在・権限チェックを強化
4. **Preview/Confirm**: 既存のpreview_id機構を保持

---

## 📋 **Phase 1実装ステップ（10段階）**

### **Phase 1-A: 準備段階 (Step 1-3)**

#### **🔍 Step 1**: 既存実装解析
- `src/core/commands/edit/`内の関数を詳細分析
- SessionData依存箇所の特定
- ファイル操作ロジックの抽出

#### **💾 Step 2**: バックアップ作成  
- 現在の編集機能実装の完全保護
- 既存機能の動作確認

#### **📁 Step 3**: Direct Mode構造作成
- `src/core/commands/direct_edit/`ディレクトリ作成
- 基本ヘッダーファイル作成

### **Phase 1-B: 実装段階 (Step 4-7)**

#### **📝 Step 4**: direct_replace.cpp実装
- `cmd_replace_preview`からSessionData除去
- ファイルパス解決をcurrent_path()ベースに変更
- プロジェクト境界チェック機能実装

#### **📝 Step 5**: direct_insert.cpp実装  
- `cmd_insert_preview`からSessionData除去
- 位置指定（start/end/行番号）処理を独立化
- コンテキスト表示機能を軽量化

#### **📝 Step 6**: direct_movelines.cpp実装
- `cmd_movelines_preview`からSessionData除去  
- ファイル間移動ロジックの独立化
- エラーハンドリングの強化

#### **🔧 Step 7**: 共通ユーティリティ実装
- `direct_edit_common.hpp`作成
- ファイルパス解決・バリデーション共通化
- Preview ID管理の統一

### **Phase 1-C: 統合段階 (Step 8-10)**

#### **⚙️ Step 8**: コマンド統合
- main_ai.cppへのDirect Mode追加
- コマンドライン引数処理更新
- ヘルプメッセージ更新

#### **✅ Step 9**: テスト・検証
- Direct Mode機能テスト
- MCPサーバー統合テスト  
- エラーケース検証

#### **🎉 Step 10**: 完成・commit
- 最終検証完了
- ドキュメント更新
- git commit実行

---

## 🚧 **Phase 2保留事項**

### **🧠 Session Mode（構造的編集）**
**⏸️ 実装保留**（move-class、move-function設計を一緒に検討後）

- クラス移動機能
- 関数移動機能  
- メソッド抽出機能
- 構造的リファクタリング機能

**保留理由**: 高度な解析機能設計を慎重に検討するため

---

## 🎯 **現在のアクション**

**✅ 設計見直し完了**: Direct vs Session分離戦略確立  
**⏭️ 次のステップ**: Phase 1 Step 1開始 - 既存実装解析

**実行準備完了** - Direct Mode実装開始！🚀

---

**最終更新**: 2025-08-07 09:50  
**発見**: 編集機能の設計根本ミス  
**新戦略**: Direct Mode優先実装  
**Phase 2**: 構造的編集は将来設計