#!/bin/bash

# Script de build simple - compile directement dans examples/
# Usage: ./simple_build.sh [cat002|cat034|all]

set -e

echo "======================================"
echo "ASTERIX Tests - Simple Build"
echo "======================================"
echo ""

# Couleurs
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${SCRIPT_DIR}/.."

echo "Working directory: ${SCRIPT_DIR}"
echo ""

# Déterminer quelle cible construire
TARGET="${1:-all}"

if [[ "$TARGET" != "cat002" && "$TARGET" != "cat034" && "$TARGET" != "all" ]]; then
    echo -e "${RED}✗ Invalid target: ${TARGET}${NC}"
    echo "Usage: $0 [cat002|cat034|all]"
    echo "  cat002 - Build only test_cat002"
    echo "  cat034 - Build only test_cat034"
    echo "  all    - Build both tests (default)"
    exit 1
fi

echo -e "${BLUE}Target: ${TARGET}${NC}"
echo ""

# 1. Vérifier que la bibliothèque asterix existe
if [ ! -f "${PROJECT_ROOT}/build/libasterix.a" ]; then
    echo -e "${RED}✗ Library not found: ${PROJECT_ROOT}/build/libasterix.a${NC}"
    echo ""
    echo "Please build the library first:"
    echo "  cd ${PROJECT_ROOT}"
    echo "  mkdir -p build && cd build"
    echo "  cmake .. && make -j"
    echo ""
    exit 1
fi

echo -e "${GREEN}✓ Found libasterix.a${NC}"
echo ""

# 2. Compiler le(s) test(s) dans le dossier examples
cd "${SCRIPT_DIR}"

echo "Configuring CMake..."
cmake . -DCMAKE_BUILD_TYPE=Release

if [ $? -ne 0 ]; then
    echo -e "${RED}✗ CMake configuration failed${NC}"
    exit 1
fi

echo ""
echo "Building test program(s)..."

# Construire selon la cible
if [ "$TARGET" = "all" ]; then
    cmake --build . --config Release
    BUILD_SUCCESS=$?
elif [ "$TARGET" = "cat002" ]; then
    cmake --build . --config Release --target test_cat002
    BUILD_SUCCESS=$?
elif [ "$TARGET" = "cat034" ]; then
    cmake --build . --config Release --target test_cat034
    BUILD_SUCCESS=$?
fi

if [ $BUILD_SUCCESS -eq 0 ]; then
    echo ""
    echo -e "${GREEN}======================================"
    echo "✓ Build successful!"
    echo "======================================${NC}"
    echo ""
    
    # 3. Exécuter les tests selon la cible
    if [ "$TARGET" = "cat002" ] || [ "$TARGET" = "all" ]; then
        # Test CAT002
        SPEC_FILE_002="${PROJECT_ROOT}/specs/cat002_v1.1.xml"
        if [ ! -f "$SPEC_FILE_002" ]; then
            SPEC_FILE_002="${SCRIPT_DIR}/cat002_v1.1_minimal.xml"
            if [ ! -f "$SPEC_FILE_002" ]; then
                echo -e "${YELLOW}Warning: CAT002 specification file not found${NC}"
                SPEC_FILE_002=""
            else
                echo -e "${YELLOW}Using minimal CAT002 spec file${NC}"
            fi
        fi
        
        if [ -n "$SPEC_FILE_002" ] && [ -f "./test_cat002" ]; then
            echo ""
            echo -e "${BLUE}Running CAT002 test...${NC}"
            echo "Spec file: ${SPEC_FILE_002}"
            echo ""
            ./test_cat002 "${SPEC_FILE_002}"
            echo ""
        fi
    fi
    
    if [ "$TARGET" = "cat034" ] || [ "$TARGET" = "all" ]; then
        # Test CAT034
        SPEC_FILE_034="${PROJECT_ROOT}/specs/cat034_v1.1.xml"
        if [ ! -f "$SPEC_FILE_034" ]; then
            echo -e "${YELLOW}Warning: CAT034 specification file not found${NC}"
            echo "Expected: ${SPEC_FILE_034}"
            SPEC_FILE_034=""
        fi
        
        if [ -n "$SPEC_FILE_034" ] && [ -f "./test_cat034" ]; then
            echo ""
            echo -e "${BLUE}Running CAT034 test...${NC}"
            echo "Spec file: ${SPEC_FILE_034}"
            echo ""
            ./test_cat034 "${SPEC_FILE_034}"
            echo ""
        fi
    fi
    
    echo "======================================"
    echo "Tests complete!"
    echo "======================================"
    echo ""
    echo "Available executables:"
    [ -f "./test_cat002" ] && echo "  - test_cat002"
    [ -f "./test_cat034" ] && echo "  - test_cat034"
    echo ""
    
    echo "To run tests manually:"
    if [ -f "./test_cat002" ] && [ -n "$SPEC_FILE_002" ]; then
        echo "  CAT002:"
        echo "    cd ${SCRIPT_DIR}"
        echo "    ./test_cat002 \"${SPEC_FILE_002}\""
        echo "    ./test_cat002 \"${SPEC_FILE_002}\" \"02 00 0A E0 01 12 34 05\""
        echo ""
    fi
    
    if [ -f "./test_cat034" ] && [ -n "$SPEC_FILE_034" ]; then
        echo "  CAT034:"
        echo "    cd ${SCRIPT_DIR}"
        echo "    ./test_cat034 \"${SPEC_FILE_034}\""
        echo "    ./test_cat034 \"${SPEC_FILE_034}\" \"22 00 1A E0 03 05 01 00 04 D2 F0\""
        echo ""
    fi
    
    echo "To build specific target:"
    echo "  ./simple_build.sh cat002"
    echo "  ./simple_build.sh cat034"
    echo "  ./simple_build.sh all"
    echo ""
else
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
fi