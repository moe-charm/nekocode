#pragma once

//=============================================================================
// 🧩 Unity Analyzer Components - コンポジション設計の機能部品
//
// 各機能を独立したクラスとして実装
// 単一責任原則に従い、テストしやすく、再利用可能
//=============================================================================

#include "base_analyzer.hpp"
#include "unity_patterns.hpp"
#include <unordered_set>
#include <sstream>
#include <algorithm>

namespace nekocode {
namespace unity {

//=============================================================================
// 🎯 Unity パターン検出器
//=============================================================================

class UnityPatternDetector {
public:
    UnityPatternDetector() = default;
    
    // メイン処理: Unity 固有パターンの検出と結果拡張
    void enhance_analysis(AnalysisResult& result, const std::string& content) {
        detect_unity_class_types(result, content);
        classify_lifecycle_methods(result);
        detect_unity_attributes(result, content);
        detect_coroutines(result, content);
        detect_unity_member_variables(result, content);  // 🎮 Unity特化メンバ変数検出
        calculate_unity_statistics(result);
    }
    
private:
    // Unity クラスタイプの検出（MonoBehaviour, ScriptableObject等）
    void detect_unity_class_types(AnalysisResult& result, const std::string& content) {
        detect_monobehaviour_classes(result, content);
        detect_scriptableobject_classes(result, content);
        detect_editor_classes(result, content);
    }
    
    void detect_monobehaviour_classes(AnalysisResult& result, const std::string& content) {
        // MonoBehaviour 継承の検出（文字列ベース）
        size_t pos = 0;
        while ((pos = content.find(": MonoBehaviour", pos)) != std::string::npos) {
            // クラス名を逆方向に探す
            size_t class_start = content.rfind("class ", pos);
            if (class_start != std::string::npos) {
                size_t name_start = class_start + 6; // "class " の長さ
                size_t name_end = content.find_first_of(" \t\n\r:", name_start);
                if (name_end != std::string::npos && name_end < pos) {
                    std::string class_name = content.substr(name_start, name_end - name_start);
                    
                    // 既存のクラス情報を拡張
                    for (auto& cls : result.classes) {
                        if (cls.name == class_name) {
                            cls.metadata["unity_type"] = "MonoBehaviour";
                            cls.metadata["is_monobehaviour"] = "true";
                            break;
                        }
                    }
                }
            }
            pos++;
        }
    }
    
    void detect_scriptableobject_classes(AnalysisResult& result, const std::string& content) {
        // ScriptableObject 継承の検出（文字列ベース）
        size_t pos = 0;
        while ((pos = content.find(": ScriptableObject", pos)) != std::string::npos) {
            // クラス名を逆方向に探す
            size_t class_start = content.rfind("class ", pos);
            if (class_start != std::string::npos) {
                size_t name_start = class_start + 6; // "class " の長さ
                size_t name_end = content.find_first_of(" \t\n\r:", name_start);
                if (name_end != std::string::npos && name_end < pos) {
                    std::string class_name = content.substr(name_start, name_end - name_start);
                    
                    for (auto& cls : result.classes) {
                        if (cls.name == class_name) {
                            cls.metadata["unity_type"] = "ScriptableObject";
                            cls.metadata["is_scriptableobject"] = "true";
                            break;
                        }
                    }
                }
            }
            pos++;
        }
    }
    
    void detect_editor_classes(AnalysisResult& result, const std::string& content) {
        // Editor クラス継承の検出（文字列ベース）
        std::vector<std::string> editor_types = {": Editor", ": EditorWindow", ": PropertyDrawer"};
        
        for (const auto& editor_type : editor_types) {
            size_t pos = 0;
            while ((pos = content.find(editor_type, pos)) != std::string::npos) {
                // クラス名を逆方向に探す
                size_t class_start = content.rfind("class ", pos);
                if (class_start != std::string::npos) {
                    size_t name_start = class_start + 6; // "class " の長さ
                    size_t name_end = content.find_first_of(" \t\n\r:", name_start);
                    if (name_end != std::string::npos && name_end < pos) {
                        std::string class_name = content.substr(name_start, name_end - name_start);
                        
                        for (auto& cls : result.classes) {
                            if (cls.name == class_name) {
                                cls.metadata["unity_type"] = "Editor";
                                cls.metadata["is_editor"] = "true";
                                break;
                            }
                        }
                    }
                }
                pos++;
            }
        }
    }
    
