#!/bin/bash
# Perfect Analysis Workflow - NekoCode + LTO + Clang-Tidy統合

echo "🚀 Perfect Analysis Workflow for C++"
echo "=================================="

# 対象ファイル
TARGET_FILE="${1:-test_dead_code.cpp}"

# Step 1: NekoCode構造解析
echo "📊 Step 1: NekoCode Structure Analysis"
./bin/nekocode_ai analyze "$TARGET_FILE" --io-threads 8 > nekocode_result.json
echo "✅ Structure analysis complete"

# Step 2: LTO未使用コード検出
echo -e "\n🔬 Step 2: LTO Dead Code Detection"
g++ -flto -ffunction-sections -fdata-sections \
    -Wall -Wextra -Wunused-function -Wunused-variable \
    "$TARGET_FILE" -o temp_lto \
    -Wl,--gc-sections -Wl,--print-gc-sections 2> lto_result.txt
echo "✅ LTO analysis complete"

# Step 3: Clang-Tidy静的解析（インストール済みの場合）
echo -e "\n🔧 Step 3: Clang-Tidy Static Analysis"
if command -v clang-tidy &> /dev/null; then
    clang-tidy -checks='misc-unused-*,bugprone-*,performance-*,modernize-*' \
        "$TARGET_FILE" > clang_tidy_result.txt
    echo "✅ Clang-Tidy analysis complete"
else
    echo "⚠️  Clang-Tidy not installed - skipping"
    echo "   Install with: sudo apt install clang-tidy"
fi

# Step 4: 結果統合
echo -e "\n📋 Step 4: Integrated Results"
echo "============================="

# NekoCode結果表示
echo "🏗️ Defined structures:"
jq -r '.functions[] | "  - Function: " + .name' nekocode_result.json 2>/dev/null || echo "  (parsing error)"

# LTO結果表示
echo -e "\n❌ Unused code (LTO):"
grep "removing unused section" lto_result.txt | grep -E "(text|data)" | while read line; do
    if [[ $line =~ \.text\.([^\']+) ]]; then
        mangled="${BASH_REMATCH[1]}"
        demangled=$(c++filt "$mangled" 2>/dev/null || echo "$mangled")
        echo "  - Function: $demangled"
    elif [[ $line =~ \.data\.([^\']+) ]]; then
        echo "  - Variable: ${BASH_REMATCH[1]}"
    fi
done

# Clang-Tidy結果表示（ある場合）
if [ -f clang_tidy_result.txt ]; then
    echo -e "\n⚠️  Clang-Tidy warnings:"
    grep -E "warning:|error:" clang_tidy_result.txt | head -10
fi

# クリーンアップ
rm -f temp_lto nekocode_result.json lto_result.txt clang_tidy_result.txt

echo -e "\n✨ Perfect Analysis Complete!"