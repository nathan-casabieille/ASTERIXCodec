cmake_minimum_required(VERSION 3.15)
project(asterix VERSION 1.0.0 LANGUAGES CXX)

# Configuration C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options de compilation
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ASTERIX_BUILD_TESTS "Build tests" ON)
option(ASTERIX_BUILD_EXAMPLES "Build examples" ON)

# Télécharger TinyXML2 automatiquement
include(FetchContent)
FetchContent_Declare(
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG 9.0.0
)
FetchContent_MakeAvailable(tinyxml2)

# Sources de la bibliothèque
set(ASTERIX_SOURCES
    src/core/asterix_category.cpp
    src/core/asterix_decoder.cpp
    src/core/asterix_message.cpp
    src/core/types.cpp
    src/data/data_item.cpp
    src/data/field.cpp
    src/data/field_value.cpp
    src/spec/data_item_spec.cpp
    src/spec/field_spec.cpp
    src/spec/uap_spec.cpp
    src/spec/xml_parser.cpp
    src/utils/bit_reader.cpp
    src/utils/byte_buffer.cpp
    src/utils/exceptions.cpp
)

# Headers publics
set(ASTERIX_HEADERS
    include/asterix/asterix.hpp
    include/asterix/core/asterix_category.hpp
    include/asterix/core/asterix_decoder.hpp
    include/asterix/core/asterix_message.hpp
    include/asterix/core/types.hpp
    include/asterix/data/data_item.hpp
    include/asterix/data/field.hpp
    include/asterix/data/field_value.hpp
    include/asterix/spec/data_item_spec.hpp
    include/asterix/spec/field_spec.hpp
    include/asterix/spec/uap_spec.hpp
    include/asterix/spec/xml_parser.hpp
    include/asterix/utils/bit_reader.hpp
    include/asterix/utils/byte_buffer.hpp
    include/asterix/utils/exceptions.hpp
)

# Créer la bibliothèque
add_library(asterix ${ASTERIX_SOURCES} ${ASTERIX_HEADERS})

# Alias pour usage interne
add_library(asterix::asterix ALIAS asterix)

# Inclure les répertoires
target_include_directories(asterix
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Lier les dépendances
target_link_libraries(asterix
    PUBLIC
        tinyxml2::tinyxml2
)

# Options de compilation
if(MSVC)
    target_compile_options(asterix PRIVATE /W4)
else()
    target_compile_options(asterix PRIVATE 
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
    )
endif()

# Configuration pour l'installation
include(GNUInstallDirs)

install(TARGETS asterix
    EXPORT asterixTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/asterix
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
)

install(EXPORT asterixTargets
    FILE asterixTargets.cmake
    NAMESPACE asterix::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/asterix
)