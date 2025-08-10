//=============================================================================
// 🦀 NekoCode Rust - Rust Language Analyzer
//=============================================================================

use crate::analyzer::LanguageAnalyzer;
use crate::types::{AnalysisResult, Language, FileInfo};
use anyhow::Result;

pub struct RustAnalyzer;

impl RustAnalyzer {
    pub fn new() -> Self { Self }
}

impl LanguageAnalyzer for RustAnalyzer {
    fn get_language(&self) -> Language { Language::Rust }
    fn get_language_name(&self) -> &'static str { "Rust" }
    fn get_supported_extensions(&self) -> Vec<&'static str> { vec!["rs"] }
    
    fn analyze(&self, content: &str, filename: &str) -> Result<AnalysisResult> {
        let mut result = AnalysisResult::new(Language::Rust);
        result.file_info = FileInfo::new(filename.to_string());
        let (total_lines, code_lines, comment_lines) = crate::analyzer::analyzer_utils::count_lines(content);
        result.file_info.total_lines = total_lines;
        result.file_info.code_lines = code_lines;
        result.file_info.comment_lines = comment_lines;
        result.file_info.size_bytes = content.len() as u64;
        result.complexity = self.calculate_complexity(content);
        result.update_statistics();
        Ok(result)
    }
}