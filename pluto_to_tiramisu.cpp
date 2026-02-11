/*
 * PLUTO to Tiramisu Bridge Implementation
 */

#include "pluto_to_tiramisu.h"
#include <iostream>
#include <cstring>

using namespace tiramisu;

namespace pluto_tiramisu {

PlutoToTiramisuConverter::PlutoToTiramisuConverter(function *fct)
    : func(fct) {}

PlutoToTiramisuConverter::~PlutoToTiramisuConverter() {}

/**
 * 打印PLUTO Transformation Matrix
 */
static void print_pluto_transformation_matrix(PlutoProg *pluto_prog) {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  PLUTO Transformation Matrix (Real Output from PLUTO)     ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
    
    if (!pluto_prog || pluto_prog->nstmts == 0) {
        std::cout << "[Warning] No PLUTO program or statements" << std::endl;
        return;
    }
    
    std::cout << "\nNumber of statements: " << pluto_prog->nstmts << std::endl;
    std::cout << "Number of hyperplanes: " << pluto_prog->num_hyperplanes << std::endl;
    
    for (unsigned s = 0; s < pluto_prog->nstmts; s++) {
        Stmt *stmt = pluto_prog->stmts[s];
        
        std::cout << "\n┌─────────────────────────────────────────────────────────┐" << std::endl;
        std::cout << "│ Statement " << s << ": " << (stmt->text ? stmt->text : "unnamed") << std::endl;
        std::cout << "├─────────────────────────────────────────────────────────┤" << std::endl;
        std::cout << "│ Dimensions: " << stmt->dim << std::endl;
        
        if (stmt->iterators) {
            std::cout << "│ Iterators: ";
            for (int d = 0; d < stmt->dim; d++) {
                if (stmt->iterators[d]) {
                    std::cout << stmt->iterators[d];
                    if (d < stmt->dim - 1) std::cout << ", ";
                }
            }
            std::cout << std::endl;
        }
        
        if (stmt->trans) {
            std::cout << "│" << std::endl;
            std::cout << "│ Transformation Matrix T (" << stmt->trans->nrows 
                      << "×" << stmt->trans->ncols << "):" << std::endl;
            
            // 打印矩阵
            for (int i = 0; i < stmt->trans->nrows; i++) {
                std::cout << "│   h" << i << " = [";
                for (int j = 0; j < stmt->trans->ncols; j++) {
                    printf(" %3lld", stmt->trans->val[i][j]);
                }
                std::cout << " ]";
                
                if (i == stmt->trans->nrows - 1) {
                    std::cout << "  ← innermost";
                    int last_coeff = stmt->trans->val[i][stmt->dim - 1];
                    if (last_coeff >= 1) {
                        std::cout << " ✓ COALESCED";
                    }
                }
                std::cout << std::endl;
            }
            
        } else {
            std::cout << "│ [No transformation matrix]" << std::endl;
        }
        
        std::cout << "└─────────────────────────────────────────────────────────┘" << std::endl;
    }
    std::cout << std::endl;
}

/**
 * 从transformation matrix分析循环顺序
 */
static std::vector<int> extract_loop_order(PlutoMatrix *trans, int dim) {
    std::vector<int> loop_order;
    
    if (!trans || trans->nrows == 0) {
        // 默认顺序
        for (int i = 0; i < dim; i++) {
            loop_order.push_back(i);
        }
        return loop_order;
    }
    
    // 从外到内分析每个hyperplane
    for (int h = 0; h < trans->nrows && h < dim; h++) {
        // 找到这个hyperplane中系数最大的维度
        int max_dim = 0;
        int64_t max_coeff = 0;
        
        for (int d = 0; d < dim; d++) {
            if (abs((int)trans->val[h][d]) > abs((int)max_coeff)) {
                max_coeff = trans->val[h][d];
                max_dim = d;
            }
        }
        
        loop_order.push_back(max_dim);
    }
    
    return loop_order;
}

/**
 * 从PLUTO程序提取迭代器名称
 */
static std::vector<std::string> extract_iterator_names(Stmt *stmt) {
    std::vector<std::string> names;
    
    if (stmt->iterators) {
        for (int d = 0; d < stmt->dim; d++) {
            if (stmt->iterators[d]) {
                names.push_back(std::string(stmt->iterators[d]));
            } else {
                // 默认名称
                char default_name[3] = {'i', 'j', 'k'};
                if (d < 3) {
                    names.push_back(std::string(1, default_name[d]));
                } else {
                    names.push_back("i" + std::to_string(d));
                }
            }
        }
    } else {
        // 生成默认迭代器名称
        const char* default_names[] = {"i", "j", "k", "l", "m", "n"};
        for (int d = 0; d < stmt->dim; d++) {
            if (d < 6) {
                names.push_back(default_names[d]);
            } else {
                names.push_back("i" + std::to_string(d));
            }
        }
    }
    
    return names;
}

/**
 * 从PLUTO程序提取变换（完整版本 - 支持任意维度）
 */
std::vector<Transformation> 
PlutoToTiramisuConverter::extract_transformations(PlutoProg *pluto_prog) {
    std::vector<Transformation> transforms;
    
    std::cout << "[Bridge] Extracting PLUTO transformations (Full Version)..." << std::endl;
    
    // 打印PLUTO找到的transformation matrix
    print_pluto_transformation_matrix(pluto_prog);
    
    if (!pluto_prog || pluto_prog->nstmts == 0) {
        std::cout << "[Bridge] Warning: No statements to process" << std::endl;
        return transforms;
    }
    
    // 处理所有statements（不只是第一个）
    for (unsigned s = 0; s < pluto_prog->nstmts; s++) {
        Stmt *stmt = pluto_prog->stmts[s];
        
        std::cout << "\n[Bridge] Processing statement " << s 
                  << " (dim=" << stmt->dim << ")" << std::endl;
        
        // 提取迭代器名称
        std::vector<std::string> iterator_names = extract_iterator_names(stmt);
        
        std::cout << "[Bridge] Iterators: ";
        for (const auto& name : iterator_names) {
            std::cout << name << " ";
        }
        std::cout << std::endl;
        
        // 分析transformation matrix
        if (stmt->trans && stmt->trans->nrows > 0) {
            // 提取循环顺序
            std::vector<int> loop_order = extract_loop_order(stmt->trans, stmt->dim);
            
            std::cout << "[Bridge] Loop order (outer→inner): ";
            for (int idx : loop_order) {
                std::cout << iterator_names[idx] << " ";
            }
            std::cout << std::endl;
            
            // 检查GPU coalescing
            int last_hyp = stmt->trans->nrows - 1;
            bool is_coalescing = false;
            
            if (stmt->dim > 0 && last_hyp >= 0) {
                int innermost_coeff = stmt->trans->val[last_hyp][stmt->dim - 1];
                is_coalescing = (innermost_coeff >= 1);
                
                std::cout << "[Bridge] GPU Coalescing: " 
                          << (is_coalescing ? "YES ✓" : "NO") << std::endl;
            }
            
            // 根据维度创建合适的变换
            if (stmt->dim >= 2) {
                Transformation tile(is_coalescing ? TRANS_GPU_TILE : TRANS_TILE);
                
                // 动态设置tile维度
                int max_dims = (stmt->dim < 3) ? stmt->dim : 3;
                for (int d = 0; d < max_dims; d++) {
                    tile.loop_dims.push_back(d);
                }
                
                // 设置tile大小（根据维度）
                if (stmt->dim == 2) {
                    tile.tile_sizes = {32, 32};
                } else if (stmt->dim == 3) {
                    tile.tile_sizes = {32, 32, 32};
                } else {
                    // 更高维度
                    for (int d = 0; d < stmt->dim; d++) {
                        tile.tile_sizes.push_back(32);
                    }
                }
                
                // 存储迭代器名称（用于后续应用）
                tile.iterator_names = iterator_names;
                tile.statement_id = s;
                
                transforms.push_back(tile);
                
                std::cout << "[Bridge] Added " 
                          << (is_coalescing ? "GPU" : "CPU") 
                          << " tile: ";
                for (auto size : tile.tile_sizes) {
                    std::cout << size << "×";
                }
                std::cout << "\b " << std::endl;
            }
        } else {
            std::cout << "[Bridge] No transformation matrix for statement " 
                      << s << std::endl;
        }
    }
    
    std::cout << "\n[Bridge] Extracted " << transforms.size() 
              << " transformations total" << std::endl;
    
    return transforms;
}

/**
 * 应用变换到Tiramisu
 */
void PlutoToTiramisuConverter::apply_transformations(
    computation &comp,
    const std::vector<Transformation> &transforms) {
    
    std::cout << "[Bridge] Applying " << transforms.size() 
              << " transformations to Tiramisu..." << std::endl;
    
    for (const auto &trans : transforms) {
        switch (trans.type) {
            case TRANS_GPU_TILE:
                apply_gpu_tile(comp, trans);
                break;
            case TRANS_TILE:
                apply_tile(comp, trans);
                break;
            case TRANS_INTERCHANGE:
                apply_interchange(comp, trans);
                break;
            default:
                std::cout << "[Bridge] Warning: Unsupported transformation type" 
                          << std::endl;
        }
    }
    
    std::cout << "[Bridge] All transformations applied" << std::endl;
}

/**
 * 应用GPU tile（通用版本 - 支持任意维度和迭代器名称）
 */
void PlutoToTiramisuConverter::apply_gpu_tile(
    computation &comp,
    const Transformation &trans) {
    
    if (trans.tile_sizes.size() < 2) {
        std::cerr << "[Bridge] Error: GPU tile needs at least 2 dimensions" << std::endl;
        return;
    }
    
    std::cout << "[Bridge] Applying GPU tile: ";
    for (size_t i = 0; i < trans.tile_sizes.size(); i++) {
        std::cout << trans.tile_sizes[i];
        if (i < trans.tile_sizes.size() - 1) std::cout << "×";
    }
    std::cout << std::endl;
    
    // 使用动态迭代器名称
    if (trans.iterator_names.size() >= 2) {
        std::cout << "[Bridge] Using iterators: ";
        for (const auto& name : trans.iterator_names) {
            std::cout << name << " ";
        }
        std::cout << std::endl;
        
        // 创建var对象
        var v0(trans.iterator_names[0].c_str());
        var v1(trans.iterator_names[1].c_str());
        
        // 应用GPU tiling
        if (trans.tile_sizes.size() == 2) {
            comp.gpu_tile(v0, v1, trans.tile_sizes[0], trans.tile_sizes[1]);
            
            std::cout << "[Bridge] 2D GPU tile applied:" << std::endl;
            std::cout << "[Bridge]   " << trans.iterator_names[0] << ": " 
                      << trans.tile_sizes[0] << " (blockIdx.y, threadIdx.y)" << std::endl;
            std::cout << "[Bridge]   " << trans.iterator_names[1] << ": " 
                      << trans.tile_sizes[1] << " (blockIdx.x, threadIdx.x)" << std::endl;
        } else if (trans.tile_sizes.size() == 3 && trans.iterator_names.size() >= 3) {
            var v2(trans.iterator_names[2].c_str());
            comp.gpu_tile(v0, v1, v2, 
                         trans.tile_sizes[0], trans.tile_sizes[1], trans.tile_sizes[2]);
            
            std::cout << "[Bridge] 3D GPU tile applied:" << std::endl;
            std::cout << "[Bridge]   " << trans.iterator_names[0] << ": " 
                      << trans.tile_sizes[0] << " (blockIdx.z, threadIdx.z)" << std::endl;
            std::cout << "[Bridge]   " << trans.iterator_names[1] << ": " 
                      << trans.tile_sizes[1] << " (blockIdx.y, threadIdx.y)" << std::endl;
            std::cout << "[Bridge]   " << trans.iterator_names[2] << ": " 
                      << trans.tile_sizes[2] << " (blockIdx.x, threadIdx.x)" << std::endl;
        } else {
            std::cout << "[Bridge] Warning: " << trans.tile_sizes.size() 
                      << "D GPU tiling not fully supported, using 2D" << std::endl;
            comp.gpu_tile(v0, v1, trans.tile_sizes[0], trans.tile_sizes[1]);
        }
        
        std::cout << "[Bridge] ✓ Memory coalescing enabled" << std::endl;
    } else {
        std::cerr << "[Bridge] Error: Not enough iterator names" << std::endl;
    }
}

/**
 * 应用普通tile（通用版本）
 */
void PlutoToTiramisuConverter::apply_tile(
    computation &comp,
    const Transformation &trans) {
    
    if (trans.tile_sizes.size() < 2) {
        std::cerr << "[Bridge] Error: Tile needs at least 2 dimensions" << std::endl;
        return;
    }
    
    std::cout << "[Bridge] Applying CPU tile: ";
    for (size_t i = 0; i < trans.tile_sizes.size(); i++) {
        std::cout << trans.tile_sizes[i];
        if (i < trans.tile_sizes.size() - 1) std::cout << "×";
    }
    std::cout << std::endl;
    
    // 使用动态迭代器名称
    if (trans.iterator_names.size() >= 2) {
        var v0(trans.iterator_names[0].c_str());
        var v1(trans.iterator_names[1].c_str());
        
        if (trans.tile_sizes.size() == 2) {
            comp.tile(v0, v1, trans.tile_sizes[0], trans.tile_sizes[1]);
        } else if (trans.tile_sizes.size() == 3 && trans.iterator_names.size() >= 3) {
            var v2(trans.iterator_names[2].c_str());
            comp.tile(v0, v1, v2, 
                     trans.tile_sizes[0], trans.tile_sizes[1], trans.tile_sizes[2]);
        }
        
        std::cout << "[Bridge] CPU tile applied for dimensions: ";
        for (size_t i = 0; i < std::min(trans.iterator_names.size(), trans.tile_sizes.size()); i++) {
            std::cout << trans.iterator_names[i] << " ";
        }
        std::cout << std::endl;
    } else {
        // Fallback to default names
        var i("i"), j("j");
        comp.tile(i, j, trans.tile_sizes[0], trans.tile_sizes[1]);
        std::cout << "[Bridge] CPU tile applied (using default names)" << std::endl;
    }
}

/**
 * 应用interchange（通用版本）
 */
void PlutoToTiramisuConverter::apply_interchange(
    computation &comp,
    const Transformation &trans) {
    
    std::cout << "[Bridge] Applying interchange" << std::endl;
    
    if (trans.loop_dims.size() >= 2 && trans.iterator_names.size() >= 2) {
        int dim1 = trans.loop_dims[0];
        int dim2 = trans.loop_dims[1];
        
        if (dim1 < trans.iterator_names.size() && dim2 < trans.iterator_names.size()) {
            var v1(trans.iterator_names[dim1].c_str());
            var v2(trans.iterator_names[dim2].c_str());
            
            comp.interchange(v1, v2);
            
            std::cout << "[Bridge] Interchanged " << trans.iterator_names[dim1] 
                      << " ↔ " << trans.iterator_names[dim2] << std::endl;
        } else {
            std::cerr << "[Bridge] Error: Invalid loop dimensions for interchange" << std::endl;
        }
    } else {
        // Fallback
        var i("i"), j("j");
        comp.interchange(i, j);
        std::cout << "[Bridge] Interchange applied (using default names)" << std::endl;
    }
}

/**
 * 完整转换流程
 */
void PlutoToTiramisuConverter::convert_and_apply(
    PlutoProg *pluto_prog,
    computation &comp) {
    
    std::cout << "\n╔══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  PLUTO → Tiramisu Conversion                              ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    // 提取变换
    auto transforms = extract_transformations(pluto_prog);
    
    // 打印信息
    print_transformation_info(transforms);
    
    // 应用变换
    apply_transformations(comp, transforms);
    
    std::cout << "\n╔══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Conversion Complete                                      ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════╝\n" << std::endl;
}

/**
 * 打印变换信息
 */
void PlutoToTiramisuConverter::print_transformation_info(
    const std::vector<Transformation> &transforms) {
    
    std::cout << "\n[Bridge] Transformation Summary:" << std::endl;
    std::cout << "  Total transformations: " << transforms.size() << std::endl;
    
    for (size_t i = 0; i < transforms.size(); i++) {
        const auto &trans = transforms[i];
        std::cout << "  " << (i + 1) << ". ";
        
        switch (trans.type) {
            case TRANS_GPU_TILE:
                std::cout << "GPU Tile: ";
                if (trans.tile_sizes.size() >= 2) {
                    std::cout << trans.tile_sizes[0] << "x" << trans.tile_sizes[1];
                }
                std::cout << " (Memory coalescing enabled)";
                break;
            case TRANS_TILE:
                std::cout << "CPU Tile: ";
                if (trans.tile_sizes.size() >= 2) {
                    std::cout << trans.tile_sizes[0] << "x" << trans.tile_sizes[1];
                }
                break;
            case TRANS_INTERCHANGE:
                std::cout << "Interchange";
                break;
            default:
                std::cout << "Unknown";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

/**
 * 检查是否适合GPU
 */
bool PlutoToTiramisuConverter::is_gpu_suitable(const Transformation &trans) {
    return trans.type == TRANS_GPU_TILE;
}

} // namespace pluto_tiramisu
