#!/bin/bash

# Script de build simple - compile directement dans examples/
# Usage: ./simple_build.sh

set -e

echo "======================================"
echo "ASTERIX CAT002 - Simple Build"
echo "======================================"
echo ""

# Couleurs
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${SCRIPT_DIR}/.."

echo "Working directory: ${SCRIPT_DIR}"
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

# 2. Compiler le test dans le dossier examples
cd "${SCRIPT_DIR}"

echo "Configuring CMake..."
cmake . -DCMAKE_BUILD_TYPE=Release

if [ $? -ne 0 ]; then
    echo -e "${RED}✗ CMake configuration failed${NC}"
    exit 1
fi

echo ""
echo "Building test program..."
cmake --build . --config Release

if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}======================================"
    echo "✓ Build successful!"
    echo "======================================${NC}"
    echo ""
    
    # 3. Trouver le fichier de spécification
    SPEC_FILE="${PROJECT_ROOT}/specs/cat002_v1.1.xml"
    if [ ! -f "$SPEC_FILE" ]; then
        SPEC_FILE="${SCRIPT_DIR}/cat002_v1.1_minimal.xml"
        if [ ! -f "$SPEC_FILE" ]; then
            echo -e "${YELLOW}Warning: No specification file found${NC}"
            echo "Expected: ${PROJECT_ROOT}/specs/cat002_v1.1.xml"
            echo "      or: ${SCRIPT_DIR}/cat002_v1.1_minimal.xml"
            echo ""
            echo "You can run manually with:"
            echo "  ./test_cat002 /path/to/spec.xml"
            exit 0
        fi
        echo -e "${YELLOW}Using minimal spec file${NC}"
    fi
    
    echo "Running test..."
    echo "Spec file: ${SPEC_FILE}"
    echo ""
    ./test_cat002 "${SPEC_FILE}"
    
    echo ""
    echo "======================================"
    echo "Test complete!"
    echo "======================================"
    echo ""
    echo "To run again:"
    echo "  cd ${SCRIPT_DIR}"
    echo "  ./test_cat002 \"${SPEC_FILE}\""
    echo ""
    echo "To decode custom message:"
    echo "  ./test_cat002 \"${SPEC_FILE}\" \"02 00 0A E0 01 12 34 05\""
    echo ""
else
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
fi