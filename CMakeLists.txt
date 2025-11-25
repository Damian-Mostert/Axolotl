cmake_minimum_required(VERSION 3.10)
project(CompilerEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${LLVM_INCLUDE_DIRS})

# Source files
set(SOURCES
    src/main.cpp
    src/token.cpp
    src/lexer.cpp
    src/ast.cpp
    src/parser.cpp
    src/interpreter.cpp
    src/operators.cpp
    src/jit.cpp
)

# Create executable
add_executable(compiler ${SOURCES})

# Enable optimizations for Release builds
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Enable aggressive optimizations
if(MSVC)
    target_compile_options(compiler PRIVATE /W4 /O2)
else()
    target_compile_options(compiler PRIVATE -Wall -Wextra -Wpedantic -O3 -march=native -flto)
    target_link_options(compiler PRIVATE -flto)
endif()

# Link LLVM libraries
llvm_map_components_to_libnames(llvm_libs core native ExecutionEngine MCJIT RuntimeDyld)
target_link_libraries(compiler ${llvm_libs})

# Link stdc++fs for GNU < 9 and also for AppleClang for filesystem support
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
        OUTPUT_VARIABLE GCC_VERSION
    )
    string(REGEX MATCHALL "[0-9]+" GCC_VERSION_COMPONENTS ${GCC_VERSION})
    list(GET GCC_VERSION_COMPONENTS 0 GCC_VERSION_MAJOR)
    if(GCC_VERSION_MAJOR LESS 9)
        target_link_libraries(compiler stdc++fs)
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    # No need to link stdc++fs for AppleClang on macOS
endif()

# Test executable
enable_testing()

# Example test file
set(TEST_FILE "${CMAKE_SOURCE_DIR}/examples/test.lang")
if(EXISTS ${TEST_FILE})
    add_test(
        NAME CompilerTest
        COMMAND compiler ${TEST_FILE}
    )
endif()
