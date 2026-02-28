//
//  spirv_tools_c.cpp
//  SPIRV-Cross
//
//  Created by Stuart Carnie on 6/19/20.
//

#include "spirv_tools_c.h"

#include "spirv-tools/optimizer.hpp"
#include "spirv-tools/libspirv.h"

#include <memory>
#include <new>
#include <string.h>

using namespace std;
using namespace spvtools;

#ifndef STATIC_ASSERT
#define STATIC_ASSERT(_condition, ...) static_assert(_condition, "" __VA_ARGS__)
#endif

#ifndef ENUM_CHECK
#define ENUM_CHECK(_enum, _src, _c99enumcount) \
    STATIC_ASSERT(_enum::_src == _enum(_c99enumcount) )
#endif

ENUM_CHECK(spv_target_env, SPV_ENV_UNIVERSAL_1_0, SPV_TARGET_ENV_UNIVERSAL_1_0);
ENUM_CHECK(spv_target_env, SPV_ENV_MAX, SPV_TARGET_ENV_MAX);

#pragma mark - Vector

struct spvt_vector_s
{
    std::vector<uint32_t> buf;
};

void spvt_vector_destroy(spvt_vector vec)
{
    delete vec;
}

size_t spvt_vector_get_size(spvt_vector vec)
{
    return vec->buf.size() * sizeof(uint32_t);
}

void *spvt_vector_get_ptr(spvt_vector vec)
{
    return static_cast<void *>(vec->buf.data());
}

#pragma mark - Optimizer

struct spvt_optimizer_s
{
    unique_ptr<Optimizer> optimizer;
};

spvt_optimizer spvt_optimizer_create(spv_target_env_t env)
{
    auto opt = new spvt_optimizer_s();
    opt->optimizer.reset(new Optimizer(static_cast<spv_target_env>(env)));
    return opt;
}

void spvt_optimizer_destroy(spvt_optimizer optimizer)
{
    delete optimizer;
}

void spvt_optimizer_set_consumer(spvt_optimizer optimizer, message_consumer_t callback)
{
    optimizer->optimizer->SetMessageConsumer(
                                             [=](spv_message_level_t level, const char *source, const spv_position_t &position, const char *message) {
        callback(level, source, &position, message);
    });
}

void spvt_optimizer_clear_consumer(spvt_optimizer optimizer)
{
    optimizer->optimizer->SetMessageConsumer(nullptr);
}

spvt_vector spvt_optimizer_run(spvt_optimizer optimizer, uint32_t const * original_binary, size_t original_binary_size)
{
    return spvt_optimizer_run_options(optimizer, original_binary, original_binary_size, spv_optimizer_options());
}

spvt_vector spvt_optimizer_run_options(spvt_optimizer optimizer,
                                       uint32_t const * original_binary, size_t original_binary_size,
                                       spv_optimizer_options options)
{
    vector<uint32_t> optimized;
    optimized.reserve(original_binary_size);
    
    auto res = optimizer->optimizer->Run(original_binary, original_binary_size, &optimized, options);
    if (!res)
    {
        return nullptr;
    }
    
    auto vec = new spvt_vector_s();
    vec->buf = std::move(optimized);
    return vec;
}


void spvt_optimizer_register_performance_passes(spvt_optimizer optimizer)
{
    optimizer->optimizer->RegisterPerformancePasses();
}

void spvt_optimizer_register_size_passes(spvt_optimizer optimizer)
{
    optimizer->optimizer->RegisterSizePasses();
}

bool spvt_optimizer_register_pass_from_flag(spvt_optimizer optimizer, char const * flag)
{
    return optimizer->optimizer->RegisterPassFromFlag(flag);
}

#pragma mark - Optimizer Passes

void spvt_optimizer_register_null_pass(spvt_optimizer optimizer)
{
    optimizer->optimizer->RegisterPass(CreateNullPass());
}

void spvt_optimizer_register_strip_debug_info_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateStripDebugInfoPass());
}

void spvt_optimizer_register_strip_reflect_info_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateStripReflectInfoPass());
}

void spvt_optimizer_register_strip_non_semantic_info_pass(spvt_optimizer optimizer)
{
    optimizer->optimizer->RegisterPass(CreateStripNonSemanticInfoPass());
}

