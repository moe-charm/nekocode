cmake_minimum_required(VERSION 3.16)
project(NekoCodeCpp VERSION 1.0.0 LANGUAGES C CXX)

#=============================================================================
# 🐱 NekoCode C++ - 高速解析ツール
#
# 実行ファイル２個大作戦：
# - nekocode_ai: AI/Claude用JSON出力
#
# Python版からの大幅性能向上:
# - 10-100倍高速化
# - 低メモリ消費
# - 並列処理対応
#=============================================================================

# C++20必須（string::starts_with/ends_with対応）
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# std::regex使用防止オプション（開発者向け警告）
option(NEKOCODE_PREVENT_REGEX "Prevent std::regex usage" ON)
if(NEKOCODE_PREVENT_REGEX)
    add_compile_definitions(NEKOCODE_PREVENT_REGEX)
    message(STATUS "⚠️  std::regex prevention enabled - Use PEGTL instead!")
endif()

# 🌟 Universal AST Revolution オプション（6言語統一システム）
option(NEKOCODE_ENABLE_UNIVERSAL_AST "Enable Universal AST system for all 6 languages" OFF)
if(NEKOCODE_ENABLE_UNIVERSAL_AST)
    add_compile_definitions(NEKOCODE_USE_UNIVERSAL_AST=1)
    message(STATUS "🌟 Universal AST Revolution enabled - 6言語統一システム起動!")
endif()

# 🧠 Memory System オプション（時間軸Memory革命）
option(NEKOCODE_ENABLE_MEMORY_SYSTEM "Enable Memory System for analysis result persistence" ON)
if(NEKOCODE_ENABLE_MEMORY_SYSTEM)
    add_compile_definitions(NEKOCODE_USE_MEMORY_SYSTEM=1)
    message(STATUS "🧠 Memory System enabled - 時間軸Memory革命起動!")
endif()

# コンパイラー警告設定
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-unused-variable)
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

