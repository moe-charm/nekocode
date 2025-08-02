# 🐱 NekoCode シンプル再構造化案 - 「だいたいこれでいい」バージョン

## 🎯 基本方針：凝らない！シンプルに！

### 現状維持でOKなもの
- `include/nekocode/` - そのまま使う（すでに整理済み）
- `src/utils/` - 便利ツールはそのまま
- `src/legacy/` - 触らない（動いてるから）

## 📦 シンプルな3分割案

### 1. コア機能（libnekocode）
```
src/lib/
├── analyzers/     # 全言語解析器（現状のまま移動）
├── core/          # セッション管理等（現状のまま移動）
├── formatters/    # 出力整形（現状のまま移動）
└── finders/       # 検索機能（現状のまま移動）
```

### 2. 実行ファイル（apps）
```
src/apps/
├── cli/           # main_ai.cpp, main_human.cpp
└── commands/      # コマンド実装
```

### 3. その他（そのまま）
```
src/
├── utils/         # 動いてるからそのまま！
└── legacy/        # 触らない！
```

## 🚀 超シンプル実装手順

### Step 1: フォルダ作るだけ（5分）
```bash
mkdir -p src/lib
mkdir -p src/apps/cli
```

### Step 2: ファイル移動（git mv使用）（30分）
```bash
# コア機能をlib/へ
git mv src/analyzers src/lib/
git mv src/core src/lib/
git mv src/formatters src/lib/
git mv src/finders src/lib/

# 実行ファイルをapps/へ
git mv src/main/*.cpp src/apps/cli/
git mv src/commands src/apps/
```

### Step 3: CMakeLists.txt修正（30分）
```cmake
# シンプルに2つのターゲット
add_library(nekocode_lib STATIC
    # src/lib/配下の全*.cpp
)

add_executable(nekocode_ai
    src/apps/cli/main_ai.cpp
    src/apps/commands/find_command.cpp
    # 他必要なもの
)

target_link_libraries(nekocode_ai nekocode_lib)
```

### Step 4: インクルードパス修正（1時間）
```bash
# 一括置換スクリプト
find . -name "*.cpp" -o -name "*.hpp" | xargs sed -i 's|src/analyzers|src/lib/analyzers|g'
find . -name "*.cpp" -o -name "*.hpp" | xargs sed -i 's|src/core|src/lib/core|g'
# など...
```

## ✅ これだけ！

**やらないこと**：
- ❌ 完璧なモジュール分離
- ❌ Public/Privateヘッダ分離
- ❌ 複雑な名前空間設計
- ❌ 将来のGUI/LSP対応準備

**やること**：
- ✅ ライブラリとアプリの分離（最小限）
- ✅ フォルダ整理（見やすくなる程度）
- ✅ 動作確認

## 🎉 メリット

1. **簡単**: 2時間で終わる
2. **安全**: 大きな変更なし
3. **実用的**: 必要十分な整理
4. **理解しやすい**: lib/とapps/だけ

## 🤔 Gemini先生への説明

「完璧を求めすぎると時間かかるし、リスクも高いにゃ！まずはシンプルに『ライブラリ』と『アプリ』を分けるだけでも十分価値があるにゃ！将来もっと整理したくなったら、その時にまた考えればいいにゃ～」

## 📝 最終チェック

- [ ] ビルド通る？ → OK
- [ ] テスト通る？ → OK  
- [ ] 前と同じ動作？ → OK

これで十分にゃ！ 🐱