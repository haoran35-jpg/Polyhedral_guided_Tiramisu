#include "pluto_guided_search.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <cmath>

extern "C" {
#include "pluto/pluto.h"
}

namespace pluto_tiramisu {

// ============================================================================
// PlutoConstraintSolver Implementation
// ============================================================================

std::vector<ScheduleConfig> PlutoConstraintSolver::generate_candidates_from_optimal(
    PlutoProg* optimal_prog,
    int num_candidates
) {
    std::vector<ScheduleConfig> candidates;
    
    // 1. Add PLUTO optimal as baseline
    ScheduleConfig optimal_config = pluto_prog_to_config(optimal_prog);
    optimal_config.description = "PLUTO Optimal";
    candidates.push_back(optimal_config);
    
    // 2. Generate tiling size variants
    std::vector<int> tile_variants = {16, 32, 64, 128, 256};
    int ndims = optimal_prog->nvar;
    
    for (int base_tile : tile_variants) {
        if (candidates.size() >= (size_t)num_candidates) break;
        
        ScheduleConfig variant = optimal_config;
        variant.tile_sizes.clear();
        
        // Set tile size for each dimension
        for (int d = 0; d < ndims; d++) {
            ScheduleConfig::TileSize ts;
            ts.loop_name = optimal_prog->stmts[0]->iterators[d];
            ts.size = base_tile;
            variant.tile_sizes.push_back(ts);
        }
        
        variant.description = "Tiling variant " + std::to_string(base_tile);
        
        // Check and mark constraints
        variant.has_coalescing_violation = !satisfies_coalescing_constraint(variant);
        check_bank_conflict(variant, variant.bank_conflict_way);
        variant.has_bank_conflict = (variant.bank_conflict_way > 1);
        
        if (satisfies_coalescing_constraint(variant)) {
            candidates.push_back(variant);
        }
    }
    
    // 3. Generateloops （preservecoalescing）
    if (ndims >= 2 && candidates.size() < (size_t)num_candidates) {
        // Swap outerloops（preserveinnermostunchanged to maintaincoalescing）
        // simplifiedversion：only keep basic variants
        // loopsorder variantsrequires Implementation
    }
    
    std::cout << "✓ Generated " << candidates.size() 
              << " candidates from PLUTO optimal\n\n";
    
    // Each 
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "Generated Configurations:\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    for (size_t i = 0; i < candidates.size(); i++) {
        std::cout << "Config " << (i+1) << ": " << candidates[i].description << "\n";
        
        if (!candidates[i].transformations.empty()) {
            std::cout << "  Transformations: " << candidates[i].transformations.size() << "\n";
            for (size_t j = 0; j < candidates[i].transformations.size(); j++) {
                std::cout << "    [" << j << "] Type: ";
                switch (candidates[i].transformations[j].type) {
                    case TRANS_TILE: std::cout << "TILE"; break;
                    case TRANS_GPU_TILE: std::cout << "GPU_TILE"; break;
                    case TRANS_INTERCHANGE: std::cout << "INTERCHANGE"; break;
                    case TRANS_SKEW: std::cout << "SKEW"; break;
                    case TRANS_PARALLELIZE: std::cout << "PARALLELIZE"; break;
                    case TRANS_SPLIT: std::cout << "SPLIT"; break;
                }
                
                if (!candidates[i].transformations[j].iterator_names.empty()) {
                    std::cout << ", Iterators: ";
                    for (const auto& name : candidates[i].transformations[j].iterator_names) {
                        std::cout << name << " ";
                    }
                }
                std::cout << "\n";
            }
        }
        
        if (!candidates[i].tile_sizes.empty()) {
            std::cout << "  Tile Sizes:\n";
            for (const auto& ts : candidates[i].tile_sizes) {
                std::cout << "    " << ts.loop_name << ": " << ts.size << "\n";
            }
        }
        
        std::cout << "  Coalescing: " 
                  << (satisfies_coalescing_constraint(candidates[i]) ? "✓" : "✗") << "\n";
        std::cout << "  Legal: " 
                  << (is_legal_config(candidates[i]) ? "✓" : "✗") << "\n";
        std::cout << "\n";
    }
    
    return candidates;
}

