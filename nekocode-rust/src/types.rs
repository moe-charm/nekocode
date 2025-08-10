//=============================================================================
// 🦀 NekoCode Rust - Core Types
//
// Rust port of NekoCode C++ types with equivalent functionality
// Maintains compatibility with the original C++ JSON output format
//=============================================================================

use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::path::PathBuf;
use std::time::SystemTime;

//=============================================================================
// 🌍 Language Support - Language Type Definitions
//=============================================================================

/// Supported language types
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
#[serde(rename_all = "lowercase")]
pub enum Language {
    #[serde(rename = "javascript")]
    JavaScript,
    #[serde(rename = "typescript")]
    TypeScript,
    #[serde(rename = "cpp")]
    Cpp,
    #[serde(rename = "c")]
    C,
    #[serde(rename = "python")]
    Python,
    #[serde(rename = "csharp")]
    CSharp,
    #[serde(rename = "go")]
    Go,
    #[serde(rename = "rust")]
    Rust,
    #[serde(rename = "unknown")]
    Unknown,
}

impl Language {
    pub fn from_extension(extension: &str) -> Self {
        match extension.to_lowercase().as_str() {
            "js" | "jsx" => Language::JavaScript,
            "ts" | "tsx" => Language::TypeScript,
            "cpp" | "cc" | "cxx" | "hpp" | "h++" => Language::Cpp,
            "c" | "h" => Language::C,
            "py" | "pyw" => Language::Python,
            "cs" => Language::CSharp,
            "go" => Language::Go,
            "rs" => Language::Rust,
            _ => Language::Unknown,
        }
    }

    pub fn name(&self) -> &'static str {
        match self {
            Language::JavaScript => "JavaScript",
            Language::TypeScript => "TypeScript",
            Language::Cpp => "C++",
            Language::C => "C",
            Language::Python => "Python",
            Language::CSharp => "C#",
            Language::Go => "Go",
            Language::Rust => "Rust",
            Language::Unknown => "Unknown",
        }
    }
}

//=============================================================================
// 🎯 Core Types - Type-safe Design Compatible with Python Version
//=============================================================================

pub type LineNumber = u32;
pub type FileSize = u64;

//=============================================================================
// 📄 File Information - Equivalent to Python FileInfo
//=============================================================================

#[derive(Debug, Clone, Serialize)]
pub struct FileInfo {
    pub name: String,
    #[serde(skip)]
    pub path: Option<PathBuf>,
    pub size_bytes: FileSize,
    pub total_lines: LineNumber,
    pub code_lines: LineNumber,
    pub comment_lines: LineNumber,
    pub empty_lines: LineNumber,
    pub code_ratio: f64,
    #[serde(skip)]
    pub analyzed_at: SystemTime,
    pub metadata: HashMap<String, String>,
}

impl FileInfo {
    pub fn new(name: String) -> Self {
        Self {
            name,
            path: None,
            size_bytes: 0,
            total_lines: 0,
            code_lines: 0,
            comment_lines: 0,
            empty_lines: 0,
            code_ratio: 0.0,
            analyzed_at: SystemTime::now(),
            metadata: HashMap::new(),
        }
    }

    pub fn from_path(path: PathBuf) -> Self {
        let name = path
            .file_name()
            .and_then(|n| n.to_str())
            .unwrap_or("unknown")
            .to_string();
        
        Self {
            name,
            path: Some(path),
            size_bytes: 0,
            total_lines: 0,
            code_lines: 0,
            comment_lines: 0,
            empty_lines: 0,
            code_ratio: 0.0,
            analyzed_at: SystemTime::now(),
            metadata: HashMap::new(),
        }
    }
}

