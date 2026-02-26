cmake_minimum_required(VERSION 3.20)
project(ASTERIXCodec VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── External dependency: pugixml (header+source, no heavy deps) ──────────────
include(FetchContent)
FetchContent_Declare(
    pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG        v1.14
)
FetchContent_MakeAvailable(pugixml)

# ── Library ───────────────────────────────────────────────────────────────────
add_library(ASTERIXCodec
    src/SpecLoader.cpp
    src/Codec.cpp
)

target_include_directories(ASTERIXCodec
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ASTERIXCodec
    PRIVATE pugixml::pugixml
)

target_compile_options(ASTERIXCodec PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# ── Tests ─────────────────────────────────────────────────────────────────────
option(ASTERIX_BUILD_TESTS "Build CAT01 test executable" ON)
if(ASTERIX_BUILD_TESTS)
    add_executable(test_cat01 tests/test_cat01.cpp)
    target_link_libraries(test_cat01 PRIVATE ASTERIXCodec)
endif()
