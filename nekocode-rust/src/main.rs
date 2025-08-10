//=============================================================================
// 🦀 NekoCode Rust - Main Entry Point
//
// Rust port of NekoCode high-speed code analysis tool
// Compatible JSON output with C++ version for AI/Claude integration
//=============================================================================

mod types;
mod analyzer;
mod analyzers;
mod core;
mod universal;

use clap::{Parser, Subcommand};
use anyhow::Result;
use std::path::PathBuf;
use core::session::SessionManager;

#[derive(Parser)]
#[command(
    name = "nekocode-rust",
    about = "🦀 NekoCode Rust - High-speed code analysis tool (C++ to Rust port)",
    version = "0.1.0"
)]
struct Cli {
    #[command(subcommand)]
    command: Commands,

    /// Number of parallel I/O threads
    #[arg(long, default_value = "4")]
    io_threads: usize,

    /// Number of CPU analysis threads (0 = auto)
    #[arg(long, default_value = "0")]
    cpu_threads: usize,

    /// Force execution without confirmation
    #[arg(long)]
    force: bool,

    /// High-speed statistics only (skip complexity analysis)
    #[arg(long)]
    stats_only: bool,

    /// Complete analysis including dead code detection
    #[arg(long)]
    complete: bool,

    /// Show performance statistics
    #[arg(long)]
    performance: bool,

    /// Show progress information
    #[arg(long)]
    progress: bool,

    /// Debug mode - verbose logging
    #[arg(long)]
    debug: bool,

    /// Skip pre-analysis checks for large projects
    #[arg(long)]
    no_check: bool,

    /// Check only - size check without analysis
    #[arg(long)]
    check_only: bool,
}

#[derive(Subcommand)]
enum Commands {
    /// Single-shot analysis of file or directory
    Analyze {
        /// Path to file or directory to analyze
        path: PathBuf,
    },

    /// Direct edit operations (no session required)
    Replace {
        file: PathBuf,
        pattern: String,
        replacement: String,
    },
    ReplacePreview {
        file: PathBuf,
        pattern: String,
        replacement: String,
    },
    ReplaceConfirm {
        preview_id: String,
    },

    Insert {
        file: PathBuf,
        position: usize,
        content: String,
    },
    InsertPreview {
        file: PathBuf,
        position: usize,
        content: String,
    },
    InsertConfirm {
        preview_id: String,
    },

    Movelines {
        src: PathBuf,
        start: usize,
        count: usize,
        dst: PathBuf,
        position: usize,
    },
    MovelinesPreview {
        src: PathBuf,
        start: usize,
        count: usize,
        dst: PathBuf,
        position: usize,
    },
    MovelinesConfirm {
        preview_id: String,
    },

    /// Session-based analysis
    SessionCreate {
        path: PathBuf,
    },
    SessionCommand {
        session_id: String,
        command: String,
    },

    /// System commands
    Config {
        #[command(subcommand)]
        config_cmd: ConfigCommands,
    },
    Memory {
        command: String,
    },
    Languages,
}

#[derive(Subcommand)]
enum ConfigCommands {
    Show,
    Set { key: String, value: String },
}

#[tokio::main]
async fn main() -> Result<()> {
    let cli = Cli::parse();

    // Initialize logging if debug mode
    if cli.debug {
        tracing_subscriber::fmt::init();
    }

    // Set up thread pool for parallel processing
    if cli.io_threads > 0 {
        rayon::ThreadPoolBuilder::new()
            .num_threads(cli.io_threads)
            .build_global()?;
    }

    // Process commands
    match cli.command {
        Commands::Analyze { ref path } => {
            handle_analyze(path.clone(), &cli).await
        }
        Commands::Replace { file, pattern, replacement } => {
            handle_replace(file, pattern, replacement).await
        }
        Commands::ReplacePreview { file, pattern, replacement } => {
            handle_replace_preview(file, pattern, replacement).await
        }
        Commands::ReplaceConfirm { preview_id } => {
            handle_replace_confirm(preview_id).await
        }
        Commands::Insert { file, position, content } => {
            handle_insert(file, position, content).await
        }
        Commands::InsertPreview { file, position, content } => {
            handle_insert_preview(file, position, content).await
        }
        Commands::InsertConfirm { preview_id } => {
            handle_insert_confirm(preview_id).await
        }
        Commands::Movelines { src, start, count, dst, position } => {
            handle_movelines(src, start, count, dst, position).await
        }
        Commands::MovelinesPreview { src, start, count, dst, position } => {
            handle_movelines_preview(src, start, count, dst, position).await
        }
        Commands::MovelinesConfirm { preview_id } => {
            handle_movelines_confirm(preview_id).await
        }
        Commands::SessionCreate { path } => {
            handle_session_create(path).await
        }
        Commands::SessionCommand { session_id, command } => {
            handle_session_command(session_id, command).await
        }
        Commands::Config { config_cmd } => {
            handle_config(config_cmd).await
        }
        Commands::Memory { command } => {
            handle_memory(command).await
        }
        Commands::Languages => {
            handle_languages().await
        }
    }
}

