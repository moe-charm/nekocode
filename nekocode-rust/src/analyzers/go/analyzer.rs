//! Go analyzer implementation
use anyhow::Result;
use async_trait::async_trait;

use crate::core::types::{
    AnalysisResult, ClassInfo, ComplexityInfo, FileInfo, FunctionInfo, ImportInfo, 
    ImportType, Language
};
use crate::analyzers::traits::LanguageAnalyzer;

pub struct GoAnalyzer;

impl GoAnalyzer {
    pub fn new() -> Self {
        Self
    }
    
    fn calculate_complexity(&self, content: &str) -> ComplexityInfo {
        let mut complexity = ComplexityInfo::new();
        
        let complexity_keywords = [
            "if ", "else ", "for ", "switch ", "case ", "select ", 
            "go ", "defer ", "&&", "||", "?"
        ];
        
        for keyword in &complexity_keywords {
            complexity.cyclomatic_complexity += content.matches(keyword).count() as u32;
        }
        
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
        
        if let Ok(re) = regex::Regex::new(r"func\s+(?:\([^)]*\)\s+)?(\w+)\s*\([^)]*\)") {
            for caps in re.captures_iter(content) {
                if let Some(name) = caps.get(1) {
                    let func = FunctionInfo::new(name.as_str().to_string());
                    functions.push(func);
                }
            }
        }
        
        functions
    }
    
    fn extract_structs(&self, content: &str) -> Vec<ClassInfo> {
        let mut structs = Vec::new();
        
        if let Ok(re) = regex::Regex::new(r"type\s+(\w+)\s+struct\s*\{") {
            for caps in re.captures_iter(content) {
                if let Some(name) = caps.get(1) {
                    let mut class = ClassInfo::new(name.as_str().to_string());
                    class.metadata.insert("is_struct".to_string(), "true".to_string());
                    structs.push(class);
                }
            }
        }
        
        // Also extract interfaces
        if let Ok(re) = regex::Regex::new(r"type\s+(\w+)\s+interface\s*\{") {
            for caps in re.captures_iter(content) {
                if let Some(name) = caps.get(1) {
                    let mut class = ClassInfo::new(name.as_str().to_string());
                    class.metadata.insert("is_interface".to_string(), "true".to_string());
                    structs.push(class);
                }
            }
        }
        
        structs
    }
    
    fn extract_imports(&self, content: &str) -> Vec<ImportInfo> {
        let mut imports = Vec::new();
        
        if let Ok(re) = regex::Regex::new(r#"import\s+(?:\(\s*([^)]+)\s*\)|"([^"]+)")"#) {
            for caps in re.captures_iter(content) {
                if let Some(import_block) = caps.get(1) {
                    // Multi-line import block
                    for line in import_block.as_str().lines() {
                        let trimmed = line.trim();
                        if trimmed.starts_with('"') && trimmed.ends_with('"') {
                            let path = &trimmed[1..trimmed.len()-1];
                            imports.push(ImportInfo::new(ImportType::GoImport, path.to_string()));
                        }
                    }
                } else if let Some(single_import) = caps.get(2) {
                    // Single import
                    imports.push(ImportInfo::new(ImportType::GoImport, single_import.as_str().to_string()));
                }
            }
        }
        
        imports
    }
}

#[async_trait]
impl LanguageAnalyzer for GoAnalyzer {
    fn get_language(&self) -> Language {
        Language::Go
    }
    
    fn get_language_name(&self) -> &'static str {
        "Go"
    }
    
    fn get_supported_extensions(&self) -> Vec<&'static str> {
        vec![".go"]
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
        
        let mut result = AnalysisResult::new(file_info, Language::Go);
        
        result.functions = self.extract_functions(content);
        result.classes = self.extract_structs(content);
        result.imports = self.extract_imports(content);
        result.complexity = self.calculate_complexity(content);
        
        result.update_statistics();
        Ok(result)
    }
}

impl Default for GoAnalyzer {
    fn default() -> Self {
        Self::new()
    }
}