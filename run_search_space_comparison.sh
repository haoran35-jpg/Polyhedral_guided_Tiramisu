#!/bin/bash

# 搜索空间对比实验运行脚本

set -e

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Building Search Space Comparison Benchmark"
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

echo "Building benchmark_search_space_comparison..."
make benchmark_search_space_comparison -j4

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Running Search Space Comparison"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

./benchmark_search_space_comparison | tee ../search_space_results.txt

echo ""
echo "✅ Results saved to search_space_results.txt"
echo ""
echo "This output can be included in your paper's evaluation section!"
echo ""
