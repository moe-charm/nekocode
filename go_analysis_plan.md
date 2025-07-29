# 🐹 Go Language Analyzer Implementation Plan

## 🎯 Go言語の特徴（解析すべきポイント）

### 基本構文
```go
package main                    // パッケージ宣言
import "fmt"                   // インポート
func main() { }                // 関数
var x int = 10                 // 変数宣言
type User struct { }           // 構造体
```

### 🚀 Go特有の面白い機能
```go
// 1. Goroutine（並行処理）
go func() { /* 処理 */ }()
go myFunction()

// 2. Channel（通信）
ch := make(chan int, 10)
ch <- 42
value := <-ch
close(ch)

// 3. Interface
type Writer interface {
    Write([]byte) (int, error)
}

// 4. 独特な制御構文
select {
case v := <-ch1:
case v := <-ch2:
default:
}

// 5. defer（遅延実行）
defer file.Close()
defer func() { recover() }()

// 6. 複数戻り値
func divide(a, b int) (int, error) {
    return a/b, nil
}

// 7. レシーバー付きメソッド
func (u *User) GetName() string {
    return u.name
}
```

## 📊 解析項目設計

### 基本解析
- [x] パッケージ名検出
- [x] インポート解析
- [x] 関数検出（引数・戻り値）
- [x] 構造体検出
- [x] インターフェース検出
- [x] 変数・定数検出

### Go特化解析
- [x] Goroutine検出（`go` キーワード）
- [x] Channel操作検出（`make(chan`, `<-`）
- [x] Select文検出
- [x] Defer文検出
- [x] 複数戻り値検出
- [x] メソッドレシーバー検出

### 複雑度解析
- [x] Cyclomatic complexity
- [x] Goroutine数カウント
- [x] Channel使用頻度

## 🛠️ 実装戦略

### PEGTL文法設計
```cpp
// Go特有のキーワード
struct go_keyword : pegtl::string<'g','o'> {};
struct chan_keyword : pegtl::string<'c','h','a','n'> {};
struct select_keyword : pegtl::string<'s','e','l','e','c','t'> {};
struct defer_keyword : pegtl::string<'d','e','f','e','r'> {};

// Goroutine検出
struct goroutine : pegtl::seq<
    go_keyword,
    pegtl::plus<pegtl::space>,
    // 関数呼び出しまたは関数リテラル
> {};

// Channel操作
struct channel_send : pegtl::seq<
    identifier,
    pegtl::star<pegtl::space>,
    pegtl::string<'<','-'>
> {};
```

## 🎮 実装手順

1. **types.hpp** にGo列挙値追加
2. **GoAnalyzer** クラス作成
3. **PEGTL文法** 実装
4. **AnalyzerFactory** に登録
5. **テストファイル** 作成
6. **実戦テスト**（Docker、Kubernetesプロジェクト）

## 🧪 テスト計画

### テストプロジェクト候補
- Docker CE（大規模Goプロジェクト）
- Kubernetes（超大規模）
- 自作小規模Goプロジェクト

### 検証項目
- Goroutine検出精度
- Channel解析精度
- パフォーマンス（他言語と比較）
- 複雑度計算の妥当性

## 🎯 期待される成果

- Go開発者に大歓迎される
- Docker/K8s解析で実用性証明
- Claude Codeの対応言語拡大
- DevOps分野での採用促進

---
**実装時間予想**: 2-3時間
**難易度**: 中（TypeScriptより簡単、C++より易しい）
**面白さ**: 🌟🌟🌟🌟🌟（Goroutine解析が楽しそう！）