# 🎯 実行ファイル出力先をbin/フォルダに設定（Geminiクラッシュ対策版を安全に配備）
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

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
    ${SRC_DIR}/core/session_data.cpp
    ${SRC_DIR}/core/config_manager.cpp
    # SessionCommands分割後ファイル群（実装保持完了）
    ${SRC_DIR}/core/cmd/basic_commands.cpp
    ${SRC_DIR}/core/cmd/structure_commands.cpp
    ${SRC_DIR}/core/cmd/include_commands.cpp
    ${SRC_DIR}/core/cmd/search_commands.cpp
    ${SRC_DIR}/core/cmd/ast_commands.cpp
    
    # フォーマッター
    ${SRC_DIR}/formatters/formatters.cpp
    
    # 🚀 Phase 5完了: Symbol Converter層削除済み
    # Phase 5で全アナライザーが直接生成するためConverter層不要
    # Phase 3: Symbol Converters
    # ${SRC_DIR}/converters/rust_symbol_converter.cpp
    # 
    # Phase 4: Symbol Converters  
    # ${SRC_DIR}/converters/js_symbol_converter.cpp
    # ${SRC_DIR}/converters/python_symbol_converter.cpp
    # ${SRC_DIR}/converters/cpp_symbol_converter.cpp
    # ${SRC_DIR}/converters/csharp_symbol_converter.cpp
    
    # ユーティリティ
    ${SRC_DIR}/utils/utf8_utils.cpp
    ${SRC_DIR}/utils/include_analyzer.cpp
    ${SRC_DIR}/utils/progress_tracker.cpp
    
    # ファインダー機能
    ${SRC_DIR}/finders/symbol_finder.cpp
    
    # 依存関係解析
    ${SRC_DIR}/core/dependency_graph.cpp
    
    # コマンド
    ${SRC_DIR}/commands/find_command.cpp
    
    # 🔥 Direct Edit機能（セッション不要）
    ${SRC_DIR}/core/commands/direct_edit/direct_replace.cpp
    ${SRC_DIR}/core/commands/direct_edit/direct_insert.cpp
    ${SRC_DIR}/core/commands/direct_edit/direct_movelines.cpp
    
    # 🧠 Memory System - 時間軸Memory革命（条件付きコンパイル）
    $<$<BOOL:${NEKOCODE_ENABLE_MEMORY_SYSTEM}>:${SRC_DIR}/memory/memory_system.cpp>
    $<$<BOOL:${NEKOCODE_ENABLE_MEMORY_SYSTEM}>:${SRC_DIR}/commands/memory_command.cpp>
    
    # レガシー（必要に応じて削除予定）
    ${SRC_DIR}/legacy/tree_sitter_analyzer.cpp
    ${SRC_DIR}/legacy/pegtl_analyzer.cpp
    
    # 言語別アナライザー（新構造）
    ${SRC_DIR}/analyzers/analyzer_factory.cpp
    ${SRC_DIR}/analyzers/python/python_analyzer.cpp
    # ${SRC_DIR}/analyzers/javascript/javascript_analyzer.cpp # regex版は削除済み
    ${SRC_DIR}/analyzers/cpp/cpp_language_analyzer.cpp
    ${SRC_DIR}/analyzers/cpp/cpp_analyzer.cpp
    # ${SRC_DIR}/analyzers/csharp/csharp_analyzer.cpp # regex版は削除済み
    ${SRC_DIR}/analyzers/csharp/csharp_pegtl_analyzer.cpp
    ${SRC_DIR}/analyzers/unity/unity_analyzer.cpp
    ${SRC_DIR}/analyzers/go/go_analyzer.cpp
    ${SRC_DIR}/analyzers/rust/rust_analyzer.cpp
    
    # 🌟 Universal AST Revolution - 6言語統一システム（条件付きコンパイル）
    $<$<BOOL:${NEKOCODE_ENABLE_UNIVERSAL_AST}>:${SRC_DIR}/universal/universal_tree_builder.cpp>
    $<$<BOOL:${NEKOCODE_ENABLE_UNIVERSAL_AST}>:${SRC_DIR}/universal/language_traits.cpp>
    $<$<BOOL:${NEKOCODE_ENABLE_UNIVERSAL_AST}>:${SRC_DIR}/universal/universal_code_analyzer.cpp>
    $<$<BOOL:${NEKOCODE_ENABLE_UNIVERSAL_AST}>:${SRC_DIR}/adapters/javascript_universal_adapter.cpp>
    $<$<BOOL:${NEKOCODE_ENABLE_UNIVERSAL_AST}>:${SRC_DIR}/adapters/python_universal_adapter.cpp>
    $<$<BOOL:${NEKOCODE_ENABLE_UNIVERSAL_AST}>:${SRC_DIR}/adapters/cpp_universal_adapter.cpp>
    $<$<BOOL:${NEKOCODE_ENABLE_UNIVERSAL_AST}>:${SRC_DIR}/adapters/csharp_universal_adapter.cpp>
    $<$<BOOL:${NEKOCODE_ENABLE_UNIVERSAL_AST}>:${SRC_DIR}/adapters/go_universal_adapter.cpp>
    $<$<BOOL:${NEKOCODE_ENABLE_UNIVERSAL_AST}>:${SRC_DIR}/adapters/rust_universal_adapter.cpp>
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
    ${INCLUDE_DIR}/nekocode/symbol_finder.hpp
    ${INCLUDE_DIR}/nekocode/progress_tracker.hpp
    # 言語別アナライザーヘッダー（新構造対応）
    ${SRC_DIR}/analyzers/base_analyzer.hpp
    ${SRC_DIR}/analyzers/javascript/javascript_pegtl_analyzer.hpp
    ${SRC_DIR}/analyzers/javascript/javascript_minimal_grammar.hpp
    ${INCLUDE_DIR}/nekocode/analyzers/python_analyzer.hpp
    ${INCLUDE_DIR}/nekocode/analyzers/cpp_language_analyzer.hpp
    ${INCLUDE_DIR}/nekocode/analyzers/unity_analyzer.hpp
    ${INCLUDE_DIR}/nekocode/analyzers/unity_patterns.hpp
    ${INCLUDE_DIR}/nekocode/analyzers/unity_components.hpp
    ${INCLUDE_DIR}/nekocode/analyzers/go_analyzer.hpp
    ${INCLUDE_DIR}/nekocode/analyzers/rust_analyzer.hpp
    
    # 🧠 Memory System Headers（条件付き）
    $<$<BOOL:${NEKOCODE_ENABLE_MEMORY_SYSTEM}>:${SRC_DIR}/memory/memory_system.hpp>
    $<$<BOOL:${NEKOCODE_ENABLE_MEMORY_SYSTEM}>:${INCLUDE_DIR}/nekocode/memory_command.hpp>
    
    # 🌟 Universal AST Revolution ヘッダー（条件付き）
    $<$<BOOL:${NEKOCODE_ENABLE_UNIVERSAL_AST}>:${SRC_DIR}/universal/universal_tree_builder.hpp>
    $<$<BOOL:${NEKOCODE_ENABLE_UNIVERSAL_AST}>:${SRC_DIR}/universal/language_traits.hpp>
    $<$<BOOL:${NEKOCODE_ENABLE_UNIVERSAL_AST}>:${SRC_DIR}/universal/universal_code_analyzer.hpp>
    $<$<BOOL:${NEKOCODE_ENABLE_UNIVERSAL_AST}>:${SRC_DIR}/adapters/javascript_universal_adapter.hpp>
    $<$<BOOL:${NEKOCODE_ENABLE_UNIVERSAL_AST}>:${SRC_DIR}/adapters/python_universal_adapter.hpp>
    $<$<BOOL:${NEKOCODE_ENABLE_UNIVERSAL_AST}>:${SRC_DIR}/adapters/cpp_universal_adapter.hpp>
    $<$<BOOL:${NEKOCODE_ENABLE_UNIVERSAL_AST}>:${SRC_DIR}/adapters/csharp_universal_adapter.hpp>
    $<$<BOOL:${NEKOCODE_ENABLE_UNIVERSAL_AST}>:${SRC_DIR}/adapters/go_universal_adapter.hpp>
    $<$<BOOL:${NEKOCODE_ENABLE_UNIVERSAL_AST}>:${SRC_DIR}/adapters/rust_universal_adapter.hpp>
)

