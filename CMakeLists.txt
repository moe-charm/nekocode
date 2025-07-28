cmake_minimum_required(VERSION 3.16)
project(NekoCodeCpp VERSION 1.0.0 LANGUAGES C CXX)

#=============================================================================
# 🐱 NekoCode C++ - 高速解析ツール
#
# 実行ファイル２個大作戦：
# - nekocode_ai: AI/Claude用JSON出力
# - nekocode_human: Human用美しいテキスト出力
#
# Python版からの大幅性能向上:
# - 10-100倍高速化
# - 低メモリ消費
# - 並列処理対応
#=============================================================================

# C++17必須
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# std::regex使用防止オプション（開発者向け警告）
option(NEKOCODE_PREVENT_REGEX "Prevent std::regex usage" ON)
if(NEKOCODE_PREVENT_REGEX)
    add_compile_definitions(NEKOCODE_PREVENT_REGEX)
    message(STATUS "⚠️  std::regex prevention enabled - Use PEGTL instead!")
endif()

# コンパイラー警告設定
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 最適化設定
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -DNDEBUG)
    message(STATUS "🚀 Release build: 最大最適化モード")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -DDEBUG)
    message(STATUS "🐛 Debug build: デバッグモード")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "⚡ Default: Release build")
endif()

#=============================================================================
# 📁 ディレクトリ設定
#=============================================================================

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# インクルードパス
include_directories(${INCLUDE_DIR})

#=============================================================================
# 📦 External Dependencies
#=============================================================================

# nlohmann/json for JSON formatting
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "📦 nlohmann/json not found, using single header")
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# UTF8-CPP for Unicode support
message(STATUS "🌍 Fetching UTF8-CPP for Unicode support...")
include(FetchContent)
FetchContent_Declare(
    utf8cpp
    GIT_REPOSITORY https://github.com/nemtrif/utfcpp.git
    GIT_TAG v3.2.4
)
FetchContent_MakeAvailable(utf8cpp)

# 🔥 PEGTL - 軽量高性能パーサーライブラリ統合
message(STATUS "🔥 Fetching PEGTL for advanced parsing...")
include(FetchContent)
FetchContent_Declare(
    pegtl
    GIT_REPOSITORY https://github.com/taocpp/PEGTL.git
    GIT_TAG 3.2.7
)
FetchContent_MakeAvailable(pegtl)

# 🌳 Tree-sitter Phase 3: プレースホルダー統合（PEGTL移行予定）
message(STATUS "🌳 Tree-sitter placeholder integration (migrating to PEGTL)")

