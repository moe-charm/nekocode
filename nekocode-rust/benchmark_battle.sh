#!/bin/bash

echo "🥊 NekoCode ULTIMATE BATTLE: C++ vs Rust(Tree-sitter) 🥊"
echo "=============================================="
echo ""

# Test files
SMALL_FILE="small.js"
MEDIUM_FILE="medium_test.js"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "📊 Round 1: Small File Test (${SMALL_FILE})"
echo "----------------------------------------------"

# C++ version
echo -e "${BLUE}[C++ PEGTL]${NC}"
CPP_START=$(date +%s%N)
CPP_RESULT=$(../bin/nekocode_ai analyze "$SMALL_FILE" --output json 2>/dev/null)
CPP_END=$(date +%s%N)
CPP_TIME=$((($CPP_END - $CPP_START) / 1000000))
CPP_FUNCTIONS=$(echo "$CPP_RESULT" | jq '.functions | length // 0')
CPP_CLASSES=$(echo "$CPP_RESULT" | jq '.classes | length // 0')

# Rust PEST version  
echo -e "${YELLOW}[Rust PEST]${NC}"
RUST_PEST_START=$(date +%s%N)
RUST_PEST_RESULT=$(./target/release/nekocode-rust analyze "$SMALL_FILE" --parser pest 2>/dev/null)
RUST_PEST_END=$(date +%s%N)
RUST_PEST_TIME=$((($RUST_PEST_END - $RUST_PEST_START) / 1000000))
RUST_PEST_FUNCTIONS=$(echo "$RUST_PEST_RESULT" | jq '[.files[].functions | length] | add // 0')
RUST_PEST_CLASSES=$(echo "$RUST_PEST_RESULT" | jq '[.files[].classes | length] | add // 0')

# Rust Tree-sitter version
echo -e "${GREEN}[Rust Tree-sitter]${NC}"
RUST_TS_START=$(date +%s%N)
RUST_TS_RESULT=$(./target/release/nekocode-rust analyze "$SMALL_FILE" --parser tree-sitter 2>/dev/null)
RUST_TS_END=$(date +%s%N)
RUST_TS_TIME=$((($RUST_TS_END - $RUST_TS_START) / 1000000))
RUST_TS_FUNCTIONS=$(echo "$RUST_TS_RESULT" | jq '[.files[].functions | length] | add // 0')
RUST_TS_CLASSES=$(echo "$RUST_TS_RESULT" | jq '[.files[].classes | length] | add // 0')

echo ""
echo "⏱️  Performance Results:"
printf "%-20s %10s ms | Functions: %2d | Classes: %2d\n" "C++ (PEGTL):" "$CPP_TIME" "$CPP_FUNCTIONS" "$CPP_CLASSES"
printf "%-20s %10s ms | Functions: %2d | Classes: %2d\n" "Rust (PEST):" "$RUST_PEST_TIME" "$RUST_PEST_FUNCTIONS" "$RUST_PEST_CLASSES"  
printf "%-20s %10s ms | Functions: %2d | Classes: %2d\n" "Rust (Tree-sitter):" "$RUST_TS_TIME" "$RUST_TS_FUNCTIONS" "$RUST_TS_CLASSES"

# Calculate speedup
if [ $CPP_TIME -gt 0 ]; then
    SPEEDUP_PEST=$(echo "scale=2; $CPP_TIME / $RUST_PEST_TIME" | bc)
    SPEEDUP_TS=$(echo "scale=2; $CPP_TIME / $RUST_TS_TIME" | bc)
    echo ""
    echo "🚀 Speed comparison (vs C++):"
    echo "   Rust PEST:        ${SPEEDUP_PEST}x"
    echo "   Rust Tree-sitter: ${SPEEDUP_TS}x"
fi

echo ""
echo "=============================================="
echo "📊 Round 2: Medium File Test (${MEDIUM_FILE})"
echo "----------------------------------------------"

# C++ version
echo -e "${BLUE}[C++ PEGTL]${NC}"
CPP_START=$(date +%s%N)
CPP_RESULT=$(../bin/nekocode_ai analyze "$MEDIUM_FILE" --output json 2>/dev/null)
CPP_END=$(date +%s%N)
CPP_TIME=$((($CPP_END - $CPP_START) / 1000000))
CPP_FUNCTIONS=$(echo "$CPP_RESULT" | jq '.functions | length // 0')
CPP_CLASSES=$(echo "$CPP_RESULT" | jq '.classes | length // 0')