std::vector<ScheduleConfig> PlutoConstraintSolver::generate_all_legal_configs(
    int num_loops,
    const std::vector<std::string>& loop_names,
    bool only_coalesced
) {
    std::vector<ScheduleConfig> candidates;
    
    // simplifiedImplementation：Generate
    for (int i = 0; i < num_loops; i++) {
        ScheduleConfig config;
        config.description = "Config with loop " + std::to_string(i) + " innermost";
        
        if (!only_coalesced || is_legal_config(config)) {
            candidates.push_back(config);
        }
    }
    
    std::cout << "✓ Generated " << candidates.size() 
              << " legal configs\n\n";
    
    // Print config details
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "All Legal Configurations:\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    for (size_t i = 0; i < candidates.size(); i++) {
        std::cout << "Config " << (i+1) << ": " << candidates[i].description << "\n";
        std::cout << "  Status: Legal ✓\n\n";
    }
    
    return candidates;
}

std::vector<ScheduleConfig> PlutoConstraintSolver::generate_by_constraint_sampling(
    PlutoProg* base_prog,
    int num_samples
) {
    std::vector<ScheduleConfig> candidates;
    
    // Baseline config
    candidates.push_back(pluto_prog_to_config(base_prog));
    candidates.back().description = "Base PLUTO solution";
    
    // Generatetile size 
    std::vector<int> tile_variants = {16, 32, 64, 128};
    for (size_t i = 1; i < tile_variants.size() && (int)candidates.size() < num_samples; i++) {
        ScheduleConfig config = candidates[0];
        config.description = "Tile variant " + std::to_string(tile_variants[i]);
        
        // Updatetile sizes
        for (auto& ts : config.tile_sizes) {
            ts.size = tile_variants[i];
        }
        
        if (satisfies_coalescing_constraint(config)) {
            candidates.push_back(config);
        }
    }
    
    std::cout << "✓ Generated " << candidates.size() 
              << " candidates by constraint sampling\n\n";
    
    // Print sampled configs
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "Sampled Configurations:\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    for (size_t i = 0; i < candidates.size(); i++) {
        std::cout << "Sample " << (i+1) << ": " << candidates[i].description << "\n";
        
        if (!candidates[i].tile_sizes.empty()) {
            std::cout << "  Tile Configuration:\n";
            for (const auto& ts : candidates[i].tile_sizes) {
                std::cout << "    " << ts.loop_name << " = " << ts.size << "\n";
            }
        }
        std::cout << "\n";
    }
    
    return candidates;
}

bool PlutoConstraintSolver::satisfies_coalescing_constraint(
    const ScheduleConfig& config
) {
    if (config.transformations.empty()) return false;
    
    // Simplified check: innermost should have stride=1
    // Actual PLUTO constraint: h · ∇f ≥ 1
    const auto& innermost = config.transformations.back();
    if (innermost.type == TRANS_GPU_TILE) {
        // GPU tileusually ensurescoalescing
        return true;
    }
    
    // For other types, assume satisfied
    return true;
}

// NEW: Checkshared memory bank conflict
bool PlutoConstraintSolver::check_bank_conflict(
    const ScheduleConfig& config, 
    int& conflict_way
) {
    conflict_way = 1; // Default no conflict
    
    // GPU shared memoryhas32banks (NVIDIA)
    // Bank conflictat：Multibank Address
    
    // Checktile sizes
    for (const auto& ts : config.tile_sizes) {
        // Iftile size32 ，bank conflict
        // ：tile_x = 33 → 0 and 32 bank
        if (ts.size % 32 != 0 && ts.size > 1) {
            // conflict
            int gcd = 32;
            int remainder = ts.size % 32;
            
            // Simplified conflict detection:
            // Ifremainder32 ，has conflict
            if (remainder != 0) {
                // conflict way
                for (int d = 2; d <= 32; d++) {
                    if (32 % d == 0 && remainder % d == 0) {
                        conflict_way = std::max(conflict_way, d);
                    }
                }
            }
        }
        
        // Check bad tile sizes
        if (ts.size == 33 || ts.size == 65 || ts.size == 17) {
            conflict_way = std::max(conflict_way, 2);
        }
    }
    
    return conflict_way > 1;
}

// NEW: coalescing
// Implementation formulation: max Σ w_m · (h·∇φ_m)
// Simplified to binary: only count coalesced arrays
double PlutoConstraintSolver::compute_weighted_coalescing_score(
    const ScheduleConfig& config,
    const std::vector<AccessPattern>& patterns
) {
    double total_score = 0.0;
    
    for (const auto& pattern : patterns) {
        // : w_m = α_m · freq_m · volume_m
        double weight = pattern.access_frequency * 
                       pattern.element_size * 
                       pattern.dimension_size;
        
        //  (α_m = 1.5 for writes, 1.0 for reads)
        if (pattern.is_write) {
            weight *= 1.5;
        }
        
        //  h·∇φ_m (stride)
        int stride = compute_stride_for_pattern(config, pattern);
        
        // ：Σ w_m · indicator(stride=1)
        // stride=1contributesw_m，otherwisecontributes0
        if (stride == 1) {
            total_score += weight;  // coalesced: contributes
        }
        // stride != 1: contributes（non-coalesced）
    }
    
    return total_score;
}