    // ライフサイクルメソッドの分類
    void classify_lifecycle_methods(AnalysisResult& result) {
        for (auto& func : result.functions) {
            // MonoBehaviour ライフサイクルチェック
            auto it = std::find(MONOBEHAVIOUR_LIFECYCLE.begin(), 
                              MONOBEHAVIOUR_LIFECYCLE.end(), 
                              func.name);
            if (it != MONOBEHAVIOUR_LIFECYCLE.end()) {
                func.metadata["method_type"] = "lifecycle";
                func.metadata["lifecycle_order"] = std::to_string(
                    std::distance(MONOBEHAVIOUR_LIFECYCLE.begin(), it));
            }
            
            // 物理イベントチェック
            if (std::find(PHYSICS_EVENTS.begin(), PHYSICS_EVENTS.end(), func.name) 
                != PHYSICS_EVENTS.end()) {
                func.metadata["method_type"] = "physics_event";
            }
            
            // レンダリングイベントチェック
            if (std::find(RENDER_EVENTS.begin(), RENDER_EVENTS.end(), func.name) 
                != RENDER_EVENTS.end()) {
                func.metadata["method_type"] = "render_event";
            }
        }
    }
    
    // Unity 属性の検出
    void detect_unity_attributes(AnalysisResult& result, const std::string& content) {
        // Unity 属性の文字列ベース検出（unity_patterns.hpp の定数を使用）
        for (const auto& attr : UNITY_ATTRIBUTES) {
            size_t pos = 0;
            int count = 0;
            while ((pos = content.find(attr, pos)) != std::string::npos) {
                count++;
                pos += attr.length();
            }
            
            if (count > 0) {
                // 属性名を抽出（[]を除去）
                std::string attr_name = attr.substr(1, attr.length() - 2);
                result.metadata["unity_attribute_" + attr_name] = std::to_string(count);
            }
        }
    }
    
    // コルーチンの検出
    void detect_coroutines(AnalysisResult& result, const std::string& content) {
        for (auto& func : result.functions) {
            // IEnumerator を戻り値とする関数をコルーチンとして分類
            if (content.find("IEnumerator " + func.name) != std::string::npos) {
                func.metadata["method_type"] = "coroutine";
            }
        }
    }
    
    // 🎮 Unity特化メンバ変数検出
    void detect_unity_member_variables(AnalysisResult& result, const std::string& content) {
        std::istringstream stream(content);
        std::string line;
        size_t line_number = 1;
        
        // Unity属性付きフィールドの検出パターン
        std::vector<std::string> unity_field_attributes = {
            "[SerializeField]", "[Header", "[Range", "[Tooltip", "[Space", 
            "[TextArea", "[Multiline", "[RequireComponent"
        };
        
        bool in_class = false;
        std::string current_class_name;
        std::vector<std::string> pending_attributes;  // 次のフィールドに適用される属性
        
        while (std::getline(stream, line)) {
            // クラス開始検出
            if (line.find("class ") != std::string::npos && 
                (line.find(": MonoBehaviour") != std::string::npos ||
                 line.find(": ScriptableObject") != std::string::npos ||
                 line.find(": Editor") != std::string::npos)) {
                in_class = true;
                
                // クラス名抽出
                size_t class_pos = line.find("class ");
                if (class_pos != std::string::npos) {
                    size_t name_start = class_pos + 6;
                    size_t name_end = line.find_first_of(" \t:", name_start);
                    if (name_end != std::string::npos) {
                        current_class_name = line.substr(name_start, name_end - name_start);
                    }
                }
            }
            
            // クラス終了検出（簡易版）
            if (in_class && line.find("}") != std::string::npos && 
                line.find("class") == std::string::npos) {
                // メソッド内の}は除外（完全ではないが実用的）
                size_t brace_pos = line.find("}");
                if (brace_pos == 0 || line.substr(0, brace_pos).find_first_not_of(" \t") == std::string::npos) {
                    in_class = false;
                    current_class_name.clear();
                    pending_attributes.clear();
                }
            }
            
            if (in_class) {
                // Unity属性の検出と蓄積
                for (const auto& attr : unity_field_attributes) {
                    if (line.find(attr) != std::string::npos) {
                        // 属性名を抽出
                        size_t attr_start = line.find(attr);
                        size_t attr_end = line.find("]", attr_start);
                        if (attr_end != std::string::npos) {
                            std::string full_attribute = line.substr(attr_start, attr_end - attr_start + 1);
                            pending_attributes.push_back(full_attribute);
                        }
                        break;
                    }
                }
                
                // Unity型フィールドの検出
                if (detect_unity_field_line(line)) {
                    MemberVariable member_var = parse_unity_field(line, line_number);
                    
                    // Unity属性を適用
                    if (!pending_attributes.empty()) {
                        member_var.access_modifier = "serialized";  // Unity特化アクセス修飾子
                        // Unity属性をメタデータに追加
                        for (size_t i = 0; i < pending_attributes.size(); ++i) {
                            member_var.metadata["unity_attribute_" + std::to_string(i)] = pending_attributes[i];
                        }
                        pending_attributes.clear();
                    }
                    
                    // Unity型分類
                    std::string unity_type_category = classify_unity_type(member_var.type);
                    if (!unity_type_category.empty()) {
                        member_var.metadata["unity_type_category"] = unity_type_category;
                    }
                    
                    // 対応するクラスに追加
                    add_member_to_class(result, current_class_name, member_var);
                }
            }
            
            line_number++;
        }
    }
    
