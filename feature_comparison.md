# NekoCode機能比較表: C++ vs Rust Edition

## 📊 機能対応状況

| 機能分類 | 機能名 | C++版 | Rust版 | 差分/改善点 |
|----------|--------|-------|--------|-------------|
| **基本解析** | analyze | ✅ | ✅ | Rust版はTree-sitter対応で16倍高速 |
| **言語対応** | languages | ✅ | ✅ | 同等（8言語対応） |
| **統計表示** | stats | ✅ | ❌ | C++版のみ（Rust版は今後実装予定） |
| | | | | |
| **セッション** | session-create | ✅ | ✅ | 同等 |
| **セッション** | session-command | ✅ | ✅ | 同等 |
| | | | | |
| **編集機能** | replace (即実行) | ✅ | ❌ | C++版は即実行可、Rust版はpreview必須 |
| **編集機能** | replace-preview | ✅ | ✅ | 同等 |
| **編集機能** | replace-confirm | ✅ | ✅ | 同等 |
| **編集機能** | insert (即実行) | ✅ | ❌ | C++版は即実行可、Rust版はpreview必須 |
| **編集機能** | insert-preview | ✅ | ✅ | 同等 |
| **編集機能** | insert-confirm | ✅ | ✅ | 同等 |
| **編集機能** | movelines (即実行) | ✅ | ❌ | C++版は即実行可、Rust版はpreview必須 |
| **編集機能** | movelines-preview | ✅ | ✅ | 同等 |
| **編集機能** | movelines-confirm | ✅ | ✅ | 同等 |
| **編集機能** | moveclass (即実行) | ✅ | ❌ | C++版は即実行可、Rust版はpreview必須 |
| **編集機能** | moveclass-preview | ✅ | ✅ | 同等 |
| **編集機能** | moveclass-confirm | ✅ | ✅ | 同等 |
| | | | | |
| **AST機能** | ast-stats | ❌ | ✅ | **Rust版のみ**（新機能） |
| **AST機能** | ast-query | ❌ | ✅ | **Rust版のみ**（新機能） |
| **AST機能** | scope-analysis | ❌ | ✅ | **Rust版のみ**（新機能） |
| **AST機能** | ast-dump | ❌ | ✅ | **Rust版のみ**（新機能） |
| | | | | |
| **設定管理** | config show | ✅ | ✅ | 同等 |
| **設定管理** | config set | ✅ | ✅ | 同等 |
| | | | | |
| **メモリ管理** | memory | ✅ | ✅ | 同等 |

## 📈 機能数比較
- **C++版**: 20機能（即実行型が多い）
- **Rust版**: 18機能 + AST革命（4つの新機能）

## ⚡ 特記事項

### 🦀 **Rust版の優位性**
1. **Tree-sitter統合**: 16倍高速化
2. **AST Revolution**: 4つの新機能（ast-stats, ast-query, scope-analysis, ast-dump）
3. **安全性**: preview必須で安全な操作
4. **ビルド**: cargo一発 vs make地獄

### 💀 **C++版の特徴**
1. **即実行型**: replace, insert, movelines, moveclassの即実行版
2. **stats機能**: 統計表示（Rust版未実装）
3. **レガシー**: 参考実装として保持

## 🎯 結論
**Rust版の方が機能的に上位**：
- 核心機能は全て実装済み
- AST革命で4つの新機能追加
- 性能面で16倍高速
- より安全な設計思想（preview必須）