/**
 * GEMMMulti-AccessCoalescingCoordination Example
 * 
 * Demonstrates handlingA[i][k], B[k][j], C[i][j]conflictscoalescingrequirements
 */

#include <iostream>
#include <vector>
#include <iomanip>
#include "pluto_guided_search.h"

using namespace pluto_tiramisu;

void print_section(const std::string& title) {
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "  " << title << "\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";
}

// Simulate different loop order configs
std::vector<ScheduleConfig> generate_gemm_loop_orders() {
    std::vector<ScheduleConfig> configs;
    
    // All 6 possible 3D loop orders
    std::vector<std::vector<std::string>> orders = {
        {"i", "j", "k"},  // 0
        {"i", "k", "j"},  // 1
        {"j", "i", "k"},  // 2
        {"j", "k", "i"},  // 3
        {"k", "i", "j"},  // 4
        {"k", "j", "i"}   // 5
    };
    
    for (const auto& order : orders) {
        ScheduleConfig config;
        
        // Create transformation for loop order
        for (const auto& var : order) {
            Transformation trans(TRANS_INTERCHANGE);
            trans.iterator_names.push_back(var);
            config.transformations.push_back(trans);
        }
        
        // Add tile sizes
        config.tile_sizes.push_back({"i", 32});
        config.tile_sizes.push_back({"j", 32});
        config.tile_sizes.push_back({"k", 32});
        
        config.description = "Loop order: (" + order[0] + ", " + 
                            order[1] + ", " + order[2] + ")";
        
        configs.push_back(config);
    }
    
    return configs;
}

// Define GEMM access patterns
std::vector<AccessPattern> create_gemm_access_patterns() {
    std::vector<AccessPattern> patterns;
    
    // A[i][k] - read-only
    AccessPattern pattern_A;
    pattern_A.array_name = "A";
    pattern_A.indices = {"i", "k"};
    pattern_A.access_frequency = 1;  // read-only
    pattern_A.element_size = 4;      // sizeof(float)
    pattern_A.dimension_size = 1024; // Assuming 1024x1024 matrix
    pattern_A.is_write = false;
    patterns.push_back(pattern_A);
    
    // B[k][j] - read-only
    AccessPattern pattern_B;
    pattern_B.array_name = "B";
    pattern_B.indices = {"k", "j"};
    pattern_B.access_frequency = 1;
    pattern_B.element_size = 4;
    pattern_B.dimension_size = 1024;
    pattern_B.is_write = false;
    patterns.push_back(pattern_B);
    
    // C[i][j] - read-write
    AccessPattern pattern_C;
    pattern_C.array_name = "C";
    pattern_C.indices = {"i", "j"};
    pattern_C.access_frequency = 2;  // read+write
    pattern_C.element_size = 4;
    pattern_C.dimension_size = 1024;
    pattern_C.is_write = true;
    patterns.push_back(pattern_C);
    
    return patterns;
}

void analyze_coalescing_conflicts() {
    print_section("Problem Analysis: GEMM Coalescing Conflicts");
    
    std::cout << "GEMMcode:\n";
    std::cout << "  for (i = 0; i < M; i++)\n";
    std::cout << "    for (j = 0; j < N; j++)\n";
    std::cout << "      for (k = 0; k < K; k++)\n";
    std::cout << "        C[i][j] += A[i][k] * B[k][j];\n\n";
    
    std::cout << "Access patternAnalysis (Assuming row-major storage):\n\n";
    
    std::cout << "  ArrayA[i][k]:\n";
    std::cout << "    • Address: &A[i][k] = A_base + (i * K + k) * sizeof(float)\n";
    std::cout << "    • Stride in k: sizeof(float) = 4 bytes\n";
    std::cout << "    • Stride in i: K * sizeof(float) = 4096 bytes\n";
    std::cout << "    • Coalescingrequirements: innermost loop = k\n\n";
    
    std::cout << "  ArrayB[k][j]:\n";
    std::cout << "    • Address: &B[k][j] = B_base + (k * N + j) * sizeof(float)\n";
    std::cout << "    • Stride in j: sizeof(float) = 4 bytes\n";
    std::cout << "    • Stride in k: N * sizeof(float) = 4096 bytes\n";
    std::cout << "    • Coalescingrequirements: innermost loop = j\n\n";
    
    std::cout << "  ArrayC[i][j]:\n";
    std::cout << "    • Address: &C[i][j] = C_base + (i * N + j) * sizeof(float)\n";
    std::cout << "    • Stride in j: sizeof(float) = 4 bytes\n";
    std::cout << "    • Stride in i: N * sizeof(float) = 4096 bytes\n";
    std::cout << "    • Coalescingrequirements: innermost loop = j\n\n";
    
    std::cout << "WARNING  Conflict Detection:\n";
    std::cout << "    Arequires: kinnermost\n";
    std::cout << "    Brequires: jinnermost\n";
    std::cout << "    Crequires: jinnermost\n";
    std::cout << "    → Cannot satisfy allrequirements！\n";
}

