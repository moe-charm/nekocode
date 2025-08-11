# 🚨 **重要: このリポジトリは移動しました！** 🚨

<div align="center">

# 🎉 **NekoCodeは新リポジトリで16倍高速になりました！** 🎉

## 👉 **[github.com/moe-charm/nekocode-rust](https://github.com/moe-charm/nekocode-rust)** 👈

### **↑ 新しい超高速Rust版はこちら ↑**

</div>

---

## 📢 **なぜ移動したの？**

### ❌ **旧リポジトリの問題点：**
- **クローンサイズ235MB** - 軽量ツールには重すぎる
- **C++ビルド地獄** - CMake問題のデバッグに5時間以上
- **遅いパフォーマンス** - 基本解析に19.5秒

### ✅ **新リポジトリのメリット：**
- **クローンサイズ9MB** - 96%削減！超高速クローン
- **Rustのシンプルさ** - `cargo build`で3秒ビルド
- **16倍高速** - 同じ解析がわずか1.2秒
- **ビルド済みバイナリ** - ビルド不要で即座に使用可能

---

## 🚀 **移行ガイド**

```bash
# 旧版（遅い、重い） - 使わないで
git clone https://github.com/moe-charm/nekocode  # 235MBダウンロード ❌

# 新版（速い、軽い） - こっちを使って！
git clone https://github.com/moe-charm/nekocode-rust  # 9MBダウンロード ✅
cd nekocode-rust
./bin/nekocode_ai --help  # すぐ使える！
```

---

## 📌 **このリポジトリの状態**

- **🔒 開発状況**: **アーカイブ済み** - 新規開発なし
- **📅 最終更新**: 2025年8月11日
- **🔄 後継**: [nekocode-rust](https://github.com/moe-charm/nekocode-rust)
- **💾 目的**: 歴史的参照とC++実装のアーカイブ

---

## 🎯 **ユーザー別ガイド**

### **新規ユーザー**
→ 直接 **[nekocode-rust](https://github.com/moe-charm/nekocode-rust)** へ

### **既存ユーザー**
→ ツールを更新してください：
```bash
cd /path/to/your/tools
rm -rf nekocode  # 古い重いリポジトリを削除
git clone https://github.com/moe-charm/nekocode-rust
# スクリプトをnekocode-rust/bin/nekocode_aiを使うように更新
```

### **Claude Codeユーザー**
→ 新リポジトリはClaude Code最適化済み：
- MCPサーバーサポート
- 超高速セッションコマンド（3ms）
- クリーンな9MBリポジトリ

### **コントリビューター**
→ すべての新規開発は **[nekocode-rust](https://github.com/moe-charm/nekocode-rust)** で

---

## 📊 **性能比較**

| 指標 | 旧版 (C++) | 新版 (Rust) | 改善 |
|------|------------|-------------|------|
| クローンサイズ | 235MB | 9MB | **96%削減** |
| ビルド時間 | 5時間以上 | 3秒 | **6000倍高速** |
| 解析速度 | 19.5秒 | 1.2秒 | **16倍高速** |
| 検出精度 | 200関数 | 1000+関数 | **5倍向上** |

---

<div align="center">

# 🚀 **[新リポジトリはこちら](https://github.com/moe-charm/nekocode-rust)** 🚀

### **未来はRust。未来は高速。未来は今。**

</div>

---

*このリポジトリは歴史的参照のためのみ保持されています。すべての活発な開発は[nekocode-rust](https://github.com/moe-charm/nekocode-rust)に移動しました。*