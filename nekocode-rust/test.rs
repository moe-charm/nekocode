struct TestStruct {
    value: i32,
}

impl TestStruct {
    fn new(value: i32) -> Self {
        Self { value }
    }
    
    async fn async_method(&self) -> i32 {
        self.value
    }
}

fn main() {
    println\!("Hello Rust\!");
}
