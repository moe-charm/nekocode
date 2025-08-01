# 現在のタスク: コメントアウト行検出機能実装 ✅ COMPLETED!

**開始**: 2025-08-01 22:45  
**完了**: 2025-08-01 20:30  
**優先度**: High

## 🎯 目的

前処理でコメント削除を行う際に、コメントアウトされたコード行を同時に検出・保存する機能を実装する。
開発者がデバッグコードや古い実装を見つけて整理できるようにする。

## 📋 最終進捗

- [x] 要件定義・仕様検討完了
- [x] 実装戦略決定（既存解析に統合、シンプル実装）
- [x] CommentInfo構造体定義
- [x] TypeScript前処理関数修正
- [x] JavaScript前処理関数修正
- [x] C++前処理関数修正
- [x] Python前処理関数修正
- [x] 基本的なコード判定実装（全言語対応）
- [x] JSON出力統合
- [x] テスト・動作確認
- [x] ドキュメント更新（--help, README, docs/）

## 🔧 技術方針

### 既存インフラ活用
- **前処理システム**: 既存の`preprocess_content`を拡張
- **AnalysisResult**: 既存構造体に`commented_lines`配列追加
- **JSON出力**: 既存フォーマットに自然に統合

### シンプル実装戦略
```cpp
struct CommentInfo {
    uint32_t line_start;    // 開始行番号
    uint32_t line_end;      // 終了行番号  
    std::string type;       // "single_line" | "multi_line"
    std::string content;    // 生のコメント内容
    bool looks_like_code;   // 簡単な判定結果
};
```

### 段階的実装
1. **Phase 1**: 基本情報収集（行番号・内容・種類）
2. **Phase 2**: 簡単なコード判定（キーワードベース）
3. **Phase 3**: 将来拡張（カテゴリ分類・削除推奨など）

## 📁 関連ファイル

### 修正対象
- `include/nekocode/types.hpp` - CommentInfo構造体追加
- `src/analyzers/typescript/typescript_pegtl_analyzer.hpp` - 前処理関数修正
- `src/analyzers/base_analyzer.hpp` - 基底クラスに共通機能追加

### テスト用
- `tests/samples/test_ts_analyze.ts` - コメントアウト行のテストケース

## 🚀 実装手順

### Step 1: データ構造定義
1. `types.hpp`にCommentInfo構造体追加
2. AnalysisResultにcommented_lines配列追加

### Step 2: TypeScript前処理修正
1. `preprocess_content`をオーバーロード
2. コメント削除時に情報を保存
3. 行番号の正確な計算

### Step 3: 基本判定ロジック
1. `looks_like_code`関数実装
2. キーワードベースの簡単な判定

### Step 4: 出力統合
1. JSON出力にcommented_lines追加
2. サマリー情報の更新

## 💡 注意点・課題

### 技術的課題
- **行番号計算**: コメント削除後も正確な行番号を保持
- **ネストコメント**: /* /* nested */ */ の処理
- **文字列内コメント**: `"/* not a comment */"` の除外

### パフォーマンス
- **追加コスト最小化**: 既存処理への影響を最小限に
- **メモリ効率**: 必要な情報のみ保存

## 🧪 テスト戦略

### テストケース
```typescript
// 単行コメント例
// console.log('debug info');
// function oldImplementation() { return null; }

/* 複数行コメント例
function removedFunction() {
    return processData();
}
*/

// 通常のコメント（除外対象）
// This is just a regular comment explaining the code
```

### 検証方法
1. 既存の解析結果が変わらないことを確認
2. コメント情報が正確に抽出されることを確認
3. パフォーマンス劣化がないことを確認

## 📊 期待する成果

### 基本機能
- コメントアウトされた行の自動検出
- ファイル・行番号・内容の正確な情報

### JSON出力例
```json
{
  "analysis_type": "file",
  "summary": {
    "total_commented_lines": 5
  },
  "commented_lines": [
    {
      "line_start": 15,
      "line_end": 15, 
      "type": "single_line",
      "content": "// console.log('debug:', data);",
      "looks_like_code": true
    }
  ]
}
```

---
**最終更新**: 2025-08-01 22:45  
**状態**: 実装開始準備完了 🚀