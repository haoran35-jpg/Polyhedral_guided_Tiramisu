/*
 * PLUTO to Tiramisu Bridge
 * 
 * 将PLUTO的schedule转换为Tiramisu的scheduling API调用
 */

#ifndef PLUTO_TO_TIRAMISU_H
#define PLUTO_TO_TIRAMISU_H

#include <vector>
#include <string>
#include <map>

// PLUTO headers
extern "C" {
#include "pluto.h"
#include "gpu_constraints.h"
#include "program.h"
#include "pluto/matrix.h"
}

// Tiramisu headers
#include <tiramisu/tiramisu.h>

namespace pluto_tiramisu {

/**
 * PLUTO变换类型
 */
enum TransformType {
    TRANS_TILE,
    TRANS_GPU_TILE,
    TRANS_INTERCHANGE,
    TRANS_SKEW,
    TRANS_PARALLELIZE,
    TRANS_SPLIT
};

/**
 * 单个变换的表示（扩展版本 - 支持任意维度）
 */
struct Transformation {
    TransformType type;
    std::vector<int> loop_dims;         // 涉及的循环维度
    std::vector<int> tile_sizes;        // Tile大小
    std::vector<std::string> iterator_names;  // 迭代器名称（动态）
    int statement_id;                   // 语句ID（多statement支持）
    int factor;                         // Skew/split因子
    
    Transformation(TransformType t) : type(t), statement_id(0), factor(0) {}
};

/**
 * PLUTO到Tiramisu的转换器
 */
class PlutoToTiramisuConverter {
public:
    PlutoToTiramisuConverter(tiramisu::function *fct);
    ~PlutoToTiramisuConverter();
    
    /**
     * 从PLUTO程序提取变换序列
     */
    std::vector<Transformation> extract_transformations(PlutoProg *pluto_prog);
    
    /**
     * 应用变换到Tiramisu computation
     */
    void apply_transformations(
        tiramisu::computation &comp,
        const std::vector<Transformation> &transforms
    );
    
    /**
     * 检测并应用GPU优化
     */
    void apply_gpu_optimizations(
        tiramisu::computation &comp,
        const Transformation &trans
    );
    
    /**
     * 完整的转换流程
     */
    void convert_and_apply(
        PlutoProg *pluto_prog,
        tiramisu::computation &comp
    );
    
    /**
     * 打印转换信息
     */
    void print_transformation_info(const std::vector<Transformation> &transforms);
    
private:
    tiramisu::function *func;
    std::map<std::string, tiramisu::var> var_map;
    
    // 辅助函数
    bool is_gpu_suitable(const Transformation &trans);
    void apply_tile(tiramisu::computation &comp, const Transformation &trans);
    void apply_gpu_tile(tiramisu::computation &comp, const Transformation &trans);
    void apply_interchange(tiramisu::computation &comp, const Transformation &trans);
};

/**
 * 便捷函数：从PLUTO schedule生成Tiramisu代码
 */
void generate_tiramisu_from_pluto(
    const char *c_code,
    const char *output_file,
    bool enable_gpu_constraints
);

} // namespace pluto_tiramisu

#endif