//=============================================================================
// 🧮 Complexity Analysis - Python Version Complexity Analysis Extension
//=============================================================================

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum ComplexityRating {
    #[serde(rename = "simple")]
    Simple,      // <= 10
    #[serde(rename = "moderate")]
    Moderate,    // 11-20
    #[serde(rename = "complex")]
    Complex,     // 21-50
    #[serde(rename = "very_complex")]
    VeryComplex, // > 50
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ComplexityInfo {
    pub cyclomatic_complexity: u32,
    pub max_nesting_depth: u32,
    pub cognitive_complexity: u32,
    pub rating: ComplexityRating,
    pub rating_emoji: String,
}

impl ComplexityInfo {
    pub fn new() -> Self {
        let mut info = Self {
            cyclomatic_complexity: 1,
            max_nesting_depth: 0,
            cognitive_complexity: 0,
            rating: ComplexityRating::Simple,
            rating_emoji: String::new(),
        };
        info.update_rating();
        info
    }

    pub fn update_rating(&mut self) {
        let (rating, emoji) = match self.cyclomatic_complexity {
            0..=10 => (ComplexityRating::Simple, "🟢"),
            11..=20 => (ComplexityRating::Moderate, "🟡"),
            21..=50 => (ComplexityRating::Complex, "🟠"),
            _ => (ComplexityRating::VeryComplex, "🔴"),
        };
        self.rating = rating;
        self.rating_emoji = emoji.to_string();
    }

    pub fn to_string(&self) -> String {
        let rating_name = match self.rating {
            ComplexityRating::Simple => "Simple",
            ComplexityRating::Moderate => "Moderate", 
            ComplexityRating::Complex => "Complex",
            ComplexityRating::VeryComplex => "Very Complex",
        };
        format!("{} {}", rating_name, self.rating_emoji)
    }
}

impl Default for ComplexityInfo {
    fn default() -> Self {
        Self::new()
    }
}

//=============================================================================
// 🏗️ Code Structure - Class and Function Information
//=============================================================================

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FunctionInfo {
    pub name: String,
    pub start_line: LineNumber,
    pub end_line: LineNumber,
    pub parameters: Vec<String>,
    pub is_async: bool,
    pub is_arrow_function: bool,
    pub complexity: ComplexityInfo,
    pub metadata: HashMap<String, String>,
}

impl FunctionInfo {
    pub fn new(name: String) -> Self {
        Self {
            name,
            start_line: 0,
            end_line: 0,
            parameters: Vec::new(),
            is_async: false,
            is_arrow_function: false,
            complexity: ComplexityInfo::new(),
            metadata: HashMap::new(),
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MemberVariable {
    pub name: String,
    pub var_type: String,
    pub declaration_line: LineNumber,
    pub is_static: bool,
    pub is_const: bool,
    pub access_modifier: String, // public/private/protected
    pub used_by_methods: Vec<String>,
    pub modified_by_methods: Vec<String>,
    pub metadata: HashMap<String, String>,
}

impl MemberVariable {
    pub fn new(name: String, var_type: String, declaration_line: LineNumber) -> Self {
        Self {
            name,
            var_type,
            declaration_line,
            is_static: false,
            is_const: false,
            access_modifier: "private".to_string(),
            used_by_methods: Vec::new(),
            modified_by_methods: Vec::new(),
            metadata: HashMap::new(),
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ClassInfo {
    pub name: String,
    pub parent_class: String,
    pub start_line: LineNumber,
    pub end_line: LineNumber,
    pub methods: Vec<FunctionInfo>,
    pub properties: Vec<String>,
    pub member_variables: Vec<MemberVariable>,
    pub metadata: HashMap<String, String>,
}

impl ClassInfo {
    pub fn new(name: String) -> Self {
        Self {
            name,
            parent_class: String::new(),
            start_line: 0,
            end_line: 0,
            methods: Vec::new(),
            properties: Vec::new(),
            member_variables: Vec::new(),
            metadata: HashMap::new(),
        }
    }
}

//=============================================================================
// 📦 Import/Export Analysis - Dependency Information
//=============================================================================

#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum ImportType {
    #[serde(rename = "es6_import")]
    Es6Import,
    #[serde(rename = "commonjs_require")]
    CommonjsRequire,
    #[serde(rename = "dynamic_import")]
    DynamicImport,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum ExportType {
    #[serde(rename = "es6_export")]
    Es6Export,
    #[serde(rename = "es6_default")]
    Es6Default,
    #[serde(rename = "commonjs_exports")]
    CommonjsExports,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ImportInfo {
    pub import_type: ImportType,
    pub module_path: String,
    pub imported_names: Vec<String>,
    pub alias: String,
    pub line_number: LineNumber,
    pub metadata: HashMap<String, String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ExportInfo {
    pub export_type: ExportType,
    pub exported_names: Vec<String>,
    pub is_default: bool,
    pub line_number: LineNumber,
}

//=============================================================================
// 📞 Function Call Analysis - Function Call Analysis
//=============================================================================

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FunctionCall {
    pub function_name: String,
    pub object_name: String,
    pub line_number: LineNumber,
    pub is_method_call: bool,
}

impl FunctionCall {
    pub fn full_name(&self) -> String {
        if self.is_method_call {
            format!("{}.{}", self.object_name, self.function_name)
        } else {
            self.function_name.clone()
        }
    }
}

pub type FunctionCallFrequency = HashMap<String, u32>;

//=============================================================================
// 💬 Comment Analysis - Comment Line Analysis
//=============================================================================

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CommentInfo {
    pub line_start: u32,
    pub line_end: u32,
    pub comment_type: String, // "single_line" | "multi_line"
    pub content: String,
    pub looks_like_code: bool,
}

//=============================================================================
// 📊 Analysis Results - Integrated Analysis Results
//=============================================================================

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Statistics {
    pub total_classes: u32,
    pub total_functions: u32,
    pub total_imports: u32,
    pub total_exports: u32,
    pub unique_calls: u32,
    pub total_calls: u32,
    pub commented_lines_count: u32,
}

impl Default for Statistics {
    fn default() -> Self {
        Self {
            total_classes: 0,
            total_functions: 0,
            total_imports: 0,
            total_exports: 0,
            unique_calls: 0,
            total_calls: 0,
            commented_lines_count: 0,
        }
    }
}

#[derive(Debug, Clone, Serialize)]
pub struct AnalysisResult {
    // Basic information
    pub file_info: FileInfo,
    pub language: Language,

    // Structure information
    pub classes: Vec<ClassInfo>,
    pub functions: Vec<FunctionInfo>,

    // Dependencies
    pub imports: Vec<ImportInfo>,
    pub exports: Vec<ExportInfo>,

    // Function calls
    pub function_calls: Vec<FunctionCall>,
    pub call_frequency: FunctionCallFrequency,

    // Complexity
    pub complexity: ComplexityInfo,

    // Comment analysis
    pub commented_lines: Vec<CommentInfo>,

    // Extended metadata (Unity etc.)
    pub metadata: HashMap<String, String>,

    // Statistics (Python version compatible)
    pub statistics: Statistics,

    // Universal Symbol information (optional)
    // Note: This will be added in a separate module
    #[serde(skip)]
    pub universal_symbols: Option<()>, // Placeholder for now

    #[serde(skip)]
    pub generated_at: SystemTime,
}

impl AnalysisResult {
    pub fn new(language: Language) -> Self {
        Self {
            file_info: FileInfo::new("unknown".to_string()),
            language,
            classes: Vec::new(),
            functions: Vec::new(),
            imports: Vec::new(),
            exports: Vec::new(),
            function_calls: Vec::new(),
            call_frequency: HashMap::new(),
            complexity: ComplexityInfo::new(),
            commented_lines: Vec::new(),
            metadata: HashMap::new(),
            statistics: Statistics::default(),
            universal_symbols: None,
            generated_at: SystemTime::now(),
        }
    }

    pub fn update_statistics(&mut self) {
        self.statistics.total_classes = self.classes.len() as u32;
        self.statistics.total_functions = self.functions.len() as u32;
        self.statistics.total_imports = self.imports.len() as u32;
        self.statistics.total_exports = self.exports.len() as u32;
        self.statistics.unique_calls = self.call_frequency.len() as u32;
        self.statistics.total_calls = self.function_calls.len() as u32;
        self.statistics.commented_lines_count = self.commented_lines.len() as u32;
    }
}