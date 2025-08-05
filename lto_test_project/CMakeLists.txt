cmake_minimum_required(VERSION 3.10)
project(LTODeadCodeTest)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main.cpp
    src/used_module.cpp
    src/unused_module.cpp
)

# Create executable
add_executable(lto_test ${SOURCES})

# Optional: Enable LTO by default for this test project
# (can be overridden by command line)
option(ENABLE_LTO "Enable Link Time Optimization" OFF)

if(ENABLE_LTO)
    set_property(TARGET lto_test PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    message(STATUS "LTO enabled for lto_test")
endif()