void spvt_optimizer_register_eliminate_dead_functions_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateEliminateDeadFunctionsPass());
}

void spvt_optimizer_register_eliminate_dead_members_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateEliminateDeadMembersPass());
}

//Optimizer::PassToken CreateSetSpecConstantDefaultValuePass( const std::unordered_map<uint32_t, std::string>& id_value_map);

//Optimizer::PassToken CreateSetSpecConstantDefaultValuePass(const std::unordered_map<uint32_t, std::vector<uint32_t>>& id_value_map);

void spvt_optimizer_register_flatten_decoration_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateFlattenDecorationPass());
}

void spvt_optimizer_register_freeze_spec_constant_value_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateFreezeSpecConstantValuePass());
}

void spvt_optimizer_register_fold_spec_constant_op_and_composite_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateFoldSpecConstantOpAndCompositePass());
}

void spvt_optimizer_register_unify_constant_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateUnifyConstantPass());
}

void spvt_optimizer_register_eliminate_dead_constant_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateEliminateDeadConstantPass());
}

void spvt_optimizer_register_strength_reduction_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateStrengthReductionPass());
}

void spvt_optimizer_register_block_merge_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateBlockMergePass());
}

void spvt_optimizer_register_inline_exhaustive_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateInlineExhaustivePass());
}

void spvt_optimizer_register_inline_opaque_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateInlineOpaquePass());
}

void spvt_optimizer_register_local_single_block_load_store_elim_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateLocalSingleBlockLoadStoreElimPass());
}

void spvt_optimizer_register_dead_branch_elim_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateDeadBranchElimPass());
}

void spvt_optimizer_register_local_multi_store_elim_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateLocalMultiStoreElimPass());
}

void spvt_optimizer_register_local_access_chain_convert_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateLocalAccessChainConvertPass());
}

void spvt_optimizer_register_local_single_store_elim_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateLocalSingleStoreElimPass());
}

void spvt_optimizer_register_insert_extract_elim_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateInsertExtractElimPass());
}

void spvt_optimizer_register_dead_insert_elim_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateDeadInsertElimPass());
}

void spvt_optimizer_register_aggressive_dce_pass(spvt_optimizer optimizer)
{
    optimizer->optimizer->RegisterPass(CreateAggressiveDCEPass());
}

void spvt_optimizer_register_remove_unused_interface_variables_pass(spvt_optimizer optimizer)
{
    optimizer->optimizer->RegisterPass(CreateRemoveUnusedInterfaceVariablesPass());
}

void spvt_optimizer_register_propagate_line_info_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreatePropagateLineInfoPass());
}

void spvt_optimizer_register_redundant_line_info_elim_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateRedundantLineInfoElimPass());
}

void spvt_optimizer_register_compact_ids_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateCompactIdsPass());
}

void spvt_optimizer_register_remove_duplicates_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateRemoveDuplicatesPass());
}

void spvt_optimizer_register_cfg_cleanup_pass(spvt_optimizer optimizer)
{
    optimizer->optimizer->RegisterPass(CreateCFGCleanupPass());
}

void spvt_optimizer_register_dead_variable_elimination_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateDeadVariableEliminationPass());
}

void spvt_optimizer_register_merge_return_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateMergeReturnPass());
}

void spvt_optimizer_register_local_redundancy_elimination_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateLocalRedundancyEliminationPass());
}

void spvt_optimizer_register_loop_invariant_code_motion_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateLoopInvariantCodeMotionPass());
}

void spvt_optimizer_register_loop_fission_pass(spvt_optimizer optimizer, size_t threshold) 
{
    optimizer->optimizer->RegisterPass(CreateLoopFissionPass(threshold));
}

void spvt_optimizer_register_loop_fusion_pass(spvt_optimizer optimizer, size_t max_registers_per_loop) 
{
    optimizer->optimizer->RegisterPass(CreateLoopFusionPass(max_registers_per_loop));
}

void spvt_optimizer_register_loop_peeling_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateLoopPeelingPass());
}

void spvt_optimizer_register_loop_unswitch_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateLoopUnswitchPass());
}

void spvt_optimizer_register_redundancy_elimination_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateRedundancyEliminationPass());
}