    // Unity フィールド行の検出
    bool detect_unity_field_line(const std::string& line) {
        // Unity特有の型を含むフィールド検出
        std::vector<std::string> unity_types = {
            "GameObject", "Transform", "Rigidbody", "Collider", "AudioSource",
            "Button", "Text", "Image", "Slider", "Canvas",
            "Vector3", "Vector2", "Quaternion", "Color", 
            "LayerMask", "AnimationCurve", "Gradient"
        };
        
        // 基本的なC#フィールドパターン + Unity型
        bool has_access_modifier = (line.find("public ") != std::string::npos ||
                                   line.find("private ") != std::string::npos ||
                                   line.find("protected ") != std::string::npos ||
                                   line.find("internal ") != std::string::npos);
        
        bool has_unity_type = false;
        for (const auto& type : unity_types) {
            if (line.find(type) != std::string::npos) {
                has_unity_type = true;
                break;
            }
        }
        
        // セミコロンで終わるフィールド宣言
        bool is_field_declaration = (line.find(";") != std::string::npos &&
                                    line.find("(") == std::string::npos);  // メソッドではない
        
        return is_field_declaration && (has_access_modifier || has_unity_type);
    }
    
    // Unity フィールドのパース
    MemberVariable parse_unity_field(const std::string& line, size_t line_number) {
        MemberVariable member_var;
        member_var.declaration_line = line_number;
        
        // アクセス修飾子の検出
        if (line.find("public ") != std::string::npos) {
            member_var.access_modifier = "public";
        } else if (line.find("private ") != std::string::npos) {
            member_var.access_modifier = "private";
        } else if (line.find("protected ") != std::string::npos) {
            member_var.access_modifier = "protected";
        } else {
            member_var.access_modifier = "private";  // C# default
        }
        
        // static/const修飾子
        member_var.is_static = (line.find("static ") != std::string::npos);
        member_var.is_const = (line.find("const ") != std::string::npos || 
                              line.find("readonly ") != std::string::npos);
        
        // 型と変数名の抽出（簡易版）
        size_t semicolon_pos = line.find(";");
        if (semicolon_pos != std::string::npos) {
            std::string declaration = line.substr(0, semicolon_pos);
            
            // 最後の単語が変数名、その前が型
            size_t last_space = declaration.find_last_of(" \t");
            if (last_space != std::string::npos) {
                member_var.name = declaration.substr(last_space + 1);
                
                // 型を抽出（アクセス修飾子以降）
                size_t type_start = 0;
                if (declaration.find("public ") != std::string::npos) type_start = declaration.find("public ") + 7;
                else if (declaration.find("private ") != std::string::npos) type_start = declaration.find("private ") + 8;
                else if (declaration.find("protected ") != std::string::npos) type_start = declaration.find("protected ") + 10;
                
                if (declaration.find("static ") != std::string::npos) {
                    size_t static_pos = declaration.find("static ");
                    if (static_pos > type_start) type_start = static_pos + 7;
                }
                
                if (last_space > type_start) {
                    member_var.type = declaration.substr(type_start, last_space - type_start);
                    // 前後の空白を削除
                    size_t start = member_var.type.find_first_not_of(" \t");
                    size_t end = member_var.type.find_last_not_of(" \t");
                    if (start != std::string::npos && end != std::string::npos) {
                        member_var.type = member_var.type.substr(start, end - start + 1);
                    }
                }
            }
        }
        
        return member_var;
    }
    
    // Unity型の分類
    std::string classify_unity_type(const std::string& type) {
        if (type == "GameObject" || type == "Transform") return "core_component";
        if (type == "Rigidbody" || type == "Collider") return "physics";
        if (type == "AudioSource") return "audio";
        if (type.find("UI.") != std::string::npos || type == "Button" || type == "Text" || type == "Image") return "ui";
        if (type == "Vector3" || type == "Vector2" || type == "Quaternion") return "math";
        if (type == "Color" || type == "LayerMask") return "utility";
        return "";
    }
    
