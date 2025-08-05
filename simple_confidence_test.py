#!/usr/bin/env python3
"""
シンプルな確信度表示テスト
"""

def parse_vulture_output(vulture_output):
    """Vultureの出力をシンプルにパース"""
    items = []
    
    for line in vulture_output.strip().split('\n'):
        if line.strip() and ':' in line:
            # "test.py:6: unused import 'os' (90% confidence)"
            # → "unused import 'os' (90%)"
            
            parts = line.split(': ', 1)
            if len(parts) >= 2:
                location = parts[0]  # "test.py:6"
                message = parts[1]   # "unused import 'os' (90% confidence)"
                
                # "(90% confidence)" → "(90%)"に変換
                clean_message = message.replace("% confidence)", "%)")
                
                items.append({
                    "item": clean_message,
                    "location": location
                })
    
    return items

# テスト
vulture_output = """test_python_deadcode.py:6: unused import 'os' (90% confidence)
test_python_deadcode.py:8: unused import 'unused_module' (90% confidence)
test_python_deadcode.py:14: unused function 'unused_function' (60% confidence)
test_python_deadcode.py:23: unused class 'UnusedClass' (60% confidence)"""

result = parse_vulture_output(vulture_output)

print("🔍 シンプル確信度表示テスト")
print("=" * 50)

# Option 1: 文字列のみ
print("\n📋 Option 1: 文字列のみ")
simple_items = [item["item"] for item in result]
for item in simple_items:
    print(f"  • {item}")

# Option 2: 位置情報付き
print("\n📋 Option 2: 位置情報付き")  
for item in result:
    print(f"  • {item['item']} at {item['location']}")

# Option 3: JSON形式
print("\n📋 Option 3: JSON形式")
json_format = {
    "dead_code": {
        "tool": "Vulture",
        "language": "python",
        "items": simple_items,
        "total_found": len(simple_items)
    }
}

import json
print(json.dumps(json_format, indent=2))

print("\n💡 どれが一番見やすいかにゃ？")