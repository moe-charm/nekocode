#pragma once

//=============================================================================
// 🎮 Unity Analyzer - Unity 特化解析エンジン（コンポジション設計）
//
// C# アナライザーを継承し、機能部品を組み合わせて Unity 解析を実現
// 単一責任原則に従い、テストしやすく、保守しやすい設計
//=============================================================================

#include "csharp_pegtl_analyzer.hpp"
#include "unity_components.hpp"
#include <memory>

namespace nekocode {

//=============================================================================
// 🎮 UnityAnalyzer - コンポジション設計版
//=============================================================================

class UnityAnalyzer : public CSharpPEGTLAnalyzer {
private:
    // 🧩 機能部品（コンポジション）
    unity::UnityPatternDetector unity_detector;
    unity::PerformanceWarningDetector perf_detector;
    unity::LifecycleMethodClassifier lifecycle_classifier;
    
public:
    UnityAnalyzer() = default;
    virtual ~UnityAnalyzer() = default;
    
    std::string get_language_name() const override {
        return "Unity/C# (Composition)";
    }
    
    AnalysisResult analyze(const std::string& content, const std::string& filename) override {
        std::cout << "🎮 UnityAnalyzer (Composition) analyzing: " << filename << std::endl;
        
        // 1. 基本的な C# 解析を実行
        auto result = CSharpPEGTLAnalyzer::analyze(content, filename);
        
        // 2. Unity 機能を順次適用（明確な処理順序）
        std::cout << "  🎯 Applying Unity pattern detection..." << std::endl;
        unity_detector.enhance_analysis(result, content);
        
        std::cout << "  ⚠️  Checking performance issues..." << std::endl;
        perf_detector.add_warnings(result, content);
        
        std::cout << "  🔄 Classifying lifecycle methods..." << std::endl;
        lifecycle_classifier.classify_methods(result);
        
        // 3. Unity 解析完了の統計更新
        update_unity_analysis_metadata(result);
        
        std::cout << "  ✅ Unity analysis completed!" << std::endl;
        return result;
    }
    
private:
    // Unity 解析のメタデータ更新
    void update_unity_analysis_metadata(AnalysisResult& result) {
        // 解析バージョン情報
        result.metadata["unity_analyzer_version"] = "2.0_composition";
        result.metadata["analysis_components"] = "pattern_detector,performance_warnings,lifecycle_classifier";
        
        // 処理済みフラグ
        result.metadata["unity_analysis_completed"] = "true";
        
        // 統計情報を再計算
        result.update_statistics();
    }
};

} // namespace nekocode