cmake_minimum_required(VERSION 3.15)
project(asterix_tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Chemins relatifs depuis le dossier examples
set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(ASTERIX_INCLUDE_DIR "${PROJECT_ROOT}/include")
set(ASTERIX_BUILD_DIR "${PROJECT_ROOT}/build")

message(STATUS "=================================")
message(STATUS "ASTERIX Tests Configuration")
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

# Fonction pour créer un exécutable de test
function(add_asterix_test target_name source_file)
    add_executable(${target_name} ${source_file})
    
    target_include_directories(${target_name} PRIVATE 
        ${ASTERIX_INCLUDE_DIR}
        ${TINYXML2_INCLUDE_DIR}
    )
    
    target_link_libraries(${target_name} PRIVATE 
        ${ASTERIX_BUILD_DIR}/libasterix.a
        ${TINYXML2_LIBRARY}
    )
    
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    
    message(STATUS "✓ Added test target: ${target_name}")
endfunction()

# Créer les exécutables de test
add_asterix_test(test_cat002 test_cat002.cpp)
add_asterix_test(test_cat034 test_cat034.cpp)

message(STATUS "=================================")
message(STATUS "Configuration Summary:")
message(STATUS "  ASTERIX include: ${ASTERIX_INCLUDE_DIR}")
message(STATUS "  ASTERIX lib:     ${ASTERIX_BUILD_DIR}/libasterix.a")
message(STATUS "  TinyXML2 inc:    ${TINYXML2_INCLUDE_DIR}")
message(STATUS "  TinyXML2 lib:    ${TINYXML2_LIBRARY}")
message(STATUS "  Targets:         test_cat002, test_cat034")
message(STATUS "=================================")