void spvt_optimizer_register_scalar_replacement_pass(spvt_optimizer optimizer, uint32_t size_limit)
{
  optimizer->optimizer->RegisterPass(CreateScalarReplacementPass(size_limit));
}

void spvt_optimizer_register_private_to_local_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreatePrivateToLocalPass());
}

void spvt_optimizer_register_ccp_pass(spvt_optimizer optimizer)
{
    optimizer->optimizer->RegisterPass(CreateCCPPass());
}

void spvt_optimizer_register_workaround_1209_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateWorkaround1209Pass());
}

void spvt_optimizer_register_if_conversion_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateIfConversionPass());
}

void spvt_optimizer_register_replace_invalid_opcode_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateReplaceInvalidOpcodePass());
}

void spvt_optimizer_register_simplification_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateSimplificationPass());
}

void spvt_optimizer_register_loop_unroll_pass(spvt_optimizer optimizer, bool fully_unroll, int factor)
{
    optimizer->optimizer->RegisterPass(CreateLoopUnrollPass(fully_unroll, factor));
}

void spvt_optimizer_register_ssa_rewrite_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateSSARewritePass());
}

void spvt_optimizer_register_convert_relaxed_to_half_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateConvertRelaxedToHalfPass());
}

void spvt_optimizer_register_relax_float_ops_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateRelaxFloatOpsPass());
}

void spvt_optimizer_register_copy_propagate_arrays_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateCopyPropagateArraysPass());
}

void spvt_optimizer_register_vector_dce_pass(spvt_optimizer optimizer)
{
    optimizer->optimizer->RegisterPass(CreateVectorDCEPass());
}

void spvt_optimizer_register_reduce_load_size_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateReduceLoadSizePass());
}

void spvt_optimizer_register_combine_access_chains_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateCombineAccessChainsPass());
}

void spvt_optimizer_register_inst_bindless_check_pass(spvt_optimizer optimizer, uint32_t desc_set, uint32_t shader_id, bool input_length_enable, bool input_init_enable)
{
  optimizer->optimizer->RegisterPass(CreateInstBindlessCheckPass(desc_set, shader_id, input_length_enable, input_init_enable));
}

void spvt_optimizer_register_inst_buff_addr_check_pass(spvt_optimizer optimizer, uint32_t desc_set, uint32_t shader_id) 
{
  optimizer->optimizer->RegisterPass(CreateInstBuffAddrCheckPass(desc_set, shader_id));
}

void spvt_optimizer_register_inst_debug_printf_pass(spvt_optimizer optimizer, uint32_t desc_set, uint32_t shader_id) 
{
  optimizer->optimizer->RegisterPass(CreateInstDebugPrintfPass(desc_set, shader_id));
}

void spvt_optimizer_register_upgrade_memory_model_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateUpgradeMemoryModelPass());
}

void spvt_optimizer_register_code_sinking_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateCodeSinkingPass());
}

void spvt_optimizer_register_fix_storage_class_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateFixStorageClassPass());
}

void spvt_optimizer_register_graphics_robust_access_pass(spvt_optimizer optimizer)
{
    optimizer->optimizer->RegisterPass(CreateGraphicsRobustAccessPass());
}

void spvt_optimizer_register_spread_volatile_semantics_pass(spvt_optimizer optimizer)
{
    optimizer->optimizer->RegisterPass(CreateSpreadVolatileSemanticsPass());
}

void spvt_optimizer_register_descriptor_scalar_replacement_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateDescriptorScalarReplacementPass());
}

void spvt_optimizer_register_wrap_op_kill_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateWrapOpKillPass());
}

void spvt_optimizer_register_amd_ext_to_khr_pass(spvt_optimizer optimizer) 
{
    optimizer->optimizer->RegisterPass(CreateAmdExtToKhrPass());
}

void spvt_optimizer_register_interpolate_fixup_pass(spvt_optimizer optimizer)
{
    optimizer->optimizer->RegisterPass(CreateInterpolateFixupPass());
}

void spvt_optimizer_register_eliminate_dead_input_components_pass(spvt_optimizer optimizer)
{
    optimizer->optimizer->RegisterPass(CreateEliminateDeadInputComponentsPass());
}

void spvt_optimizer_register_remove_dont_inline_pass(spvt_optimizer optimizer)
{
    optimizer->optimizer->RegisterPass(CreateRemoveDontInlinePass());
}
