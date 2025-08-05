# 🚀 NekoCode 完全解析モード設計

## 📊 2つのモード

### 1. **ノーマルモード**（デフォルト）
```bash
nekocode_ai analyze <path>
```
- **内容**: 通常のNekoCode構造解析
- **速度**: ⚡ 高速（数秒）
- **取得情報**:
  - 関数、クラス、構造体
  - 依存関係、インクルード
  - 複雑度メトリクス

### 2. **完全解析モード**（新機能）
```bash
nekocode_ai analyze <path> --complete
```
- **内容**: 構造解析 + デッドコード + 品質改善
- **速度**: 🐢 遅い（数分）
- **取得情報**:
  - ノーマルモードの全情報
  - ＋ LTOによるデッドコード検出
  - ＋ Clang-Tidyによる品質提案

## 🎯 実装方針

### コマンドライン引数
```cpp
// command_line_args.hpp に追加
struct CommandLineArgs {
    // 既存
    std::string command;
    std::string target;
    
    // 新規追加
    bool complete_analysis = false;  // --complete フラグ
};
```

### 処理フロー
```
1. 通常のNekoCode解析（必須）
   ↓
2. --complete が指定されたら
   ↓
3. 言語判定
   ↓
4. 言語別の完全解析実行
   - C++: LTO + Clang-Tidy
   - Python: Vulture
   - Go: staticcheck
   - etc...
```

## 💡 AIモード限定の理由

### なぜAIモードだけ？
1. **処理時間**: 完全解析は時間がかかる
2. **出力量**: 大量の情報をAIが要約・分析
3. **使用頻度**: 人間は普段は高速な構造解析で十分

### 実装例
```cpp
// session_commands.cpp
void handle_analyze_command(const SessionContext& ctx) {
    // 通常解析
    auto result = analyze_structure(ctx.path);
    
    // AIモードかつ--completeフラグ
    if (ctx.ai_mode && ctx.args.complete_analysis) {
        // 言語別の完全解析
        if (is_cpp_file(ctx.path)) {
            result.dead_code = run_lto_analysis(ctx.path);
            result.quality = run_clang_tidy(ctx.path);
        } else if (is_python_file(ctx.path)) {
            result.dead_code = run_vulture(ctx.path);
        }
        // ... 他の言語
    }
    
    output_json(result);
}
```

## 📋 JSON出力フォーマット

### ノーマルモード
```json
{
  "mode": "normal",
  "analysis": {
    "functions": [...],
    "classes": [...],
    "complexity": {...}
  }
}
```

### 完全解析モード
```json
{
  "mode": "complete",
  "analysis": {
    "functions": [...],
    "classes": [...],
    "complexity": {...}
  },
  "dead_code": {
    "unused_functions": [...],
    "unused_variables": [...],
    "detection_method": "LTO"
  },
  "quality": {
    "modernization": [...],
    "performance": [...],
    "readability": [...],
    "tool": "clang-tidy"
  }
}
```

## 🔧 実装優先順位

### Phase 1: 基本実装
1. コマンドライン引数追加（--complete）
2. C++のLTO統合
3. JSON出力拡張

### Phase 2: ツール統合
1. Clang-Tidy統合（C++）
2. Vulture統合（Python）
3. staticcheck統合（Go）

### Phase 3: 最適化
1. キャッシュ機能
2. 並列実行
3. 差分解析

## 💭 深い考察

### メリット
1. **シンプル**: ユーザーは1つのフラグで切り替え
2. **互換性**: 既存の使い方を壊さない
3. **拡張性**: 言語ごとに最適なツール追加可能

### 注意点
1. **依存関係**: 外部ツール（clang-tidy等）の存在確認
2. **エラー処理**: ツールがない場合は部分的な結果を返す
3. **パフォーマンス**: 大規模プロジェクトでのタイムアウト対策

これで「普段は高速」「必要時は完全」の使い分けが簡単にできるにゃ！