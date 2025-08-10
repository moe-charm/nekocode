# 🦀 NekoCode Rust

**NekoCode C++からRustへの移植プロジェクト** - 高速コード解析ツールの実験的Rust実装

## 概要

これは既存のC++版NekoCodeのRust移植版です。現在の機能とアーキテクチャパターンをすべて維持しながら、Rustの安全性とパフォーマンスの利点を活用した実験的プロジェクトです。

## アーキテクチャ

### コアコンポーネント構造

```
nekocode-rust/
├── src/
│   ├── analyzers/           # 言語別アナライザー（分離を維持）
│   │   ├── javascript/      # JavaScript/TypeScriptアナライザー
│   │   ├── python/          # Pythonアナライザー
│   │   ├── cpp/             # C/C++アナライザー
│   │   ├── csharp/          # C#アナライザー
│   │   ├── go/              # Goアナライザー
│   │   └── rust_lang/       # Rustアナライザー（名前衝突回避）
│   ├── core/                # コア機能
│   │   ├── session/         # セッション管理
│   │   ├── commands/        # コマンドシステム
│   │   └── edit/            # 編集操作
│   ├── universal/           # Universal ASTシステム
│   └── main.rs              # エントリポイント
```

## 主要機能

### ✅ 実装済み機能

- **8言語サポート**: JavaScript、TypeScript、Python、C++、C、C#、Go、Rust
- **コマンドライン界面**: C++版と互換性のあるCLI
- **JSON出力**: AI/Claude統合のための構造化出力
- **基本解析**: ファイル統計、複雑度分析、基本構造解析
- **セッション管理**: 解析セッションの作成と管理（基本実装）

### 🚧 開発中機能

- **MoveClass機能**: クラス/関数をファイル間で移動
- **置換操作**: パターンベースの置換とプレビュー
- **編集履歴**: 編集操作の追跡と表示
- **パフォーマンス最適化**: 並列処理の完全実装
- **詳細パーサー**: 各言語の包括的な文法解析

## インストール・ビルド

### 必要条件

- Rust 1.70.0以上
- Cargo（Rustに含まれる）

### ビルド手順

```bash
cd nekocode-rust
cargo build --release
```

### 実行

```bash
# ヘルプ表示
./target/release/nekocode-rust --help

# ファイル解析
./target/release/nekocode-rust analyze path/to/file.js

# サポート言語表示
./target/release/nekocode-rust languages
```

## 使用例

### 基本的なファイル解析

```bash
# JavaScript ファイルの解析
./target/release/nekocode-rust analyze example.js

# Python ファイルの解析（統計のみ）
./target/release/nekocode-rust analyze --stats-only example.py

# C++ ファイルの詳細解析
./target/release/nekocode-rust analyze --complete example.cpp
```

### JSON出力例

```json
{
  "file_info": {
    "name": "example.js",
    "size_bytes": 158,
    "total_lines": 8,
    "code_lines": 6,
    "comment_lines": 1,
    "empty_lines": 1,
    "code_ratio": 0.75,
    "metadata": {}
  },
  "language": "javascript",
  "classes": [
    {
      "name": "Example",
      "parent_class": "",
      "start_line": 1,
      "end_line": 6,
      "methods": [],
      "properties": [],
      "member_variables": [],
      "metadata": {}
    }
  ],
  "complexity": {
    "cyclomatic_complexity": 2,
    "max_nesting_depth": 1,
    "cognitive_complexity": 1,
    "rating": "simple",
    "rating_emoji": "🟢"
  },
  "statistics": {
    "total_classes": 1,
    "total_functions": 0,
    "total_imports": 0,
    "total_exports": 0,
    "unique_calls": 0,
    "total_calls": 0,
    "commented_lines_count": 0
  }
}
```

## 開発状況

### Phase 1: コアインフラ ✅
- [x] 基本的なRustプロジェクト構造
- [x] 言語アナライザートレイト
- [x] 8言語の基本アナライザー
- [x] CLI インターフェース
- [x] JSON出力形式

### Phase 2: 言語アナライザー強化 🚧
- [ ] Pest文法ファイルの完全実装
- [ ] 詳細な構文解析
- [ ] エラーハンドリングの改善

### Phase 3: 機能パリティ 🚧  
- [ ] MoveClass機能の完全実装
- [ ] 置換操作とプレビュー機能
- [ ] 編集履歴システム
- [ ] 並列処理の最適化

### Phase 4: 統合・テスト 🚧
- [ ] Memory システム
- [ ] パフォーマンスベンチマーク
- [ ] C++版との機能互換性確認

## 技術的詳細

### 設計思想

1. **型安全性**: Rustの型システムを活用した安全なコード
2. **パフォーマンス**: ゼロコスト抽象化と並列処理
3. **互換性**: C++版との出力形式互換性
4. **拡張性**: 新しい言語サポートの容易な追加

### 主要依存関係

- **clap**: コマンドライン引数解析
- **serde/serde_json**: JSON シリアライゼーション
- **tokio**: 非同期ランタイム
- **rayon**: データ並列処理
- **anyhow**: エラーハンドリング

### アーキテクチャの特徴

- **トレイトベース設計**: `LanguageAnalyzer`トレイトによる統一インターフェース
- **モジュラー構造**: 各言語アナライザーは独立したモジュール
- **非同期処理**: tokioによる高性能I/O
- **型安全**: Rustの所有権システムによるメモリ安全性

## C++版との比較

| 機能 | C++版 | Rust版 |
|------|-------|--------|
| 基本解析 | ✅ | ✅ |
| 8言語サポート | ✅ | ✅ |
| JSON出力 | ✅ | ✅ |
| セッション管理 | ✅ | ✅（基本） |
| MoveClass | ✅ | 🚧 |
| Memory System | ✅ | 🚧 |
| 並列処理 | ✅ | 🚧 |
| パフォーマンス | 高速 | 調整中 |

## 貢献

このプロジェクトは実験的なものですが、プルリクエストや問題報告を歓迎します。

### 開発環境のセットアップ

```bash
# リポジトリのクローン
git clone <repository-url>
cd nekocode/nekocode-rust

# 依存関係のインストール
cargo build

# テスト実行
cargo test

# 開発版の実行
cargo run -- analyze example.js
```

### コントリビューションガイドライン

1. 既存のコード様式に従う
2. 新機能にはテストを追加
3. C++版との互換性を維持
4. パフォーマンスを考慮した実装

## ライセンス

元のNekoCodeプロジェクトと同じライセンスに従います。

## 謝辞

このプロジェクトは元のC++版NekoCodeの設計とアーキテクチャに基づいています。元の開発チームに感謝します。