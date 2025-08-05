using System;
using System.Collections.Generic;  // 未使用using
using System.Linq;                 // 未使用using

namespace TestProject
{
    class Program
    {
        // 使用される関数
        static string UsedFunction()
        {
            return "used";
        }
        
        // 未使用の関数1
        static string UnusedFunction1()
        {
            return "unused1";
        }
        
        // 未使用の関数2
        static void UnusedFunction2()
        {
            Console.WriteLine("never called");
        }
        
        // 未使用の変数
        private static string unusedField1 = "not used";
        private static int unusedField2 = 42;
        
        // 使用される変数
        private static string usedField = "used";
        
        // 未使用のクラス
        class UnusedClass
        {
            public string Value { get; set; }
            
            // 未使用のメソッド
            public void UnusedMethod()
            {
                Console.WriteLine("unused method");
            }
        }
        
        // 使用されるクラス
        class UsedClass
        {
            public string Value { get; set; }
        }

        static void Main(string[] args)
        {
            string result = UsedFunction();
            Console.WriteLine(result + " " + usedField);
            
            // UsedClassを使用
            var used = new UsedClass { Value = "test" };
            Console.WriteLine(used.Value);
        }
    }
}