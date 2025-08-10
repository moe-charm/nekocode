//=============================================================================
// 🦀 NekoCode Rust - Python Analyzer
//
// Python language analyzer implementation
//=============================================================================

use crate::analyzer::LanguageAnalyzer;
use crate::types::{AnalysisResult, Language, FileInfo, ClassInfo, FunctionInfo};
use anyhow::Result;

pub struct PythonAnalyzer;

impl PythonAnalyzer {
    pub fn new() -> Self {
        Self
    }
}

impl LanguageAnalyzer for PythonAnalyzer {
    fn get_language(&self) -> Language {
        Language::Python
    }

    fn get_language_name(&self) -> &'static str {
        "Python"
    }

    fn get_supported_extensions(&self) -> Vec<&'static str> {
        vec!["py", "pyw"]
    }

    fn analyze(&self, content: &str, filename: &str) -> Result<AnalysisResult> {
        let mut result = AnalysisResult::new(Language::Python);
        
        // Set up file info
        result.file_info = FileInfo::new(filename.to_string());
        let (total_lines, code_lines, comment_lines) = crate::analyzer::analyzer_utils::count_lines(content);
        result.file_info.total_lines = total_lines;
        result.file_info.code_lines = code_lines;
        result.file_info.comment_lines = comment_lines;
        result.file_info.size_bytes = content.len() as u64;

        // Basic complexity analysis
        result.complexity = self.calculate_complexity(content);

        // Simple Python parsing (placeholder implementation)
        self.parse_python_content(content, &mut result)?;

        // Update statistics
        result.update_statistics();

        Ok(result)
    }
}

impl PythonAnalyzer {
    fn parse_python_content(&self, content: &str, result: &mut AnalysisResult) -> Result<()> {
        let lines: Vec<&str> = content.lines().collect();
        
        for (line_num, line) in lines.iter().enumerate() {
            let line_number = (line_num + 1) as u32;
            let trimmed = line.trim();
            
            // Simple class detection: class ClassName:
            if trimmed.starts_with("class ") && trimmed.ends_with(':') {
                if let Some(class_name) = self.extract_python_class_name(trimmed) {
                    let mut class_info = ClassInfo::new(class_name);
                    class_info.start_line = line_number;
                    class_info.end_line = line_number; // TODO: Find actual end
                    result.classes.push(class_info);
                }
            }
            
            // Simple function detection: def function_name():
            if trimmed.starts_with("def ") && trimmed.contains('(') {
                if let Some(function_name) = self.extract_python_function_name(trimmed) {
                    let mut function_info = FunctionInfo::new(function_name);
                    function_info.start_line = line_number;
                    function_info.end_line = line_number; // TODO: Find actual end
                    
                    // Check if it's async
                    if trimmed.starts_with("async def ") {
                        function_info.is_async = true;
                    }
                    
                    result.functions.push(function_info);
                }
            }
        }
        
        Ok(())
    }

    fn extract_python_class_name(&self, line: &str) -> Option<String> {
        // line format: "class ClassName:" or "class ClassName(BaseClass):"
        let without_class = line.strip_prefix("class ")?.trim();
        let class_part = without_class.strip_suffix(':')?.trim();
        
        if let Some(paren_pos) = class_part.find('(') {
            // Has inheritance
            Some(class_part[..paren_pos].trim().to_string())
        } else {
            // No inheritance
            Some(class_part.to_string())
        }
    }

    fn extract_python_function_name(&self, line: &str) -> Option<String> {
        // line format: "def function_name(...):" or "async def function_name(...):"
        let without_async = if line.trim().starts_with("async def ") {
            line.trim().strip_prefix("async def ")?
        } else {
            line.trim().strip_prefix("def ")?
        };
        
        let paren_pos = without_async.find('(')?;
        Some(without_async[..paren_pos].trim().to_string())
    }
}