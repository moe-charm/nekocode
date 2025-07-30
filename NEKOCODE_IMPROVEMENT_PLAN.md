# 🐱 NekoCode 改善計画書
作成日: 2025-07-30
フィードバック提供: Claude Code

## 📋 概要
実際にNekoCodeを使用したClaude Codeからの貴重なフィードバックを基に、
「分析ツール」から「リファクタリング支援ツール」への進化を目指す改善計画。

## 🎯 改善の優先順位

### 🔴 Phase 1: 緊急バグ修正（1週間以内）

#### 1. セッション内ファイルパス問題
**問題**: セッション作成時と実行時のパス解決が異なる
```bash
# 現状の問題
cd /project && nekocode_ai session-create .
cd /home && nekocode_ai session-cmd xxx "find FileSystemCore"  # 見つからない！
```
**解決策**: 
- セッション作成時の絶対パスを`session_state.json`に保存
- findコマンドで相対パス・絶対パス両方をサポート

#### 2. デバッグ出力の混入
**問題**: 標準出力にDEBUGメッセージが混入し、パイプ処理が困難
```bash
complexity | head -50  # [DEBUG]が邪魔をする
```
**解決策**:
- すべてのデバッグ出力を`std::cerr`へ
- `--quiet`オプションでデバッグ出力を完全抑制

### 🟡 Phase 2: 実用機能追加（2週間以内）

#### 3. duplicatesコマンド
**目的**: リファクタリング中の一時ファイル検出
```bash
./nekocode_ai session-cmd <id> duplicates
# 出力例:
# FileSystemCore_v23_Integrated.cpp (1904 lines)
#   - FileSystemCore_v23_backup.cpp (1900 lines, 98% similar)
#   - FileSystemCore_Fixed.cpp (1850 lines, 95% similar)
```
**実装内容**:
- パターンマッチング: `*_backup*`, `*_Fixed*`, `*_Original*`, `*_old*`, `*.bak`
- ファイルサイズ・行数比較
- 簡易的な内容類似度計算（オプション）

#### 4. large-filesコマンド
**目的**: リファクタリング対象の特定
```bash
./nekocode_ai session-cmd <id> large-files --threshold 1000
# 出力例:
# Files over 1000 lines:
# 1. FileSystemCore_v23_Integrated.cpp: 1904 lines, complexity: 1121 🔴
# 2. TransportManager.cpp: 1456 lines, complexity: 856 🟠
# 3. IntentProcessor.cpp: 1203 lines, complexity: 543 🟡
```

#### 5. 出力フォーマット改善
**目的**: Claude Code向け最適化
- 50行制限を意識した段階的出力
- JSON出力の充実（suggestions含む）
- 人間向けサマリーの改善

### 🟢 Phase 3: AI支援機能（1ヶ月以内）

#### 6. suggest-splitコマンド
**目的**: インテリジェントな分割提案
```bash
./nekocode_ai session-cmd <id> suggest-split FileSystemCore_v23_Integrated.cpp
# 出力例:
# Suggested split for FileSystemCore_v23_Integrated.cpp:
# 
# 1. FileSystemCore_Base.cpp (estimated 400 lines)
#    - Core functionality and interfaces
#    - Functions: Initialize(), Shutdown(), GetInstance()
# 
# 2. FileSystemCore_Operations.cpp (estimated 800 lines)
#    - File operations
#    - Functions: CreateFile(), ReadFile(), WriteFile(), DeleteFile()
# 
# 3. FileSystemCore_Intent.cpp (estimated 500 lines)
#    - Intent handling
#    - Functions: HandleFileIntent(), ProcessFileRequest()
# 
# 4. FileSystemCore_Platform.cpp (estimated 200 lines)
#    - Platform-specific code
#    - Functions: GetPlatformPath(), NormalizePath()
```

#### 7. cleanupコマンド
**目的**: 安全なファイル整理
```bash
# ドライラン
./nekocode_ai session-cmd <id> cleanup --dry-run
# Suggested for deletion:
# - FileSystemCore_backup_20250729.cpp (duplicate of FileSystemCore.cpp)
# - test_old.cpp (no references found)
# - TODO_old.txt (outdated, last modified 3 months ago)

# 実行
./nekocode_ai session-cmd <id> cleanup --execute
# ⚠️  This will delete 3 files. Confirm? [y/N]
```

### 🔵 Phase 4: 将来的な拡張（3ヶ月以内）

#### 8. 追加コマンド群
- **todo**: TODOコメント一覧と優先度
- **unused**: 未使用関数・変数の検出
- **dependencies**: ファイル間依存関係グラフ
- **metrics**: プロジェクト全体の健全性スコア
- **compare**: 2ファイル間の差分と改善点

#### 9. インタラクティブモード
```bash
./nekocode_ai session-interactive <id>
NekoCode> find FileSystemCore
NekoCode> complexity top 10
NekoCode> suggest-split FileSystemCore_v23_Integrated.cpp
NekoCode> exit
```

#### 10. ~~Claude Code専用モード~~ → デフォルト動作として実装
**重要な方針変更**: NekoCodeはそもそもClaude Code向けツールなので、
すべての出力を最初からClaude Code向けに最適化する。
```bash
# 特別なオプション不要、デフォルトで：
# - 自動的に50行で区切る
# - コピペしやすいコマンド例を含む
# - 段階的詳細化をサポート
```
*新しいAIが登場したら、その時に別モードを検討*

## 📊 成功指標

1. **バグ修正**: パス問題とデバッグ出力が解決
2. **実用性**: duplicatesとlarge-filesで即効果
3. **AI価値**: suggest-splitが実用レベルの提案
4. **安全性**: cleanup --dry-runで事故防止
5. **使いやすさ**: Claude Codeとの相性改善

## 🚀 実装アプローチ

### Phase 1-2: C++での着実な実装
- 既存のセッションマネージャー拡張
- 新コマンドハンドラー追加
- 出力フォーマッター改善

### Phase 3: AI機能の段階的導入
- まずはルールベースの分割提案
- 使用パターンを学習して改善
- 将来的にLLM連携も視野に

## 📝 次のアクション

1. **session_manager.cpp**の修正開始（パス問題）
2. **debug_logger.cpp**の出力先変更
3. **duplicates_command.cpp**の新規作成

---
*この計画は実際の使用フィードバックに基づいており、
実用性を最優先に段階的な改善を目指します。*