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
    src/error_handler.cpp
    src/builtins/registry.cpp
    src/builtins/string_functions.cpp
    src/builtins/io_functions.cpp
    src/builtins/array_functions.cpp
    src/builtins/math_functions.cpp
    src/builtins/conversion_functions.cpp
    src/builtins/utility_functions.cpp
    src/builtins/json_functions.cpp
    src/builtins/http_functions.cpp
    src/builtins/canvas_functions.cpp
    src/builtins/canvas_3d_functions.cpp
    src/builtins/input_functions.cpp
    src/builtins/env_loader.cpp
    src/builtins/ui_functions.cpp
    src/ui_ast.cpp
    src/ui_parser.cpp
    src/ui_interpreter.cpp
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

# Find and link curl
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})

# Find SDL2
find_library(SDL2_LIBRARY SDL2 PATHS /opt/homebrew/lib REQUIRED)
find_library(SDL2_IMAGE_LIBRARY SDL2_image PATHS /opt/homebrew/lib REQUIRED)
find_path(SDL2_INCLUDE_DIR SDL2/SDL.h PATHS /opt/homebrew/include REQUIRED)

# Find OpenGL
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

# Find GTK3
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
include_directories(${GTK3_INCLUDE_DIRS})

if(NOT SDL2_LIBRARY)
    message(FATAL_ERROR "SDL2 not found")
endif()
if(NOT SDL2_IMAGE_LIBRARY)
    message(FATAL_ERROR "SDL2_image not found")
endif()

include_directories(${SDL2_INCLUDE_DIR})
message(STATUS "Found SDL2: ${SDL2_LIBRARY}")
message(STATUS "Found SDL2_image: ${SDL2_IMAGE_LIBRARY}")

# Link LLVM libraries
llvm_map_components_to_libnames(llvm_libs core native ExecutionEngine MCJIT RuntimeDyld)
target_link_libraries(compiler ${llvm_libs} ${CURL_LIBRARIES} ${SDL2_LIBRARY} ${SDL2_IMAGE_LIBRARY} ${OPENGL_LIBRARIES})
target_link_libraries(compiler ${GTK3_LDFLAGS})

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

# Install targets
install(TARGETS compiler DESTINATION bin RENAME axolotl)
install(DIRECTORY modules DESTINATION lib/axolotl OPTIONAL)
install(DIRECTORY env DESTINATION lib/axolotl OPTIONAL)
install(FILES README.md DESTINATION share/axolotl)
install(DIRECTORY tests DESTINATION share/axolotl/examples OPTIONAL)

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
