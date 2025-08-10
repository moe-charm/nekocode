//! Session management for NekoCode Rust
//! 
//! This module handles analysis sessions, file discovery, and orchestrates
//! the analysis process across different file types and analyzers.

use anyhow::{Context, Result};
use std::path::{Path, PathBuf};
use walkdir::WalkDir;
use rayon::prelude::*;

use crate::core::types::{
    AnalysisConfig, AnalysisResult, DirectoryAnalysis, FileInfo, Language,
};
use crate::analyzers::javascript::JavaScriptAnalyzer;
use crate::analyzers::traits::LanguageAnalyzer;

/// Main analysis session coordinator
pub struct AnalysisSession {
    config: AnalysisConfig,
}

impl AnalysisSession {
    pub fn new() -> Self {
        Self {
            config: AnalysisConfig::default(),
        }
    }
    
    pub fn with_config(config: AnalysisConfig) -> Self {
        Self { config }
    }
    
    /// Analyze a single file or directory
    pub async fn analyze_path(&mut self, path: &Path, include_tests: bool) -> Result<DirectoryAnalysis> {
        self.config.include_test_files = include_tests;
        
        if path.is_file() {
            self.analyze_single_file(path).await
        } else if path.is_dir() {
            self.analyze_directory(path).await
        } else {
            anyhow::bail!("Path does not exist or is not accessible: {}", path.display());
        }
    }
    
    /// Analyze a single file
    async fn analyze_single_file(&self, file_path: &Path) -> Result<DirectoryAnalysis> {
        let mut directory_analysis = DirectoryAnalysis::new(
            file_path.parent().unwrap_or_else(|| Path::new(".")).to_path_buf()
        );
        
        let result = self.analyze_file(file_path).await
            .with_context(|| format!("Failed to analyze file: {}", file_path.display()))?;
        
        directory_analysis.files.push(result);
        directory_analysis.update_summary();
        
        Ok(directory_analysis)
    }
    
    /// Analyze a directory
    async fn analyze_directory(&self, dir_path: &Path) -> Result<DirectoryAnalysis> {
        let mut directory_analysis = DirectoryAnalysis::new(dir_path.to_path_buf());
        
        // Discover files
        let files = self.discover_files(dir_path)?;
        
        if self.config.verbose_output {
            println!("📁 Found {} files to analyze", files.len());
        }
        
        // Analyze files in parallel
        let results: Result<Vec<_>> = if self.config.enable_parallel_processing {
            files.par_iter()
                .map(|file_path| {
                    tokio::task::block_in_place(|| {
                        tokio::runtime::Handle::current().block_on(self.analyze_file(file_path))
                    })
                })
                .collect()
        } else {
            let mut results = Vec::new();
            for file_path in &files {
                results.push(self.analyze_file(file_path).await);
            }
            results.into_iter().collect()
        };
        
        directory_analysis.files = results?;
        directory_analysis.update_summary();
        
        if self.config.verbose_output {
            println!("✅ Analyzed {} files successfully", directory_analysis.files.len());
        }
        
        Ok(directory_analysis)
    }
    
    /// Discover files in a directory based on configuration
    fn discover_files(&self, dir_path: &Path) -> Result<Vec<PathBuf>> {
        let mut files = Vec::new();
        
        for entry in WalkDir::new(dir_path)
            .follow_links(false)
            .into_iter()
            .filter_map(|e| e.ok())
        {
            let path = entry.path();
            
            // Skip directories
            if !path.is_file() {
                continue;
            }
            
            // Check if path should be excluded
            if self.should_exclude_path(path) {
                continue;
            }
            
            // Check if file extension is supported
            if let Some(extension) = path.extension().and_then(|e| e.to_str()) {
                let ext_with_dot = format!(".{}", extension);
                if self.config.included_extensions.contains(&ext_with_dot) {
                    // Skip test files if not requested
                    if !self.config.include_test_files && self.is_test_file(path) {
                        continue;
                    }
                    
                    files.push(path.to_path_buf());
                }
            }
        }
        
        Ok(files)
    }
    
    /// Check if a path should be excluded based on patterns
    fn should_exclude_path(&self, path: &Path) -> bool {
        let path_str = path.to_string_lossy();
        
        for pattern in &self.config.excluded_patterns {
            if path_str.contains(pattern) {
                return true;
            }
        }
        
        false
    }
    
    /// Check if a file is a test file
    fn is_test_file(&self, path: &Path) -> bool {
        let path_str = path.to_string_lossy().to_lowercase();
        
        path_str.contains("test") ||
        path_str.contains("spec") ||
        path_str.contains("__tests__") ||
        path_str.ends_with(".test.js") ||
        path_str.ends_with(".test.ts") ||
        path_str.ends_with(".spec.js") ||
        path_str.ends_with(".spec.ts")
    }
    
    /// Analyze a specific file
    async fn analyze_file(&self, file_path: &Path) -> Result<AnalysisResult> {
        // Read file content
        let content = tokio::fs::read_to_string(file_path).await
            .with_context(|| format!("Failed to read file: {}", file_path.display()))?;
        
        // Create file info
        let metadata = tokio::fs::metadata(file_path).await
            .with_context(|| format!("Failed to get metadata for: {}", file_path.display()))?;
        
        let mut file_info = FileInfo::new(file_path.to_path_buf());
        file_info.size_bytes = metadata.len();
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
        
        // Determine language
        let language = if let Some(extension) = file_path.extension().and_then(|e| e.to_str()) {
            Language::from_extension(&format!(".{}", extension))
        } else {
            Language::Unknown
        };
        
        // Create base analysis result
        let mut result = AnalysisResult::new(file_info, language);
        
        // Perform language-specific analysis
        match language {
            Language::JavaScript | Language::TypeScript => {
                let mut analyzer = JavaScriptAnalyzer::new();
                result = analyzer.analyze(&content, file_path.to_string_lossy().as_ref()).await?;
                result.language = language; // Ensure correct language is set
            }
            Language::Unknown => {
                if self.config.verbose_output {
                    println!("⚠️  Skipping unknown file type: {}", file_path.display());
                }
            }
            _ => {
                if self.config.verbose_output {
                    println!("⚠️  Language not yet implemented: {:?} for {}", language, file_path.display());
                }
            }
        }
        
        // Update statistics
        result.update_statistics();
        
        Ok(result)
    }
}

impl Default for AnalysisSession {
    fn default() -> Self {
        Self::new()
    }
}