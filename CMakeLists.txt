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
option(ASTERIX_BUILD_TESTS "Build test executables" ON)
if(ASTERIX_BUILD_TESTS)
    add_executable(test_cat01 tests/test_cat01.cpp)
    target_link_libraries(test_cat01 PRIVATE ASTERIXCodec)

    add_executable(test_cat02 tests/test_cat02.cpp)
    target_link_libraries(test_cat02 PRIVATE ASTERIXCodec)

    add_executable(test_cat34 tests/test_cat34.cpp)
    target_link_libraries(test_cat34 PRIVATE ASTERIXCodec)

    add_executable(test_cat48 tests/test_cat48.cpp)
    target_link_libraries(test_cat48 PRIVATE ASTERIXCodec)

    add_executable(test_cat62 tests/test_cat62.cpp)
    target_link_libraries(test_cat62 PRIVATE ASTERIXCodec)

    add_executable(test_cat21 tests/test_cat21.cpp)
    target_link_libraries(test_cat21 PRIVATE ASTERIXCodec)

    add_executable(test_cat63 tests/test_cat63.cpp)
    target_link_libraries(test_cat63 PRIVATE ASTERIXCodec)

    add_executable(test_cat08 tests/test_cat08.cpp)
    target_link_libraries(test_cat08 PRIVATE ASTERIXCodec)

    add_executable(test_cat09 tests/test_cat09.cpp)
    target_link_libraries(test_cat09 PRIVATE ASTERIXCodec)

    add_executable(test_cat16 tests/test_cat16.cpp)
    target_link_libraries(test_cat16 PRIVATE ASTERIXCodec)

    add_executable(test_cat17 tests/test_cat17.cpp)
    target_link_libraries(test_cat17 PRIVATE ASTERIXCodec)

    add_executable(test_cat19 tests/test_cat19.cpp)
    target_link_libraries(test_cat19 PRIVATE ASTERIXCodec)

    add_executable(test_cat23 tests/test_cat23.cpp)
    target_link_libraries(test_cat23 PRIVATE ASTERIXCodec)

    add_executable(test_cat32 tests/test_cat32.cpp)
    target_link_libraries(test_cat32 PRIVATE ASTERIXCodec)
endif()
