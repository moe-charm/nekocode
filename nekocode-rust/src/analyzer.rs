//=============================================================================
// 🦀 NekoCode Rust - Core Analyzer Trait
//
// Equivalent to BaseAnalyzer.hpp from C++ version
// Provides the base interface for all language analyzers
//=============================================================================

use crate::types::{AnalysisResult, Language, ComplexityInfo, LineNumber};
use anyhow::Result;

//=============================================================================
// 🎯 LanguageAnalyzer Trait - Base Interface for All Language Analyzers
//=============================================================================

/// Base trait that all language analyzers must implement
/// This is the Rust equivalent of the C++ BaseAnalyzer class
pub trait LanguageAnalyzer: Send + Sync {
    /// Return the language type this analyzer handles
    fn get_language(&self) -> Language;

    /// Main analysis function - analyzes content and returns structured results
    fn analyze(&self, content: &str, filename: &str) -> Result<AnalysisResult>;

    /// Return human-readable language name for display
    fn get_language_name(&self) -> &'static str;

    /// Return supported file extensions for this language
    fn get_supported_extensions(&self) -> Vec<&'static str>;

    /// Calculate line number from position in content (common functionality)
    fn calculate_line_number(&self, content: &str, position: usize) -> LineNumber {
        if position >= content.len() {
            return 1;
        }
        
        let mut line_count = 1;
        for (i, ch) in content.char_indices() {
            if i >= position {
                break;
            }
            if ch == '\n' {
                line_count += 1;
            }
        }
        line_count
    }

    /// Calculate basic complexity (language-agnostic version)
    /// Note: Rust version uses controlled regex where the C++ version explicitly avoided it
    fn calculate_complexity(&self, content: &str) -> ComplexityInfo {
        let mut complexity = ComplexityInfo::new();
        
        // Language-common complexity keywords
        let complexity_keywords = [
            "if", "else", "for", "while", "switch", "case", "catch"
        ];
        
        // Simple word boundary-aware search (avoiding regex for now)
        for keyword in &complexity_keywords {
            let mut pos = 0;
            while let Some(found) = content[pos..].find(keyword) {
                let absolute_pos = pos + found;
                
                // Simple word boundary check
                let is_word_start = absolute_pos == 0 || 
                    !content.chars().nth(absolute_pos - 1).unwrap_or(' ').is_alphanumeric();
                let keyword_end = absolute_pos + keyword.len();
                let is_word_end = keyword_end >= content.len() || 
                    !content.chars().nth(keyword_end).unwrap_or(' ').is_alphanumeric();
                
                if is_word_start && is_word_end {
                    complexity.cyclomatic_complexity += 1;
                }
                
                pos = absolute_pos + keyword.len();
            }
        }
        
        complexity.update_rating();
        complexity
    }

    /// Extract next word from content starting at position (string processing helper)
    fn extract_next_word(&self, content: &str, pos: &mut usize) -> String {
        // Skip whitespace
        while *pos < content.len() && content.chars().nth(*pos).unwrap_or(' ').is_whitespace() {
            *pos += 1;
        }
        
        let start = *pos;
        while *pos < content.len() {
            let ch = content.chars().nth(*pos).unwrap_or(' ');
            if ch.is_alphanumeric() || ch == '_' {
                *pos += 1;
            } else {
                break;
            }
        }
        
        content[start..*pos].to_string()
    }

    /// Skip until target character (string processing helper)
    fn skip_until(&self, content: &str, pos: &mut usize, target: char) {
        while *pos < content.len() && content.chars().nth(*pos).unwrap_or('\0') != target {
            *pos += 1;
        }
    }

    /// Skip string literal (string processing helper)
    fn skip_string_literal(&self, content: &str, pos: &mut usize, quote: char) {
        if *pos < content.len() && content.chars().nth(*pos).unwrap_or('\0') == quote {
            *pos += 1; // Opening quote
            
            while *pos < content.len() {
                let ch = content.chars().nth(*pos).unwrap_or('\0');
                if ch == '\\' && *pos + 1 < content.len() {
                    *pos += 2; // Escape sequence
                } else if ch == quote {
                    *pos += 1; // Closing quote
                    break;
                } else {
                    *pos += 1;
                }
            }
        }
    }
}

//=============================================================================
// 🏭 AnalyzerFactory - Factory for Creating Language Analyzers
//=============================================================================

/// Factory for creating appropriate language analyzers
pub struct AnalyzerFactory;

