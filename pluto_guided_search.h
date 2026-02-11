#ifndef PLUTO_GUIDED_SEARCH_H
#define PLUTO_GUIDED_SEARCH_H

#include <vector>
#include <string>
#include <functional>
#include "pluto_to_tiramisu.h"

namespace pluto_tiramisu {

// ============================================================================
// Schedule Configuration Structure
// ============================================================================

struct ScheduleConfig {
    // Transformation information
    std::vector<Transformation> transformations;
    
    // Tiling parameters
    struct TileSize {
        std::string loop_name;
        int size;
    };
    std::vector<TileSize> tile_sizes;
    
    // Evaluation results
    double execution_time_ms;  // Evaluated by Tiramisu
    bool is_valid;             // Whether it passes Tiramisu validation
    
    // GPU optimization properties
    bool has_coalescing_violation;     // Whether it violates coalescing
    bool has_bank_conflict;            // Whether it has bank conflict
    int bank_conflict_way;             // Conflict degree (2-way, 4-way, etc.)
    
    // NEW: Multi-access coalescing score
    double weighted_coalescing_score;  // Weighted coalescing score
    std::map<std::string, bool> array_coalescing_status;  // Coalescing status per array
    
    // Description
    std::string description;
    
    ScheduleConfig() : execution_time_ms(-1.0), is_valid(true),
                       has_coalescing_violation(false), 
                       has_bank_conflict(false),
                       bank_conflict_way(0),
                       weighted_coalescing_score(0.0) {}
};

// ============================================================================
// Access Pattern Definition
// ============================================================================

struct AccessPattern {
    std::string array_name;
    std::vector<std::string> indices;  // e.g., ["i", "k"] for A[i][k]
    int64_t access_frequency;          // Access frequency (1=read-only, 2=read-write)
    size_t element_size;               // sizeof(type)
    int64_t dimension_size;            // Dimension size (for stride calculation)
    bool is_write;                     // Whether it's a write operation
    
    AccessPattern() : access_frequency(1), element_size(4), 
                      dimension_size(1024), is_write(false) {}
};

// ============================================================================
// PLUTO Constraint Solver - Generate Candidates
// ============================================================================

// Constraint Mode Enumeration
enum class ConstraintMode {
    HARD_CONSTRAINT,     // Hard constraint: exclude violating configs
    SOFT_CONSTRAINT,     // Soft constraint: mark but dont exclude
    PENALTY_BASED,       // Penalty mode: add performance penalty
    NO_CONSTRAINT        // No constraint: accept all
};

class PlutoConstraintSolver {
public:
    PlutoConstraintSolver(PlutoContext* ctx, PlutoOptions* options)
        : context_(ctx), options_(options),
          coalescing_mode_(ConstraintMode::HARD_CONSTRAINT),
          bank_conflict_mode_(ConstraintMode::SOFT_CONSTRAINT) {}
    
    // Set constraint mode
    void set_coalescing_mode(ConstraintMode mode) { coalescing_mode_ = mode; }
    void set_bank_conflict_mode(ConstraintMode mode) { bank_conflict_mode_ = mode; }
    
    // NEW: Set access patterns for multi-access coordination
    void set_access_patterns(const std::vector<AccessPattern>& patterns) {
        access_patterns_ = patterns;
    }
    
    // Method 1: Generate neighbors from optimal
    // Generate variants from PLUTO optimal
    std::vector<ScheduleConfig> generate_candidates_from_optimal(
        PlutoProg* optimal_prog,
        int num_candidates = 10
    );
    
    // Method 2: Enumerate all legal loop orders
    // Keep only those satisfying coalescing
    std::vector<ScheduleConfig> generate_all_legal_configs(
        int num_loops,
        const std::vector<std::string>& loop_names,
        bool only_coalesced = true
    );
    
    // Method 3: Constraint-based ILP sampling
    // Solve PLUTO multiple times with varying weights
    std::vector<ScheduleConfig> generate_by_constraint_sampling(
        PlutoProg* base_prog,
        int num_samples = 5
    );
    
    // Helper: Check if config satisfies coalescing
    bool satisfies_coalescing_constraint(const ScheduleConfig& config);
    
    // NEW: Check for bank conflict
    bool check_bank_conflict(const ScheduleConfig& config, int& conflict_way);
    
    // NEW: Compute weighted coalescing score
    double compute_weighted_coalescing_score(
        const ScheduleConfig& config,
        const std::vector<AccessPattern>& patterns
    );
    
    // NEW: Check if pattern is coalesced
    bool check_coalescing_for_pattern(
        const ScheduleConfig& config,
        const AccessPattern& pattern
    );
    