# 静的ライブラリ作成
add_library(nekocode_static STATIC ${NEKOCODE_SOURCES} ${NEKOCODE_HEADERS})
target_include_directories(nekocode_static PUBLIC 
    ${INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SRC_DIR}/analyzers
    ${SRC_DIR}/include
    $<$<BOOL:${NEKOCODE_ENABLE_UNIVERSAL_AST}>:${SRC_DIR}/universal>
    $<$<BOOL:${NEKOCODE_ENABLE_UNIVERSAL_AST}>:${SRC_DIR}/adapters>
    $<$<BOOL:${NEKOCODE_ENABLE_UNIVERSAL_AST}>:${SRC_DIR}>
)
target_link_libraries(nekocode_static 
    nlohmann_json::nlohmann_json 
    utf8::cpp
    tree-sitter-core
    tree-sitter-langs
    taocpp::pegtl
    Threads::Threads
)

# C++20 features
target_compile_features(nekocode_static PUBLIC cxx_std_20)

#=============================================================================
# 🤖 AI Tool - Claude Code最適化実行ファイル
#=============================================================================

add_executable(nekocode_ai 
    ${SRC_DIR}/main/main_ai.cpp
    ${SRC_DIR}/main/command_dispatcher.cpp
    ${SRC_DIR}/main/command_line_args.cpp
)
target_link_libraries(nekocode_ai nekocode_static)

# AI用特別最適化
target_compile_definitions(nekocode_ai PRIVATE 
    NEKOCODE_AI_MODE=1
    NEKOCODE_PERFORMANCE_PRIORITY=1
)


#=============================================================================
# 🧪 Tests
#=============================================================================

option(BUILD_TESTS "Build test suite" ON)
if(BUILD_TESTS)
    enable_testing()
    
    # 簡易テスト（テストファイルが存在する場合のみ）
    # add_executable(test_core ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_core.cpp)
    # target_link_libraries(test_core nekocode_static)
    # add_test(NAME CoreTest COMMAND test_core)
    
    # add_executable(test_performance ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_performance.cpp)
    # target_link_libraries(test_performance nekocode_static)
    # add_test(NAME PerformanceTest COMMAND test_performance)
endif()

#=============================================================================
# 📦 Install Configuration
#=============================================================================

option(ENABLE_INSTALL "Enable install target" ON)
if(ENABLE_INSTALL)
    # 実行ファイルインストール
    install(TARGETS nekocode_ai
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
    COMMENT "🚀 Build both AI and Human tools"
)

# カスタムターゲット: AI専用ビルド
add_custom_target(build_ai
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target nekocode_ai --parallel
    COMMENT "🤖 Build AI tool only"
)

# カスタムターゲット: Human専用ビルド
add_custom_target(build_human
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