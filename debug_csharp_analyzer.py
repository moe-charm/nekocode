#!/usr/bin/env python3
"""
.NET SDKアナライザー出力デバッグ
"""

import subprocess
import re
from pathlib import Path

print("🔍 .NET SDK Analyzer Debug")
print("=" * 40)

# .NETビルド実行
csharp_dir = "csharp_test"
print(f"target directory: {csharp_dir}")

try:
    result = subprocess.run(
        ["dotnet", "build", "--verbosity", "normal"],
        cwd=csharp_dir, capture_output=True, text=True
    )
    
    print(f"Return code: {result.returncode}")
    print(f"stdout length: {len(result.stdout)}")
    print(f"stderr length: {len(result.stderr)}")
    print()
    
    # 警告をフィルタリング
    warnings = []
    for line in result.stdout.split('\n'):
        if 'warning CS' in line and '.cs(' in line:
            warnings.append(line.strip())
    
    print(f"📋 Found {len(warnings)} warnings:")
    for i, warning in enumerate(warnings):
        print(f"  {i+1}: {warning}")
    print()
    
    # 詳細解析
    print("📋 Parsed warnings:")
    for warning in warnings:
        # "/path/file.cs(line,col): warning CSxxxx: message [project]"
        match = re.search(r'([^/]+\.cs)\((\d+),(\d+)\): warning (CS\d+): (.+?) \[', warning)
        if match:
            file_name, line_num, col_num, code, message = match.groups()
            location = f"{file_name}:{line_num}"
            print(f"  {code}: {message} [{location}]")
                
except Exception as e:
    print(f"Error: {e}")