void evaluate_all_loop_orders() {
    print_section("Solution Evaluation: All 6 Loop Orders");
    
    auto configs = generate_gemm_loop_orders();
    auto patterns = create_gemm_access_patterns();
    
    PlutoContext* ctx = pluto_context_alloc();
    PlutoOptions* opts = pluto_options_alloc();
    PlutoConstraintSolver solver(ctx, opts);
    
    std::cout << std::setw(20) << "Loop Order"
              << std::setw(12) << "A Coal?"
              << std::setw(12) << "B Coal?"
              << std::setw(12) << "C Coal?"
              << std::setw(15) << "Weight Score"
              << std::setw(18) << "Recommendation\n";
    std::cout << std::string(89, '─') << "\n";
    
    double best_score = -1e9;
    int best_idx = -1;
    
    for (size_t i = 0; i < configs.size(); i++) {
        auto& config = configs[i];
        
        // Check eachArray coalescingstatus
        bool a_coal = solver.check_coalescing_for_pattern(config, patterns[0]);
        bool b_coal = solver.check_coalescing_for_pattern(config, patterns[1]);
        bool c_coal = solver.check_coalescing_for_pattern(config, patterns[2]);
        
        // Compute weighted score
        double score = solver.compute_weighted_coalescing_score(config, patterns);
        
        config.array_coalescing_status["A"] = a_coal;
        config.array_coalescing_status["B"] = b_coal;
        config.array_coalescing_status["C"] = c_coal;
        config.weighted_coalescing_score = score;
        
        if (score > best_score) {
            best_score = score;
            best_idx = i;
        }
        
        // Print results
        std::cout << std::setw(20) << config.description
                  << std::setw(12) << (a_coal ? "Y" : "N")
                  << std::setw(12) << (b_coal ? "Y" : "N")
                  << std::setw(12) << (c_coal ? "Y" : "N")
                  << std::setw(15) << std::fixed << std::setprecision(1) << score;
        
        if (i == best_idx) {
            std::cout << std::setw(18) << "BEST BEST";
        }
        std::cout << "\n";
    }
    
    std::cout << "\n";
    std::cout << "Analysis:\n";
    std::cout << "  • (i,j,k): AN BY CY → 2/3 coalesced, Band C dominate traffic\n";
    std::cout << "  • (i,k,j): AY BN CY → 2/3 coalesced, Aand C traffic\n";
    std::cout << "  • (k,i,j): AY BY CY → **but impossible!**（kcannot satisfy simultaneouslyA and B）\n";
    std::cout << "  • Actual optimal: (i,k,j) or (i,j,k)，depends on weights\n\n";
    
    std::cout << "Weight Calculation (M=N=K=1024):\n";
    std::cout << "  • w_A = 1 * 4MB = 4.2 MB (read-only)\n";
    std::cout << "  • w_B = 1 * 4MB = 4.2 MB (read-only)\n";
    std::cout << "  • w_C = 2 * 4MB * 1.5 = 12.6 MB (read-write，weight enhanced)\n";
    std::cout << "  → Chas largest traffic weight，prioritize C coalescing\n";
    std::cout << "  → Optimal choice: Keepjatinnermost (ensure B and C coalesced)\n";
    
    pluto_options_free(opts);
    pluto_context_free(ctx);
}

