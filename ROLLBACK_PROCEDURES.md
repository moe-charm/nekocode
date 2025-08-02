# 🔄 NekoCode再構造化 - ロールバック手順書

## 🎯 ロールバックの原則

1. **即座に判断**: 問題発見から15分以内に継続/ロールバック判断
2. **完全復元**: 部分的な修正より完全なロールバックを優先
3. **記録重視**: 失敗の原因を必ず記録してから次回再挑戦

## 📋 Phase別ロールバック手順

### Phase 0: 準備段階のロールバック
```bash
#!/bin/bash
# ほぼ不要だが、念のため
git branch -D reorganization-phase1
git tag -d pre-reorganization-backup  # タグ削除
rm test_build.sh  # スクリプト削除
```

### Phase 1: CMake分離のロールバック

#### 症状別対応

**ビルドエラー: undefined reference**
```bash
# CMakeLists.txtのみ復元
git checkout HEAD -- CMakeLists.txt
rm -rf build/
mkdir build && cd build
cmake .. && make
```

**ビルドエラー: 循環依存**
```bash
# 完全ロールバック
git reset --hard pre-reorganization-backup
git clean -fd
```

**実行時エラー**
```bash
# バイナリ比較
diff <(nm ../bin/nekocode_ai.old) <(nm ../bin/nekocode_ai)
# 差分が大きい場合は完全ロールバック
```

### Phase 2: インターフェース定義のロールバック

```bash
#!/bin/bash
# 作成したファイルを削除
rm -rf include/nekocode/api/

# 変更したファイルを復元
git checkout HEAD -- include/nekocode/*.hpp

# ビルドクリーン
cd build && make clean && cmake .. && make
```

### Phase 3: モジュール整理のロールバック

**最も危険なフェーズ - 慎重に！**

```bash
#!/bin/bash
# 1. 移動記録から逆操作を生成
git status --porcelain | grep "^R" | while read status old new; do
    git mv "$new" "$old"
done

# 2. CMakeLists.txt復元
git checkout HEAD -- CMakeLists.txt

# 3. インクルードパス復元
find . -name "*.bak" -exec bash -c 'mv "$1" "${1%.bak}"' _ {} \;

# 4. 完全性確認
git status  # クリーンな状態か確認
```

### Phase 4: 最終構造化のロールバック

```bash
#!/bin/bash
# ディレクトリ構造を元に戻す
mv lib/src/* src/
mv lib/include/* include/
mv apps/cli/* src/main/
rmdir lib/src lib/include lib apps/cli apps

# Git履歴確認
git log --oneline -10
git reflog  # 必要に応じて特定コミットへreset
```

## 🚨 緊急ロールバック（全Phase共通）

### 完全にめちゃくちゃになった場合

```bash
#!/bin/bash
echo "🚨 EMERGENCY ROLLBACK INITIATED"

# 1. 現在の状態を保存（後で分析用）
git stash save "emergency-$(date +%Y%m%d-%H%M%S)"

# 2. 強制的に元の状態へ
git reset --hard pre-reorganization-backup

# 3. 未追跡ファイルも削除
git clean -fdx

# 4. ビルドディレクトリ再作成
rm -rf build/
mkdir build && cd build
cmake .. && make -j$(nproc)

# 5. 動作確認
../bin/nekocode_ai --help
```

## 📊 ロールバック判断基準

### 即座にロールバックすべき状況
- [ ] コンパイルエラーが50個以上
- [ ] セグメンテーションフォルト発生
- [ ] テストの50%以上が失敗
- [ ] git statusで100ファイル以上が modified

### 修正を試みてもよい状況
- [ ] コンパイルエラーが10個以下
- [ ] 単純なパス間違い
- [ ] CMakeの設定ミスのみ
- [ ] 特定の1機能のみ動作しない

## 🔍 ロールバック後の分析

```bash
# 失敗ログ作成
cat > rollback_analysis_$(date +%Y%m%d).md << EOF
# Rollback Analysis

## Date: $(date)
## Phase: $PHASE
## Reason: $REASON

### What happened:
$DESCRIPTION

### Error messages:
\`\`\`
$ERROR_LOG
\`\`\`

### Lessons learned:
- 

### Next attempt changes:
- 
EOF
```

## ✅ ロールバック成功確認

1. **ビルド確認**
   ```bash
   cd build && make clean && cmake .. && make -j$(nproc)
   ```

2. **機能確認**
   ```bash
   ../bin/nekocode_ai analyze ../src --stats-only
   ../bin/nekocode_ai analyze ../examples/basic_analysis.cpp
   ```

3. **Git状態確認**
   ```bash
   git status  # nothing to commit, working tree clean
   git log --oneline -5  # 履歴が正常
   ```

4. **差分確認**
   ```bash
   # 出力が同じことを確認
   ../bin/nekocode_ai analyze ../src > after_rollback.json
   diff before_reorganization.json after_rollback.json
   ```

## 💡 ロールバック回避のコツ

1. **小さく進める**: 一度に多くを変更しない
2. **テスト駆動**: 各ステップでテスト実行
3. **バックアップ**: 各フェーズ開始時にタグ/ブランチ作成
4. **ペアプログラミング**: 可能なら2人で確認しながら
5. **時間制限**: 深夜の作業は避ける（判断力低下）

---

*Remember: ロールバックは恥ではない。安全第一！* 🛡️