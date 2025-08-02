# 🛡️ NekoCode再構造化 - 詳細リスク分析と軽減策

## 🔴 高リスク要因と対策

### 1. インクルードパスの破壊
**リスク**: ファイル移動により#includeが解決できなくなる
```cpp
// 現在: #include "nekocode/core.hpp"
// 移動後: パスが変わると全ファイル修正が必要
```

**軽減策**:
```bash
# 1. 現在の全インクルードを記録
find ../src -name "*.cpp" -o -name "*.hpp" | xargs grep "^#include" > includes_backup.txt

# 2. シンボリックリンクで一時的に両方のパスを有効化
ln -s new_path old_path

# 3. 段階的に修正
# スクリプトで自動置換（検証付き）
#!/bin/bash
for file in $(find src -name "*.cpp" -o -name "*.hpp"); do
    cp $file $file.bak
    sed -i 's|#include "old/path|#include "new/path|g' $file
    # コンパイル確認
    if ! make -j1 2>/dev/null; then
        mv $file.bak $file
        echo "Failed: $file"
    fi
done
```

### 2. CMake依存関係の循環
**リスク**: ライブラリ分離時に循環依存が発生

**検出方法**:
```cmake
# CMakeで依存関係グラフ生成
cmake .. --graphviz=deps.dot
dot -Tpng deps.dot -o deps.png
```

**軽減策**:
- インターフェース分離（IAnalyzer等）
- 前方宣言の活用
- Pimplイディオムで実装隠蔽

### 3. Git履歴の消失
**リスク**: ファイル移動で履歴が追跡できなくなる

**防止策**:
```bash
# 必ずgit mvを使用
git mv src/old/file.cpp src/new/file.cpp

# 履歴追跡確認
git log --follow src/new/file.cpp

# マージ時は--no-ffで履歴保持
git merge --no-ff reorganization-branch
```

### 4. ビルドシステムの不整合
**リスク**: 部分的な変更でビルドが壊れる

**対策チェックリスト**:
```yaml
pre_change:
  - [ ] 全ファイルのバックアップ
  - [ ] 現在のビルド成功確認
  - [ ] テスト全通過確認

per_file_move:
  - [ ] git mv実行
  - [ ] CMakeLists.txt更新
  - [ ] インクルードパス修正
  - [ ] ビルド確認

post_change:
  - [ ] フルビルド成功
  - [ ] 実行ファイル動作確認
  - [ ] git status確認（追跡漏れなし）
```

## 🟡 中リスク要因と対策

### 1. 名前空間の衝突
**リスク**: ファイル統合時に同名シンボルが衝突

**検出**:
```bash
# 重複シンボル検出
nm -C build/*.o | grep " T " | cut -d' ' -f3 | sort | uniq -d
```

**対策**: 
- 名前空間で明確に分離
- `namespace nekocode::analysis { }`
- `namespace nekocode::core { }`

### 2. テストカバレッジの低下
**リスク**: 構造変更でテストが無効化

**対策**:
```cmake
# テストも同時に移行
set(TEST_SOURCES
    tests/analysis/test_analyzers.cpp
    tests/core/test_session.cpp
)

# カバレッジ計測
add_compile_options(--coverage)
```

## 🟢 低リスク要因（でも注意）

### 1. パフォーマンス劣化
**監視項目**:
- ビルド時間
- 実行時パフォーマンス
- バイナリサイズ

```bash
# ベンチマーク記録
time ./test_build.sh > benchmark_before.txt
ls -lh bin/nekocode_ai >> benchmark_before.txt
```

### 2. ドキュメント不整合
**対策**:
- README.md同時更新
- 開発者向けドキュメント作成

## 📊 リスクマトリックス

| リスク | 発生確率 | 影響度 | 対策優先度 |
|-------|---------|--------|------------|
| インクルード破壊 | 高 | 高 | 最優先 |
| Git履歴消失 | 中 | 高 | 高 |
| CMake循環依存 | 中 | 中 | 中 |
| 名前空間衝突 | 低 | 中 | 低 |
| パフォーマンス | 低 | 低 | 監視のみ |

## 🚨 緊急時対応手順

### ビルド完全失敗時
```bash
#!/bin/bash
echo "🚨 Emergency rollback initiated"
git stash
git checkout main
git branch -D reorganization-current
git tag emergency-rollback-$(date +%Y%m%d-%H%M%S)
```

### 部分的失敗時
```bash
# 特定ファイルのみ復元
git checkout HEAD -- path/to/broken/file
# インクルードパスのみリセット
find . -name "*.bak" -exec bash -c 'mv "$1" "${1%.bak}"' _ {} \;
```

## ✅ 各フェーズ完了基準

### Phase完了チェックリスト
- [ ] ビルド成功（Warning含む）
- [ ] 全自動テスト合格
- [ ] 手動動作確認完了
- [ ] パフォーマンス劣化なし（±5%以内）
- [ ] Git履歴確認済み
- [ ] ドキュメント更新済み
- [ ] チームレビュー完了

この基準を満たさない場合は次フェーズに進まない！