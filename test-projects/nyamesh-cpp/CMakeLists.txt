# =============================================
# nyamesh_v22 革命的軽量メッセージング
# =============================================
# 
# 特徴:
# - JSON完全排除（純粋バイナリメッセージング）
# - Interface+Pimpl完全分離
# - UltraLightCoreBase実装
# - Phase B: ヘッダー完全独立化完成
# - Phase C: C++20 modules準備完了
#
# @version 2.0 - Pure Implementation
# @date 2025-07-25

cmake_minimum_required(VERSION 3.20)

# Project definition
project(nyamesh_v22 VERSION 2.0 LANGUAGES CXX)

# C++20 required for nyamesh_v22
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================
# nyamesh_v22 Core Sources - 純粋実装
# =============================================

set(NYAMESH_V22_CORE_SOURCES
    # Core messaging system (JSON-free)
    core/LightMessaging.cpp
    core/UltraLightCoreBase.cpp
    core/nyamesh.cpp
    
    # Debug system
    debug/NyaMeshDebugLogger.cpp
    
    # Intent system (lightweight)
    intents/IntentCommonOptions.cpp
    
    # Message structures (pure C++)
    messages/CoreMessages.cpp
)

set(NYAMESH_V22_CORES_SOURCES
    # Revolutionary v22 cores
    cores/SettingsCore_v22.cpp
    cores/EditorCore_v22.cpp
    cores/FileSystemCore_v22.cpp
)

set(NYAMESH_V22_TRANSPORT_SOURCES
    # Pure transport layer (temporarily disabled - needs v22 migration)
    # transport/InProcessTransport.cpp
    # transport/NyaMesh2TransportAdapter.cpp  
    # transport/P2PTransportAdapter.cpp
    # transport/P2PTransport.cpp
)

# =============================================
# nyamesh_v22 Headers - 完全独立
# =============================================

set(NYAMESH_V22_HEADERS
    # Public interfaces (JSON-free)
    core/LightMessaging.h
    core/UltraLightCoreBase.h
    core/nyamesh.h
    
    # Core interfaces (pure)
    interfaces/ISettingsCore.h
    interfaces/IEditorCore.h
    interfaces/IFileSystemCore.h
    
    # Revolutionary cores
    cores/SettingsCore_v22.h
    cores/EditorCore_v22.h
    cores/FileSystemCore_v22.h
    
    # Message types
    messages/CoreMessages.h
    
    # Debug
    debug/NyaMeshDebugLogger.h
)

# =============================================
# nyamesh_v22 Library - 軽量独立ライブラリ
# =============================================

# Create static library for nyamesh_v22
add_library(nyamesh_v22 STATIC
    ${NYAMESH_V22_CORE_SOURCES}
    ${NYAMESH_V22_CORES_SOURCES}
    ${NYAMESH_V22_TRANSPORT_SOURCES}
)

# Include directories for nyamesh_v22
target_include_directories(nyamesh_v22 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/interfaces
    ${CMAKE_CURRENT_SOURCE_DIR}/cores
    ${CMAKE_CURRENT_SOURCE_DIR}/messages
    ${CMAKE_CURRENT_SOURCE_DIR}/debug
)

# =============================================
# Dependencies - 最小限
# =============================================

# Qt6 for EditorCore_v22 (hidden in .cpp)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Enable Qt MOC for Qt objects
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

target_link_libraries(nyamesh_v22 PRIVATE
    Qt6::Core
    Qt6::Widgets
)

# Threading support
if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(nyamesh_v22 PRIVATE Threads::Threads)
endif()

# =============================================
# Test Applications - Core Integration
# =============================================

# CharmEditor_2Core_v22 - 革命的統合テスト
add_executable(CharmEditor_2Core_v22
    demos/CharmEditor_2Core_v22.cpp
)

# CharmEditor_3Core_v22 - 3コア統合テスト（FileSystem追加）
add_executable(CharmEditor_3Core_v22
    demos/CharmEditor_3Core_v22.cpp
)

target_link_libraries(CharmEditor_2Core_v22 PRIVATE
    nyamesh_v22
    Qt6::Core
    Qt6::Widgets
)

target_link_libraries(CharmEditor_3Core_v22 PRIVATE
    nyamesh_v22
    Qt6::Core
    Qt6::Widgets
)

# =============================================
# Windows Support - 実証済みパターン
# =============================================

if(WIN32)
    # Enable Qt MOC for Qt integration
    set(CMAKE_AUTOMOC ON)
    
    # Windows executable settings
    set_target_properties(CharmEditor_2Core_v22 PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    set_target_properties(CharmEditor_3Core_v22 PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    
    target_compile_definitions(CharmEditor_2Core_v22 PRIVATE
        _WIN32_WINNT=0x0601
        NOMINMAX
    )
    target_compile_definitions(CharmEditor_3Core_v22 PRIVATE
        _WIN32_WINNT=0x0601
        NOMINMAX
    )
    
    # Copy Qt6 DLLs (proven success pattern)
    add_custom_command(TARGET CharmEditor_2Core_v22 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Core>
            $<TARGET_FILE:Qt6::Widgets>
            $<TARGET_FILE:Qt6::Gui>
            $<TARGET_FILE_DIR:CharmEditor_2Core_v22>
    )
    
    # Copy Qt6 platform plugin (essential)
    add_custom_command(TARGET CharmEditor_2Core_v22 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:CharmEditor_2Core_v22>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE_DIR:Qt6::Core>/../plugins/platforms/qwindows$<$<CONFIG:Debug>:d>.dll"
            "$<TARGET_FILE_DIR:CharmEditor_2Core_v22>/platforms/"
    )
    
    # Copy Qt6 DLLs for 3Core app
    add_custom_command(TARGET CharmEditor_3Core_v22 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Core>
            $<TARGET_FILE:Qt6::Widgets>
            $<TARGET_FILE:Qt6::Gui>
            $<TARGET_FILE_DIR:CharmEditor_3Core_v22>
    )
    
    # Copy Qt6 platform plugin for 3Core app
    add_custom_command(TARGET CharmEditor_3Core_v22 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:CharmEditor_3Core_v22>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE_DIR:Qt6::Core>/../plugins/platforms/qwindows$<$<CONFIG:Debug>:d>.dll"
            "$<TARGET_FILE_DIR:CharmEditor_3Core_v22>/platforms/"
    )
    
    # Auto-copy to windows-executables
    add_custom_command(TARGET CharmEditor_2Core_v22 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/../../windows-executables"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:CharmEditor_2Core_v22>
            "${CMAKE_CURRENT_SOURCE_DIR}/../../windows-executables/"
    )
    add_custom_command(TARGET CharmEditor_3Core_v22 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/../../windows-executables"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:CharmEditor_3Core_v22>
            "${CMAKE_CURRENT_SOURCE_DIR}/../../windows-executables/"
    )
endif()

# =============================================
# Configuration Summary
# =============================================

message(STATUS "nyamesh_v22 configuration complete - Pure Implementation")
message(STATUS "Features: JSON-free binary messaging + Interface+Pimpl")
message(STATUS "Cores: SettingsCore_v22 + EditorCore_v22 + FileSystemCore_v22")
message(STATUS "Phase: B (Header Independence) → Phase 2 (3-Core Integration)")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")