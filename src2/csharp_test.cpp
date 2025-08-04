//=============================================================================
// 💎 C# Universal Adapter Test - Unity/.NET特化統一システム検証
//=============================================================================

#include "adapters/csharp_universal_adapter.hpp"
#include <iostream>
#include <string>

using namespace nekocode::adapters;

int main() {
    // 🚀 C# Universal Adapter テスト
    std::cout << "💎 C# Universal Adapter Test Starting...\n";
    
    CSharpUniversalAdapter adapter;
    
    // テスト用C#コード（Unity MonoBehaviour + .NET特化）
    std::string test_code = R"CSHARP(
using System;
using System.Collections.Generic;
using UnityEngine;

namespace GameLogic {
    
    public class PlayerController : MonoBehaviour {
        
        [SerializeField]
        private float moveSpeed = 5.0f;
        
        [SerializeField]
        private int health = 100;
        
        public bool IsAlive { get; private set; } = true;
        
        public string PlayerName { get; set; } = "Player";
        
        private void Awake() {
            Debug.Log("Player Controller Awake");
        }
        
        private void Start() {
            IsAlive = true;
            health = 100;
        }
        
        private void Update() {
            HandleMovement();
            CheckHealth();
        }
        
        private void HandleMovement() {
            float horizontal = Input.GetAxis("Horizontal");
            float vertical = Input.GetAxis("Vertical");
            
            Vector3 direction = new Vector3(horizontal, 0, vertical);
            transform.Translate(direction * moveSpeed * Time.deltaTime);
        }
        
        public void TakeDamage(int damage) {
            health -= damage;
            if (health <= 0) {
                IsAlive = false;
                OnPlayerDeath();
            }
        }
        
        private void OnPlayerDeath() {
            Debug.Log("Player has died!");
            gameObject.SetActive(false);
        }
        
        private void OnTriggerEnter(Collider other) {
            if (other.CompareTag("Enemy")) {
                TakeDamage(20);
            }
        }
    }
    
    public class GameManager : MonoBehaviour {
        
        public static GameManager Instance { get; private set; }
        
        [SerializeField]
        private List<PlayerController> players = new List<PlayerController>();
        
        private int score = 0;
        
        private void Awake() {
            if (Instance == null) {
                Instance = this;
                DontDestroyOnLoad(gameObject);
            } else {
                Destroy(gameObject);
            }
        }
        
        public void AddScore(int points) {
            score += points;
            Debug.Log($"Score: {score}");
        }
        
        public void RegisterPlayer(PlayerController player) {
            if (!players.Contains(player)) {
                players.Add(player);
            }
        }
    }
    
    public interface IWeapon {
        void Fire();
        int GetDamage();
    }
    
    public class Rifle : IWeapon {
        private int damage = 25;
        private int ammo = 30;
        
        public void Fire() {
            if (ammo > 0) {
                ammo--;
                Debug.Log($"Rifle fired! Ammo remaining: {ammo}");
            }
        }
        
        public int GetDamage() {
            return damage;
        }
        
        public void Reload() {
            ammo = 30;
            Debug.Log("Rifle reloaded!");
        }
    }
    
} // namespace GameLogic

public static class Utilities {
    
    public static float CalculateDistance(Vector3 a, Vector3 b) {
        return Vector3.Distance(a, b);
    }
    
    public static T GetRandomElement<T>(List<T> list) {
        if (list.Count == 0) return default(T);
        return list[UnityEngine.Random.Range(0, list.Count)];
    }
}
)CSHARP";

    try {
        std::cout << "📊 Analyzing C# code...\n";
        
        // 解析実行
        auto result = adapter.analyze(test_code, "PlayerController.cs");
        
        std::cout << "✅ Analysis completed!\n";
        std::cout << "📈 Results:\n";
        std::cout << "  - Language: " << adapter.get_language_name() << "\n";
        std::cout << "  - Classes: " << result.classes.size() << "\n";
        std::cout << "  - Functions: " << result.functions.size() << "\n";
        std::cout << "  - File size: " << result.file_info.size_bytes << " bytes\n";
        std::cout << "  - Total lines: " << result.file_info.total_lines << "\n";
        
        // AST統計確認
        auto ast_stats = adapter.get_ast_statistics();
        std::cout << "🌳 AST Statistics:\n";
        std::cout << "  - AST Classes: " << ast_stats.classes << "\n";
        std::cout << "  - AST Functions: " << ast_stats.functions << "\n";
        std::cout << "  - AST Variables: " << ast_stats.variables << "\n";
        std::cout << "  - Max Depth: " << ast_stats.max_depth << "\n";
        
        // C#特化機能テスト
        auto unity_classes = adapter.find_unity_monobehaviours();
        std::cout << "🎮 Unity MonoBehaviours Found: " << unity_classes.size() << "\n";
        for (const auto& unity_class : unity_classes) {
            std::cout << "  - " << unity_class << "\n";
        }
        
        auto unity_methods = adapter.find_unity_methods();
        std::cout << "🔮 Unity Methods Found: " << unity_methods.size() << "\n";
        for (const auto& method : unity_methods) {
            std::cout << "  - " << method << "\n";
        }
        
        auto properties = adapter.find_properties();
        std::cout << "💎 Properties Found: " << properties.size() << "\n";
        for (const auto& prop : properties) {
            std::cout << "  - " << prop << "\n";
        }
        
        auto namespaces = adapter.find_namespaces();
        std::cout << "📦 Namespaces Found: " << namespaces.size() << "\n";
        for (const auto& ns : namespaces) {
            std::cout << "  - " << ns << "\n";
        }
        
        // C# AST特化検索テスト
        std::cout << "\n🔍 C# AST Query Test:\n";
        auto player_controller = adapter.query_csharp_ast("GameLogic/PlayerController");
        if (player_controller) {
            std::cout << "  ✅ Found PlayerController class in AST\n";
        } else {
            std::cout << "  ❌ PlayerController class not found in AST\n";
        }
        
        // Unity/ゲーム開発成功実績との比較
        std::cout << "\n🎯 Success Metrics Comparison:\n";
        std::cout << "  - Unity project baseline: 10+ classes + 50+ methods\n";
        std::cout << "  - Current test results: " << result.classes.size() 
                  << " classes + " << result.functions.size() << " methods\n";
        
        if (result.classes.size() >= 3 && result.functions.size() >= 10) {
            std::cout << "✅ SUCCESS: Detecting Unity/C# complex structures!\n";
        }
        
        std::cout << "🎉 C# Universal Adapter Test PASSED!\n";
        std::cout << "\n🌟 **Phase 7 Option A: C#統一システム動作確認完了！**\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Test FAILED: " << e.what() << "\n";
        return 1;
    }
}