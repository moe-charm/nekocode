#!/bin/bash

echo "🥊 FINAL BATTLE: TypeScript Compiler Analysis 🥊"
echo "================================================="
echo ""
echo "📁 Target: test-real-projects/typescript/src/compiler"
echo "   (68 TypeScript files, ~3MB each)"
echo ""

# Test directory
TEST_DIR="test-real-projects/typescript/src/compiler"

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🔵 C++ (PEGTL) Performance"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
START=$(date +%s%N)
../bin/nekocode_ai analyze "$TEST_DIR" --output json > /tmp/cpp_result.json 2>&1
END=$(date +%s%N)
CPP_TIME=$((($END - $START) / 1000000))
echo "⏱️  Time: ${CPP_TIME}ms"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🟡 Rust (PEST) Performance"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
START=$(date +%s%N)
./target/release/nekocode-rust analyze "$TEST_DIR" --parser pest 2>&1 | grep -v "🔄\|📁\|🔍\|⚡\|🏁\|🔄" > /tmp/rust_pest_result.json
END=$(date +%s%N)
RUST_PEST_TIME=$((($END - $START) / 1000000))
echo "⏱️  Time: ${RUST_PEST_TIME}ms"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🟢 Rust (Tree-sitter) Performance 🚀"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
START=$(date +%s%N)
./target/release/nekocode-rust analyze "$TEST_DIR" --parser tree-sitter 2>&1 | grep -v "🔄\|📁\|🔍\|⚡\|🏁\|🔄" > /tmp/rust_ts_result.json
END=$(date +%s%N)
RUST_TS_TIME=$((($END - $START) / 1000000))
echo "⏱️  Time: ${RUST_TS_TIME}ms"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📊 PERFORMANCE COMPARISON"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "┌──────────────────┬────────────┬────────────┐"
echo "│ Parser           │ Time (ms)  │ Speed      │"
echo "├──────────────────┼────────────┼────────────┤"
printf "│ C++ (PEGTL)      │ %10d │ 1.00x      │\n" $CPP_TIME
if [ $CPP_TIME -gt 0 ]; then
    PEST_SPEED=$(echo "scale=2; $CPP_TIME / $RUST_PEST_TIME" | bc)
    TS_SPEED=$(echo "scale=2; $CPP_TIME / $RUST_TS_TIME" | bc)
    printf "│ Rust (PEST)      │ %10d │ %.2fx      │\n" $RUST_PEST_TIME $PEST_SPEED
    printf "│ Rust (Tree-sitter)│ %10d │ %.2fx     │\n" $RUST_TS_TIME $TS_SPEED
fi
echo "└──────────────────┴────────────┴────────────┘"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🏆 WINNER: "

# Determine winner
if [ $RUST_TS_TIME -lt $CPP_TIME ] && [ $RUST_TS_TIME -lt $RUST_PEST_TIME ]; then
    SPEEDUP=$(echo "scale=1; $CPP_TIME / $RUST_TS_TIME" | bc)
    echo "   🎊 Rust with Tree-sitter! 🚀"
    echo "   ${SPEEDUP}x faster than C++!"
elif [ $CPP_TIME -lt $RUST_PEST_TIME ] && [ $CPP_TIME -lt $RUST_TS_TIME ]; then
    echo "   🎊 C++ with PEGTL! 💪"
    echo "   Still the performance king!"
else
    SPEEDUP=$(echo "scale=1; $CPP_TIME / $RUST_PEST_TIME" | bc)
    echo "   🎊 Rust with PEST! 🦀"
    echo "   ${SPEEDUP}x faster than C++!"
fi
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"