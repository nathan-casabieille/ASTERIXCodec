cmake_minimum_required(VERSION 3.15)
project(asterix_cat002_test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Chemins relatifs depuis le dossier examples
set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(ASTERIX_INCLUDE_DIR "${PROJECT_ROOT}/include")
set(ASTERIX_BUILD_DIR "${PROJECT_ROOT}/build")

message(STATUS "=================================")
message(STATUS "ASTERIX CAT002 Test Configuration")
message(STATUS "=================================")
message(STATUS "Project root: ${PROJECT_ROOT}")
message(STATUS "Build dir: ${ASTERIX_BUILD_DIR}")

# Vérifier que la bibliothèque asterix existe
if(NOT EXISTS "${ASTERIX_BUILD_DIR}/libasterix.a")
    message(FATAL_ERROR 
        "Library libasterix.a not found in ${ASTERIX_BUILD_DIR}\n"
        "Please build the library first:\n"
        "  cd ${PROJECT_ROOT}/build\n"
        "  cmake .. && make\n")
endif()

message(STATUS "✓ Found libasterix.a")

# Trouver TinyXML2
find_path(TINYXML2_INCLUDE_DIR 
    NAMES tinyxml2.h
    PATHS "${ASTERIX_BUILD_DIR}/_deps/tinyxml2-src"
    NO_DEFAULT_PATH
)

find_library(TINYXML2_LIBRARY
    NAMES tinyxml2 libtinyxml2.a
    PATHS "${ASTERIX_BUILD_DIR}/_deps/tinyxml2-build"
    NO_DEFAULT_PATH
)

if(NOT TINYXML2_INCLUDE_DIR OR NOT TINYXML2_LIBRARY)
    message(FATAL_ERROR "TinyXML2 not found in build directory")
endif()

message(STATUS "✓ Found TinyXML2")

# Créer l'exécutable de test
add_executable(test_cat002 test_cat002.cpp)

# Inclure les headers
target_include_directories(test_cat002 PRIVATE 
    ${ASTERIX_INCLUDE_DIR}
    ${TINYXML2_INCLUDE_DIR}
)

# Lier les bibliothèques
target_link_libraries(test_cat002 PRIVATE 
    ${ASTERIX_BUILD_DIR}/libasterix.a
    ${TINYXML2_LIBRARY}
)

# Options de compilation
if(MSVC)
    target_compile_options(test_cat002 PRIVATE /W4)
else()
    target_compile_options(test_cat002 PRIVATE -Wall -Wextra -Wpedantic)
endif()

message(STATUS "=================================")
message(STATUS "Configuration Summary:")
message(STATUS "  ASTERIX include: ${ASTERIX_INCLUDE_DIR}")
message(STATUS "  ASTERIX lib:     ${ASTERIX_BUILD_DIR}/libasterix.a")
message(STATUS "  TinyXML2 inc:    ${TINYXML2_INCLUDE_DIR}")
message(STATUS "  TinyXML2 lib:    ${TINYXML2_LIBRARY}")
message(STATUS "=================================")