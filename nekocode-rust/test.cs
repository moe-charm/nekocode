using System;

namespace TestNamespace {
    public class TestClass {
        public TestClass() {
            Console.WriteLine("Constructor");
        }
        
        public async Task<int> AsyncMethod() {
            return 42;
        }
    }
}
