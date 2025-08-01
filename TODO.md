# TODOリスト - NekoCode C++

## 🔥 最優先（今日中）- コメントアウト検出機能

### Phase 1: 基本実装（必須）
- [ ] **CommentInfo構造体定義** - `types.hpp`に追加
  - line_start, line_end, type, content, looks_like_code フィールド
  - AnalysisResultに commented_lines 配列追加
  - 推定時間: 15分

- [ ] **TypeScript前処理関数修正** - 最重要！
  - `preprocess_content` オーバーロード実装
  - `remove_multiline_comments` でコメント情報保存
  - `remove_single_line_comments` でコメント情報保存
  - 行番号の正確な計算ロジック
  - 推定時間: 45分

- [ ] **シンプルコード判定実装**
  - `looks_like_code` 関数実装
  - キーワードベース判定（function, class, void, return, {, ; など）
  - 推定時間: 20分

- [ ] **JSON出力統合**
  - 既存のJSON出力に commented_lines 追加
  - サマリーに total_commented_lines カウント追加
  - 推定時間: 15分

### Phase 1 完了確認
- [ ] **動作テスト**
  - TypeScriptファイルでの基本動作確認
  - 既存解析結果への影響がないことを確認
  - パフォーマンス劣化なしを確認
  - 推定時間: 30分

## 🎯 高優先（今週中）- 機能拡張

### Phase 2: 他言語展開
- [ ] **JavaScript対応** - TypeScriptと同じ仕組み
- [ ] **C++対応** - // と /* */ コメント対応
- [ ] **Python対応** - # コメント対応
- [ ] **C#対応** - // と /* */ コメント対応

### Phase 3: 判定精度向上
- [ ] **カテゴリ分類実装**
  - debug_code: console.log, debugger, print
  - old_implementation: old_, legacy_, deprecated
  - todo_items: TODO, FIXME, HACK
  - experimental: test_, experiment_, draft_

- [ ] **削除安全性判定**
  - safe: デバッグコード
  - risky: 重要そうな処理
  - unknown: 判定困難

### Phase 4: ユーザビリティ向上
- [ ] **専用コマンド実装**
  - `nekocode_ai find-commented-code src/`
  - `nekocode_ai clean-debug-comments src/`

- [ ] **出力フォーマット拡張**
  - HTML レポート出力
  - CSV 出力（スプレッドシート用）
  - コンソール表示の改善

## 💭 アイデア・将来的な機能

### 高度な解析機能
- [ ] **コメントアウト理由推測**
  - 周辺コンテキストの解析
  - 関連TODOコメントとの紐付け
  - Git履歴との連携（いつコメントアウトされたか）

- [ ] **自動クリーンアップ提案**
  - 安全に削除できるコードの特定
  - 削除の優先順位付け
  - 差分プレビュー機能

### CI/CD統合
- [ ] **品質ゲート機能**
  - デバッグコードが残っていたら警告
  - 本番デプロイ前のチェック
  - チーム向けのレポート生成

### 開発者体験向上
- [ ] **VSCode拡張**
  - コメントアウトされたコードのハイライト
  - 削除推奨の警告表示
  - ワンクリック削除機能

- [ ] **GitHub Actions統合**
  - PR作成時の自動チェック
  - コードレビューでの警告表示

## 🛠️ 技術的改善項目

### パフォーマンス最適化
- [ ] **メモリ使用量最適化**
  - コメント内容の効率的な保存
  - 不要な情報の削減

- [ ] **処理速度向上**
  - 並列処理での最適化
  - キャッシュ機構の活用

### コード品質
- [ ] **テストカバレッジ向上**
  - 各言語での包括的テスト
  - エッジケースのテスト追加

- [ ] **エラーハンドリング強化**
  - 不正なコメント構文への対応
  - ファイル読み込みエラーの処理

## 📊 進捗管理

### 完了タスク ✅
- [x] 要件定義・仕様検討
- [x] 技術方針決定
- [x] 実装戦略策定
- [x] 開発計画作成

### 今日の目標 🎯
**Phase 1を完了させる！**
- CommentInfo実装 → TypeScript対応 → テスト → リリース

### 今週の目標 📅
- 全言語対応完了
- 基本的な判定機能実装
- ドキュメント整備

---

**📝 更新ルール**: 
- タスク完了時は即座に ✅ マーク
- 新しいアイデアは「💭 アイデア」セクションに追加
- 優先度変更は随時更新

**最終更新**: 2025-08-01 22:50