impl AnalyzerFactory {
    /// Create analyzer based on language enum
    pub fn create_analyzer(language: Language) -> Result<Box<dyn LanguageAnalyzer>> {
        match language {
            Language::JavaScript => {
                let analyzer = crate::analyzers::javascript::JavaScriptAnalyzer::new();
                Ok(Box::new(analyzer))
            }
            Language::TypeScript => {
                let analyzer = crate::analyzers::javascript::JavaScriptAnalyzer::new(); // TODO: Separate TS analyzer
                Ok(Box::new(analyzer))
            }
            Language::Python => {
                let analyzer = crate::analyzers::python::PythonAnalyzer::new();
                Ok(Box::new(analyzer))
            }
            Language::Cpp => {
                let analyzer = crate::analyzers::cpp::CppAnalyzer::new();
                Ok(Box::new(analyzer))
            }
            Language::C => {
                let analyzer = crate::analyzers::cpp::CppAnalyzer::new(); // TODO: Separate C analyzer
                Ok(Box::new(analyzer))
            }
            Language::CSharp => {
                let analyzer = crate::analyzers::csharp::CsharpAnalyzer::new();
                Ok(Box::new(analyzer))
            }
            Language::Go => {
                let analyzer = crate::analyzers::go::GoAnalyzer::new();
                Ok(Box::new(analyzer))
            }
            Language::Rust => {
                let analyzer = crate::analyzers::rust_lang::RustAnalyzer::new();
                Ok(Box::new(analyzer))
            }
            Language::Unknown => {
                anyhow::bail!("Cannot create analyzer for unknown language")
            }
        }
    }

    /// Create analyzer from file extension
    pub fn create_analyzer_from_extension(extension: &str) -> Result<Box<dyn LanguageAnalyzer>> {
        let language = Language::from_extension(extension);
        Self::create_analyzer(language)
    }

    /// Get file extension from filename
    pub fn get_extension(filename: &str) -> Option<&str> {
        std::path::Path::new(filename)
            .extension()
            .and_then(|ext| ext.to_str())
    }
}

//=============================================================================
// 🔧 Utility Functions for Analyzers
//=============================================================================

/// Common utility functions that can be used by all analyzers
pub mod analyzer_utils {
    /// Count lines in content
    pub fn count_lines(content: &str) -> (u32, u32, u32) {
        let mut total_lines = 0;
        let mut code_lines = 0;
        let mut comment_lines = 0;

        for line in content.lines() {
            total_lines += 1;
            let trimmed = line.trim();
            
            if trimmed.is_empty() {
                // Empty line - don't count as code or comment
                continue;
            } else if trimmed.starts_with("//") || trimmed.starts_with("#") || trimmed.starts_with("--") {
                comment_lines += 1;
            } else {
                code_lines += 1;
            }
        }

        (total_lines, code_lines, comment_lines)
    }

    /// Extract identifier from position in content
    pub fn extract_identifier_at_position(content: &str, position: usize) -> Option<String> {
        let chars: Vec<char> = content.chars().collect();
        if position >= chars.len() {
            return None;
        }

        // Find start of identifier
        let mut start = position;
        while start > 0 && (chars[start - 1].is_alphanumeric() || chars[start - 1] == '_') {
            start -= 1;
        }

        // Find end of identifier
        let mut end = position;
        while end < chars.len() && (chars[end].is_alphanumeric() || chars[end] == '_') {
            end += 1;
        }

        if start < end {
            Some(chars[start..end].iter().collect())
        } else {
            None
        }
    }

    /// Simple string-based pattern matching (alternative to regex)
    pub fn find_pattern_boundaries(content: &str, pattern: &str) -> Vec<(usize, usize)> {
        let mut matches = Vec::new();
        let mut start = 0;

        while let Some(pos) = content[start..].find(pattern) {
            let absolute_pos = start + pos;
            
            // Check word boundaries
            let is_word_start = absolute_pos == 0 || 
                !content.chars().nth(absolute_pos - 1).unwrap_or(' ').is_alphanumeric();
            let pattern_end = absolute_pos + pattern.len();
            let is_word_end = pattern_end >= content.len() || 
                !content.chars().nth(pattern_end).unwrap_or(' ').is_alphanumeric();
            
            if is_word_start && is_word_end {
                matches.push((absolute_pos, pattern_end));
            }
            
            start = absolute_pos + 1;
        }

        matches
    }
}