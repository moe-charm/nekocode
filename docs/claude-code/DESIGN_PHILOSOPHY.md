# 🐱 NekoCode Design Philosophy for Claude Code

## 📌 重要な設計方針

**NekoCodeはClaude Code専用ツールです**

他のAIツールへの対応は、新しいAIが登場してから考えます。
現時点では、すべての機能がClaude Codeに最適化されています。

## 🎯 Claude Code向け最適化

### 1. 出力行数制限への対応
- **50行制限**を常に意識
- 長い出力は自動的に分割
- 「続きを見る」コマンドを提供

### 2. コピペしやすい出力
```bash
# 実行例を常に含める
./nekocode_ai session-cmd ai_session_xxx "find main"
```

### 3. 段階的詳細化
```
概要 → 詳細 → 具体例
```
の流れで情報を提示

### 4. エラーメッセージの明確化
- 何が問題か
- どう修正するか
- 具体的なコマンド例

## 💡 将来の拡張性

新しいAIツールが登場したら：
```bash
# 将来的にはこんな感じ？
./nekocode_ai --mode gemini analyze file.cpp
./nekocode_ai --mode gpt4 analyze file.cpp
```

でも今は、**Claude Code一筋**で開発を進めます！

---
*この哲学は、実際の使用経験に基づいて策定されました。*