    // クラスにメンバ変数を追加
    void add_member_to_class(AnalysisResult& result, const std::string& class_name, const MemberVariable& member_var) {
        for (auto& cls : result.classes) {
            if (cls.name == class_name) {
                cls.member_variables.push_back(member_var);
                return;
            }
        }
    }

    // Unity 統計の計算
    void calculate_unity_statistics(AnalysisResult& result) {
        int monobehaviour_count = 0;
        int scriptableobject_count = 0;
        int editor_count = 0;
        int coroutine_count = 0;
        int lifecycle_count = 0;
        
        for (const auto& cls : result.classes) {
            if (cls.metadata.count("is_monobehaviour")) monobehaviour_count++;
            if (cls.metadata.count("is_scriptableobject")) scriptableobject_count++;
            if (cls.metadata.count("is_editor")) editor_count++;
        }
        
        for (const auto& func : result.functions) {
            if (func.metadata.count("method_type")) {
                std::string type = func.metadata.at("method_type");
                if (type == "coroutine") coroutine_count++;
                if (type == "lifecycle") lifecycle_count++;
            }
        }
        
        // 結果にUnity統計を記録
        result.metadata["unity_monobehaviour_count"] = std::to_string(monobehaviour_count);
        result.metadata["unity_scriptableobject_count"] = std::to_string(scriptableobject_count);
        result.metadata["unity_editor_count"] = std::to_string(editor_count);
        result.metadata["unity_coroutine_count"] = std::to_string(coroutine_count);
        result.metadata["unity_lifecycle_count"] = std::to_string(lifecycle_count);
    }
};

//=============================================================================
// ⚠️ パフォーマンス警告検出器
//=============================================================================

class PerformanceWarningDetector {
public:
    struct PerformanceIssue {
        int line_number;
        std::string issue_type;
        std::string description;
        std::string suggestion;
    };
    
    PerformanceWarningDetector() = default;
    
    // メイン処理: パフォーマンス問題の検出と警告追加
    void add_warnings(AnalysisResult& result, const std::string& content) {
        std::vector<PerformanceIssue> issues;
        
        detect_update_performance_issues(issues, content);
        detect_memory_allocation_issues(issues, content);
        detect_inefficient_patterns(issues, content);
        
        // 警告をメタデータに追加
        add_issues_to_result(result, issues);
    }
    
private:
    // Update メソッド内のパフォーマンス問題
    void detect_update_performance_issues(std::vector<PerformanceIssue>& issues, 
                                        const std::string& content) {
        std::istringstream stream(content);
        std::string line;
        int line_number = 1;
        
        // unity_patterns.hpp の定数を使用
        while (std::getline(stream, line)) {
            for (const auto& pattern : PERFORMANCE_PATTERNS) {
                if (line.find(pattern.search_pattern) != std::string::npos) {
                    // Update系メソッド内かチェック（必要に応じて）
                    bool in_update_method = (line.find("Update") != std::string::npos || 
                                           line.find("FixedUpdate") != std::string::npos ||
                                           line.find("LateUpdate") != std::string::npos);
                    
                    // 全体的なパフォーマンス問題として検出
                    issues.push_back({
                        line_number,
                        pattern.name,
                        pattern.warning_message,
                        pattern.suggestion
                    });
                }
            }
            line_number++;
        }
    }
    
    // メモリアロケーション問題
    void detect_memory_allocation_issues(std::vector<PerformanceIssue>& issues,
                                       const std::string& content) {
        // Update/FixedUpdate/LateUpdate 内の new 演算子検出（文字列ベース）
        bool in_update_method = false;
        std::istringstream stream(content);
        std::string line;
        int line_number = 1;
        
        while (std::getline(stream, line)) {
            // Update系メソッドの開始検出
            if (line.find("Update(") != std::string::npos ||
                line.find("FixedUpdate(") != std::string::npos ||
                line.find("LateUpdate(") != std::string::npos) {
                in_update_method = true;
            }
            
            // メソッド終了の検出（簡易版）
            if (in_update_method && line.find("}") != std::string::npos) {
                in_update_method = false;
            }
            
            // Update系メソッド内でのnew検出
            if (in_update_method && line.find("new ") != std::string::npos) {
                issues.push_back({
                    line_number,
                    "memory_allocation",
                    "Update系メソッド内でのメモリアロケーション",
                    "オブジェクトプールやキャッシュの使用を推奨"
                });
            }
            
            line_number++;
        }
    }
    
