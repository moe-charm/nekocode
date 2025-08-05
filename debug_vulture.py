#!/usr/bin/env python3
"""
Vulture出力デバッグ
"""

import subprocess

print("🔍 Vulture Output Debug")
print("=" * 40)

# Vulture実行
try:
    result = subprocess.run(
        ["vulture", "test_python_deadcode.py", "--min-confidence", "60"],
        capture_output=True, text=True
    )
    
    print(f"Return code: {result.returncode}")
    print(f"stdout length: {len(result.stdout)}")
    print(f"stderr length: {len(result.stderr)}")
    print()
    
    print("📋 stdout:")
    print(repr(result.stdout))
    print()
    
    print("📋 stderr:")
    print(repr(result.stderr))
    print()
    
    print("📋 stderr lines:")
    for i, line in enumerate(result.stderr.split('\n')):
        print(f"  {i}: {repr(line)}")
        
except Exception as e:
    print(f"Error: {e}")