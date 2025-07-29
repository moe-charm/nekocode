// シンプルなネスト関数テスト（10行）
// 期待値: outer + inner + deepNested = 3関数
export function outer() {
    function inner() {
        const deepNested = () => {
            console.log("Deep nested");
        };
        deepNested();
    }
    inner();
}