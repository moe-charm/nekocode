//=============================================================================
// 🔥 PEGTL Analyzer - リファクタリング版
//
// 各言語専用アナライザーを統合するファサード
//=============================================================================

#include "nekocode/pegtl_analyzer.hpp"
#include "nekocode/analyzers/base_analyzer.hpp"
#include "nekocode/utf8_utils.hpp"
#include <chrono>

namespace nekocode {

//=============================================================================
// 🧠 PEGTLAnalyzer::Impl - シンプル化された実装
//=============================================================================

class PEGTLAnalyzer::Impl {
public:
    ParseMetrics last_metrics_;
};

//=============================================================================
// 🌟 PEGTLAnalyzer実装
//=============================================================================

PEGTLAnalyzer::PEGTLAnalyzer() 
    : impl_(std::make_unique<Impl>()) {
}

PEGTLAnalyzer::~PEGTLAnalyzer() = default;

PEGTLAnalyzer::PEGTLAnalyzer(PEGTLAnalyzer&&) noexcept = default;
PEGTLAnalyzer& PEGTLAnalyzer::operator=(PEGTLAnalyzer&&) noexcept = default;

//=============================================================================
// 🚀 革命的解析API実装 - ファクトリーパターン使用
//=============================================================================

Result<AnalysisResult> PEGTLAnalyzer::analyze(const std::string& content, 
                                              const std::string& filename,
                                              Language language) {
    try {
        auto start_time = std::chrono::steady_clock::now();
        
        // 言語自動検出
        if (language == Language::UNKNOWN) {
            language = detect_language_from_content(content, filename);
        }
        
        // 適切なアナライザーを生成
        auto analyzer = AnalyzerFactory::create_analyzer(language);
        if (!analyzer) {
            return Result<AnalysisResult>(
                AnalysisError(ErrorCode::UNKNOWN_ERROR, "Unsupported language")
            );
        }
        
        // 解析実行
        AnalysisResult result = analyzer->analyze(content, filename);
        
        // メトリクス更新
        auto end_time = std::chrono::steady_clock::now();
        impl_->last_metrics_.parse_time = 
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        impl_->last_metrics_.bytes_processed = static_cast<uint32_t>(content.length());
        impl_->last_metrics_.has_errors = false;
        impl_->last_metrics_.nodes_parsed = result.classes.size() + result.functions.size();
        
        return Result<AnalysisResult>(std::move(result));
        
    } catch (const std::exception& e) {
        return Result<AnalysisResult>(AnalysisError(ErrorCode::PARSING_ERROR, e.what()));
    }
}

//=============================================================================
// 🎯 言語検出
//=============================================================================

Language PEGTLAnalyzer::detect_language_from_content(const std::string& content, 
                                                     const std::string& filename) {
    // 拡張子ベース検出
    auto has_suffix = [](const std::string& str, const std::string& suffix) {
        return str.length() >= suffix.length() &&
               str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    };
    
    // C++
    if (has_suffix(filename, ".cpp") || has_suffix(filename, ".hpp") || 
        has_suffix(filename, ".cc") || has_suffix(filename, ".cxx") || 
        has_suffix(filename, ".hxx")) {
        return Language::CPP;
    }
    
    // JavaScript
    if (has_suffix(filename, ".js") || has_suffix(filename, ".mjs") || 
        has_suffix(filename, ".jsx")) {
        return Language::JAVASCRIPT;
    }
    
    // TypeScript
    if (has_suffix(filename, ".ts") || has_suffix(filename, ".tsx")) {
        return Language::TYPESCRIPT;
    }
    
    // Python
    if (has_suffix(filename, ".py") || has_suffix(filename, ".pyw") || 
        has_suffix(filename, ".pyi")) {
        return Language::PYTHON;
    }
    
    // C#
    if (has_suffix(filename, ".cs") || has_suffix(filename, ".csx")) {
        return Language::CSHARP;
    }
    
    // C
    if (has_suffix(filename, ".c")) {
        return Language::C;
    }
    
    // コンテンツベース検出
    if (content.find("#include") != std::string::npos || 
        content.find("namespace") != std::string::npos) {
        return Language::CPP;
    }
    
    if (content.find("import ") != std::string::npos || 
        content.find("export ") != std::string::npos) {
        if (content.find("interface ") != std::string::npos || 
            content.find(": string") != std::string::npos) {
            return Language::TYPESCRIPT;
        }
        return Language::JAVASCRIPT;
    }
    
    if (content.find("def ") != std::string::npos || 
        content.find("class ") != std::string::npos ||
        content.find("import ") != std::string::npos) {
        return Language::PYTHON;
    }
    
    if (content.find("using System") != std::string::npos ||
        content.find("namespace ") != std::string::npos ||
        content.find("public class ") != std::string::npos ||
        content.find("{ get; set; }") != std::string::npos) {
        return Language::CSHARP;
    }
    
    return Language::UNKNOWN;
}

//=============================================================================
// 🧮 統計専用解析
//=============================================================================

Result<AnalysisResult> PEGTLAnalyzer::analyze_statistics_only(const std::string& content,
                                                              const std::string& filename,
                                                              Language language) {
    // 現在は通常の解析と同じ（将来的に最適化可能）
    return analyze(content, filename, language);
}

const PEGTLAnalyzer::ParseMetrics& PEGTLAnalyzer::get_last_parse_metrics() const {
    return impl_->last_metrics_;
}

//=============================================================================
// 🎯 個別言語解析メソッド（互換性のため残す）
//=============================================================================

Result<AnalysisResult> PEGTLAnalyzer::analyze_cpp(const std::string& content, 
                                                   const std::string& filename) {
    return analyze(content, filename, Language::CPP);
}

Result<AnalysisResult> PEGTLAnalyzer::analyze_javascript(const std::string& content, 
                                                          const std::string& filename) {
    return analyze(content, filename, Language::JAVASCRIPT);
}

Result<AnalysisResult> PEGTLAnalyzer::analyze_typescript(const std::string& content, 
                                                          const std::string& filename) {
    return analyze(content, filename, Language::TYPESCRIPT);
}

//=============================================================================
// 以下、互換性のための空実装
//=============================================================================

AnalysisResult PEGTLAnalyzer::extract_cpp_elements(const std::string& content) {
    auto analyzer = AnalyzerFactory::create_analyzer(Language::CPP);
    return analyzer ? analyzer->analyze(content, "temp.cpp") : AnalysisResult{};
}

AnalysisResult PEGTLAnalyzer::extract_javascript_elements(const std::string& content) {
    auto analyzer = AnalyzerFactory::create_analyzer(Language::JAVASCRIPT);
    return analyzer ? analyzer->analyze(content, "temp.js") : AnalysisResult{};
}

AnalysisResult PEGTLAnalyzer::extract_typescript_elements(const std::string& content) {
    auto analyzer = AnalyzerFactory::create_analyzer(Language::TYPESCRIPT);
    return analyzer ? analyzer->analyze(content, "temp.ts") : AnalysisResult{};
}

AnalysisResult PEGTLAnalyzer::extract_python_elements(const std::string& content) {
    auto analyzer = AnalyzerFactory::create_analyzer(Language::PYTHON);
    return analyzer ? analyzer->analyze(content, "temp.py") : AnalysisResult{};
}

ComplexityInfo PEGTLAnalyzer::calculate_complexity(const std::string& content, 
                                                   Language language) {
    auto analyzer = AnalyzerFactory::create_analyzer(language);
    if (analyzer) {
        auto result = analyzer->analyze(content, "temp");
        return result.complexity;
    }
    return ComplexityInfo{};
}

void PEGTLAnalyzer::extract_js_imports_regex(const std::string& content, 
                                             std::vector<ImportInfo>& imports) {
    // JavaScriptAnalyzerに委譲
    auto analyzer = AnalyzerFactory::create_analyzer(Language::JAVASCRIPT);
    if (analyzer) {
        auto result = analyzer->analyze(content, "temp.js");
        imports = result.imports;
    }
}

uint32_t PEGTLAnalyzer::calculate_line_number(const std::string& content, size_t position) {
    if (position >= content.length()) {
        return 1;
    }
    
    uint32_t line_count = 1;
    for (size_t i = 0; i < position; ++i) {
        if (content[i] == '\n') {
            line_count++;
        }
    }
    return line_count;
}

//=============================================================================
// 🎯 PEGTL統合ヘルパー実装
//=============================================================================

namespace pegtl_helper {

AnalysisResult convert_to_analysis_result(const std::string& content, 
                                          const std::string& filename,
                                          Language language) {
    // 未使用
    AnalysisResult result;
    result.file_info.name = filename;
    result.language = language;
    return result;
}

VersionInfo get_version_info() {
    return VersionInfo{};
}

} // namespace pegtl_helper

} // namespace nekocode