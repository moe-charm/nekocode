#!/usr/bin/env python3
"""
staticcheck出力デバッグ
"""

import subprocess
import os
from pathlib import Path

print("🔍 staticcheck Debug")
print("=" * 40)

# staticcheck実行
staticcheck_cmd = str(Path("~/go/bin/staticcheck").expanduser())
filepath = "test_go_deadcode.go"

print(f"staticcheck command: {staticcheck_cmd}")
print(f"target file: {filepath}")

try:
    # GOPATHを設定してstaticcheck実行
    env = os.environ.copy()
    env["PATH"] = env.get("PATH", "") + ":/usr/local/go/bin"
    
    result = subprocess.run(
        [staticcheck_cmd, "-checks=U1000,U1001", filepath],
        capture_output=True, text=True, env=env
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
    
    # 実際のフォーマット確認
    if result.stderr:
        print("📋 stderr lines:")
        for i, line in enumerate(result.stderr.split('\n')):
            if line.strip():
                print(f"  {i}: {repr(line)}")
                
except Exception as e:
    print(f"Error: {e}")