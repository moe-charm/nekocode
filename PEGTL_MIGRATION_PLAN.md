# 🚀 PEGTL完全移行計画 - std::regex撲滅作戦

## 🎯 ミッション
**全てのstd::regexをPEGTLに置き換え、二度と後戻りしない**

## 📊 現状分析

### 感染状況（std::regex使用箇所）
```bash
src/analyzers/csharp_analyzer.cpp    # 15個の std::regex
src/analyzers/javascript_analyzer.cpp # 10個の std::regex  
src/analyzers/python_analyzer.cpp     # 8個の std::regex
src/analyzers/cpp_language_analyzer.cpp # 12個の std::regex
src/core.cpp                         # 残存している可能性
```

### 根本原因
1. **増分開発の罠** - 「とりあえず動くもの」から始めた
2. **Claudeの習慣** - std::regexがデフォルト思考
3. **統一設計の欠如** - 各言語バラバラに実装

## 🗺️ 移行ロードマップ

### Phase 1: 基盤整備（1日目）
- [x] NEVER_USE_REGEX.md 作成
- [x] NO_REGEX_BASE.hpp 作成  
- [x] .claude-rules 作成
- [ ] 共通PEGTL基盤クラス作成

### Phase 2: C#から始める（2日目）
- [x] csharp_pegtl_grammar.hpp 作成済み
- [x] csharp_pegtl_analyzer.hpp 作成済み
- [ ] 既存のcsharp_analyzer.cpp を削除
- [ ] ファクトリーをPEGTL版に切り替え
- [ ] テスト実行・検証

### Phase 3: JavaScript/TypeScript（3日目）
- [ ] js_pegtl_grammar.hpp 作成
- [ ] 既存analyzer削除
- [ ] ES6/CommonJS両対応

### Phase 4: Python（4日目）
- [ ] python_pegtl_grammar.hpp 作成
- [ ] インデントベース構文対応
- [ ] 既存analyzer削除

### Phase 5: C/C++（5日目）
- [ ] cpp_pegtl_grammar.hpp 作成
- [ ] プリプロセッサ対応
- [ ] 既存analyzer削除

### Phase 6: 完全移行（6日目）
- [ ] core.cppからstd::regex完全削除
- [ ] CMakeLists.txt更新
- [ ] ベンチマーク実施

## 🛡️ 二度と戻らないための仕組み

### 1. コンパイル時防御
```cmake
# CMakeLists.txt に追加
add_compile_definitions(NEKOCODE_NO_REGEX)
```

### 2. CI/CD チェック
```bash
#!/bin/bash
# check-no-regex.sh
if grep -r "std::regex" src/; then
  echo "ERROR: std::regex found! Use PEGTL instead!"
  exit 1
fi
```

### 3. プリコミットフック
```bash
# .git/hooks/pre-commit
#!/bin/bash
if git diff --cached --name-only | xargs grep -l "std::regex"; then
  echo "std::regex detected! Commit blocked."
  exit 1
fi
```

## 📈 期待される効果

### パフォーマンス
- 解析速度: 10-50倍高速化
- メモリ使用量: 50%削減
- コンパイル時間: 変わらず（ヘッダーオンリー）

### 保守性
- 文法定義が明確で読みやすい
- エラーメッセージが具体的
- 新機能追加が容易

### 正確性
- ネスト構造の完全サポート
- 文脈依存の解析
- あいまいさゼロ

## 🎯 成功の定義

```bash
# これが実行できたら成功
find src -name "*.cpp" -o -name "*.hpp" | xargs grep -c "std::regex"
# 結果: 0
```

## 💪 宣言

**「今日をもって、NekoCodeはstd::regexと決別する」**

二度と正規表現の誘惑に負けない。
PEGTLで新しい解析の世界を切り開く。

---
作成日: 2025-01-27
決意: 不退転