    // 非効率なパターン
    void detect_inefficient_patterns(std::vector<PerformanceIssue>& issues,
                                   const std::string& content) {
        // GameObject.Find の頻繁な使用
        if (content.find("GameObject.Find") != std::string::npos) {
            issues.push_back({
                0,
                "frequent_find",
                "GameObject.Find の使用検出",
                "参照をキャッシュすることを推奨"
            });
        }
        
        // GetComponent の頻繁な使用
        size_t getcomponent_count = 0;
        size_t pos = 0;
        while ((pos = content.find("GetComponent", pos)) != std::string::npos) {
            getcomponent_count++;
            pos += 12;
        }
        
        if (getcomponent_count > 5) {  // 閾値は調整可能
            issues.push_back({
                0,
                "frequent_getcomponent",
                "GetComponent の頻繁な使用検出",
                "コンポーネント参照をキャッシュすることを推奨"
            });
        }
    }
    
    // 問題を結果に追加
    void add_issues_to_result(AnalysisResult& result, 
                            const std::vector<PerformanceIssue>& issues) {
        if (!issues.empty()) {
            result.metadata["performance_warnings_count"] = std::to_string(issues.size());
            
            // 各警告の詳細をメタデータに追加
            for (size_t i = 0; i < issues.size(); ++i) {
                const auto& issue = issues[i];
                std::string prefix = "perf_warning_" + std::to_string(i) + "_";
                result.metadata[prefix + "type"] = issue.issue_type;
                result.metadata[prefix + "description"] = issue.description;
                result.metadata[prefix + "suggestion"] = issue.suggestion;
                result.metadata[prefix + "line"] = std::to_string(issue.line_number);
            }
        }
    }
};

//=============================================================================
// 🔄 ライフサイクルメソッド分類器
//=============================================================================

class LifecycleMethodClassifier {
public:
    LifecycleMethodClassifier() = default;
    
    // メイン処理: メソッドのライフサイクル分類
    void classify_methods(AnalysisResult& result) {
        classify_unity_lifecycle(result);
        add_execution_order_info(result);
        detect_lifecycle_patterns(result);
    }
    
private:
    // Unity ライフサイクルの分類
    void classify_unity_lifecycle(AnalysisResult& result) {
        for (auto& func : result.functions) {
            auto it = std::find(MONOBEHAVIOUR_LIFECYCLE.begin(),
                              MONOBEHAVIOUR_LIFECYCLE.end(),
                              func.name);
            
            if (it != MONOBEHAVIOUR_LIFECYCLE.end()) {
                int order = std::distance(MONOBEHAVIOUR_LIFECYCLE.begin(), it);
                func.metadata["lifecycle_method"] = "true";
                func.metadata["execution_order"] = std::to_string(order);
                func.metadata["lifecycle_phase"] = get_lifecycle_phase(func.name);
            }
        }
    }
    
    // 実行順序情報の追加
    void add_execution_order_info(AnalysisResult& result) {
        // ライフサイクルメソッドの実行順序をメタデータに追加
        std::string execution_info = "Lifecycle execution order: ";
        bool first = true;
        
        for (const auto& lifecycle : MONOBEHAVIOUR_LIFECYCLE) {
            for (const auto& func : result.functions) {
                if (func.name == lifecycle) {
                    if (!first) execution_info += " -> ";
                    execution_info += lifecycle;
                    first = false;
                    break;
                }
            }
        }
        
        if (!first) {  // ライフサイクルメソッドが見つかった場合
            result.metadata["lifecycle_execution_order"] = execution_info;
        }
    }
    
    // ライフサイクルパターンの検出
    void detect_lifecycle_patterns(AnalysisResult& result) {
        bool has_awake = false, has_start = false, has_update = false;
        
        for (const auto& func : result.functions) {
            if (func.name == "Awake") has_awake = true;
            if (func.name == "Start") has_start = true;
            if (func.name == "Update") has_update = true;
        }
        
        // 典型的なパターンの検出
        if (has_awake && has_start && has_update) {
            result.metadata["lifecycle_pattern"] = "typical_monobehaviour";
        } else if (has_awake || has_start) {
            result.metadata["lifecycle_pattern"] = "initialization_only";
        } else if (has_update) {
            result.metadata["lifecycle_pattern"] = "update_only";
        }
    }
    
    // ライフサイクルフェーズの取得
    std::string get_lifecycle_phase(const std::string& method_name) {
        if (method_name == "Awake") return "initialization";
        if (method_name == "Start") return "initialization";
        if (method_name == "Update" || method_name == "FixedUpdate" || method_name == "LateUpdate") 
            return "update";
        if (method_name == "OnDestroy") return "cleanup";
        return "other";
    }
};

} // namespace unity
} // namespace nekocode