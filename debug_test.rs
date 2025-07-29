/**
 * 🦀 Rust Complex Test Sample - trait・impl・macro・async テスト
 */

use std::collections::HashMap;
use std::sync::{Arc, Mutex};
use async_trait::async_trait;
use tokio::sync::RwLock;
use serde::{Serialize, Deserialize};

// 🎨 マクロ定義
macro_rules! log_debug {
    ($($arg:tt)*) => {
        #[cfg(debug_assertions)]
        println!("[DEBUG] {}", format!($($arg)*));
    };
}

macro_rules! create_struct {
    ($name:ident { $($field:ident: $ty:ty),* }) => {
        #[derive(Debug, Clone)]
        pub struct $name {
            $(pub $field: $ty),*
        }
    };
}

// 🏗️ trait定義
trait DataProcessor {
    fn process(&self, data: &str) -> Result<String, ProcessError>;
    fn validate(&self, data: &str) -> bool;
}

#[async_trait]
trait AsyncDataProcessor: Send + Sync {
    async fn process_async(&self, data: &str) -> Result<String, ProcessError>;
    async fn batch_process(&self, items: Vec<String>) -> Vec<Result<String, ProcessError>>;
}

// 🔧 ジェネリックtrait
trait Repository<T: Entity> {
    fn find(&self, id: u64) -> Option<&T>;
    fn find_all(&self) -> Vec<&T>;
    fn save(&mut self, entity: T) -> Result<(), RepositoryError>;
    fn delete(&mut self, id: u64) -> Result<(), RepositoryError>;
}

// 🎯 Entity trait
trait Entity: Clone + Send + Sync {
    fn id(&self) -> u64;
    fn created_at(&self) -> chrono::DateTime<chrono::Utc>;
}

// エラー型定義
#[derive(Debug, Clone)]
enum ProcessError {
    ValidationError(String),
    ProcessingError(String),
    Unknown,
}

#[derive(Debug, Clone)]
enum RepositoryError {
    NotFound,
    AlreadyExists,
    DatabaseError(String),
}

// 🌟 構造体定義（マクロ使用）
create_struct!(User {
    id: u64,
    name: String,
    email: String,
    active: bool
});

// 🔧 Entity実装
impl Entity for User {
    fn id(&self) -> u64 {
        self.id
    }
    
    fn created_at(&self) -> chrono::DateTime<chrono::Utc> {
        chrono::Utc::now()
    }
}

// 🎮 ジェネリック構造体
pub struct InMemoryRepository<T: Entity> {
    storage: Arc<RwLock<HashMap<u64, T>>>,
    next_id: Arc<Mutex<u64>>,
}

impl<T: Entity> InMemoryRepository<T> {
    pub fn new() -> Self {
        Self {
            storage: Arc::new(RwLock::new(HashMap::new())),
            next_id: Arc::new(Mutex::new(1)),
        }
    }
    
    async fn get_next_id(&self) -> u64 {
        let mut id = self.next_id.lock().unwrap();
        let current = *id;
        *id += 1;
        current
    }
}

// 🏛️ Repository trait実装
impl<T: Entity + 'static> Repository<T> for InMemoryRepository<T> {
    fn find(&self, id: u64) -> Option<&T> {
        // 簡略実装（実際は非同期版を使うべき）
        None
    }
    
    fn find_all(&self) -> Vec<&T> {
        vec![]
    }
    
    fn save(&mut self, entity: T) -> Result<(), RepositoryError> {
        // 複雑な条件分岐
        if entity.id() == 0 {
            return Err(RepositoryError::DatabaseError("Invalid ID".to_string()));
        } else if entity.id() > 1000000 {
            return Err(RepositoryError::DatabaseError("ID too large".to_string()));
        } else {
            log_debug!("Saving entity with ID: {}", entity.id());
            Ok(())
        }
    }
    
    fn delete(&mut self, id: u64) -> Result<(), RepositoryError> {
        if id == 0 {
            Err(RepositoryError::NotFound)
        } else {
            Ok(())
        }
    }
}

// 🌟 DataProcessor実装
pub struct SimpleProcessor {
    prefix: String,
    suffix: String,
}

impl SimpleProcessor {
    pub fn new(prefix: &str, suffix: &str) -> Self {
        Self {
            prefix: prefix.to_string(),
            suffix: suffix.to_string(),
        }
    }
    
    fn internal_process(&self, data: &str) -> String {
        format!("{}{}{}", self.prefix, data, self.suffix)
    }
}

