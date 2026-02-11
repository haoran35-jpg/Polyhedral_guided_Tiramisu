#!/bin/bash

# GEMM多访问模式Coalescing协调示例运行脚本

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
echo "✅ Demo completed!"
echo ""
echo "💡 关键发现:"
echo "   1. GEMM中A[i][k], B[k][j], C[i][j]无法同时coalesced"
echo "   2. 加权优化自动选择最大化coalesced流量的循环顺序"
echo "   3. 优先保证流量大的数组(C和B) coalesced"
echo "   4. 详细文档: MULTI_ACCESS_COALESCING.md"
echo ""