# プレースホルダー用Tree-sitterライブラリ（暫定維持）
add_library(tree-sitter-core STATIC ${SRC_DIR}/legacy/tree_sitter_placeholder.c)
target_include_directories(tree-sitter-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 言語統合用インターフェース（プレースホルダー）
add_library(tree-sitter-langs INTERFACE)

# Threads for parallel processing
find_package(Threads REQUIRED)

#=============================================================================
# 🧠 NekoCode Core Library
#=============================================================================

# 🎯 コアライブラリソース（新構造対応）
set(NEKOCODE_SOURCES
    # Core機能
    ${SRC_DIR}/core/core.cpp
    ${SRC_DIR}/core/language_detection.cpp
    ${SRC_DIR}/core/session_manager.cpp
    
    # フォーマッター
    ${SRC_DIR}/formatters/formatters.cpp
    
    # ユーティリティ
    ${SRC_DIR}/utils/utf8_utils.cpp
    ${SRC_DIR}/utils/include_analyzer.cpp
    
    # レガシー（必要に応じて削除予定）
    ${SRC_DIR}/legacy/tree_sitter_analyzer.cpp
    ${SRC_DIR}/legacy/pegtl_analyzer.cpp
    
    # 言語別アナライザー（新構造）
    ${SRC_DIR}/analyzers/analyzer_factory.cpp
    ${SRC_DIR}/analyzers/python/python_analyzer.cpp
    ${SRC_DIR}/analyzers/javascript/javascript_analyzer.cpp
    ${SRC_DIR}/analyzers/cpp/cpp_language_analyzer.cpp
    ${SRC_DIR}/analyzers/cpp/cpp_analyzer.cpp
    ${SRC_DIR}/analyzers/csharp/csharp_analyzer.cpp
    ${SRC_DIR}/analyzers/csharp/csharp_pegtl_analyzer.cpp
    ${SRC_DIR}/analyzers/unity/unity_analyzer.cpp
)

# ヘッダーファイル
set(NEKOCODE_HEADERS
    ${INCLUDE_DIR}/nekocode/types.hpp
    ${INCLUDE_DIR}/nekocode/core.hpp
    ${INCLUDE_DIR}/nekocode/tree_sitter_analyzer.hpp
    ${INCLUDE_DIR}/nekocode/pegtl_analyzer.hpp
    ${INCLUDE_DIR}/nekocode/formatters.hpp
    ${INCLUDE_DIR}/nekocode/utf8_utils.hpp
    ${INCLUDE_DIR}/nekocode/language_detection.hpp
    ${INCLUDE_DIR}/nekocode/cpp_analyzer.hpp
    ${INCLUDE_DIR}/nekocode/session_manager.hpp
    ${INCLUDE_DIR}/nekocode/include_analyzer.hpp
    # 言語別アナライザーヘッダー
    ${INCLUDE_DIR}/nekocode/analyzers/base_analyzer.hpp
    ${INCLUDE_DIR}/nekocode/analyzers/python_analyzer.hpp
    ${INCLUDE_DIR}/nekocode/analyzers/javascript_analyzer.hpp
    ${INCLUDE_DIR}/nekocode/analyzers/cpp_language_analyzer.hpp
    ${INCLUDE_DIR}/nekocode/analyzers/csharp_analyzer.hpp
    ${INCLUDE_DIR}/nekocode/analyzers/unity_analyzer.hpp
    ${INCLUDE_DIR}/nekocode/analyzers/unity_patterns.hpp
    ${INCLUDE_DIR}/nekocode/analyzers/unity_components.hpp
)

# 静的ライブラリ作成
add_library(nekocode_static STATIC ${NEKOCODE_SOURCES} ${NEKOCODE_HEADERS})
target_include_directories(nekocode_static PUBLIC 
    ${INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(nekocode_static 
    nlohmann_json::nlohmann_json 
    utf8::cpp
    tree-sitter-core
    tree-sitter-langs
    taocpp::pegtl
    Threads::Threads
)

# C++17 features
target_compile_features(nekocode_static PUBLIC cxx_std_17)

#=============================================================================
# 🤖 AI Tool - Claude Code最適化実行ファイル
#=============================================================================

add_executable(nekocode_ai ${SRC_DIR}/main/main_ai.cpp)
target_link_libraries(nekocode_ai nekocode_static)

# AI用特別最適化
target_compile_definitions(nekocode_ai PRIVATE 
    NEKOCODE_AI_MODE=1
    NEKOCODE_PERFORMANCE_PRIORITY=1
)

#=============================================================================
# 👨‍💻 Human Tool - 美しい出力実行ファイル
#=============================================================================

add_executable(nekocode_human ${SRC_DIR}/main/main_human.cpp)
target_link_libraries(nekocode_human nekocode_static)

# Human用見た目重視
target_compile_definitions(nekocode_human PRIVATE 
    NEKOCODE_HUMAN_MODE=1
    NEKOCODE_VISUAL_PRIORITY=1
)

#=============================================================================
# 🧪 Tests
#=============================================================================

option(BUILD_TESTS "Build test suite" ON)
if(BUILD_TESTS)
    enable_testing()
    
    # 簡易テスト
    add_executable(test_core ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_core.cpp)
    target_link_libraries(test_core nekocode_static)
    add_test(NAME CoreTest COMMAND test_core)
    
    add_executable(test_performance ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_performance.cpp)
    target_link_libraries(test_performance nekocode_static)
    add_test(NAME PerformanceTest COMMAND test_performance)
endif()

#=============================================================================
# 📦 Install Configuration
#=============================================================================

option(ENABLE_INSTALL "Enable install target" ON)
if(ENABLE_INSTALL)
    # 実行ファイルインストール
    install(TARGETS nekocode_ai nekocode_human
        RUNTIME DESTINATION bin
    )
    
    # ヘッダーファイルインストール（ライブラリとして使用時）
    install(DIRECTORY ${INCLUDE_DIR}/nekocode
        DESTINATION include
        FILES_MATCHING PATTERN "*.hpp"
    )
    
    # ライブラリインストール
    install(TARGETS nekocode_static
        ARCHIVE DESTINATION lib
    )
endif()

#=============================================================================
# 🔧 Development Tools
#=============================================================================

# Clang-tidy（コード品質チェック）
option(ENABLE_CLANG_TIDY "Enable clang-tidy checks" OFF)
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
        message(STATUS "✅ clang-tidy enabled")
    else()
        message(STATUS "❌ clang-tidy not found")
    endif()
endif()

# Address Sanitizer（メモリリークチェック）
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if(ENABLE_ASAN)
    target_compile_options(nekocode_static PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(nekocode_static PRIVATE -fsanitize=address)
    message(STATUS "🛡️ AddressSanitizer enabled")
endif()

# Thread Sanitizer（データレースチェック）
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
if(ENABLE_TSAN)
    target_compile_options(nekocode_static PRIVATE -fsanitize=thread)
    target_link_options(nekocode_static PRIVATE -fsanitize=thread)
    message(STATUS "🔒 ThreadSanitizer enabled")
endif()

#=============================================================================
# 📊 Build Summary
#=============================================================================

message(STATUS "")
message(STATUS "🐱 NekoCode C++ Build Configuration:")
message(STATUS "   Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "   C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "   Tests: ${BUILD_TESTS}")
message(STATUS "   Install Target: ${ENABLE_INSTALL}")
message(STATUS "")
message(STATUS "🎯 実行ファイル２個大作戦:")
message(STATUS "   🤖 nekocode_ai     - AI/Claude用JSON出力")
message(STATUS "   👨‍💻 nekocode_human  - Human用美しいテキスト")
message(STATUS "")
message(STATUS "⚡ Python版からの革命的改善:")
message(STATUS "   📈 性能: 10-100倍高速化")
message(STATUS "   🧠 メモリ: 大幅削減")
message(STATUS "   🚀 並列: マルチスレッド対応")
message(STATUS "   🔒 型安全: コンパイル時チェック")
message(STATUS "")

#=============================================================================
# 🚀 Quick Build Commands
#=============================================================================

# カスタムターゲット: 両方ビルド
add_custom_target(build_all
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target nekocode_ai --parallel
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target nekocode_human --parallel
    COMMENT "🚀 Build both AI and Human tools"
)

# カスタムターゲット: AI専用ビルド
add_custom_target(build_ai
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target nekocode_ai --parallel
    COMMENT "🤖 Build AI tool only"
)

# カスタムターゲット: Human専用ビルド
add_custom_target(build_human
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target nekocode_human --parallel
    COMMENT "👨‍💻 Build Human tool only"
)

# カスタムターゲット: テスト実行
add_custom_target(run_tests
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target test_core --parallel
    COMMAND ./test_core
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "🧪 Build and run tests"
)

# カスタムターゲット: パフォーマンステスト
add_custom_target(benchmark
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target test_performance --parallel
    COMMAND ./test_performance
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "⚡ Run performance benchmark"
)