impl DataProcessor for SimpleProcessor {
    fn process(&self, data: &str) -> Result<String, ProcessError> {
        if self.validate(data) {
            Ok(self.internal_process(data))
        } else {
            Err(ProcessError::ValidationError("Invalid data".to_string()))
        }
    }
    
    fn validate(&self, data: &str) -> bool {
        !data.is_empty() && data.len() < 1000
    }
}

// 🔄 AsyncDataProcessor実装
#[async_trait]
impl AsyncDataProcessor for SimpleProcessor {
    async fn process_async(&self, data: &str) -> Result<String, ProcessError> {
        tokio::time::sleep(tokio::time::Duration::from_millis(10)).await;
        self.process(data)
    }
    
    async fn batch_process(&self, items: Vec<String>) -> Vec<Result<String, ProcessError>> {
        let mut results = Vec::new();
        
        for item in items {
            let result = self.process_async(&item).await;
            results.push(result);
        }
        
        results
    }
}

// 🎯 高度なジェネリック関数
pub fn pipe<T, F, G>(f: F, g: G) -> impl Fn(T) -> T
where
    F: Fn(T) -> T,
    G: Fn(T) -> T,
{
    move |x| g(f(x))
}

pub async fn concurrent_map<T, U, F, Fut>(items: Vec<T>, f: F) -> Vec<U>
where
    T: Send + 'static,
    U: Send + 'static,
    F: Fn(T) -> Fut + Send + Sync + 'static,
    Fut: std::future::Future<Output = U> + Send,
{
    let tasks: Vec<_> = items
        .into_iter()
        .map(|item| {
            let f = &f;
            tokio::spawn(async move { f(item).await })
        })
        .collect();
    
    let mut results = Vec::new();
    for task in tasks {
        if let Ok(result) = task.await {
            results.push(result);
        }
    }
    
    results
}

// 🚀 ライフタイム付き構造体
#[derive(Debug)]
pub struct BorrowedData<'a, T> {
    data: &'a T,
    metadata: HashMap<String, String>,
}

impl<'a, T: std::fmt::Debug> BorrowedData<'a, T> {
    pub fn new(data: &'a T) -> Self {
        Self {
            data,
            metadata: HashMap::new(),
        }
    }
    
    pub fn with_metadata(mut self, key: &str, value: &str) -> Self {
        self.metadata.insert(key.to_string(), value.to_string());
        self
    }
    
    pub fn get_data(&self) -> &T {
        self.data
    }
}

// 🏛️ module定義
mod utils {
    use super::*;
    
    pub fn validate_email(email: &str) -> bool {
        email.contains('@') && email.contains('.')
    }
    
    pub async fn fetch_user_data(id: u64) -> Result<String, ProcessError> {
        tokio::time::sleep(tokio::time::Duration::from_millis(100)).await;
        
        match id {
            0 => Err(ProcessError::ValidationError("Invalid ID".to_string())),
            1..=100 => Ok(format!("User data for ID: {}", id)),
            _ => Err(ProcessError::Unknown),
        }
    }
}

// 🎮 テスト
#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_simple_processor() {
        let processor = SimpleProcessor::new("[", "]");
        let result = processor.process("test").unwrap();
        assert_eq!(result, "[test]");
    }
    
    #[tokio::test]
    async fn test_async_processor() {
        let processor = SimpleProcessor::new("<", ">");
        let result = processor.process_async("async").await.unwrap();
        assert_eq!(result, "<async>");
    }
    
    #[test]
    fn test_repository() {
        let mut repo = InMemoryRepository::<User>::new();
        let user = User {
            id: 1,
            name: "Test".to_string(),
            email: "test@example.com".to_string(),
            active: true,
        };
        
        assert!(repo.save(user).is_ok());
    }
}

// メイン関数
#[tokio::main]
async fn main() {
    println!("🦀 Rust Complex Test Sample");
    
    // プロセッサーテスト
    let processor = SimpleProcessor::new(">>> ", " <<<");
    match processor.process("Hello Rust") {
        Ok(result) => println!("Processed: {}", result),
        Err(e) => eprintln!("Error: {:?}", e),
    }
    
    // 非同期処理
    let async_result = processor.process_async("Async Rust").await;
    println!("Async result: {:?}", async_result);
    
    // ユーティリティ関数
    let email = "test@example.com";
    if utils::validate_email(email) {
        println!("Valid email: {}", email);
    }
    
    // ジェネリック関数
    let add_one = |x: i32| x + 1;
    let multiply_two = |x: i32| x * 2;
    let composed = pipe(add_one, multiply_two);
    println!("Composed function: {} -> {}", 5, composed(5));
}