# Rust PEST version
echo -e "${YELLOW}[Rust PEST]${NC}"
RUST_PEST_START=$(date +%s%N)
RUST_PEST_RESULT=$(./target/release/nekocode-rust analyze "$MEDIUM_FILE" --parser pest 2>/dev/null)
RUST_PEST_END=$(date +%s%N)
RUST_PEST_TIME=$((($RUST_PEST_END - $RUST_PEST_START) / 1000000))
RUST_PEST_FUNCTIONS=$(echo "$RUST_PEST_RESULT" | jq '[.files[].functions | length] | add // 0')
RUST_PEST_CLASSES=$(echo "$RUST_PEST_RESULT" | jq '[.files[].classes | length] | add // 0')

# Rust Tree-sitter version
echo -e "${GREEN}[Rust Tree-sitter]${NC}"
RUST_TS_START=$(date +%s%N)
RUST_TS_RESULT=$(./target/release/nekocode-rust analyze "$MEDIUM_FILE" --parser tree-sitter 2>/dev/null)
RUST_TS_END=$(date +%s%N)
RUST_TS_TIME=$((($RUST_TS_END - $RUST_TS_START) / 1000000))
RUST_TS_FUNCTIONS=$(echo "$RUST_TS_RESULT" | jq '[.files[].functions | length] | add // 0')
RUST_TS_CLASSES=$(echo "$RUST_TS_RESULT" | jq '[.files[].classes | length] | add // 0')

echo ""
echo "⏱️  Performance Results:"
printf "%-20s %10s ms | Functions: %2d | Classes: %2d\n" "C++ (PEGTL):" "$CPP_TIME" "$CPP_FUNCTIONS" "$CPP_CLASSES"
printf "%-20s %10s ms | Functions: %2d | Classes: %2d\n" "Rust (PEST):" "$RUST_PEST_TIME" "$RUST_PEST_FUNCTIONS" "$RUST_PEST_CLASSES"
printf "%-20s %10s ms | Functions: %2d | Classes: %2d\n" "Rust (Tree-sitter):" "$RUST_TS_TIME" "$RUST_TS_FUNCTIONS" "$RUST_TS_CLASSES"

# Calculate speedup
if [ $CPP_TIME -gt 0 ]; then
    SPEEDUP_PEST=$(echo "scale=2; $CPP_TIME / $RUST_PEST_TIME" | bc)
    SPEEDUP_TS=$(echo "scale=2; $CPP_TIME / $RUST_TS_TIME" | bc)
    echo ""
    echo "🚀 Speed comparison (vs C++):"
    echo "   Rust PEST:        ${SPEEDUP_PEST}x"
    echo "   Rust Tree-sitter: ${SPEEDUP_TS}x"
fi

echo ""
echo "=============================================="
echo "🏆 WINNER ANNOUNCEMENT 🏆"
echo "----------------------------------------------"

# Determine winner based on medium file test
if [ $RUST_TS_TIME -lt $CPP_TIME ] && [ $RUST_TS_TIME -lt $RUST_PEST_TIME ]; then
    echo -e "${GREEN}🥇 Rust with Tree-sitter WINS! 🚀${NC}"
    echo "   Fastest parser with best detection accuracy!"
elif [ $CPP_TIME -lt $RUST_PEST_TIME ] && [ $CPP_TIME -lt $RUST_TS_TIME ]; then
    echo -e "${BLUE}🥇 C++ with PEGTL WINS! 💪${NC}"
    echo "   Proven performance champion!"
else
    echo -e "${YELLOW}🥇 Rust with PEST WINS! 🦀${NC}"
    echo "   Solid balance of speed and accuracy!"
fi

echo ""
echo "=============================================="
echo "📈 Detection Accuracy Summary:"
echo "----------------------------------------------"
echo "                  Functions | Classes | Total"
printf "C++ (PEGTL):      %9d | %7d | %5d\n" "$CPP_FUNCTIONS" "$CPP_CLASSES" $((CPP_FUNCTIONS + CPP_CLASSES))
printf "Rust (PEST):      %9d | %7d | %5d\n" "$RUST_PEST_FUNCTIONS" "$RUST_PEST_CLASSES" $((RUST_PEST_FUNCTIONS + RUST_PEST_CLASSES))
printf "Rust (Tree-sitter): %7d | %7d | %5d\n" "$RUST_TS_FUNCTIONS" "$RUST_TS_CLASSES" $((RUST_TS_FUNCTIONS + RUST_TS_CLASSES))