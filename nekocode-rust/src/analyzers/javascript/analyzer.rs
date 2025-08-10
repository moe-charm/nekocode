//=============================================================================
// 🦀 NekoCode Rust - JavaScript Analyzer
//
// JavaScript/TypeScript language analyzer implementation
// Uses Pest parser as equivalent to PEGTL in C++ version
//=============================================================================

use crate::analyzer::LanguageAnalyzer;
use crate::types::{
    AnalysisResult, Language, FileInfo, ClassInfo, FunctionInfo
};
use anyhow::Result;

pub struct JavaScriptAnalyzer {
    // Configuration or state can be added here if needed
}

impl JavaScriptAnalyzer {
    pub fn new() -> Self {
        Self {}
    }
}

impl LanguageAnalyzer for JavaScriptAnalyzer {
    fn get_language(&self) -> Language {
        Language::JavaScript
    }

    fn get_language_name(&self) -> &'static str {
        "JavaScript"
    }

    fn get_supported_extensions(&self) -> Vec<&'static str> {
        vec!["js", "jsx", "mjs"]
    }

    fn analyze(&self, content: &str, filename: &str) -> Result<AnalysisResult> {
        let mut result = AnalysisResult::new(Language::JavaScript);
        
        // Set up file info
        result.file_info = FileInfo::new(filename.to_string());
        let (total_lines, code_lines, comment_lines) = crate::analyzer::analyzer_utils::count_lines(content);
        result.file_info.total_lines = total_lines;
        result.file_info.code_lines = code_lines;
        result.file_info.comment_lines = comment_lines;
        result.file_info.size_bytes = content.len() as u64;

        // Basic complexity analysis
        result.complexity = self.calculate_complexity(content);

        // Simple JavaScript parsing (placeholder implementation)
        self.parse_javascript_content(content, &mut result)?;

        // Update statistics
        result.update_statistics();

        Ok(result)
    }
}

impl JavaScriptAnalyzer {
    /// Parse JavaScript content for classes, functions, etc.
    /// This is a simplified implementation - in a full version, this would use Pest grammar
    fn parse_javascript_content(&self, content: &str, result: &mut AnalysisResult) -> Result<()> {
        let lines: Vec<&str> = content.lines().collect();
        
        for (line_num, line) in lines.iter().enumerate() {
            let line_number = (line_num + 1) as u32;
            let trimmed = line.trim();
            
            // Simple class detection
            if let Some(class_name) = self.extract_class_declaration(trimmed) {
                let mut class_info = ClassInfo::new(class_name);
                class_info.start_line = line_number;
                // TODO: Find end line by parsing the class body
                class_info.end_line = line_number;
                result.classes.push(class_info);
            }
            
            // Simple function detection
            if let Some(function_name) = self.extract_function_declaration(trimmed) {
                let mut function_info = FunctionInfo::new(function_name);
                function_info.start_line = line_number;
                function_info.end_line = line_number;
                
                // Check if it's an arrow function
                if trimmed.contains("=>") {
                    function_info.is_arrow_function = true;
                }
                
                // Check if it's async
                if trimmed.contains("async") {
                    function_info.is_async = true;
                }
                
                result.functions.push(function_info);
            }
        }
        
        Ok(())
    }

    /// Extract class name from class declaration line
    fn extract_class_declaration(&self, line: &str) -> Option<String> {
        if line.starts_with("class ") {
            let parts: Vec<&str> = line.split_whitespace().collect();
            if parts.len() >= 2 {
                let class_name = parts[1].trim_end_matches('{').trim();
                if !class_name.is_empty() {
                    return Some(class_name.to_string());
                }
            }
        }
        None
    }

    /// Extract function name from function declaration line
    fn extract_function_declaration(&self, line: &str) -> Option<String> {
        // Function declaration: function name(...)
        if line.starts_with("function ") {
            let parts: Vec<&str> = line.split_whitespace().collect();
            if parts.len() >= 2 {
                let func_name = parts[1].split('(').next().unwrap_or("").trim();
                if !func_name.is_empty() {
                    return Some(func_name.to_string());
                }
            }
        }
        
        // Arrow function: const name = (...) =>
        if line.contains("=>") {
            if let Some(eq_pos) = line.find('=') {
                let before_eq = &line[..eq_pos];
                if let Some(name) = before_eq.split_whitespace().last() {
                    return Some(name.to_string());
                }
            }
        }
        
        // Method in class: methodName(...) {
        if line.contains('(') && line.contains(')') && line.trim_end().ends_with('{') {
            if let Some(paren_pos) = line.find('(') {
                let before_paren = &line[..paren_pos];
                if let Some(name) = before_paren.split_whitespace().last() {
                    // Skip if this looks like a control structure
                    if !["if", "while", "for", "switch", "catch"].contains(&name) {
                        return Some(name.to_string());
                    }
                }
            }
        }
        
        None
    }
}