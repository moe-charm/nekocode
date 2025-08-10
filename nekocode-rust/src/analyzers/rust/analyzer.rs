//! Rust analyzer implementation
use anyhow::Result;
use async_trait::async_trait;

use crate::core::types::{
    AnalysisResult, ClassInfo, ComplexityInfo, FileInfo, FunctionInfo, ImportInfo, 
    ImportType, Language
};
use crate::analyzers::traits::LanguageAnalyzer;

pub struct RustAnalyzer;

impl RustAnalyzer {
    pub fn new() -> Self {
        Self
    }
    
    fn calculate_complexity(&self, content: &str) -> ComplexityInfo {
        let mut complexity = ComplexityInfo::new();
        
        let complexity_keywords = [
            "if ", "else ", "for ", "while ", "loop ", "match ", 
            "if let", "while let", "&&", "||", "?", "async ", "await"
        ];
        
        for keyword in &complexity_keywords {
            complexity.cyclomatic_complexity += content.matches(keyword).count() as u32;
        }
        
        // Count match arms which add complexity
        complexity.cyclomatic_complexity += content.matches(" => ").count() as u32;
        
        // Calculate nesting depth
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
    
    fn extract_functions(&self, content: &str) -> Vec<FunctionInfo> {
        let mut functions = Vec::new();
        
        if let Ok(re) = regex::Regex::new(r"(?:pub\s+)?(?:async\s+)?fn\s+(\w+)") {
            for caps in re.captures_iter(content) {
                if let Some(name) = caps.get(1) {
                    let mut func = FunctionInfo::new(name.as_str().to_string());
                    let full_match = caps.get(0).unwrap().as_str();
                    
                    if full_match.contains("async") {
                        func.is_async = true;
                    }
                    if full_match.contains("pub") {
                        func.metadata.insert("is_public".to_string(), "true".to_string());
                    }
                    
                    functions.push(func);
                }
            }
        }
        
        functions
    }
    
    fn extract_structs(&self, content: &str) -> Vec<ClassInfo> {
        let mut items = Vec::new();
        
        // Extract structs
        if let Ok(re) = regex::Regex::new(r"(?:pub\s+)?struct\s+(\w+)") {
            for caps in re.captures_iter(content) {
                if let Some(name) = caps.get(1) {
                    let mut class = ClassInfo::new(name.as_str().to_string());
                    class.metadata.insert("type".to_string(), "struct".to_string());
                    items.push(class);
                }
            }
        }
        
        // Extract enums
        if let Ok(re) = regex::Regex::new(r"(?:pub\s+)?enum\s+(\w+)") {
            for caps in re.captures_iter(content) {
                if let Some(name) = caps.get(1) {
                    let mut class = ClassInfo::new(name.as_str().to_string());
                    class.metadata.insert("type".to_string(), "enum".to_string());
                    items.push(class);
                }
            }
        }
        
        // Extract traits
        if let Ok(re) = regex::Regex::new(r"(?:pub\s+)?trait\s+(\w+)") {
            for caps in re.captures_iter(content) {
                if let Some(name) = caps.get(1) {
                    let mut class = ClassInfo::new(name.as_str().to_string());
                    class.metadata.insert("type".to_string(), "trait".to_string());
                    items.push(class);
                }
            }
        }
        
        items
    }
    
    fn extract_imports(&self, content: &str) -> Vec<ImportInfo> {
        let mut imports = Vec::new();
        
        if let Ok(re) = regex::Regex::new(r"use\s+([^;]+);") {
            for caps in re.captures_iter(content) {
                if let Some(use_path) = caps.get(1) {
                    let path = use_path.as_str().trim();
                    if !path.is_empty() {
                        imports.push(ImportInfo::new(ImportType::RustUse, path.to_string()));
                    }
                }
            }
        }
        
        imports
    }
}

#[async_trait]
impl LanguageAnalyzer for RustAnalyzer {
    fn get_language(&self) -> Language {
        Language::Rust
    }
    
    fn get_language_name(&self) -> &'static str {
        "Rust"
    }
    
    fn get_supported_extensions(&self) -> Vec<&'static str> {
        vec![".rs"]
    }
    
    async fn analyze(&mut self, content: &str, filename: &str) -> Result<AnalysisResult> {
        let file_path = std::path::PathBuf::from(filename);
        let mut file_info = FileInfo::new(file_path);
        file_info.total_lines = content.lines().count() as u32;
        
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
        
        let mut result = AnalysisResult::new(file_info, Language::Rust);
        
        result.functions = self.extract_functions(content);
        result.classes = self.extract_structs(content);
        result.imports = self.extract_imports(content);
        result.complexity = self.calculate_complexity(content);
        
        result.update_statistics();
        Ok(result)
    }
}

impl Default for RustAnalyzer {
    fn default() -> Self {
        Self::new()
    }
}