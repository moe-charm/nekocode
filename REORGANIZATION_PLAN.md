# 🎯 NekoCode再構造化計画 - 安全で段階的な移行戦略

## 📊 現状分析

### 現在の構造
- **総ファイル数**: 31ファイル
- **総行数**: 17,058行
- **関数数**: 989個
- **クラス数**: 39個

### 主要コンポーネント
1. **analyzers/** - 言語別解析器（BaseAnalyzer継承）
2. **core/** - セッション管理、言語検出
3. **formatters/** - 出力フォーマット（JSON/HTML/Text）
4. **utils/** - UTF8処理、インクルード解析、プログレス追跡
5. **main/** - エントリポイント（AI用・Human用）
6. **legacy/** - Tree-sitter関連（PEGTL移行中）

### 依存関係の特徴
- includeは`nekocode/`プレフィックスで統一済み
- analyzerは`BaseAnalyzer`を継承
- 外部依存: nlohmann/json, UTF8-CPP, PEGTL, Threads

## 🚀 段階的移行計画

### Phase 0: 準備とバックアップ（リスク: 極低）
**目的**: 安全な作業環境の確保
```bash
# 1. 現在の状態をタグ付け
git tag pre-reorganization-backup

# 2. 作業ブランチ作成
git checkout -b reorganization-phase1

# 3. ビルド確認スクリプト作成
echo '#!/bin/bash
mkdir -p build && cd build
cmake .. && make -j$(nproc)
../bin/nekocode_ai analyze ../src --stats-only' > test_build.sh
chmod +x test_build.sh
```

### Phase 1: CMakeレベルの分離（リスク: 低）
**目的**: ファイル移動なしでライブラリ/実行ファイルを論理的に分離

#### 1.1 静的ライブラリ作成
```cmake
# CMakeLists.txt修正
add_library(nekocode_core STATIC
    ${NEKOCODE_SOURCES}  # 既存のソースリスト
)

target_include_directories(nekocode_core PUBLIC
    ${INCLUDE_DIR}
    ${utf8cpp_SOURCE_DIR}/source
)

target_link_libraries(nekocode_core PUBLIC
    nlohmann_json::nlohmann_json
    taocpp::pegtl
    tree-sitter-core
    Threads::Threads
)
```

#### 1.2 実行ファイルの依存を変更
```cmake
add_executable(nekocode_ai
    ${SRC_DIR}/main/main_ai.cpp
    ${SRC_DIR}/main/command_line_args.cpp
    ${SRC_DIR}/main/command_dispatcher.cpp
)

target_link_libraries(nekocode_ai PRIVATE nekocode_core)
```

**検証**: `./test_build.sh`でビルド確認

### Phase 2: インターフェース定義（リスク: 低）
**目的**: 公開APIと内部実装の境界を明確化

#### 2.1 インターフェースヘッダ作成
```bash
# 公開インターフェース用ディレクトリ
mkdir -p include/nekocode/api
```

#### 2.2 主要インターフェース抽出
- `IAnalyzer.hpp` - 解析器インターフェース
- `IFormatter.hpp` - フォーマッタインターフェース  
- `ISession.hpp` - セッション管理インターフェース

### Phase 3: モジュール別整理（リスク: 中）
**目的**: 関連ファイルを論理的にグループ化

#### 3.1 分析モジュール統合
```bash
# analyzers/ + finders/ + symbol関連を統合
mkdir -p src/analysis
# git mvで移動（履歴保持）
git mv src/analyzers/* src/analysis/languages/
git mv src/finders/* src/analysis/
```

#### 3.2 CMakeLists.txt更新
```cmake
# ソースリストを新パスに更新
set(ANALYSIS_SOURCES
    ${SRC_DIR}/analysis/symbol_finder.cpp
    ${SRC_DIR}/analysis/languages/analyzer_factory.cpp
    # ...
)
```

### Phase 4: ディレクトリ構造最終化（リスク: 中）
**目的**: Gemini提案の理想構造に近づける

```
nekocode-cpp/
├── lib/              # コアライブラリ
│   ├── include/      # 公開ヘッダ
│   └── src/          # 実装
├── apps/             # アプリケーション
│   ├── cli/          # CLIツール
│   └── future/       # 将来のGUI/LSP用
├── tests/            # テスト
└── docs/             # ドキュメント
```

## 🛡️ リスク軽減策

### 各フェーズ共通
1. **自動ビルドテスト**: 各変更後に`test_build.sh`実行
2. **機能テスト**: 実際のソースコード解析を実行して結果比較
3. **git履歴保持**: `git mv`使用でファイル履歴維持

### ロールバック手順
```bash
# Phase Nで問題発生時
git reset --hard
git checkout main
git branch -D reorganization-phaseN
```

### 高リスク領域
1. **インクルードパス変更**: 段階的に修正、sed/awkで自動化
2. **CMake依存関係**: 各フェーズで小さく変更
3. **外部依存**: FetchContentは触らない

## 📅 実施スケジュール案

| Phase | 作業内容 | 所要時間 | リスク |
|-------|---------|---------|--------|
| 0 | 準備・バックアップ | 30分 | 極低 |
| 1 | CMake分離 | 1時間 | 低 |
| 2 | インターフェース定義 | 2時間 | 低 |
| 3 | モジュール整理 | 3時間 | 中 |
| 4 | 最終構造化 | 4時間 | 中 |

**合計**: 約10時間（テスト込み）

## ✅ 成功基準

各フェーズで以下を確認:
1. ビルド成功
2. 全テスト合格
3. `nekocode_ai analyze`の出力が変更前と同一
4. git履歴が保持されている

## 🎯 最終目標

- **保守性**: 新機能追加が容易
- **拡張性**: GUI/LSP対応の基盤
- **明確性**: 新規開発者が理解しやすい
- **安定性**: 既存機能に影響なし