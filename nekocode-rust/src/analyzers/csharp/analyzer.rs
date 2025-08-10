//! C# analyzer implementation using regex-based parsing
//! 
//! This module provides C# analysis capabilities,
//! detecting classes, methods, interfaces, properties, and complexity metrics.

use anyhow::Result;
use async_trait::async_trait;

use crate::core::types::{
    AnalysisResult, ClassInfo, ComplexityInfo, FileInfo, FunctionInfo, ImportInfo, 
    ImportType, Language
};
use crate::analyzers::traits::LanguageAnalyzer;

/// C# analyzer
pub struct CSharpAnalyzer {
    // Internal state for parsing
}

impl CSharpAnalyzer {
    pub fn new() -> Self {
        Self {}
    }
    
    /// Calculate complexity metrics for C#
    fn calculate_complexity(&self, content: &str) -> ComplexityInfo {
        let mut complexity = ComplexityInfo::new();
        
        // Count complexity-increasing constructs specific to C#
        let complexity_keywords = [
            "if (", "else if", "else {", "for (", "foreach (", "while (", "do {",
            "switch (", "case ", "catch (", "&&", "||", "? ", "??",
            "async ", "await ", "yield ", "try {", "throw "
        ];
        
        for keyword in &complexity_keywords {
            let count = content.matches(keyword).count() as u32;
            complexity.cyclomatic_complexity += count;
        }
        
        // Count LINQ expressions which add complexity
        let linq_keywords = ["from ", "where ", "select ", "group ", "join ", "orderby "];
        for keyword in &linq_keywords {
            let count = content.matches(keyword).count() as u32;
            complexity.cyclomatic_complexity += count / 2; // LINQ is less complex per keyword
        }
        
        // Calculate nesting depth using braces
        let mut current_depth = 0;
        let mut max_depth = 0;
        
        for ch in content.chars() {
            match ch {
                '{' => {
                    current_depth += 1;
                    max_depth = max_depth.max(current_depth);
                }
                '}' => {
                    if current_depth > 0 {
                        current_depth -= 1;
                    }
                }
                _ => {}
            }
        }
        
        complexity.max_nesting_depth = max_depth;
        complexity.update_rating();
        
        complexity
    }
}

#[async_trait]
impl LanguageAnalyzer for CSharpAnalyzer {
    fn get_language(&self) -> Language {
        Language::CSharp
    }
    
    fn get_language_name(&self) -> &'static str {
        "C#"
    }
    
    fn get_supported_extensions(&self) -> Vec<&'static str> {
        vec![".cs"]
    }
    
    async fn analyze(&mut self, content: &str, filename: &str) -> Result<AnalysisResult> {
        // Create file info
        let file_path = std::path::PathBuf::from(filename);
        let mut file_info = FileInfo::new(file_path);
        file_info.total_lines = content.lines().count() as u32;
        
        // Calculate basic line statistics
        for line in content.lines() {
            let trimmed = line.trim();
            if trimmed.is_empty() {
                file_info.empty_lines += 1;
            } else if trimmed.starts_with("//") || trimmed.starts_with("/*") {
                file_info.comment_lines += 1;
            } else {
                file_info.code_lines += 1;
            }
        }
        
        file_info.code_ratio = if file_info.total_lines > 0 {
            file_info.code_lines as f64 / file_info.total_lines as f64
        } else {
            0.0
        };
        
        // Create analysis result
        let mut result = AnalysisResult::new(file_info, Language::CSharp);
        
        // Use regex-based parsing for now (simpler and more reliable)
        result.functions = self.regex_fallback_functions(content);
        result.classes = self.regex_fallback_classes(content);
        result.imports = self.regex_fallback_imports(content);
        
        // Calculate complexity
        result.complexity = self.calculate_complexity(content);
        
        // Update statistics
        result.update_statistics();
        
        Ok(result)
    }
}

