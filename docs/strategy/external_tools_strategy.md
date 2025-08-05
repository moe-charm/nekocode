# 🎯 外部ツール戦略 - NekoCodeは軽量のまま！

## 💡 素晴らしい気づき

### 現実
```
LTO = gccの機能（NekoCodeに組み込む必要なし！）
Vulture = Pythonツール（pip install vulture）
staticcheck = Goツール（go install）
```

### 提案
```
NekoCode本体 = 軽量のまま
完全解析 = 外部ツールを呼び出すだけ
```

## 🚀 実装戦略

### NekoCode本体の役割
```cpp
// 擬似コード
if (--complete && is_cpp_file()) {
    // 1. 通常解析
    auto structure = analyze_structure(file);
    
    // 2. 外部ツール確認
    if (has_command("g++")) {
        auto deadcode = call_external_lto_analyzer(file);
        structure.merge(deadcode);
    } else {
        structure.add_warning("LTO not available. Install g++ with LTO support");
    }
}
```

### 外部ツール確認
```bash
# C++完全解析に必要
g++ --version | grep -q "LTO support" || echo "❌ LTO not supported"

# Python完全解析に必要  
vulture --version || pip install vulture

# Go完全解析に必要
staticcheck --version || go install honnef.co/go/tools/cmd/staticcheck@latest
```

## 📋 ユーザーガイド

### C++完全解析セットアップ
```bash
# Ubuntu/Debian
sudo apt install gcc g++

# 確認
g++ --help | grep -q "flto" && echo "✅ LTO available"

# 使用
nekocode_ai analyze file.cpp --complete
```

### Python完全解析セットアップ
```bash
# インストール
pip install vulture

# 確認  
vulture --version

# 使用
nekocode_ai analyze file.py --complete
```

### Go完全解析セットアップ
```bash
# インストール
go install honnef.co/go/tools/cmd/staticcheck@latest

# 確認
staticcheck --version

# 使用
nekocode_ai analyze file.go --complete
```

## 🎊 メリット

### 1. **NekoCode軽量**
- バイナリサイズ変化なし
- 依存関係なし
- ビルド時間変化なし

### 2. **柔軟性**
```
完全解析不要なユーザー → 何もインストール不要
C++完全解析ユーザー → gccだけ
Python完全解析ユーザー → Vultureだけ
全部使いたいユーザー → 全ツール
```

### 3. **保守性**
- 外部ツールの更新はユーザー責任
- NekoCode本体は影響を受けない
- 各言語のベストプラクティスツールを選択可能

### 4. **拡張性**
```bash
# 将来的に追加も簡単
nekocode_ai analyze --complete --tool=custom_analyzer
```

## 🔧 実装例

### 外部ツール検出
```python
def check_external_tools(language):
    tools = {
        'cpp': ['g++', '--help'],
        'python': ['vulture', '--version'], 
        'go': ['staticcheck', '--version']
    }
    
    if language in tools:
        try:
            subprocess.run(tools[language], check=True, capture_output=True)
            return True
        except:
            return False
    return False
```

### エラーメッセージ
```json
{
  "analysis_mode": "complete",
  "structure": { /* 通常解析結果 */ },
  "dead_code": {
    "status": "tool_not_found",
    "message": "Install 'vulture' for Python dead code detection: pip install vulture",
    "documentation": "https://nekocode.dev/complete-analysis-setup"
  }
}
```

## 📚 ドキュメント戦略

### README.md に追加
```markdown
## 完全解析モード

基本使用:
nekocode_ai analyze <file> --complete

### セットアップ（言語別）

**C++**: `sudo apt install gcc g++`
**Python**: `pip install vulture` 
**Go**: `go install honnef.co/go/tools/cmd/staticcheck@latest`

詳細: [完全解析セットアップガイド](docs/complete-analysis.md)
```

## 🎯 結論

**完璧な戦略にゃ！**

- NekoCode本体は軽量のまま
- 必要な人だけが外部ツール導入
- 実装もシンプル
- 各言語のベストツールを活用

これで「軽量だけど強力」なNekoCodeが実現するにゃ！🚀