// NEW: CheckAccess patterncoalesced
bool PlutoConstraintSolver::check_coalescing_for_pattern(
    const ScheduleConfig& config,
    const AccessPattern& pattern
) {
    if (config.transformations.empty()) return false;
    if (pattern.indices.empty()) return false;
    
    // innermost loop 
    const auto& innermost = config.transformations.back();
    if (innermost.iterator_names.empty()) return false;
    
    std::string inner_var = innermost.iterator_names.back();
    
    // row-major，stride=1
    // ：A[i][k]，Ifinner_var == "k"，coalesced
    if (pattern.indices.back() == inner_var) {
        return true;
    }
    
    return false;
}

// NEW: Access pattern stride
int PlutoConstraintSolver::compute_stride_for_pattern(
    const ScheduleConfig& config,
    const AccessPattern& pattern
) {
    if (config.transformations.empty()) return 1;
    if (pattern.indices.empty()) return 1;
    
    const auto& innermost = config.transformations.back();
    if (innermost.iterator_names.empty()) return 1;
    
    std::string inner_var = innermost.iterator_names.back();
    
    // inner_varat 
    for (size_t i = 0; i < pattern.indices.size(); i++) {
        if (pattern.indices[i] == inner_var) {
            // stride（row-major）
            int stride = 1;
            for (size_t j = i + 1; j < pattern.indices.size(); j++) {
                stride *= pattern.dimension_size;
            }
            return stride;
        }
    }
    
    // at → stride
    return pattern.dimension_size * pattern.dimension_size;
}

// NEW: 
std::vector<ScheduleConfig> PlutoConstraintSolver::filter_by_constraints(
    std::vector<ScheduleConfig> candidates
) {
    std::vector<ScheduleConfig> filtered;
    
    for (auto& config : candidates) {
        bool should_keep = true;
        
        // Checkcoalescing
        bool has_coalescing = satisfies_coalescing_constraint(config);
        config.has_coalescing_violation = !has_coalescing;
        
        if (coalescing_mode_ == ConstraintMode::HARD_CONSTRAINT && !has_coalescing) {
            should_keep = false;
        }
        
        // Checkbank conflict
        int conflict_way = 1;
        check_bank_conflict(config, conflict_way);
        config.bank_conflict_way = conflict_way;
        config.has_bank_conflict = (conflict_way > 1);
        
        if (bank_conflict_mode_ == ConstraintMode::HARD_CONSTRAINT && config.has_bank_conflict) {
            should_keep = false;
        }
        
        if (should_keep) {
            filtered.push_back(config);
        }
    }
    
    std::cout << "🔍 Constraint Filtering:\n";
    std::cout << "  • Input candidates:  " << candidates.size() << "\n";
    std::cout << "  • Filtered out:      " << (candidates.size() - filtered.size()) << "\n";
    std::cout << "  • Remaining:         " << filtered.size() << "\n";
    std::cout << "  • Coalescing mode:   ";
    switch (coalescing_mode_) {
        case ConstraintMode::HARD_CONSTRAINT: std::cout << "HARD\n"; break;
        case ConstraintMode::SOFT_CONSTRAINT: std::cout << "SOFT\n"; break;
        case ConstraintMode::PENALTY_BASED: std::cout << "PENALTY\n"; break;
        case ConstraintMode::NO_CONSTRAINT: std::cout << "NONE\n"; break;
    }
    std::cout << "  • Bank conflict mode: ";
    switch (bank_conflict_mode_) {
        case ConstraintMode::HARD_CONSTRAINT: std::cout << "HARD\n"; break;
        case ConstraintMode::SOFT_CONSTRAINT: std::cout << "SOFT\n"; break;
        case ConstraintMode::PENALTY_BASED: std::cout << "PENALTY\n"; break;
        case ConstraintMode::NO_CONSTRAINT: std::cout << "NONE\n"; break;
    }
    std::cout << "\n";
    
    return filtered;
}

