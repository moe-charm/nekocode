/**
 * 🎯 ネスト関数テスト専用ファイル - 無限ネストアタック検証用
 * 期待値: 21個の関数
 * - クラスメソッド: 3個 (regularMethod, complexMethod, asyncMethodWithNesting)
 * - ネスト関数: 13個
 * - クラス外関数: 4個 (outerFunction, outerNested1, tripleNested, outerNestedArrow)
 * - async関数: 3個
 */

export class NestedFunctionTester {
    // 通常のメソッド
    public regularMethod(): void {
        console.log("Regular method");
        
        // 🎯 ネストした関数1
        function nestedFunction1() {
            console.log("Nested function 1");
            
            // 🎯 ネストした関数2（さらに深い）
            function deeplyNestedFunction() {
                console.log("Deeply nested function");
                
                // 🎯 ネストしたアロー関数
                const nestedArrow = () => {
                    console.log("Nested arrow in deep function");
                };
                nestedArrow();
            }
            
            // 🎯 ネストしたアロー関数1
            const nestedArrow1 = (x: number) => {
                console.log("Nested arrow 1:", x);
                
                // 🎯 さらにネストしたアロー関数
                const doubleNested = () => {
                    console.log("Double nested arrow");
                };
                doubleNested();
            };
            
            deeplyNestedFunction();
            nestedArrow1(42);
        }
        
        // 🎯 ネストした関数式
        const nestedFunctionExpression = function() {
            console.log("Nested function expression");
        };
        
        nestedFunction1();
        nestedFunctionExpression();
    }
    
    public complexMethod(): void {
        // 🎯 ネストした関数の中にif文
        function complexNested() {
            if (true) {
                // ここには関数はない（制御フロー）
                console.log("Inside if");
                
                // 🎯 でもここにはネスト関数がある
                function ifNestedFunction() {
                    console.log("Function inside if block");
                }
                ifNestedFunction();
            }
            
            // 🎯 for文の中にもネスト関数
            for (let i = 0; i < 1; i++) {
                function forLoopNested() {
                    console.log("Function inside for loop");
                }
                forLoopNested();
            }
        }
        
        complexNested();
    }
    
    public async asyncMethodWithNesting(): Promise<void> {
        // 🎯 async関数の中のネスト関数
        async function nestedAsync() {
            await new Promise(resolve => setTimeout(resolve, 100));
            
            // 🎯 ネストしたasync arrow関数
            const nestedAsyncArrow = async () => {
                await new Promise(resolve => setTimeout(resolve, 50));
                console.log("Nested async arrow");
            };
            
            await nestedAsyncArrow();
        }
        
        await nestedAsync();
    }
}

// 🎯 クラス外のネスト関数
export function outerFunction() {
    console.log("Outer function");
    
    // 🎯 ネスト関数1
    function outerNested1() {
        console.log("Outer nested 1");
        
        // 🎯 さらにネスト
        function tripleNested() {
            console.log("Triple nested");
        }
        tripleNested();
    }
    
    // 🎯 ネストアロー関数
    const outerNestedArrow = () => {
        console.log("Outer nested arrow");
    };
    
    outerNested1();
    outerNestedArrow();
}