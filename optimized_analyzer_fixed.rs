//! 🚀 Fixed Optimization: Smart Single-pass Analyzer
//! 
//! This implementation fixes the bug in the original optimization attempt.
//! The key insight: We need to keep the existing extraction methods AND
//! add the optimized version as an alternative path.

use anyhow::Result;
use async_trait::async_trait;
use pest::Parser;
use pest_derive::Parser;

use crate::core::types::{
    AnalysisResult, ClassInfo, ComplexityInfo, FileInfo, FunctionInfo, ImportInfo, 
    ImportType, Language, ExportInfo, ExportType, FunctionCall
};

/// 🚀 Revolutionary optimization: Multi-collector for single-pass extraction
#[derive(Default, Debug)]
struct ExtractedConstructs {
    functions: Vec<FunctionInfo>,
    classes: Vec<ClassInfo>,
    imports: Vec<ImportInfo>,
    exports: Vec<ExportInfo>,
    calls: Vec<FunctionCall>,
}

impl ExtractedConstructs {
    fn merge(&mut self, other: ExtractedConstructs) {
        self.functions.extend(other.functions);
        self.classes.extend(other.classes);
        self.imports.extend(other.imports);
        self.exports.extend(other.exports);
        self.calls.extend(other.calls);
    }
    
    fn is_empty(&self) -> bool {
        self.functions.is_empty() && self.classes.is_empty() && 
        self.imports.is_empty() && self.exports.is_empty() && self.calls.is_empty()
    }
    
    fn has_meaningful_content(&self) -> bool {
        !self.functions.is_empty() || !self.classes.is_empty()
    }
}

impl JavaScriptAnalyzer {
    /// 🚀 OPTIMIZED VERSION: Extract all constructs in a single pass
    /// This replaces the 5x clone() operations with smart iteration
    fn extract_all_constructs_single_pass(&self, pairs: pest::iterators::Pairs<Rule>) -> ExtractedConstructs {
        let mut result = ExtractedConstructs::default();
        
        for pair in pairs {
            match pair.as_rule() {
                // Functions
                Rule::function_decl => {
                    if let Some(func) = self.parse_function_declaration(pair) {
                        result.functions.push(func);
                    }
                }
                Rule::arrow_function => {
                    if let Some(func) = self.parse_arrow_function(pair) {
                        result.functions.push(func);
                    }
                }
                
                // Classes  
                Rule::class_decl => {
                    if let Some(class) = self.parse_class_declaration(pair) {
                        result.classes.push(class);
                    }
                }
                
                // Imports
                Rule::import_stmt => {
                    if let Some(import) = self.parse_import_statement(pair) {
                        result.imports.push(import);
                    }
                }
                
                // Exports
                Rule::export_stmt => {
                    if let Some(export) = self.parse_export_statement(pair) {
                        result.exports.push(export);
                    }
                }
                
                // Function calls
                Rule::function_call => {
                    if let Some(call) = self.parse_function_call(pair, false) {
                        result.calls.push(call);
                    }
                }
                Rule::method_call => {
                    if let Some(call) = self.parse_function_call(pair, true) {
                        result.calls.push(call);
                    }
                }
                
                // Recursive descent
                _ => {
                    let inner_constructs = self.extract_all_constructs_single_pass(pair.into_inner());
                    result.merge(inner_constructs);
                }
            }
        }
        
        result
    }
    
    /// 🎯 OPTIMIZATION ENTRY POINT: Replace the 5x clone operations
    /// This is the method that should replace the clone-heavy extraction
    pub async fn analyze_optimized(&mut self, content: &str, filename: &str) -> Result<AnalysisResult> {
        // ... same setup code as original ...
        
        // Create file info
        let file_path = std::path::PathBuf::from(filename);
        let mut file_info = FileInfo::new(file_path);
        file_info.total_lines = content.lines().count() as u32;
        
        // Calculate basic line statistics
        for line in content.lines() {
            let trimmed = line.trim();
            if trimmed.is_empty() {
                file_info.empty_lines += 1;
            } else if trimmed.starts_with("//") || trimmed.starts_with("/*") || trimmed.starts_with("*") {
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
        
        // Determine language based on file extension
        let language = if filename.ends_with(".ts") || filename.ends_with(".tsx") {
            Language::TypeScript
        } else {
            Language::JavaScript
        };
        
        // Create analysis result
        let mut result = AnalysisResult::new(file_info, language);
        
        // Parse the JavaScript/TypeScript content
        let mut parsing_succeeded = false;
        
        match JavaScriptParser::parse(Rule::program, content) {
            Ok(mut pairs) => {
                if let Some(program) = pairs.next() {
                    let inner_pairs = program.into_inner();
                    
                    // 🚀 SINGLE-PASS OPTIMIZATION: Extract all constructs at once
                    let extracted_constructs = self.extract_all_constructs_single_pass(inner_pairs);
                    
                    // Use results if meaningful content found
                    if extracted_constructs.has_meaningful_content() {
                        result.functions = extracted_constructs.functions;
                        result.classes = extracted_constructs.classes;
                        result.imports = extracted_constructs.imports;
                        result.exports = extracted_constructs.exports;
                        result.function_calls = extracted_constructs.calls;
                        parsing_succeeded = true;
                    }
                    
                    // Update line numbers
                    if parsing_succeeded {
                        self.update_line_numbers(content, &mut result);
                    }
                    
                    // Build call frequency map
                    for call in &result.function_calls {
                        let entry = result.call_frequency.entry(call.full_name()).or_insert(0);
                        *entry += 1;
                    }
                }
            }
            Err(e) => {
                // If parsing fails, log error but continue with fallback
                eprintln!("Warning: Pest parsing failed for {}: {}", filename, e);
            }
        }
        
        // ... rest of the original method (fallback, complexity, etc.) ...
        
        Ok(result)
    }
}

/// 🎯 Usage Example: How to use the optimization
/// 
/// In the main analyze() method, replace:
/// 
/// OLD (5x clone):
/// ```rust
/// let extracted_functions = self.extract_functions(inner_pairs.clone());
/// let extracted_classes = self.extract_classes(inner_pairs.clone());
/// let extracted_imports = self.extract_imports(inner_pairs.clone());
/// let extracted_exports = self.extract_exports(inner_pairs.clone());
/// let extracted_calls = self.extract_function_calls(inner_pairs);
/// ```
/// 
/// NEW (1x pass):
/// ```rust
/// let extracted_constructs = self.extract_all_constructs_single_pass(inner_pairs);
/// result.functions = extracted_constructs.functions;
/// result.classes = extracted_constructs.classes;
/// result.imports = extracted_constructs.imports;
/// result.exports = extracted_constructs.exports;
/// result.function_calls = extracted_constructs.calls;
/// ```