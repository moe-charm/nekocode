//=============================================================================
// 🎮 Unity Analyzer 実装 - コンポジション設計版
//
// 機能部品を組み合わせた Unity 特化解析エンジン
//=============================================================================

#include "nekocode/analyzers/unity_analyzer.hpp"
#include <iostream>

namespace nekocode {

//=============================================================================
// 🏭 Unity Analyzer ファクトリ関数
//=============================================================================

std::unique_ptr<BaseAnalyzer> create_unity_analyzer() {
    std::cout << "🎮 Creating Unity Analyzer (Composition Design)" << std::endl;
    return std::make_unique<UnityAnalyzer>();
}

} // namespace nekocode