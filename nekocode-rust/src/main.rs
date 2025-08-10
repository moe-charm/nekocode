mod core;
mod analyzers;

use anyhow::Result;
use clap::{Parser, Subcommand};
use std::path::PathBuf;

use crate::core::session::AnalysisSession;

#[derive(Parser)]
#[command(name = "nekocode-rust")]
#[command(about = "🦀 NekoCode Rust - High-performance code analysis tool")]
#[command(version = "1.0.0")]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

#[derive(Subcommand)]
enum Commands {
    /// Analyze source code files
    Analyze {
        /// Path to analyze (file or directory)
        #[arg(value_name = "PATH")]
        path: PathBuf,
        
        /// Output format
        #[arg(short, long, default_value = "json")]
        format: String,
        
        /// Enable verbose output
        #[arg(short, long)]
        verbose: bool,
        
        /// Include test files
        #[arg(long)]
        include_tests: bool,
    },
}

#[tokio::main]
async fn main() -> Result<()> {
    env_logger::init();
    
    let cli = Cli::parse();
    
    match cli.command {
        Commands::Analyze { path, format, verbose, include_tests } => {
            let mut session = AnalysisSession::new();
            
            if verbose {
                println!("🦀 NekoCode Rust Analysis Starting...");
                println!("📂 Target: {}", path.display());
            }
            
            let result = session.analyze_path(&path, include_tests).await?;
            
            match format.as_str() {
                "json" => {
                    let json = serde_json::to_string_pretty(&result)?;
                    println!("{}", json);
                }
                _ => {
                    anyhow::bail!("Unsupported output format: {}", format);
                }
            }
            
            if verbose {
                println!("✅ Analysis completed!");
            }
        }
    }
    
    Ok(())
}