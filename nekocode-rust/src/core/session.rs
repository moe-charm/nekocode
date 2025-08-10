//! Session management for NekoCode Rust
//! 
//! This module handles analysis sessions, file discovery, and orchestrates
//! the analysis process across different file types and analyzers.

use anyhow::{Context, Result};
use std::path::{Path, PathBuf};
use walkdir::WalkDir;
use std::collections::HashMap;
use chrono::{DateTime, Utc};
use serde::{Deserialize, Serialize};

use crate::core::types::{
    AnalysisConfig, AnalysisResult, DirectoryAnalysis, FileInfo, Language,
};
use crate::analyzers::javascript::JavaScriptAnalyzer;
use crate::analyzers::traits::LanguageAnalyzer;

/// Session storage for managing multiple analysis sessions
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SessionInfo {
    pub id: String,
    pub path: PathBuf,
    pub created_at: DateTime<Utc>,
    pub last_accessed: DateTime<Utc>,
    pub metadata: HashMap<String, String>,
}

/// Global session manager
static mut SESSION_MANAGER: Option<SessionManager> = None;

pub struct SessionManager {
    sessions: HashMap<String, AnalysisSession>,
    session_info: HashMap<String, SessionInfo>,
}

impl SessionManager {
    pub fn global() -> &'static mut SessionManager {
        unsafe {
            if SESSION_MANAGER.is_none() {
                SESSION_MANAGER = Some(SessionManager::new());
            }
            SESSION_MANAGER.as_mut().unwrap()
        }
    }
    
    fn new() -> Self {
        Self {
            sessions: HashMap::new(),
            session_info: HashMap::new(),
        }
    }
    
    pub fn create_session(&mut self, path: &Path) -> Result<String> {
        let session_id = uuid::Uuid::new_v4().to_string()[..8].to_string();
        let mut session = AnalysisSession::new();
        
        // Initialize session with path analysis
        let _ = session.analyze_path(path, false);
        
        let session_info = SessionInfo {
            id: session_id.clone(),
            path: path.to_path_buf(),
            created_at: Utc::now(),
            last_accessed: Utc::now(),
            metadata: HashMap::new(),
        };
        
        self.sessions.insert(session_id.clone(), session);
        self.session_info.insert(session_id.clone(), session_info);
        
        Ok(session_id)
    }
    
    pub fn get_session(&mut self, session_id: &str) -> Option<&mut AnalysisSession> {
        if let Some(info) = self.session_info.get_mut(session_id) {
            info.last_accessed = Utc::now();
        }
        self.sessions.get_mut(session_id)
    }
    
    pub fn list_sessions(&self) -> Vec<&SessionInfo> {
        self.session_info.values().collect()
    }
    
    pub fn execute_session_command(&mut self, session_id: &str, command: &str, args: &[String]) -> Result<String> {
        let session = self.get_session(session_id)
            .ok_or_else(|| anyhow::anyhow!("Session not found: {}", session_id))?;
            
        match command {
            "stats" => {
                Ok(format!("Session {} statistics:\n{:?}", session_id, session.get_stats()))
            }
            "complexity" => {
                Ok(format!("Session {} complexity analysis:\n{:?}", session_id, session.get_complexity()))
            }
            "structure" => {
                Ok(format!("Session {} structure:\n{:?}", session_id, session.get_structure()))
            }
            "find" => {
                let term = args.get(0).unwrap_or(&String::new()).clone();
                Ok(format!("Session {} find results for '{}':\n{:?}", session_id, term, session.find_symbols(&term)))
            }
            "include-cycles" => {
                Ok(format!("Session {} include cycles:\n{:?}", session_id, session.find_include_cycles()))
            }
            _ => anyhow::bail!("Unknown session command: {}", command),
        }
    }
}

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
            for file in &files {
                println!("  - {}", file.display());
            }
        }
        
        // Analyze files in parallel
        let results: Result<Vec<_>> = if self.config.enable_parallel_processing {
            // Use spawn_blocking for CPU-intensive work to avoid blocking the async runtime
            let futures: Vec<_> = files.into_iter().map(|file_path| {
                let session_ref = &self;
                async move {
                    session_ref.analyze_file(&file_path).await
                }
            }).collect();
            
            // Process futures concurrently
            let results = futures::future::join_all(futures).await;
            results.into_iter().collect()
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
        let file_name = path.file_name()
            .and_then(|n| n.to_str())
            .unwrap_or("")
            .to_lowercase();
        
        let path_str = path.to_string_lossy().to_lowercase();
        
        // Check for specific test file patterns
        file_name.contains("test") ||
        file_name.contains("spec") ||
        path_str.contains("__tests__") ||
        path_str.ends_with(".test.js") ||
        path_str.ends_with(".test.ts") ||
        path_str.ends_with(".spec.js") ||
        path_str.ends_with(".spec.ts") ||
        path_str.contains("/test/") ||
        path_str.contains("/tests/") ||
        path_str.contains("/spec/") ||
        path_str.contains("/specs/")
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
    
    // Session-specific methods for new functionality
    pub fn get_stats(&self) -> String {
        "Session statistics placeholder".to_string()
    }
    
    pub fn get_complexity(&self) -> String {
        "Session complexity analysis placeholder".to_string()
    }
    
    pub fn get_structure(&self) -> String {
        "Session structure analysis placeholder".to_string()
    }
    
    pub fn find_symbols(&self, term: &str) -> String {
        format!("Symbol search results for '{}' placeholder", term)
    }
    
    pub fn find_include_cycles(&self) -> String {
        "Include cycle analysis placeholder".to_string()
    }
}

impl Default for AnalysisSession {
    fn default() -> Self {
        Self::new()
    }
}