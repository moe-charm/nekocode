#!/bin/bash
# 🔨 NekoCode 再ビルドスクリプト

echo "🧹 Cleaning build directory..."
cd build
rm -rf *

echo "🔧 Regenerating build files..."
cmake ..

echo "🔨 Building NekoCode..."
make -j$(nproc)

echo "✅ Build complete!"
echo "Executables:"
echo "  - ./build/nekocode_ai    (for Claude Code)"
echo "  - ./build/nekocode_human  (for humans)"