# シンプルなタスク：既存のregex結果からAST構築

## やること（30分で完了）

### 1. Python AST (5分)
```rust
// src/analyzers/python/analyzer.rs に追加
fn build_python_ast(&self, functions: &[FunctionInfo], classes: &[ClassInfo]) -> ASTNode {
    let mut builder = ASTBuilder::new();
    
    for class in classes {
        builder.enter_scope(ASTNodeType::Class, class.name.clone(), class.start_line);
        for method in &class.methods {
            builder.add_node(ASTNodeType::Method, method.name.clone(), method.start_line);
        }
        builder.exit_scope(class.end_line);
    }
    
    for func in functions {
        builder.add_node(ASTNodeType::Function, func.name.clone(), func.start_line);
    }
    
    builder.build()
}
```

### 2. 同じパターンで他言語も（各5分）
- C++: regex結果 → AST
- C#: regex結果 → AST  
- Go: regex結果 → AST
- Rust: regex結果 → AST

## 注意
- PEST文法は触らない
- 新規パーサー作らない
- regex結果は既に正確

完了！