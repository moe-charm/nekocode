//=============================================================================
// 🦀 NekoCode Rust - Session Management
//
// Analysis session creation, management, and querying
//=============================================================================

use crate::types::AnalysisResult;
use dashmap::DashMap;
use std::sync::Arc;
use uuid::Uuid;

pub struct SessionManager {
    sessions: Arc<DashMap<String, AnalysisSession>>,
}

#[derive(Debug, Clone)]
pub struct AnalysisSession {
    pub id: String,
    pub results: Vec<AnalysisResult>,
    pub created_at: std::time::SystemTime,
}

impl SessionManager {
    pub fn new() -> Self {
        Self {
            sessions: Arc::new(DashMap::new()),
        }
    }

    pub fn create_session(&self) -> String {
        let session_id = Uuid::new_v4().to_string();
        let session = AnalysisSession {
            id: session_id.clone(),
            results: Vec::new(),
            created_at: std::time::SystemTime::now(),
        };
        self.sessions.insert(session_id.clone(), session);
        session_id
    }

    pub fn get_session(&self, session_id: &str) -> Option<AnalysisSession> {
        self.sessions.get(session_id).map(|entry| entry.clone())
    }

    pub fn add_result(&self, session_id: &str, result: AnalysisResult) -> Result<(), String> {
        if let Some(mut session) = self.sessions.get_mut(session_id) {
            session.results.push(result);
            Ok(())
        } else {
            Err(format!("Session {} not found", session_id))
        }
    }
}