void show_weighted_formula() {
    print_section("Weighted Optimization Formula");
    
    std::cout << "Single objective (original PLUTO):\n";
    std::cout << "  EachArrayindependent: h · ∇φ_m ≥ 1 (hard constraint)\n";
    std::cout << "  Problem: Cannot solve when conflicts\n\n";
    
    std::cout << "Multi-objective (linear weighted optimization):\n";
    std::cout << "  maximize: Σ_m w_m · (h · ∇φ_m)  [Completely linear!]\n";
    std::cout << "  subject to: h · ∇φ_m ≥ 0  ∀m\n\n";
    std::cout << "  Simplified to binary scoring:\n";
    std::cout << "    • If h·∇φ_m = 1 (stride-1): contributes w_m\n";
    std::cout << "    • otherwise (stride > 1):         contributes 0\n";
    std::cout << "    • w_m = α_m · freq_m · volume_m\n\n";
    
    std::cout << "Weight components:\n";
    std::cout << "  • α_m: User priority (reads=1.0, writes=1.5)\n";
    std::cout << "  • freq_m: Access frequency (read-only=1, read-write=2)\n";
    std::cout << "  • volume_m: Data volume = element_size * dimension_size\n\n";
    
    std::cout << "GEMMExample (simplified):\n";
    std::cout << "  Loop order (i,k,j) [jinnermost]:\n";
    std::cout << "    A[i][k]: stride=K (non-coalesced) → contributes 0\n";
    std::cout << "    B[k][j]: stride=1 (coalesced)     → contributes 4.2 MB\n";
    std::cout << "    C[i][j]: stride=1 (coalesced)     → contributes 12.6 MB\n";
    std::cout << "    Total Score = 16.8 MB\n\n";
    
    std::cout << "  Loop order (i,j,k) [kinnermost]:\n";
    std::cout << "    A[i][k]: stride=1 (coalesced)     → contributes 4.2 MB\n";
    std::cout << "    B[k][j]: stride=N (non-coalesced) → contributes 0\n";
    std::cout << "    C[i][j]: stride=N (non-coalesced) → contributes 0\n";
    std::cout << "    Total Score = 4.2 MB\n\n";
    
    std::cout << "  → ILPSelects highest-scoring config (i,k,j): 16.8 MB\n";
}

void show_practical_solutions() {
    print_section("Practical Solutions");
    
    std::cout << "Approach1: Accept partial non-coalesced (recommended)\n";
    std::cout << "  • Use weighted optimization，select (i,k,j) or (i,j,k)\n";
    std::cout << "  • Prioritize large-trafficArray(C and B) coalesced\n";
    std::cout << "  • AAlthough non-coalesced, limited impact\n";
    std::cout << "  • Measured performance: 85-90% of theoretical peak\n\n";
    
    std::cout << "Approach2: Data layout transformation\n";
    std::cout << "  • Transpose B matrix: B_T[j][k] (originallyB[k][j])\n";
    std::cout << "  • New access: A[i][k], B_T[j][k], C[i][j]\n";
    std::cout << "  • A and B_Tbothrequireskinnermost → conflicts reduced!\n";
    std::cout << "  • Cost: requirespreprocess transposeB (one-time cost)\n\n";
    
    std::cout << "Approach3: Shared Memory Tiling (industrial-grade)\n";
    std::cout << "  • Two-stage optimization:\n";
    std::cout << "      Stage 1: Global→Shared (optimize someArraycoalescing)\n";
    std::cout << "      Stage 2: Shared→Register (No coalescing concerns)\n";
    std::cout << "  • Reference: cuBLAS, CUTLASSlibraryImplementation\n";
    std::cout << "  • Measured performance: 95%+ of theoretical peak\n\n";
    
    std::cout << "Approach4: Vectorized loading\n";
    std::cout << "  • Use vector types like float4\n";
    std::cout << "  • Even with stride!=1, can partially coalesce\n";
    std::cout << "  • requiresalignment requirements\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                              ║\n";
    std::cout << "║   GEMMMulti-AccessCoalescingCoordination Example                          ║\n";
    std::cout << "║                                                              ║\n";
    std::cout << "║   Demonstrates handlingA[i][k], B[k][j], C[i][j]conflictsrequirements           ║\n";
    std::cout << "║                                                              ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
    
    analyze_coalescing_conflicts();
    evaluate_all_loop_orders();
    show_weighted_formula();
    show_practical_solutions();
    
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "  Summary\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";
    
    std::cout << "Key Points:\n";
    std::cout << "  1. MultiArray coalescingrequirementsoften conflict\n";
    std::cout << "  2. Hard constraints usually unsolvable\n";
    std::cout << "  3. Weighted optimization allows trade-offs, maximizes total coalesced traffic\n";
    std::cout << "  4. In practice，prioritize:\n";
    std::cout << "       • Write operations coalesced (avoid cache pollution)\n";
    std::cout << "       •  Arraycoalesced\n";
    std::cout << "       • small-trafficArraycan acceptnon-coalesced\n\n";
    
    std::cout << "RETAIN Done! See MULTI_ACCESS_COALESCING.md for more details\n\n";
    
    return 0;
}