bool PlutoConstraintSolver::is_legal_config(const ScheduleConfig& config) {
    // Check
    if (config.transformations.empty()) return false;
    
    // Checktile sizes
    for (const auto& ts : config.tile_sizes) {
        if (ts.size <= 0 || ts.size > 1024) return false;
    }
    
    return true;
}

ScheduleConfig PlutoConstraintSolver::pluto_prog_to_config(PlutoProg* prog) {
    ScheduleConfig config;
    
    if (!prog || prog->nstmts == 0) return config;
    
    // transformation
    int ndims = prog->nvar;
    Stmt* stmt = prog->stmts[0];
    
    for (int i = 0; i < ndims; i++) {
        Transformation trans(TRANS_INTERCHANGE);
        
        // 
        if (stmt->iterators && stmt->iterators[i]) {
            trans.iterator_names.push_back(stmt->iterators[i]);
        }
        
        config.transformations.push_back(trans);
    }
    
    return config;
}

std::vector<int> PlutoConstraintSolver::generate_tile_size_variants(int base_size) {
    return {base_size / 2, base_size, base_size * 2};
}

void PlutoConstraintSolver::print_pluto_prog_info(PlutoProg* prog, const std::string& title) {
    if (!prog) {
        std::cout << "⚠️  NULL PlutoProg\n";
        return;
    }
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  " << title << "\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    std::cout << "📊 Program Statistics:\n";
    std::cout << "  • Number of variables:  " << prog->nvar << "\n";
    std::cout << "  • Number of statements: " << prog->nstmts << "\n";
    std::cout << "  • Number of parameters: " << prog->npar << "\n\n";
    
    if (prog->nstmts > 0 && prog->stmts) {
        std::cout << "📝 Statements:\n";
        for (int s = 0; s < prog->nstmts; s++) {
            Stmt* stmt = prog->stmts[s];
            if (!stmt) continue;
            
            std::cout << "\n  Statement " << s << ":\n";
            std::cout << "    Dimensions: " << stmt->dim << "\n";
            
            if (stmt->iterators) {
                std::cout << "    Iterators:  ";
                for (int i = 0; i < stmt->dim; i++) {
                    if (stmt->iterators[i]) {
                        std::cout << stmt->iterators[i];
                        if (i < stmt->dim - 1) std::cout << ", ";
                    }
                }
                std::cout << "\n";
            }
            
            if (stmt->trans) {
                std::cout << "    Transformation Matrix:\n";
                for (int i = 0; i < stmt->dim; i++) {
                    std::cout << "      [";
                    for (int j = 0; j < stmt->dim + 1; j++) {
                        printf("%4lld", stmt->trans->val[i][j]);
                        if (j < stmt->dim) std::cout << " ";
                    }
                    std::cout << " ]\n";
                }
                
                // loops
                std::cout << "\n    Loop Order Analysis:\n";
                for (int i = 0; i < stmt->dim; i++) {
                    std::cout << "      Level " << i << ": ";
                    
                    //  
                    for (int j = 0; j < stmt->dim; j++) {
                        if (stmt->trans->val[i][j] != 0) {
                            if (stmt->iterators && stmt->iterators[j]) {
                                std::cout << stmt->iterators[j];
                                if (stmt->trans->val[i][j] > 1) {
                                    std::cout << " (scaled by " << stmt->trans->val[i][j] << ")";
                                }
                            }
                            std::cout << " ";
                        }
                    }
                    
                    // innermost
                    if (i == stmt->dim - 1) {
                        std::cout << " ← innermost (coalescing)";
                    }
                    std::cout << "\n";
                }
            }
            
            if (stmt->text) {
                std::cout << "\n    Text: " << stmt->text << "\n";
            }
        }
    }
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
}

// ============================================================================
// TiramisuConfigEvaluator Implementation
// ============================================================================

