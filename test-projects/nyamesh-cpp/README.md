# NYAMESH v23 - Simplified Architecture

## 🎯 **目標: Simple is better. Complex design is wrong design.**

### 📊 **v22からの主な変更**

#### ✅ **削除項目（複雑化の原因除去）**
- **NetworkCore除外**: 7Core→6Core ダウンサイジング
- **Intent数削減**: 300+ → 50以下の劇的簡素化
- **JSON依存軽減**: nlohmann::json のコア依存除去

#### 🔧 **簡素化項目**
- **Intent統合**: `CoreName::Concept` パターン採用
- **Payload軽量化**: `std::string` ベースのハイブリッド方式
- **Core責務明確化**: カプセル化4階層ルール徹底

### 🌟 **6Core安定版構成**
1. **SettingsCore** - 設定管理
2. **EditorCore** - テキスト編集
3. **FileSystemCore** - ファイル管理
4. **UICoordinator** - UI統合
5. **QuickAccessCore** - クイックアクセス
6. **LocalizationCore** - 多言語対応

### 🎯 **設計哲学**
- **YAGNI徹底**: 必要最小限の実装のみ
- **疎結合極致**: Intent通信による完全分離
- **軽量性重視**: 依存関係最小化
- **段階的リファクタリング**: 1Coreずつ安全に移行

### 📁 **ディレクトリ構造**
```
nyamesh_v23/
├── core/           # 軽量メッセージング基盤
├── cores/          # 6Core実装（EditorCore, FileSystemCore, SettingsCore）
├── interfaces/     # インターフェース定義
├── messages/       # 簡素化されたIntent定義
└── docs/           # 設計ドキュメント
```

### 🚀 **移行計画**
1. **Phase1**: Intent統合（`FileSystem::Operation` パターン）
2. **Phase2**: JSON依存分離（コア→個別Core）
3. **Phase3**: NetworkCore完全除去
4. **Phase4**: 6Core統合テスト

---

**コンセプト**: Gemini先生アドバイス「コアは特定フォーマットを強制しない」を実現

**v22→v23**: 7Core破綻からの6Core安定版への回帰