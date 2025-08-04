#pragma once

#include "types.hpp"
#include <memory>
#include <string>
#include <sstream>
#include <nlohmann/json.hpp>

namespace nekocode {

//=============================================================================
// 📊 IReportFormatter - レポートフォーマッター基底インターフェース
//=============================================================================

class IReportFormatter {
public:
    virtual ~IReportFormatter() = default;
    
    virtual std::string format_single_file(const AnalysisResult& result) = 0;
    virtual std::string format_directory(const DirectoryAnalysis& analysis) = 0;
    virtual std::string format_summary(const DirectoryAnalysis::Summary& summary) = 0;
};

//=============================================================================
// 🤖 AIReportFormatter - AI用フォーマッター
//=============================================================================

class AIReportFormatter : public IReportFormatter {
public:
    AIReportFormatter();
    
    std::string format_single_file(const AnalysisResult& result) override;
    std::string format_directory(const DirectoryAnalysis& analysis) override;
    std::string format_summary(const DirectoryAnalysis::Summary& summary) override;
};

//=============================================================================
// 👨‍💻 HumanReportFormatter - Human用フォーマッター
//=============================================================================

class HumanReportFormatter : public IReportFormatter {
public:
    HumanReportFormatter();
    
    std::string format_single_file(const AnalysisResult& result) override;
    std::string format_directory(const DirectoryAnalysis& analysis) override;
    std::string format_summary(const DirectoryAnalysis::Summary& summary) override;
};

//=============================================================================
// 🏭 FormatterFactory - フォーマッター作成
//=============================================================================

class FormatterFactory {
public:
    static std::unique_ptr<IReportFormatter> create_formatter(OutputFormat format);
};

} // namespace nekocode