cmake_minimum_required(VERSION 3.10)
project(CompilerEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR})

# Source files
set(SOURCES
    src/main.cpp
    src/token.cpp
    src/lexer.cpp
    src/ast.cpp
    src/parser.cpp
    src/interpreter.cpp
)

# Create executable
add_executable(compiler ${SOURCES})

# Enable all warnings
if(MSVC)
    target_compile_options(compiler PRIVATE /W4)
else()
    target_compile_options(compiler PRIVATE -Wall -Wextra -Wpedantic)
endif()

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