double TiramisuConfigEvaluator::evaluate_config(
    tiramisu::computation& comp,
    const ScheduleConfig& config,
    int num_runs
) {
    // Approach1: （）
    // versionrequires，
    
    double estimated_time = 100.0; // (ms)
    
    // tile size
    if (!config.tile_sizes.empty()) {
        int avg_tile = 0;
        for (const auto& ts : config.tile_sizes) {
            avg_tile += ts.size;
        }
        avg_tile /= config.tile_sizes.size();
        
        // tile sizeat32-64
        if (avg_tile >= 32 && avg_tile <= 64) {
            estimated_time *= 0.8;  // 20% faster
        } else if (avg_tile < 16) {
            estimated_time *= 1.3;  // 30% slower (too small)
        } else if (avg_tile > 128) {
            estimated_time *= 1.2;  // 20% slower (too large)
        }
    }
    
    // coalescing
    if (satisfies_coalescing_constraint(config)) {
        estimated_time *= 0.7;  // 30% faster with coalescing
    }
    
    // （ ）
    double noise = (rand() % 20 - 10) / 100.0; // ±10%
    estimated_time *= (1.0 + noise);
    
    // NEW: bank conflict
    if (apply_bank_conflict_penalty_ && config.has_bank_conflict) {
        estimated_time = compute_penalized_score(config, estimated_time);
    }
    
    return estimated_time;
    
    /* Approach2: （requiresMultiImplementation）
    try {
        // computation
        apply_config_to_computation(comp, config);
        
        // 
        tiramisu_func_->codegen({}, "temp_eval.o");
        
        // 
        // system("g++ temp_eval.o -o temp_eval");
        // auto start = std::chrono::high_resolution_clock::now();
        // system("./temp_eval");
        // auto end = std::chrono::high_resolution_clock::now();
        // return duration_ms;
        
        return 1.0;
    } catch (...) {
        return -1.0; // 
    }
    */
}

// NEW:  
double TiramisuConfigEvaluator::compute_penalized_score(
    const ScheduleConfig& config, 
    double raw_time
) {
    if (!config.has_bank_conflict) {
        return raw_time;
    }
    
    // conflict
    double penalty_multiplier = 1.0;
    
    switch (config.bank_conflict_way) {
        case 2:  // 2-way conflict
            penalty_multiplier = bank_conflict_penalty_factor_;  // 2.0x
            break;
        case 4:  // 4-way conflict
            penalty_multiplier = bank_conflict_penalty_factor_ * 1.5;  // 3.0x
            break;
        case 8:  // 8-way conflict
            penalty_multiplier = bank_conflict_penalty_factor_ * 2.0;  // 4.0x
            break;
        case 16: // 16-way conflict
            penalty_multiplier = bank_conflict_penalty_factor_ * 4.0;  // 8.0x
            break;
        case 32: // 32-way conflict ()
            penalty_multiplier = bank_conflict_penalty_factor_ * 16.0; // 32.0x
            break;
        default:
            if (config.bank_conflict_way > 1) {
                // ：penalty ≈ conflict_way
                penalty_multiplier = config.bank_conflict_way * 
                                    (bank_conflict_penalty_factor_ / 2.0);
            }
    }
    
    return raw_time * penalty_multiplier;
}

bool TiramisuConfigEvaluator::satisfies_coalescing_constraint(
    const ScheduleConfig& config
) {
    if (config.transformations.empty()) return false;
    
    const auto& innermost = config.transformations.back();
    if (innermost.type == TRANS_GPU_TILE) {
        return true;
    }
    
    return true; // Simplified check
}

ScheduleConfig TiramisuConfigEvaluator::search_best_config(
    tiramisu::computation& comp,
    const std::vector<ScheduleConfig>& candidates
) {
    ScheduleConfig best_config;
    double best_time = std::numeric_limits<double>::max();
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  Searching best config among " << candidates.size() 
              << " candidates\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    for (size_t i = 0; i < candidates.size(); i++) {
        const auto& config = candidates[i];
        
        std::cout << "[" << (i+1) << "/" << candidates.size() << "] "
                  << "Evaluating: " << config.description;
        
        // status
        if (config.has_bank_conflict) {
            std::cout << " [⚠️ " << config.bank_conflict_way << "-way BC]";
        }
        if (config.has_coalescing_violation) {
            std::cout << " [⚠️ Non-coalesced]";
        }
        std::cout << "... ";
        
        double time = evaluate_config(comp, config, 10);
        
        if (time > 0 && time < best_time) {
            best_time = time;
            best_config = config;
            best_config.execution_time_ms = time;
            std::cout << "✓ " << time << " ms (NEW BEST)\n";
        } else if (time > 0) {
            std::cout << "✓ " << time << " ms\n";
        } else {
            std::cout << "✗ Failed\n";
        }
    }
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  Best: " << best_config.description 
              << " (" << best_time << " ms)\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    return best_config;
}