impl CSharpAnalyzer {
    /// Fallback function extraction using regex
    fn regex_fallback_functions(&self, content: &str) -> Vec<FunctionInfo> {
        let mut functions = Vec::new();
        
        // Pattern 1: Method definitions
        if let Ok(re) = regex::Regex::new(r"(?m)^\s*(?:public|private|protected|internal)?\s*(?:static|virtual|override|abstract|sealed|async)?\s*\w+(?:<[^>]*>)?\s+(\w+)\s*\([^)]*\)\s*(?:\{|;)") {
            for caps in re.captures_iter(content) {
                if let Some(name) = caps.get(1) {
                    let mut func = FunctionInfo::new(name.as_str().to_string());
                    let full_match = caps.get(0).unwrap().as_str();
                    
                    if full_match.contains("async") {
                        func.is_async = true;
                    }
                    if full_match.contains("static") {
                        func.metadata.insert("is_static".to_string(), "true".to_string());
                    }
                    if full_match.contains("virtual") {
                        func.metadata.insert("is_virtual".to_string(), "true".to_string());
                    }
                    if full_match.contains("override") {
                        func.metadata.insert("is_override".to_string(), "true".to_string());
                    }
                    if full_match.contains("abstract") {
                        func.metadata.insert("is_abstract".to_string(), "true".to_string());
                    }
                    
                    functions.push(func);
                }
            }
        }
        
        // Pattern 2: Property definitions
        if let Ok(re) = regex::Regex::new(r"(?m)^\s*(?:public|private|protected|internal)?\s*(?:static|virtual|override|abstract)?\s*\w+(?:<[^>]*>)?\s+(\w+)\s*\{\s*get\s*;?\s*(?:set\s*;?)?\s*\}") {
            for caps in re.captures_iter(content) {
                if let Some(name) = caps.get(1) {
                    let mut prop = FunctionInfo::new(name.as_str().to_string());
                    prop.metadata.insert("is_property".to_string(), "true".to_string());
                    functions.push(prop);
                }
            }
        }
        
        functions
    }
    
    /// Fallback class extraction using regex
    fn regex_fallback_classes(&self, content: &str) -> Vec<ClassInfo> {
        let mut classes = Vec::new();
        
        // Pattern: Class/interface/struct definitions
        if let Ok(re) = regex::Regex::new(r"(?m)^\s*(?:public|private|protected|internal)?\s*(?:static|abstract|sealed|partial)?\s*(class|interface|struct)\s+(\w+)(?:<[^>]*>)?(?:\s*:\s*([^{]+))?\s*\{") {
            for caps in re.captures_iter(content) {
                if let Some(name) = caps.get(2) {
                    let mut class = ClassInfo::new(name.as_str().to_string());
                    
                    // Check type
                    if let Some(class_type) = caps.get(1) {
                        match class_type.as_str() {
                            "interface" => {
                                class.metadata.insert("is_interface".to_string(), "true".to_string());
                            }
                            "struct" => {
                                class.metadata.insert("is_struct".to_string(), "true".to_string());
                            }
                            _ => {}
                        }
                    }
                    
                    // Parse inheritance (simplified)
                    if let Some(inheritance) = caps.get(3) {
                        let parent_list: Vec<&str> = inheritance.as_str()
                            .split(',')
                            .map(|s| s.trim())
                            .filter(|s| !s.is_empty())
                            .collect();
                        
                        if !parent_list.is_empty() {
                            class.parent_class = Some(parent_list[0].to_string());
                            if parent_list.len() > 1 {
                                class.metadata.insert("implements_interfaces".to_string(), parent_list[1..].join(", "));
                            }
                        }
                    }
                    
                    classes.push(class);
                }
            }
        }
        
        classes
    }
    
    /// Fallback import extraction using regex
    fn regex_fallback_imports(&self, content: &str) -> Vec<ImportInfo> {
        let mut imports = Vec::new();
        
        // Pattern: using statements
        if let Ok(re) = regex::Regex::new(r"using\s+([^;=]+);") {
            for caps in re.captures_iter(content) {
                if let Some(namespace) = caps.get(1) {
                    let namespace_str = namespace.as_str().trim();
                    if !namespace_str.is_empty() && !namespace_str.starts_with("(") {
                        let import = ImportInfo::new(ImportType::CSharpUsing, namespace_str.to_string());
                        imports.push(import);
                    }
                }
            }
        }
        
        imports
    }
}

impl Default for CSharpAnalyzer {
    fn default() -> Self {
        Self::new()
    }
}