async fn handle_analyze(path: PathBuf, _cli: &Cli) -> Result<()> {
    if path.is_file() {
        // Analyze single file
        let content = tokio::fs::read_to_string(&path).await?;
        let filename = path.file_name()
            .and_then(|n| n.to_str())
            .unwrap_or("unknown");
        
        if let Some(extension) = path.extension().and_then(|e| e.to_str()) {
            let analyzer = analyzer::AnalyzerFactory::create_analyzer_from_extension(extension)?;
            let result = analyzer.analyze(&content, filename)?;
            
            // Output JSON compatible with C++ version
            let json_output = serde_json::to_string_pretty(&result)?;
            println!("{}", json_output);
        } else {
            eprintln!("Unknown file extension for {}", path.display());
        }
    } else if path.is_dir() {
        // TODO: Implement directory analysis
        eprintln!("Directory analysis not yet implemented");
    } else {
        eprintln!("Path does not exist: {}", path.display());
    }
    
    Ok(())
}

async fn handle_replace(file: PathBuf, pattern: String, replacement: String) -> Result<()> {
    let edit_processor = core::edit::EditProcessor::new();
    let result = edit_processor.replace_preview(
        file.to_str().unwrap_or("unknown"),
        &pattern,
        &replacement
    )?;
    println!("{}", result);
    Ok(())
}

async fn handle_replace_preview(file: PathBuf, pattern: String, replacement: String) -> Result<()> {
    let edit_processor = core::edit::EditProcessor::new();
    let result = edit_processor.replace_preview(
        file.to_str().unwrap_or("unknown"),
        &pattern,
        &replacement
    )?;
    println!("{}", result);
    Ok(())
}

async fn handle_replace_confirm(preview_id: String) -> Result<()> {
    let edit_processor = core::edit::EditProcessor::new();
    let result = edit_processor.replace_confirm(&preview_id)?;
    println!("{}", result);
    Ok(())
}

async fn handle_insert(file: PathBuf, position: usize, content: String) -> Result<()> {
    let edit_processor = core::edit::EditProcessor::new();
    let result = edit_processor.insert_preview(
        file.to_str().unwrap_or("unknown"),
        position,
        &content
    )?;
    println!("{}", result);
    Ok(())
}

async fn handle_insert_preview(file: PathBuf, position: usize, content: String) -> Result<()> {
    let edit_processor = core::edit::EditProcessor::new();
    let result = edit_processor.insert_preview(
        file.to_str().unwrap_or("unknown"),
        position,
        &content
    )?;
    println!("{}", result);
    Ok(())
}

async fn handle_insert_confirm(preview_id: String) -> Result<()> {
    let edit_processor = core::edit::EditProcessor::new();
    let result = edit_processor.insert_confirm(&preview_id)?;
    println!("{}", result);
    Ok(())
}

async fn handle_movelines(src: PathBuf, start: usize, count: usize, dst: PathBuf, position: usize) -> Result<()> {
    let edit_processor = core::edit::EditProcessor::new();
    let result = edit_processor.movelines_preview(
        src.to_str().unwrap_or("unknown"),
        start,
        count,
        dst.to_str().unwrap_or("unknown"),
        position
    )?;
    println!("{}", result);
    Ok(())
}

async fn handle_movelines_preview(src: PathBuf, start: usize, count: usize, dst: PathBuf, position: usize) -> Result<()> {
    let edit_processor = core::edit::EditProcessor::new();
    let result = edit_processor.movelines_preview(
        src.to_str().unwrap_or("unknown"),
        start,
        count,
        dst.to_str().unwrap_or("unknown"),
        position
    )?;
    println!("{}", result);
    Ok(())
}

async fn handle_movelines_confirm(preview_id: String) -> Result<()> {
    let edit_processor = core::edit::EditProcessor::new();
    let result = edit_processor.movelines_confirm(&preview_id)?;
    println!("{}", result);
    Ok(())
}

async fn handle_session_create(_path: PathBuf) -> Result<()> {
    let session_manager = SessionManager::new();
    let session_id = session_manager.create_session();
    println!("{{\"session_id\": \"{}\"}}", session_id);
    Ok(())
}

async fn handle_session_command(_session_id: String, command: String) -> Result<()> {
    let command_processor = core::commands::CommandProcessor::new();
    let result = command_processor.process_command(&command, vec![])?;
    println!("{}", result);
    Ok(())
}

async fn handle_config(config_cmd: ConfigCommands) -> Result<()> {
    match config_cmd {
        ConfigCommands::Show => {
            println!("{{\"config\": \"Configuration display not yet implemented\"}}");
        }
        ConfigCommands::Set { key, value } => {
            println!("{{\"config_set\": {{\"key\": \"{}\", \"value\": \"{}\"}}}}", key, value);
        }
    }
    Ok(())
}

async fn handle_memory(command: String) -> Result<()> {
    println!("{{\"memory\": \"Memory system not yet implemented: {}\"}}", command);
    Ok(())
}

async fn handle_languages() -> Result<()> {
    let languages = vec![
        ("JavaScript", vec!["js", "jsx", "mjs"]),
        ("TypeScript", vec!["ts", "tsx"]),
        ("Python", vec!["py", "pyw"]),
        ("C++", vec!["cpp", "cc", "cxx", "hpp", "h++"]),
        ("C", vec!["c", "h"]),
        ("C#", vec!["cs"]),
        ("Go", vec!["go"]),
        ("Rust", vec!["rs"]),
    ];

    println!("🦀 NekoCode Rust - Supported Languages:");
    for (name, extensions) in languages {
        println!("  {} ({})", name, extensions.join(", "));
    }
    Ok(())
}