    // Helper: Check config legality
    bool is_legal_config(const ScheduleConfig& config);
    
    // NEW: Filter candidates by constraint mode
    std::vector<ScheduleConfig> filter_by_constraints(
        std::vector<ScheduleConfig> candidates
    );
    
    // Print PLUTO program info
    void print_pluto_prog_info(PlutoProg* prog, const std::string& title = "PLUTO Program");

private:
    PlutoContext* context_;
    PlutoOptions* options_;
    
    // Constraint mode settings
    ConstraintMode coalescing_mode_;
    ConstraintMode bank_conflict_mode_;
    
    // NEW: Access pattern list
    std::vector<AccessPattern> access_patterns_;
    
    // Internal helper functions
    ScheduleConfig pluto_prog_to_config(PlutoProg* prog);
    std::vector<int> generate_tile_size_variants(int base_size);
    
    // NEW: Compute stride for pattern
    int compute_stride_for_pattern(
        const ScheduleConfig& config,
        const AccessPattern& pattern
    );
};

// ============================================================================
// Tiramisu Evaluator - Select optimal from candidates
// ============================================================================

class TiramisuConfigEvaluator {
public:
    TiramisuConfigEvaluator(tiramisu::function* func)
        : tiramisu_func_(func), converter_(func),
          apply_bank_conflict_penalty_(true),
          bank_conflict_penalty_factor_(2.0) {}
    
    // Set whether to apply bank conflict penalty
    void set_bank_conflict_penalty(bool enable, double factor = 2.0) {
        apply_bank_conflict_penalty_ = enable;
        bank_conflict_penalty_factor_ = factor;
    }
    
    // Evaluate single config performance
    double evaluate_config(
        tiramisu::computation& comp,
        const ScheduleConfig& config,
        int num_runs = 10
    );
    
    // Search for optimal config
    ScheduleConfig search_best_config(
        tiramisu::computation& comp,
        const std::vector<ScheduleConfig>& candidates
    );
    
    // Batch evaluation (parallel)
    std::vector<ScheduleConfig> evaluate_all_configs(
        tiramisu::computation& comp,
        std::vector<ScheduleConfig> candidates
    );

private:
    tiramisu::function* tiramisu_func_;
    PlutoToTiramisuConverter converter_;
    
    // Bank conflictpenalty settings
    bool apply_bank_conflict_penalty_;
    double bank_conflict_penalty_factor_;  // Penalty coefficient
    
    // Apply config to computation
    void apply_config_to_computation(
        tiramisu::computation& comp,
        const ScheduleConfig& config
    );
    
    // Check coalescing constraint
    bool satisfies_coalescing_constraint(const ScheduleConfig& config);
    
    // NEW: Compute penalized score
    double compute_penalized_score(const ScheduleConfig& config, double raw_time);
};

// ============================================================================
// Hybrid Optimizer - Complete Workflow
// ============================================================================

class HybridOptimizer {
public:
    HybridOptimizer(
        PlutoContext* pluto_ctx,
        PlutoOptions* pluto_opts,
        tiramisu::function* tiramisu_func
    ) : solver_(pluto_ctx, pluto_opts),
        evaluator_(tiramisu_func) {}
    
    // Complete optimization workflow
    struct OptimizationResult {
        ScheduleConfig best_config;
        std::vector<ScheduleConfig> all_candidates;
        
        // Statistics
        int num_candidates_generated;
        int num_legal_candidates;
        int num_evaluated;
        double total_search_time_ms;
        
        // Comparison info
        double best_time_ms;
        double worst_time_ms;
        double average_time_ms;
    };
    
    // Main optimization function
    OptimizationResult optimize(
        tiramisu::computation& comp,
        PlutoProg* base_prog,
        const std::string& strategy = "optimal_neighbors"
    );
    
    // Different strategies
    OptimizationResult optimize_with_neighbors(
        tiramisu::computation& comp,
        PlutoProg* optimal_prog,
        int num_neighbors = 10
    );
    
    OptimizationResult optimize_with_all_legal(
        tiramisu::computation& comp,
        int num_loops,
        const std::vector<std::string>& loop_names
    );
    
    OptimizationResult optimize_with_sampling(
        tiramisu::computation& comp,
        PlutoProg* base_prog,
        int num_samples = 5
    );

private:
    PlutoConstraintSolver solver_;
    TiramisuConfigEvaluator evaluator_;
};

} // namespace pluto_tiramisu

#endif // PLUTO_GUIDED_SEARCH_H
