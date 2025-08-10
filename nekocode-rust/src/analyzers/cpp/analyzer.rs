//! C++ analyzer implementation using regex-based parsing
//! 
//! This module provides C++ analysis capabilities,
//! detecting classes, functions, namespaces, templates, includes, and complexity metrics.

use anyhow::Result;
use async_trait::async_trait;

use crate::core::types::{
    AnalysisResult, ClassInfo, ComplexityInfo, FileInfo, FunctionInfo, ImportInfo, 
    ImportType, Language
};
use crate::analyzers::traits::LanguageAnalyzer;

/// C++ analyzer
pub struct CppAnalyzer {
    // Internal state for parsing
}

impl CppAnalyzer {
    pub fn new() -> Self {
        Self {}
    }
    
    /// Calculate complexity metrics for C++
    fn calculate_complexity(&self, content: &str) -> ComplexityInfo {
        let mut complexity = ComplexityInfo::new();
        
        // Count complexity-increasing constructs specific to C++
        let complexity_keywords = [
            "if (", "else if", "else {", "for (", "while (", "do {",
            "switch (", "case ", "catch (", "&&", "||", "? ", 
            "template<", "try {", "throw "
        ];
        
        for keyword in &complexity_keywords {
            let count = content.matches(keyword).count() as u32;
            complexity.cyclomatic_complexity += count;
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
impl LanguageAnalyzer for CppAnalyzer {
    fn get_language(&self) -> Language {
        Language::Cpp
    }
    
    fn get_language_name(&self) -> &'static str {
        "C++"
    }
    
    fn get_supported_extensions(&self) -> Vec<&'static str> {
        vec![".cpp", ".cxx", ".cc", ".hpp", ".hxx", ".hh", ".h"]
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
        let mut result = AnalysisResult::new(file_info, Language::Cpp);
        
        // Use regex-based parsing
        result.functions = self.extract_functions(content);
        result.classes = self.extract_classes(content);
        result.imports = self.extract_imports(content);
        
        // Calculate complexity
        result.complexity = self.calculate_complexity(content);
        
        // Update statistics
        result.update_statistics();
        
        Ok(result)
    }
}

impl CppAnalyzer {
    /// Extract functions using regex
    fn extract_functions(&self, content: &str) -> Vec<FunctionInfo> {
        let mut functions = Vec::new();
        
        // Pattern 1: Regular function definitions
        if let Ok(re) = regex::Regex::new(r"(?m)^\s*(?:virtual\s+|static\s+|inline\s+)*\w+(?:\s*::\s*\w+)*\s+(\w+)\s*\([^)]*\)\s*(?:const\s*)?(?:override\s*|final\s*)?(?:\s*{\s*|\s*;)") {
            for caps in re.captures_iter(content) {
                if let Some(name) = caps.get(1) {
                    let mut func = FunctionInfo::new(name.as_str().to_string());
                    let full_match = caps.get(0).unwrap().as_str();
                    
                    if full_match.contains("virtual") {
                        func.metadata.insert("is_virtual".to_string(), "true".to_string());
                    }
                    if full_match.contains("static") {
                        func.metadata.insert("is_static".to_string(), "true".to_string());
                    }
                    if full_match.contains("inline") {
                        func.metadata.insert("is_inline".to_string(), "true".to_string());
                    }
                    
                    functions.push(func);
                }
            }
        }
        
        functions
    }
    
    /// Extract classes using regex
    fn extract_classes(&self, content: &str) -> Vec<ClassInfo> {
        let mut classes = Vec::new();
        
        // Pattern: Class/struct definitions
        if let Ok(re) = regex::Regex::new(r"(?m)^\s*(?:template\s*<[^>]*>\s*)?(class|struct)\s+(\w+)(?:\s*:\s*(?:public|private|protected)?\s*([^{]+))?\s*\{") {
            for caps in re.captures_iter(content) {
                if let Some(name) = caps.get(2) {
                    let mut class = ClassInfo::new(name.as_str().to_string());
                    
                    // Check if it's a struct
                    if let Some(keyword) = caps.get(1) {
                        if keyword.as_str() == "struct" {
                            class.metadata.insert("is_struct".to_string(), "true".to_string());
                        }
                    }
                    
                    // Parse inheritance (simplified)
                    if let Some(inheritance) = caps.get(3) {
                        let parent_list: Vec<&str> = inheritance.as_str()
                            .split(',')
                            .map(|s| s.trim().split_whitespace().last().unwrap_or(""))
                            .filter(|s| !s.is_empty())
                            .collect();
                        
                        if !parent_list.is_empty() {
                            class.parent_class = Some(parent_list[0].to_string());
                            if parent_list.len() > 1 {
                                class.metadata.insert("multiple_inheritance".to_string(), parent_list.join(", "));
                            }
                        }
                    }
                    
                    classes.push(class);
                }
            }
        }
        
        classes
    }
    
    /// Extract imports using regex
    fn extract_imports(&self, content: &str) -> Vec<ImportInfo> {
        let mut imports = Vec::new();
        
        // Pattern 1: #include statements
        if let Ok(re) = regex::Regex::new(r#"#include\s*([<"])([^>"\n]+)[>"]"#) {
            for caps in re.captures_iter(content) {
                if let Some(path) = caps.get(2) {
                    let mut import = ImportInfo::new(ImportType::CppInclude, path.as_str().to_string());
                    
                    if let Some(bracket) = caps.get(1) {
                        if bracket.as_str() == "<" {
                            import.metadata.insert("is_system_header".to_string(), "true".to_string());
                        }
                    }
                    
                    imports.push(import);
                }
            }
        }
        
        // Pattern 2: using namespace statements
        if let Ok(re) = regex::Regex::new(r"using\s+namespace\s+([^;]+);") {
            for caps in re.captures_iter(content) {
                if let Some(namespace) = caps.get(1) {
                    let mut import = ImportInfo::new(ImportType::CppInclude, namespace.as_str().trim().to_string());
                    import.metadata.insert("is_using_namespace".to_string(), "true".to_string());
                    imports.push(import);
                }
            }
        }
        
        imports
    }
}

impl Default for CppAnalyzer {
    fn default() -> Self {
        Self::new()
    }
}