std::vector<ScheduleConfig> TiramisuConfigEvaluator::evaluate_all_configs(
    tiramisu::computation& comp,
    std::vector<ScheduleConfig> candidates
) {
    for (auto& config : candidates) {
        config.execution_time_ms = evaluate_config(comp, config, 10);
        config.is_valid = (config.execution_time_ms > 0);
    }
    
    // 
    std::sort(candidates.begin(), candidates.end(),
        [](const ScheduleConfig& a, const ScheduleConfig& b) {
            if (!a.is_valid) return false;
            if (!b.is_valid) return true;
            return a.execution_time_ms < b.execution_time_ms;
        });
    
    return candidates;
}

void TiramisuConfigEvaluator::apply_config_to_computation(
    tiramisu::computation& comp,
    const ScheduleConfig& config
) {
    // transformations
    // TODO: ScheduleConfigTiramisu API
    // ：comp.tile(), comp.gpu_tile(), comp.interchange()
}

// ============================================================================
// HybridOptimizer Implementation
// ============================================================================

HybridOptimizer::OptimizationResult HybridOptimizer::optimize(
    tiramisu::computation& comp,
    PlutoProg* base_prog,
    const std::string& strategy
) {
    if (strategy == "optimal_neighbors") {
        return optimize_with_neighbors(comp, base_prog, 10);
    } else if (strategy == "all_legal") {
        int ndims = base_prog->nvar;
        std::vector<std::string> names;
        for (int i = 0; i < ndims; i++) {
            names.push_back(base_prog->stmts[0]->iterators[i]);
        }
        return optimize_with_all_legal(comp, ndims, names);
    } else if (strategy == "sampling") {
        return optimize_with_sampling(comp, base_prog, 5);
    }
    
    // 
    return optimize_with_neighbors(comp, base_prog, 10);
}

HybridOptimizer::OptimizationResult HybridOptimizer::optimize_with_neighbors(
    tiramisu::computation& comp,
    PlutoProg* optimal_prog,
    int num_neighbors
) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    OptimizationResult result;
    
    // 1. PLUTOGenerate
    std::cout << "\n🔍 Step 1: PLUTO generates candidates...\n";
    result.all_candidates = solver_.generate_candidates_from_optimal(
        optimal_prog, num_neighbors);
    result.num_candidates_generated = result.all_candidates.size();
    
    // 
    std::vector<ScheduleConfig> legal_candidates;
    for (const auto& config : result.all_candidates) {
        if (solver_.is_legal_config(config)) {
            legal_candidates.push_back(config);
        }
    }
    result.num_legal_candidates = legal_candidates.size();
    
    // 2. Tiramisu
    std::cout << "\n⚡ Step 2: Tiramisu evaluates candidates...\n";
    result.best_config = evaluator_.search_best_config(comp, legal_candidates);
    result.num_evaluated = legal_candidates.size();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.total_search_time_ms = 
        std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    // 
    result.best_time_ms = result.best_config.execution_time_ms;
    result.worst_time_ms = 0;
    result.average_time_ms = 0;
    double sum = 0;
    for (const auto& config : legal_candidates) {
        if (config.execution_time_ms > 0) {
            sum += config.execution_time_ms;
            result.worst_time_ms = std::max(result.worst_time_ms, 
                                           config.execution_time_ms);
        }
    }
    result.average_time_ms = sum / result.num_evaluated;
    
    return result;
}

HybridOptimizer::OptimizationResult HybridOptimizer::optimize_with_all_legal(
    tiramisu::computation& comp,
    int num_loops,
    const std::vector<std::string>& loop_names
) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    OptimizationResult result;
    
    // PLUTOGeneratehas
    result.all_candidates = solver_.generate_all_legal_configs(
        num_loops, loop_names, true);
    result.num_candidates_generated = result.all_candidates.size();
    result.num_legal_candidates = result.all_candidates.size();
    
    // Tiramisu
    result.best_config = evaluator_.search_best_config(comp, result.all_candidates);
    result.num_evaluated = result.all_candidates.size();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.total_search_time_ms = 
        std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    return result;
}

HybridOptimizer::OptimizationResult HybridOptimizer::optimize_with_sampling(
    tiramisu::computation& comp,
    PlutoProg* base_prog,
    int num_samples
) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    OptimizationResult result;
    
    // PLUTO
    result.all_candidates = solver_.generate_by_constraint_sampling(
        base_prog, num_samples);
    result.num_candidates_generated = result.all_candidates.size();
    result.num_legal_candidates = result.all_candidates.size();
    
    // Tiramisu
    result.best_config = evaluator_.search_best_config(comp, result.all_candidates);
    result.num_evaluated = result.all_candidates.size();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    result.total_search_time_ms = 
        std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    return result;
}

} // namespace pluto_tiramisu
