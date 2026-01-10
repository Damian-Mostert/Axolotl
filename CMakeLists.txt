cmake_minimum_required(VERSION 3.10)
project(CompilerEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find LLVM (optional)
find_package(LLVM CONFIG)
if(LLVM_FOUND)
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
    list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
    include(AddLLVM)
    include_directories(${LLVM_INCLUDE_DIRS})
    set(HAVE_LLVM TRUE)
else()
    message(STATUS "LLVM not found - JIT compilation disabled")
    set(HAVE_LLVM FALSE)
endif()

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
    src/compiler.cpp
    src/operators.cpp
    src/jit.cpp
    src/error_handler.cpp
    src/physics_core.cpp
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
    src/builtins/mesh_deform.cpp
    src/builtins/env_loader.cpp
)

# Optional MySQL support
find_library(MYSQL_LIBRARY mysqlclient PATHS /usr/local/lib /opt/homebrew/lib)
if(MYSQL_LIBRARY)
    find_path(MYSQL_INCLUDE_DIR mysql/mysql.h PATHS /usr/local/include /opt/homebrew/include)
    if(MYSQL_INCLUDE_DIR)
        message(STATUS "Found MySQL: ${MYSQL_LIBRARY}")
        list(APPEND SOURCES src/builtins/mysql_functions.cpp)
        include_directories(${MYSQL_INCLUDE_DIR})
        set(HAVE_MYSQL TRUE)
    endif()
else()
    message(STATUS "MySQL not found - skipping MySQL support")
endif()

# Optional WebSocket support
find_library(WEBSOCKETS_LIBRARY websockets PATHS /usr/local/lib /opt/homebrew/lib)
if(WEBSOCKETS_LIBRARY)
    find_path(WEBSOCKETS_INCLUDE_DIR libwebsockets.h PATHS /usr/local/include /opt/homebrew/include)
    if(WEBSOCKETS_INCLUDE_DIR)
        message(STATUS "Found libwebsockets: ${WEBSOCKETS_LIBRARY}")
        list(APPEND SOURCES src/builtins/websocket_functions.cpp)
        include_directories(${WEBSOCKETS_INCLUDE_DIR})
        set(HAVE_WEBSOCKETS TRUE)
    endif()
else()
    message(STATUS "libwebsockets not found - skipping WebSocket support")
endif()

# Create static library (without main.cpp)
set(LIB_SOURCES ${SOURCES})
list(REMOVE_ITEM LIB_SOURCES src/main.cpp)
add_library(axolotl_static STATIC ${LIB_SOURCES})

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

# Find Assimp
find_library(ASSIMP_LIBRARY assimp PATHS /opt/homebrew/lib REQUIRED)
find_path(ASSIMP_INCLUDE_DIR assimp/scene.h PATHS /opt/homebrew/include REQUIRED)
include_directories(${ASSIMP_INCLUDE_DIR})
message(STATUS "Found Assimp: ${ASSIMP_LIBRARY}")

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
if(HAVE_LLVM)
    llvm_map_components_to_libnames(llvm_libs core native ExecutionEngine MCJIT RuntimeDyld)
    target_link_libraries(compiler ${llvm_libs})
endif()
target_link_libraries(compiler ${CURL_LIBRARIES} ${SDL2_LIBRARY} ${SDL2_IMAGE_LIBRARY} ${OPENGL_LIBRARIES} ${ASSIMP_LIBRARY})

# Link optional libraries
if(HAVE_MYSQL)
    target_link_libraries(compiler ${MYSQL_LIBRARY})
endif()
if(HAVE_WEBSOCKETS)
    target_link_libraries(compiler ${WEBSOCKETS_LIBRARY})
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

# Install targets
install(TARGETS compiler DESTINATION bin RENAME axolotl)
install(TARGETS axolotl_static DESTINATION lib/axolotl ARCHIVE DESTINATION lib/axolotl)
install(DIRECTORY modules DESTINATION lib/axolotl OPTIONAL)
install(DIRECTORY env DESTINATION lib/axolotl OPTIONAL)
install(FILES README.md DESTINATION share/axolotl)
install(DIRECTORY tests DESTINATION share/axolotl/examples OPTIONAL)

# Generate builtins.json for VS Code extension
if(WIN32)
    add_custom_command(
        TARGET compiler POST_BUILD
        COMMAND cmd /c ${CMAKE_SOURCE_DIR}/scripts/extract_builtins.bat
                ${CMAKE_SOURCE_DIR}/vs-code-extentions/axolotl-extension/builtins.json
                ${CMAKE_SOURCE_DIR}/src/builtins/*.cpp
        COMMENT "Generating builtins.json for VS Code extension"
    )
else()
    add_custom_command(
        TARGET compiler POST_BUILD
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/extract_builtins.sh
                ${CMAKE_SOURCE_DIR}/vs-code-extentions/axolotl-extension/builtins.json
                ${CMAKE_SOURCE_DIR}/src/builtins/*.cpp
        COMMENT "Generating builtins.json for VS Code extension"
    )
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
