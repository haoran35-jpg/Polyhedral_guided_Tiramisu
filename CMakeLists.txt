cmake_minimum_required(VERSION 3.10)
project(PlutoTiramisuBridge CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# Dependencies (as git submodules)
# PLUTO: https://github.com/bondhugula/pluto
# Tiramisu: https://github.com/Tiramisu-Compiler/tiramisu

if(DEFINED ENV{PLUTO_ROOT})
    set(PLUTO_ROOT $ENV{PLUTO_ROOT})
else()
    set(PLUTO_ROOT "${CMAKE_SOURCE_DIR}/external/pluto")
endif()
set(PLUTO_INCLUDE_DIRS 
    "${PLUTO_ROOT}/include"
    "${PLUTO_ROOT}/lib"
    "${PLUTO_ROOT}/isl/include"
)
set(PLUTO_LIBRARY_DIRS
    "${PLUTO_ROOT}/lib/.libs"
    "${PLUTO_ROOT}/isl/.libs"
)

if(DEFINED ENV{TIRAMISU_ROOT})
    set(TIRAMISU_ROOT $ENV{TIRAMISU_ROOT})
else()
    set(TIRAMISU_ROOT "${CMAKE_SOURCE_DIR}/external/tiramisu")
endif()
set(TIRAMISU_INCLUDE_DIRS
    "${TIRAMISU_ROOT}/include"
    "${TIRAMISU_ROOT}/3rdParty/Halide/include"
    "${TIRAMISU_ROOT}/3rdParty/Halide/build/include"
    "${TIRAMISU_ROOT}/3rdParty/isl/include"
    "/opt/homebrew/include"
)
set(TIRAMISU_LIBRARY_DIRS
    "${TIRAMISU_ROOT}/build"
    "${TIRAMISU_ROOT}/3rdParty/Halide/build/src"
    "${TIRAMISU_ROOT}/3rdParty/isl/.libs"
    "/opt/homebrew/lib"
)

# 包含目录
include_directories(
    ${PLUTO_INCLUDE_DIRS}
    ${TIRAMISU_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 库目录
link_directories(
    ${PLUTO_LIBRARY_DIRS}
    ${TIRAMISU_LIBRARY_DIRS}
)

# 桥接库
add_library(pluto_tiramisu_bridge
    pluto_to_tiramisu.cpp
    pluto_guided_search.cpp
)

target_link_libraries(pluto_tiramisu_bridge
    pluto
    isl
    tiramisu
    Halide
)

# 测试程序 (简化版)
add_executable(test_bridge_simple
    test_bridge_simple.cpp
)

target_link_libraries(test_bridge_simple
    pluto_tiramisu_bridge
    tiramisu
    Halide
    pthread
    dl
    z
    m
)

# 完整测试程序（真正调用PLUTO - 打印transformation matrix）
add_executable(test_bridge
    test_bridge.cpp
)

target_link_libraries(test_bridge
    pluto_tiramisu_bridge
    tiramisu
    Halide
    pthread
    dl
    z
    m
)

# Demo: 打印PLUTO transformation matrix
add_executable(demo_print_matrix
    demo_print_matrix.cpp
)

target_link_libraries(demo_print_matrix
    pluto_tiramisu_bridge
    tiramisu
    Halide
    pthread
    dl
    z
    m
)

# Demo: 完整功能 - 任意维度和迭代器名称
add_executable(demo_full_features
    demo_full_features.cpp
)

target_link_libraries(demo_full_features
    pluto_tiramisu_bridge
    tiramisu
    Halide
    pthread
    dl
    z
    m
)

# Example: 实际应用 - 3D GEMM with generic support
add_executable(example_generic_gemm
    example_generic_gemm.cpp
)

target_link_libraries(example_generic_gemm
    pluto_tiramisu_bridge
    tiramisu
    Halide
    pthread
    dl
    z
    m
)

# Example: 如何输入你自己的循环
add_executable(example_your_own_loop
    example_your_own_loop.cpp
)

target_link_libraries(example_your_own_loop
    pluto_tiramisu_bridge
    tiramisu
    Halide
    pthread
    dl
    z
    m
)

# 示例程序: 矩阵转置
add_executable(example_matrix_transpose
    example_matrix_transpose.cpp
)

target_link_libraries(example_matrix_transpose
    pluto_tiramisu_bridge
    tiramisu
    Halide
    pthread
    dl
    z
    m
)

# 基准测试: 调度搜索时间对比
add_executable(benchmark_schedule_search
    benchmark_schedule_search.cpp
)

target_link_libraries(benchmark_schedule_search
    pluto_tiramisu_bridge
    tiramisu
    Halide
    pthread
    dl
    z
    m
)

# 基准测试: PLUTO vs Tiramisu自动调度器
add_executable(benchmark_vs_autoscheduler
    benchmark_vs_autoscheduler.cpp
)

# 添加auto_scheduler库路径
link_directories("${TIRAMISU_ROOT}/build")

target_link_libraries(benchmark_vs_autoscheduler
    pluto_tiramisu_bridge
    tiramisu_auto_scheduler
    tiramisu
    Halide
    pthread
    dl
    z
    m
)

# 基准测试: GEMM (更复杂的测试)
add_executable(benchmark_gemm
    benchmark_gemm.cpp
)

target_link_libraries(benchmark_gemm
    pluto_tiramisu_bridge
    tiramisu
    Halide
    pthread
    dl
    z
    m
)

# 基准测试: Convolution (真实auto_scheduler对比)
add_executable(benchmark_convolution
    benchmark_convolution.cpp
)

target_link_libraries(benchmark_convolution
    pluto_tiramisu_bridge
    tiramisu_auto_scheduler
    tiramisu
    Halide
    pthread
    dl
    z
    m
)

# 基准测试: Blur (多阶段计算，真实auto_scheduler对比)
add_executable(benchmark_blur
    benchmark_blur.cpp
)

target_link_libraries(benchmark_blur
    pluto_tiramisu_bridge
    tiramisu_auto_scheduler
    tiramisu
    Halide
    pthread
    dl
    z
    m
)

# 基准测试: MatMul Real (基于Tiramisu官方例子)
add_executable(benchmark_matmul_real
    benchmark_matmul_real.cpp
)

target_link_libraries(benchmark_matmul_real
    pluto_tiramisu_bridge
    tiramisu_auto_scheduler
    tiramisu
    Halide
    pthread
    dl
    z
    m
)

# 基准测试: Real AutoScheduler (使用常量边界)
add_executable(benchmark_real_autoscheduler
    benchmark_real_autoscheduler.cpp
)

target_link_libraries(benchmark_real_autoscheduler
    pluto_tiramisu_bridge
    tiramisu_auto_scheduler
    tiramisu
    Halide
    pthread
    dl
    z
    m
)

# 基准测试: Simple Copy (最简单的真实对比)
add_executable(benchmark_simple_copy
    benchmark_simple_copy.cpp
)

target_link_libraries(benchmark_simple_copy
    pluto_tiramisu_bridge
    tiramisu_auto_scheduler
    tiramisu
    Halide
    pthread
    dl
    z
    m
)

# 基准测试: Final Comparison (真实程序评估)
add_executable(benchmark_final_comparison
    benchmark_final_comparison.cpp
)

target_link_libraries(benchmark_final_comparison
    pluto_tiramisu_bridge
    tiramisu
    Halide
    pthread
    dl
    z
    m
)

# 基准测试: Tiramisu Search (模拟Tiramisu Beam Search)
add_executable(benchmark_tiramisu_search
    benchmark_tiramisu_search.cpp
)

target_link_libraries(benchmark_tiramisu_search
    pluto_tiramisu_bridge
    tiramisu
    Halide
    pthread
    dl
    z
    m
)

# 基准测试: With PLUTO (真实调用PLUTO打印配置)
add_executable(benchmark_with_pluto
    benchmark_with_pluto.cpp
)

target_link_libraries(benchmark_with_pluto
    pluto_tiramisu_bridge
    tiramisu
    Halide
    pthread
    dl
    z
    m
)

# 混合优化示例
add_executable(example_hybrid_optimization
    example_hybrid_optimization.cpp
)

target_link_libraries(example_hybrid_optimization
    pluto_tiramisu_bridge
    tiramisu
    Halide
    pluto
    isl
    gmp
    pthread
    dl
    z
    m
)

# Bank conflict策略示例
add_executable(example_bank_conflict_strategies
    example_bank_conflict_strategies.cpp
)

target_link_libraries(example_bank_conflict_strategies
    pluto_tiramisu_bridge
    tiramisu
    Halide
    pluto
    isl
    gmp
    pthread
    dl
    z
    m
)

# GEMM多访问模式示例
add_executable(example_gemm_multi_access
    example_gemm_multi_access.cpp
)

target_link_libraries(example_gemm_multi_access
    pluto_tiramisu_bridge
    tiramisu
    Halide
    pluto
    isl
    gmp
    pthread
    dl
    z
    m
)

# 搜索空间对比实验
add_executable(benchmark_search_space_comparison
    benchmark_search_space_comparison.cpp
)

target_link_libraries(benchmark_search_space_comparison
    pluto_tiramisu_bridge
    tiramisu
    Halide
    pluto
    isl
    gmp
    pthread
    dl
    z
    m
)

# 安装
install(TARGETS pluto_tiramisu_bridge test_bridge_simple example_matrix_transpose benchmark_schedule_search benchmark_vs_autoscheduler benchmark_gemm benchmark_convolution benchmark_blur benchmark_matmul_real benchmark_real_autoscheduler benchmark_simple_copy example_hybrid_optimization example_bank_conflict_strategies example_gemm_multi_access benchmark_search_space_comparison
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
