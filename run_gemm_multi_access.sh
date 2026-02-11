#!/bin/bash

# GEMM multi-access coalescing coordination example runner

set -e

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Building GEMM Multi-Access Coalescing Example"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# 设置环境变量
TIRAMISU_ROOT="/Users/haorancheng/Desktop/tiramisu"
PLUTO_ROOT="/Users/haorancheng/Desktop/pluto"

export DYLD_LIBRARY_PATH="${TIRAMISU_ROOT}/build:${PLUTO_ROOT}/lib/.libs:${PLUTO_ROOT}/isl/.libs:${PLUTO_ROOT}/piplib/.libs:${PLUTO_ROOT}/polylib/.libs:${PLUTO_ROOT}/cloog-isl/.libs:${DYLD_LIBRARY_PATH}"

# 编译
mkdir -p build
cd build

if [ ! -f "Makefile" ]; then
    echo "Running CMake..."
    cmake ..
fi

echo "Building example_gemm_multi_access..."
make example_gemm_multi_access -j4

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Running GEMM Multi-Access Demo"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

./example_gemm_multi_access

echo ""
