#include <libgfd/gfd.h>

#include <cassert>
#include <common/teenyheap.h>
#include <excmd.h>
#include <fmt/format.h>
#include <fstream>
#include <gsl.h>
#include <iostream>
#include <libcpu/cpu.h>
#include <libgfd/gfd.h>
#include <libgpu/gpu_tiling.h>
#include <libgpu/gpu7_tiling.h>
#include <libgpu/latte/latte_disassembler.h>
#include <libgpu/latte/latte_formats.h>
#include <libdecaf/src/cafe/libraries/gx2/gx2_debug_dds.h>
#include <libdecaf/src/cafe/libraries/gx2/gx2_enum_string.h>
#include <libdecaf/src/cafe/libraries/gx2/gx2_internal_gfd.h>
#include <spdlog/spdlog.h>

std::shared_ptr<spdlog::logger>
gLog;

struct OutputState
{
   fmt::memory_buffer writer;
   std::string indent;
};

static void
increaseIndent(OutputState &out)
{
   out.indent += "  ";
}

static void
decreaseIndent(OutputState &out)
{
   if (out.indent.size() >= 2) {
      out.indent.resize(out.indent.size() - 2);
   }
}

static void
startGroup(OutputState &out, const std::string &group)
{
   fmt::format_to(out.writer, "{}{}\n", out.indent, group);
   increaseIndent(out);
}

static void
endGroup(OutputState &out)
{
   decreaseIndent(out);
}

template<typename Type>
static void
writeField(OutputState &out, const std::string &field, const Type &value)
{
   fmt::format_to(out.writer, "{}{:<30} = {}\n", out.indent, field, value);
}

static bool
printInfo(const std::string &filename)
{
   OutputState out;
   gfd::GFDFile file;

   try {
      if (!gfd::readFile(file, filename)) {
         return false;
      }
   } catch (gfd::GFDReadException ex) {
      gLog->error("Error reading gfd: {}", ex.what());
      return false;
   }

   for (auto &shader : file.vertexShaders) {
      startGroup(out, "VertexShaderHeader");
      {
         writeField(out, "size", shader.data.size());
         writeField(out, "mode", cafe::gx2::to_string(shader.mode));

         writeField(out, "uniformBlockCount", shader.uniformBlocks.size());

         for (auto i = 0u; i < shader.uniformBlocks.size(); ++i) {
            startGroup(out, fmt::format("uniformBlocks[{}]", i));
            {
               auto &var = shader.uniformBlocks[i];
               writeField(out, "name", var.name);
               writeField(out, "offset", var.offset);
               writeField(out, "size", var.size);
            }
            endGroup(out);
         }

         writeField(out, "uniformVarCount", shader.uniformVars.size());

         for (auto i = 0u; i < shader.uniformVars.size(); ++i) {
            startGroup(out, fmt::format("uniformVars[{}]", i));
            {
               auto &var = shader.uniformVars[i];
               writeField(out, "name", var.name);
               writeField(out, "type", cafe::gx2::to_string(var.type));
               writeField(out, "count", var.count);
               writeField(out, "offset", var.offset);
               writeField(out, "block", var.block);
            }
            endGroup(out);
         }

         writeField(out, "initialValueCount", shader.initialValues.size());

         for (auto i = 0u; i < shader.initialValues.size(); ++i) {
            startGroup(out, fmt::format("initialValues[{}]", i));
            {
               auto &var = shader.initialValues[i];
               writeField(out, "value.x", var.value[0]);
               writeField(out, "value.y", var.value[1]);
               writeField(out, "value.z", var.value[2]);
               writeField(out, "value.w", var.value[3]);
               writeField(out, "offset", var.offset);
            }
            endGroup(out);
         }

         writeField(out, "loopVarCount", shader.loopVars.size());

         for (auto i = 0u; i < shader.loopVars.size(); ++i) {
            startGroup(out, fmt::format("loopVars[{}]", i));
            {
               auto &var = shader.loopVars[i];
               writeField(out, "offset", var.offset);
               writeField(out, "value", var.value);
            }
            endGroup(out);
         }

         writeField(out, "samplerVarCount", shader.samplerVars.size());

         for (auto i = 0u; i < shader.samplerVars.size(); ++i) {
            startGroup(out, fmt::format("samplerVars[{}]", i));
            {
               auto &var = shader.samplerVars[i];
               writeField(out, "name", var.name);
               writeField(out, "type", cafe::gx2::to_string(var.type));
               writeField(out, "location", var.location);
            }
            endGroup(out);
         }

         writeField(out, "attribVarCount", shader.attribVars.size());

         for (auto i = 0u; i < shader.attribVars.size(); ++i) {
            startGroup(out, fmt::format("attribVars[{}]", i));
            {
               auto &var = shader.attribVars[i];
               writeField(out, "name", var.name);
               writeField(out, "type", cafe::gx2::to_string(var.type));
               writeField(out, "count", var.count);
               writeField(out, "location", var.location);
            }
            endGroup(out);
         }

         writeField(out, "ringItemsize", shader.ringItemSize);
         writeField(out, "hasStreamOut", shader.hasStreamOut);

         startGroup(out, "SQ_PGM_RESOURCES_VS");
         {
            auto sq_pgm_resources_vs = shader.regs.sq_pgm_resources_vs;
            writeField(out, "NUM_GPRS", sq_pgm_resources_vs.NUM_GPRS());
            writeField(out, "STACK_SIZE", sq_pgm_resources_vs.STACK_SIZE());
            writeField(out, "DX10_CLAMP", sq_pgm_resources_vs.DX10_CLAMP());
            writeField(out, "PRIME_CACHE_PGM_EN", sq_pgm_resources_vs.PRIME_CACHE_PGM_EN());
            writeField(out, "PRIME_CACHE_ON_DRAW", sq_pgm_resources_vs.PRIME_CACHE_ON_DRAW());
            writeField(out, "FETCH_CACHE_LINES", sq_pgm_resources_vs.FETCH_CACHE_LINES());
            writeField(out, "UNCACHED_FIRST_INST", sq_pgm_resources_vs.UNCACHED_FIRST_INST());
            writeField(out, "PRIME_CACHE_ENABLE", sq_pgm_resources_vs.PRIME_CACHE_ENABLE());
            writeField(out, "PRIME_CACHE_ON_CONST", sq_pgm_resources_vs.PRIME_CACHE_ON_CONST());
         }
         endGroup(out);

         startGroup(out, "VGT_PRIMITIVEID_EN");
         {
            auto vgt_primitiveid_en = shader.regs.vgt_primitiveid_en;
            writeField(out, "PRIMITIVEID_EN", vgt_primitiveid_en.PRIMITIVEID_EN());
         }
         endGroup(out);

         startGroup(out, "SPI_VS_OUT_CONFIG");
         {
            auto spi_vs_out_config = shader.regs.spi_vs_out_config;
            writeField(out, "VS_PER_COMPONENT", spi_vs_out_config.VS_PER_COMPONENT());
            writeField(out, "VS_EXPORT_COUNT", spi_vs_out_config.VS_EXPORT_COUNT());
            writeField(out, "VS_EXPORTS_FOG", spi_vs_out_config.VS_EXPORTS_FOG());
            writeField(out, "VS_OUT_FOG_VEC_ADDR", spi_vs_out_config.VS_OUT_FOG_VEC_ADDR());
         }
         endGroup(out);

         auto num_spi_vs_out_id = shader.regs.num_spi_vs_out_id;
         writeField(out, "NUM_SPI_VS_OUT_ID", num_spi_vs_out_id);

         auto spi_vs_out_id = shader.regs.spi_vs_out_id;

         for (auto i = 0u; i < std::min<size_t>(num_spi_vs_out_id, spi_vs_out_id.size()); ++i) {
            startGroup(out, fmt::format("SPI_VS_OUT_ID[{}]", i));
            {
               writeField(out, "SEMANTIC_0", spi_vs_out_id[i].SEMANTIC_0());
               writeField(out, "SEMANTIC_1", spi_vs_out_id[i].SEMANTIC_1());
               writeField(out, "SEMANTIC_2", spi_vs_out_id[i].SEMANTIC_2());
               writeField(out, "SEMANTIC_3", spi_vs_out_id[i].SEMANTIC_3());
            }
            endGroup(out);
         }

         startGroup(out, "PA_CL_VS_OUT_CNTL");
         {
            auto pa_cl_vs_out_cntl = shader.regs.pa_cl_vs_out_cntl;
            writeField(out, "CLIP_DIST_ENA_0", pa_cl_vs_out_cntl.CLIP_DIST_ENA_0());
            writeField(out, "CLIP_DIST_ENA_1", pa_cl_vs_out_cntl.CLIP_DIST_ENA_1());
            writeField(out, "CLIP_DIST_ENA_2", pa_cl_vs_out_cntl.CLIP_DIST_ENA_2());
            writeField(out, "CLIP_DIST_ENA_3", pa_cl_vs_out_cntl.CLIP_DIST_ENA_3());
            writeField(out, "CLIP_DIST_ENA_4", pa_cl_vs_out_cntl.CLIP_DIST_ENA_4());
            writeField(out, "CLIP_DIST_ENA_5", pa_cl_vs_out_cntl.CLIP_DIST_ENA_5());
            writeField(out, "CLIP_DIST_ENA_6", pa_cl_vs_out_cntl.CLIP_DIST_ENA_6());
            writeField(out, "CLIP_DIST_ENA_7", pa_cl_vs_out_cntl.CLIP_DIST_ENA_7());
            writeField(out, "CULL_DIST_ENA_0", pa_cl_vs_out_cntl.CULL_DIST_ENA_0());
            writeField(out, "CULL_DIST_ENA_1", pa_cl_vs_out_cntl.CULL_DIST_ENA_1());
            writeField(out, "CULL_DIST_ENA_2", pa_cl_vs_out_cntl.CULL_DIST_ENA_2());
            writeField(out, "CULL_DIST_ENA_3", pa_cl_vs_out_cntl.CULL_DIST_ENA_3());
            writeField(out, "CULL_DIST_ENA_4", pa_cl_vs_out_cntl.CULL_DIST_ENA_4());
            writeField(out, "CULL_DIST_ENA_5", pa_cl_vs_out_cntl.CULL_DIST_ENA_5());
            writeField(out, "CULL_DIST_ENA_6", pa_cl_vs_out_cntl.CULL_DIST_ENA_6());
            writeField(out, "CULL_DIST_ENA_7", pa_cl_vs_out_cntl.CULL_DIST_ENA_7());
            writeField(out, "USE_VTX_POINT_SIZE", pa_cl_vs_out_cntl.USE_VTX_POINT_SIZE());
            writeField(out, "USE_VTX_EDGE_FLAG", pa_cl_vs_out_cntl.USE_VTX_EDGE_FLAG());
            writeField(out, "USE_VTX_RENDER_TARGET_INDX", pa_cl_vs_out_cntl.USE_VTX_RENDER_TARGET_INDX());
            writeField(out, "USE_VTX_VIEWPORT_INDX", pa_cl_vs_out_cntl.USE_VTX_VIEWPORT_INDX());
            writeField(out, "USE_VTX_KILL_FLAG", pa_cl_vs_out_cntl.USE_VTX_KILL_FLAG());
            writeField(out, "VS_OUT_MISC_VEC_ENA", pa_cl_vs_out_cntl.VS_OUT_MISC_VEC_ENA());
            writeField(out, "VS_OUT_CCDIST0_VEC_ENA", pa_cl_vs_out_cntl.VS_OUT_CCDIST0_VEC_ENA());
            writeField(out, "VS_OUT_CCDIST1_VEC_ENA", pa_cl_vs_out_cntl.VS_OUT_CCDIST1_VEC_ENA());
            writeField(out, "VS_OUT_MISC_SIDE_BUS_ENA", pa_cl_vs_out_cntl.VS_OUT_MISC_SIDE_BUS_ENA());
            writeField(out, "USE_VTX_GS_CUT_FLAG", pa_cl_vs_out_cntl.USE_VTX_GS_CUT_FLAG());
         }
         endGroup(out);

         startGroup(out, "SQ_VTX_SEMANTIC_CLEAR");
         {
            auto sq_vtx_semantic_clear = shader.regs.sq_vtx_semantic_clear;
            writeField(out, "CLEAR", sq_vtx_semantic_clear.CLEAR());
         }
         endGroup(out);

         auto num_sq_vtx_semantic = shader.regs.num_sq_vtx_semantic;
         writeField(out, "NUM_SQ_VTX_SEMANTIC", num_sq_vtx_semantic);

         auto sq_vtx_semantic = shader.regs.sq_vtx_semantic;
         for (auto i = 0u; i < std::min<size_t>(num_sq_vtx_semantic, sq_vtx_semantic.size()); ++i) {
            startGroup(out, fmt::format("SQ_VTX_SEMANTIC[{}]", i));
            {
               writeField(out, "SEMANTIC_ID", sq_vtx_semantic[i].SEMANTIC_ID());
            }
            endGroup(out);
         }

         startGroup(out, "VGT_STRMOUT_BUFFER_EN");
         {
            auto vgt_strmout_buffer_en = shader.regs.vgt_strmout_buffer_en;
            writeField(out, "BUFFER_0_EN", vgt_strmout_buffer_en.BUFFER_0_EN());
            writeField(out, "BUFFER_1_EN", vgt_strmout_buffer_en.BUFFER_1_EN());
            writeField(out, "BUFFER_2_EN", vgt_strmout_buffer_en.BUFFER_2_EN());
            writeField(out, "BUFFER_3_EN", vgt_strmout_buffer_en.BUFFER_3_EN());
         }
         endGroup(out);

         startGroup(out, "VGT_VERTEX_REUSE_BLOCK_CNTL");
         {
            auto vgt_vertex_reuse_block_cntl = shader.regs.vgt_vertex_reuse_block_cntl;
            writeField(out, "VTX_REUSE_DEPTH", vgt_vertex_reuse_block_cntl.VTX_REUSE_DEPTH());
         }
         endGroup(out);

         startGroup(out, "VGT_HOS_REUSE_DEPTH");
         {
            auto vgt_hos_reuse_depth = shader.regs.vgt_hos_reuse_depth;
            writeField(out, "REUSE_DEPTH", vgt_hos_reuse_depth.REUSE_DEPTH());
         }
         endGroup(out);
      }
      endGroup(out);

      startGroup(out, "VertexShaderProgram");
      {
         writeField(out, "size", shader.data.size());

         std::string disassembly;
         disassembly = latte::disassemble(gsl::make_span(shader.data));
         fmt::format_to(out.writer, "\n{}", disassembly);
      }
      endGroup(out);
   }

   for (auto &shader : file.pixelShaders) {
      startGroup(out, "PixelShaderHeader");
      {
         writeField(out, "size", shader.data.size());
         writeField(out, "mode", cafe::gx2::to_string(shader.mode));

         writeField(out, "uniformBlockCount", shader.uniformBlocks.size());

         for (auto i = 0u; i < shader.uniformBlocks.size(); ++i) {
            startGroup(out, fmt::format("uniformBlocks[{}]", i));
            {
               auto &var = shader.uniformBlocks[i];
               writeField(out, "name", var.name);
               writeField(out, "offset", var.offset);
               writeField(out, "size", var.size);
            }
            endGroup(out);
         }

         writeField(out, "uniformVarCount", shader.uniformVars.size());

         for (auto i = 0u; i < shader.uniformVars.size(); ++i) {
            startGroup(out, fmt::format("uniformVars[{}]", i));
            {
               auto &var = shader.uniformVars[i];
               writeField(out, "name", var.name);
               writeField(out, "type", cafe::gx2::to_string(var.type));
               writeField(out, "count", var.count);
               writeField(out, "offset", var.offset);
               writeField(out, "block", var.block);
            }
            endGroup(out);
         }

         writeField(out, "initialValueCount", shader.initialValues.size());

         for (auto i = 0u; i < shader.initialValues.size(); ++i) {
            startGroup(out, fmt::format("initialValues[{}]", i));
            {
               auto &var = shader.initialValues[i];
               writeField(out, "value.x", var.value[0]);
               writeField(out, "value.y", var.value[1]);
               writeField(out, "value.z", var.value[2]);
               writeField(out, "value.w", var.value[3]);
               writeField(out, "offset", var.offset);
            }
            endGroup(out);
         }

         writeField(out, "loopVarCount", shader.loopVars.size());

         for (auto i = 0u; i < shader.loopVars.size(); ++i) {
            startGroup(out, fmt::format("loopVars[{}]", i));
            {
               auto &var = shader.loopVars[i];
               writeField(out, "offset", var.offset);
               writeField(out, "value", var.value);
            }
            endGroup(out);
         }

         writeField(out, "samplerVarCount", shader.samplerVars.size());

         for (auto i = 0u; i < shader.samplerVars.size(); ++i) {
            startGroup(out, fmt::format("samplerVars[{}]", i));
            {
               auto &var = shader.samplerVars[i];
               writeField(out, "name", var.name);
               writeField(out, "type", cafe::gx2::to_string(var.type));
               writeField(out, "location", var.location);
            }
            endGroup(out);
         }

         startGroup(out, "SQ_PGM_RESOURCES_PS");
         {
            auto sq_pgm_resources_ps = shader.regs.sq_pgm_resources_ps;
            writeField(out, "NUM_GPRS", sq_pgm_resources_ps.NUM_GPRS());
            writeField(out, "STACK_SIZE", sq_pgm_resources_ps.STACK_SIZE());
            writeField(out, "DX10_CLAMP", sq_pgm_resources_ps.DX10_CLAMP());
            writeField(out, "PRIME_CACHE_PGM_EN", sq_pgm_resources_ps.PRIME_CACHE_PGM_EN());
            writeField(out, "PRIME_CACHE_ON_DRAW", sq_pgm_resources_ps.PRIME_CACHE_ON_DRAW());
            writeField(out, "FETCH_CACHE_LINES", sq_pgm_resources_ps.FETCH_CACHE_LINES());
            writeField(out, "UNCACHED_FIRST_INST", sq_pgm_resources_ps.UNCACHED_FIRST_INST());
            writeField(out, "PRIME_CACHE_ENABLE", sq_pgm_resources_ps.PRIME_CACHE_ENABLE());
            writeField(out, "PRIME_CACHE_ON_CONST", sq_pgm_resources_ps.PRIME_CACHE_ON_CONST());
            writeField(out, "CLAMP_CONSTS", sq_pgm_resources_ps.CLAMP_CONSTS());
         }
         endGroup(out);

         startGroup(out, "SQ_PGM_EXPORTS_PS");
         {
            auto sq_pgm_exports_ps = shader.regs.sq_pgm_exports_ps;
            writeField(out, "EXPORT_MODE", sq_pgm_exports_ps.EXPORT_MODE());
         }
         endGroup(out);

         startGroup(out, "SPI_PS_IN_CONTROL_0");
         {
            auto spi_ps_in_control_0 = shader.regs.spi_ps_in_control_0;
            writeField(out, "NUM_INTERP", spi_ps_in_control_0.NUM_INTERP());
            writeField(out, "POSITION_ENA", spi_ps_in_control_0.POSITION_ENA());
            writeField(out, "POSITION_CENTROID", spi_ps_in_control_0.POSITION_CENTROID());
            writeField(out, "POSITION_ADDR", spi_ps_in_control_0.POSITION_ADDR());
            writeField(out, "PARAM_GEN", spi_ps_in_control_0.PARAM_GEN());
            writeField(out, "PARAM_GEN_ADDR", spi_ps_in_control_0.PARAM_GEN_ADDR());
            writeField(out, "BARYC_SAMPLE_CNTL", spi_ps_in_control_0.BARYC_SAMPLE_CNTL());
            writeField(out, "PERSP_GRADIENT_ENA", spi_ps_in_control_0.PERSP_GRADIENT_ENA());
            writeField(out, "LINEAR_GRADIENT_ENA", spi_ps_in_control_0.LINEAR_GRADIENT_ENA());
            writeField(out, "POSITION_SAMPLE", spi_ps_in_control_0.POSITION_SAMPLE());
            writeField(out, "BARYC_AT_SAMPLE_ENA", spi_ps_in_control_0.BARYC_AT_SAMPLE_ENA());
         }
         endGroup(out);

         startGroup(out, "SPI_PS_IN_CONTROL_1");
         {
            auto spi_ps_in_control_1 = shader.regs.spi_ps_in_control_1;
            writeField(out, "GEN_INDEX_PIX", spi_ps_in_control_1.GEN_INDEX_PIX());
            writeField(out, "GEN_INDEX_PIX_ADDR", spi_ps_in_control_1.GEN_INDEX_PIX_ADDR());
            writeField(out, "FRONT_FACE_ENA", spi_ps_in_control_1.FRONT_FACE_ENA());
            writeField(out, "FRONT_FACE_CHAN", spi_ps_in_control_1.FRONT_FACE_CHAN());
            writeField(out, "FRONT_FACE_ALL_BITS", spi_ps_in_control_1.FRONT_FACE_ALL_BITS());
            writeField(out, "FRONT_FACE_ADDR", spi_ps_in_control_1.FRONT_FACE_ADDR());
            writeField(out, "FOG_ADDR", spi_ps_in_control_1.FOG_ADDR());
            writeField(out, "FIXED_PT_POSITION_ENA", spi_ps_in_control_1.FIXED_PT_POSITION_ENA());
            writeField(out, "FIXED_PT_POSITION_ADDR", spi_ps_in_control_1.FIXED_PT_POSITION_ADDR());
            writeField(out, "POSITION_ULC", spi_ps_in_control_1.POSITION_ULC());
         }
         endGroup(out);

         auto num_spi_ps_input_cntl = shader.regs.num_spi_ps_input_cntl;
         auto spi_ps_input_cntls = shader.regs.spi_ps_input_cntls;
         writeField(out, "NUM_SPI_PS_INPUT_CNTL", num_spi_ps_input_cntl);

         for (auto i = 0u; i < std::min<size_t>(num_spi_ps_input_cntl, spi_ps_input_cntls.size()); ++i) {
            startGroup(out, fmt::format("SPI_PS_INPUT_CNTL[{}]", i));
            {
               writeField(out, "SEMANTIC", spi_ps_input_cntls[i].SEMANTIC());
               writeField(out, "DEFAULT_VAL", spi_ps_input_cntls[i].DEFAULT_VAL());
               writeField(out, "FLAT_SHADE", spi_ps_input_cntls[i].FLAT_SHADE());
               writeField(out, "SEL_CENTROID", spi_ps_input_cntls[i].SEL_CENTROID());
               writeField(out, "SEL_LINEAR", spi_ps_input_cntls[i].SEL_LINEAR());
               writeField(out, "CYL_WRAP", spi_ps_input_cntls[i].CYL_WRAP());
               writeField(out, "PT_SPRITE_TEX", spi_ps_input_cntls[i].PT_SPRITE_TEX());
               writeField(out, "SEL_SAMPLE", spi_ps_input_cntls[i].SEL_SAMPLE());
            }
            endGroup(out);
         }

         startGroup(out, "CB_SHADER_MASK");
         {
            auto cb_shader_mask = shader.regs.cb_shader_mask;
            writeField(out, "OUTPUT0_ENABLE", cb_shader_mask.OUTPUT0_ENABLE());
            writeField(out, "OUTPUT1_ENABLE", cb_shader_mask.OUTPUT1_ENABLE());
            writeField(out, "OUTPUT2_ENABLE", cb_shader_mask.OUTPUT2_ENABLE());
            writeField(out, "OUTPUT3_ENABLE", cb_shader_mask.OUTPUT3_ENABLE());
            writeField(out, "OUTPUT4_ENABLE", cb_shader_mask.OUTPUT4_ENABLE());
            writeField(out, "OUTPUT5_ENABLE", cb_shader_mask.OUTPUT5_ENABLE());
            writeField(out, "OUTPUT6_ENABLE", cb_shader_mask.OUTPUT6_ENABLE());
            writeField(out, "OUTPUT7_ENABLE", cb_shader_mask.OUTPUT7_ENABLE());
         }
         endGroup(out);

         startGroup(out, "CB_SHADER_CONTROL");
         {
            auto cb_shader_control = shader.regs.cb_shader_control;
            writeField(out, "RT0_ENABLE", cb_shader_control.RT0_ENABLE());
            writeField(out, "RT1_ENABLE", cb_shader_control.RT1_ENABLE());
            writeField(out, "RT2_ENABLE", cb_shader_control.RT2_ENABLE());
            writeField(out, "RT3_ENABLE", cb_shader_control.RT3_ENABLE());
            writeField(out, "RT4_ENABLE", cb_shader_control.RT4_ENABLE());
            writeField(out, "RT5_ENABLE", cb_shader_control.RT5_ENABLE());
            writeField(out, "RT6_ENABLE", cb_shader_control.RT6_ENABLE());
            writeField(out, "RT7_ENABLE", cb_shader_control.RT7_ENABLE());
         }
         endGroup(out);

         startGroup(out, "DB_SHADER_CONTROL");
         {
            auto db_shader_control = shader.regs.db_shader_control;
            writeField(out, "Z_EXPORT_ENABLE", db_shader_control.Z_EXPORT_ENABLE());
            writeField(out, "STENCIL_REF_EXPORT_ENABLE", db_shader_control.STENCIL_REF_EXPORT_ENABLE());
            writeField(out, "Z_ORDER", db_shader_control.Z_ORDER());
            writeField(out, "KILL_ENABLE", db_shader_control.KILL_ENABLE());
            writeField(out, "COVERAGE_TO_MASK_ENABLE", db_shader_control.COVERAGE_TO_MASK_ENABLE());
            writeField(out, "MASK_EXPORT_ENABLE", db_shader_control.MASK_EXPORT_ENABLE());
            writeField(out, "DUAL_EXPORT_ENABLE", db_shader_control.DUAL_EXPORT_ENABLE());
            writeField(out, "EXEC_ON_HIER_FAIL", db_shader_control.EXEC_ON_HIER_FAIL());
            writeField(out, "EXEC_ON_NOOP", db_shader_control.EXEC_ON_NOOP());
            writeField(out, "ALPHA_TO_MASK_DISABLE", db_shader_control.ALPHA_TO_MASK_DISABLE());
         }
         endGroup(out);

         startGroup(out, "SPI_INPUT_Z");
         {
            auto spi_input_z = shader.regs.spi_input_z;
            writeField(out, "PROVIDE_Z_TO_SPI", spi_input_z.PROVIDE_Z_TO_SPI());
         }
         endGroup(out);
      }
      endGroup(out);

      startGroup(out, "PixelShaderProgram");
      {
         writeField(out, "size", shader.data.size());

         std::string disassembly;
         disassembly = latte::disassemble(gsl::make_span(shader.data));
         fmt::format_to(out.writer, "\n{}", disassembly);
      }
      endGroup(out);
   }

   for (auto &shader : file.geometryShaders) {
      startGroup(out, "GeometryShaderHeader");
      {
         writeField(out, "size", shader.data.size());
         writeField(out, "vshSize", shader.vertexShaderData.size());
         writeField(out, "mode", cafe::gx2::to_string(shader.mode));

         writeField(out, "uniformBlockCount", shader.uniformBlocks.size());

         for (auto i = 0u; i < shader.uniformBlocks.size(); ++i) {
            startGroup(out, fmt::format("uniformBlocks[{}]", i));
            {
               auto &var = shader.uniformBlocks[i];
               writeField(out, "name", var.name);
               writeField(out, "offset", var.offset);
               writeField(out, "size", var.size);
            }
            endGroup(out);
         }

         writeField(out, "uniformVarCount", shader.uniformVars.size());

         for (auto i = 0u; i < shader.uniformVars.size(); ++i) {
            startGroup(out, fmt::format("uniformVars[{}]", i));
            {
               auto &var = shader.uniformVars[i];
               writeField(out, "name", var.name);
               writeField(out, "type", cafe::gx2::to_string(var.type));
               writeField(out, "count", var.count);
               writeField(out, "offset", var.offset);
               writeField(out, "block", var.block);
            }
            endGroup(out);
         }

         writeField(out, "initialValueCount", shader.initialValues.size());

         for (auto i = 0u; i < shader.initialValues.size(); ++i) {
            startGroup(out, fmt::format("initialValues[{}]", i));
            {
               auto &var = shader.initialValues[i];
               writeField(out, "value.x", var.value[0]);
               writeField(out, "value.y", var.value[1]);
               writeField(out, "value.z", var.value[2]);
               writeField(out, "value.w", var.value[3]);
               writeField(out, "offset", var.offset);
            }
            endGroup(out);
         }

         writeField(out, "loopVarCount", shader.loopVars.size());

         for (auto i = 0u; i < shader.loopVars.size(); ++i) {
            startGroup(out, fmt::format("loopVars[{}]", i));
            {
               auto &var = shader.loopVars[i];
               writeField(out, "offset", var.offset);
               writeField(out, "value", var.value);
            }
            endGroup(out);
         }

         writeField(out, "samplerVarCount", shader.samplerVars.size());

         for (auto i = 0u; i < shader.samplerVars.size(); ++i) {
            startGroup(out, fmt::format("samplerVars[{}]", i));
            {
               auto &var = shader.samplerVars[i];
               writeField(out, "name", var.name);
               writeField(out, "type", cafe::gx2::to_string(var.type));
               writeField(out, "location", var.location);
            }
            endGroup(out);
         }

         writeField(out, "ringItemSize", shader.ringItemSize);
         writeField(out, "hasStreamOut", shader.hasStreamOut);

         for (auto i = 0u; i < shader.streamOutStride.size(); ++i) {
            writeField(out, fmt::format("streamOutStride[{}]", i), shader.streamOutStride[i]);
         }

         startGroup(out, "SQ_PGM_RESOURCES_GS");
         {
            auto sq_pgm_resources_gs = shader.regs.sq_pgm_resources_gs;
            writeField(out, "NUM_GPRS", sq_pgm_resources_gs.NUM_GPRS());
            writeField(out, "STACK_SIZE", sq_pgm_resources_gs.STACK_SIZE());
            writeField(out, "DX10_CLAMP", sq_pgm_resources_gs.DX10_CLAMP());
            writeField(out, "PRIME_CACHE_PGM_EN", sq_pgm_resources_gs.PRIME_CACHE_PGM_EN());
            writeField(out, "PRIME_CACHE_ON_DRAW", sq_pgm_resources_gs.PRIME_CACHE_ON_DRAW());
            writeField(out, "FETCH_CACHE_LINES", sq_pgm_resources_gs.FETCH_CACHE_LINES());
            writeField(out, "UNCACHED_FIRST_INST", sq_pgm_resources_gs.UNCACHED_FIRST_INST());
            writeField(out, "PRIME_CACHE_ENABLE", sq_pgm_resources_gs.PRIME_CACHE_ENABLE());
            writeField(out, "PRIME_CACHE_ON_CONST", sq_pgm_resources_gs.PRIME_CACHE_ON_CONST());
         }
         endGroup(out);

         startGroup(out, "VGT_GS_OUT_PRIM_TYPE");
         {
            auto vgt_gs_out_prim_type = shader.regs.vgt_gs_out_prim_type;
            writeField(out, "PRIM_TYPE", vgt_gs_out_prim_type.PRIM_TYPE());
         }
         endGroup(out);

         startGroup(out, "VGT_GS_MODE");
         {
            auto vgt_gs_mode = shader.regs.vgt_gs_mode;
            writeField(out, "MODE", vgt_gs_mode.MODE());
            writeField(out, "ES_PASSTHRU", vgt_gs_mode.ES_PASSTHRU());
            writeField(out, "CUT_MODE", vgt_gs_mode.CUT_MODE());
            writeField(out, "MODE_HI", vgt_gs_mode.MODE_HI());
            writeField(out, "GS_C_PACK_EN", vgt_gs_mode.GS_C_PACK_EN());
            writeField(out, "COMPUTE_MODE", vgt_gs_mode.COMPUTE_MODE());
            writeField(out, "FAST_COMPUTE_MODE", vgt_gs_mode.FAST_COMPUTE_MODE());
            writeField(out, "ELEMENT_INFO_EN", vgt_gs_mode.ELEMENT_INFO_EN());
            writeField(out, "PARTIAL_THD_AT_EOI", vgt_gs_mode.PARTIAL_THD_AT_EOI());
         }
         endGroup(out);

         startGroup(out, "PA_CL_VS_OUT_CNTL");
         {
            auto pa_cl_vs_out_cntl = shader.regs.pa_cl_vs_out_cntl;
            writeField(out, "CLIP_DIST_ENA_0", pa_cl_vs_out_cntl.CLIP_DIST_ENA_0());
            writeField(out, "CLIP_DIST_ENA_1", pa_cl_vs_out_cntl.CLIP_DIST_ENA_1());
            writeField(out, "CLIP_DIST_ENA_2", pa_cl_vs_out_cntl.CLIP_DIST_ENA_2());
            writeField(out, "CLIP_DIST_ENA_3", pa_cl_vs_out_cntl.CLIP_DIST_ENA_3());
            writeField(out, "CLIP_DIST_ENA_4", pa_cl_vs_out_cntl.CLIP_DIST_ENA_4());
            writeField(out, "CLIP_DIST_ENA_5", pa_cl_vs_out_cntl.CLIP_DIST_ENA_5());
            writeField(out, "CLIP_DIST_ENA_6", pa_cl_vs_out_cntl.CLIP_DIST_ENA_6());
            writeField(out, "CLIP_DIST_ENA_7", pa_cl_vs_out_cntl.CLIP_DIST_ENA_7());
            writeField(out, "CULL_DIST_ENA_0", pa_cl_vs_out_cntl.CULL_DIST_ENA_0());
            writeField(out, "CULL_DIST_ENA_1", pa_cl_vs_out_cntl.CULL_DIST_ENA_1());
            writeField(out, "CULL_DIST_ENA_2", pa_cl_vs_out_cntl.CULL_DIST_ENA_2());
            writeField(out, "CULL_DIST_ENA_3", pa_cl_vs_out_cntl.CULL_DIST_ENA_3());
            writeField(out, "CULL_DIST_ENA_4", pa_cl_vs_out_cntl.CULL_DIST_ENA_4());
            writeField(out, "CULL_DIST_ENA_5", pa_cl_vs_out_cntl.CULL_DIST_ENA_5());
            writeField(out, "CULL_DIST_ENA_6", pa_cl_vs_out_cntl.CULL_DIST_ENA_6());
            writeField(out, "CULL_DIST_ENA_7", pa_cl_vs_out_cntl.CULL_DIST_ENA_7());
            writeField(out, "USE_VTX_POINT_SIZE", pa_cl_vs_out_cntl.USE_VTX_POINT_SIZE());
            writeField(out, "USE_VTX_EDGE_FLAG", pa_cl_vs_out_cntl.USE_VTX_EDGE_FLAG());
            writeField(out, "USE_VTX_RENDER_TARGET_INDX", pa_cl_vs_out_cntl.USE_VTX_RENDER_TARGET_INDX());
            writeField(out, "USE_VTX_VIEWPORT_INDX", pa_cl_vs_out_cntl.USE_VTX_VIEWPORT_INDX());
            writeField(out, "USE_VTX_KILL_FLAG", pa_cl_vs_out_cntl.USE_VTX_KILL_FLAG());
            writeField(out, "VS_OUT_MISC_VEC_ENA", pa_cl_vs_out_cntl.VS_OUT_MISC_VEC_ENA());
            writeField(out, "VS_OUT_CCDIST0_VEC_ENA", pa_cl_vs_out_cntl.VS_OUT_CCDIST0_VEC_ENA());
            writeField(out, "VS_OUT_CCDIST1_VEC_ENA", pa_cl_vs_out_cntl.VS_OUT_CCDIST1_VEC_ENA());
            writeField(out, "VS_OUT_MISC_SIDE_BUS_ENA", pa_cl_vs_out_cntl.VS_OUT_MISC_SIDE_BUS_ENA());
            writeField(out, "USE_VTX_GS_CUT_FLAG", pa_cl_vs_out_cntl.USE_VTX_GS_CUT_FLAG());
         }
         endGroup(out);

         auto num_spi_vs_out_id = shader.regs.num_spi_vs_out_id;
         writeField(out, "NUM_SPI_VS_OUT_ID", num_spi_vs_out_id);

         auto spi_vs_out_id = shader.regs.spi_vs_out_id;

         for (auto i = 0u; i < std::min<size_t>(num_spi_vs_out_id, spi_vs_out_id.size()); ++i) {
            startGroup(out, fmt::format("SPI_VS_OUT_ID[{}]", i));
            {
               writeField(out, "SEMANTIC_0", spi_vs_out_id[i].SEMANTIC_0());
               writeField(out, "SEMANTIC_1", spi_vs_out_id[i].SEMANTIC_1());
               writeField(out, "SEMANTIC_2", spi_vs_out_id[i].SEMANTIC_2());
               writeField(out, "SEMANTIC_3", spi_vs_out_id[i].SEMANTIC_3());
            }
            endGroup(out);
         }

         startGroup(out, "SQ_PGM_RESOURCES_VS");
         {
            auto sq_pgm_resources_vs = shader.regs.sq_pgm_resources_vs;
            writeField(out, "NUM_GPRS", sq_pgm_resources_vs.NUM_GPRS());
            writeField(out, "STACK_SIZE", sq_pgm_resources_vs.STACK_SIZE());
            writeField(out, "DX10_CLAMP", sq_pgm_resources_vs.DX10_CLAMP());
            writeField(out, "PRIME_CACHE_PGM_EN", sq_pgm_resources_vs.PRIME_CACHE_PGM_EN());
            writeField(out, "PRIME_CACHE_ON_DRAW", sq_pgm_resources_vs.PRIME_CACHE_ON_DRAW());
            writeField(out, "FETCH_CACHE_LINES", sq_pgm_resources_vs.FETCH_CACHE_LINES());
            writeField(out, "UNCACHED_FIRST_INST", sq_pgm_resources_vs.UNCACHED_FIRST_INST());
            writeField(out, "PRIME_CACHE_ENABLE", sq_pgm_resources_vs.PRIME_CACHE_ENABLE());
            writeField(out, "PRIME_CACHE_ON_CONST", sq_pgm_resources_vs.PRIME_CACHE_ON_CONST());
         }
         endGroup(out);

         startGroup(out, "SQ_GS_VERT_ITEMSIZE");
         {
            auto sq_gs_vert_itemsize = shader.regs.sq_gs_vert_itemsize;
            writeField(out, "ITEMSIZE", sq_gs_vert_itemsize.ITEMSIZE());
         }
         endGroup(out);

         startGroup(out, "SPI_VS_OUT_CONFIG");
         {
            auto spi_vs_out_config = shader.regs.spi_vs_out_config;
            writeField(out, "VS_PER_COMPONENT", spi_vs_out_config.VS_PER_COMPONENT());
            writeField(out, "VS_EXPORT_COUNT", spi_vs_out_config.VS_EXPORT_COUNT());
            writeField(out, "VS_EXPORTS_FOG", spi_vs_out_config.VS_EXPORTS_FOG());
            writeField(out, "VS_OUT_FOG_VEC_ADDR", spi_vs_out_config.VS_OUT_FOG_VEC_ADDR());
         }
         endGroup(out);

         startGroup(out, "VGT_STRMOUT_BUFFER_EN");
         {
            auto vgt_strmout_buffer_en = shader.regs.vgt_strmout_buffer_en;
            writeField(out, "BUFFER_0_EN", vgt_strmout_buffer_en.BUFFER_0_EN());
            writeField(out, "BUFFER_1_EN", vgt_strmout_buffer_en.BUFFER_1_EN());
            writeField(out, "BUFFER_2_EN", vgt_strmout_buffer_en.BUFFER_2_EN());
            writeField(out, "BUFFER_3_EN", vgt_strmout_buffer_en.BUFFER_3_EN());
         }
         endGroup(out);
      }
      endGroup(out);

      startGroup(out, "GeometryShaderProgram");
      {
         writeField(out, "size", shader.data.size());

         std::string disassembly;
         disassembly = latte::disassemble(gsl::make_span(shader.data));
         fmt::format_to(out.writer, "\n{}", disassembly);
      }
      endGroup(out);

      startGroup(out, "GeometryShaderCopyProgram");
      {
         writeField(out, "size", shader.vertexShaderData.size());

         std::string disassembly;
         disassembly = latte::disassemble(gsl::make_span(shader.vertexShaderData));
         fmt::format_to(out.writer, "\n{}", disassembly);
      }
      endGroup(out);
   }

   for (auto &tex : file.textures) {

      startGroup(out, "TextureHeader");
      {
         writeField(out, "dim", cafe::gx2::to_string(tex.surface.dim));
         writeField(out, "width", tex.surface.width);
         writeField(out, "height", tex.surface.height);
         writeField(out, "depth", tex.surface.depth);
         writeField(out, "mipLevels", tex.surface.mipLevels);
         writeField(out, "format", cafe::gx2::to_string(tex.surface.format));
         writeField(out, "aa", cafe::gx2::to_string(tex.surface.aa));
         writeField(out, "use", cafe::gx2::to_string(tex.surface.use));
         writeField(out, "imageSize", tex.surface.image.size());
         writeField(out, "mipmapSize", tex.surface.mipmap.size());
         writeField(out, "tileMode", cafe::gx2::to_string(tex.surface.tileMode));
         writeField(out, "swizzle", tex.surface.swizzle);
         writeField(out, "alignment", tex.surface.alignment);
         writeField(out, "pitch", tex.surface.pitch);

         for (auto i = 0u; i < tex.surface.mipLevelOffset.size(); ++i) {
            writeField(out, fmt::format("mipLevelOffset[{}]", i), tex.surface.mipLevelOffset[i]);
         }

         writeField(out, "viewFirstMip", tex.viewFirstMip);
         writeField(out, "viewNumMips", tex.viewNumMips);
         writeField(out, "viewFirstSlice", tex.viewFirstSlice);
         writeField(out, "viewNumSlices", tex.viewNumSlices);
         writeField(out, "compMap", tex.compMap);

         startGroup(out, "SQ_TEX_RESOURCE_WORD0_0");
         {
            auto word0 = tex.regs.word0;
            writeField(out, "DIM", word0.DIM());
            writeField(out, "TILE_MODE", word0.TILE_MODE());
            writeField(out, "TILE_TYPE", word0.TILE_TYPE());
            writeField(out, "PITCH", word0.PITCH());
            writeField(out, "TEX_WIDTH", word0.TEX_WIDTH());
         }
         endGroup(out);

         startGroup(out, "SQ_TEX_RESOURCE_WORD0_1");
         {
            auto word1 = tex.regs.word1;
            writeField(out, "TEX_HEIGHT", word1.TEX_HEIGHT());
            writeField(out, "TEX_DEPTH", word1.TEX_DEPTH());
            writeField(out, "DATA_FORMAT", word1.DATA_FORMAT());
         }
         endGroup(out);

         startGroup(out, "SQ_TEX_RESOURCE_WORD0_4");
         {
            auto word4 = tex.regs.word4;
            writeField(out, "FORMAT_COMP_X", word4.FORMAT_COMP_X());
            writeField(out, "FORMAT_COMP_Y", word4.FORMAT_COMP_Y());
            writeField(out, "FORMAT_COMP_Z", word4.FORMAT_COMP_Z());
            writeField(out, "FORMAT_COMP_W", word4.FORMAT_COMP_W());
            writeField(out, "NUM_FORMAT_ALL", word4.NUM_FORMAT_ALL());
            writeField(out, "SRF_MODE_ALL", word4.SRF_MODE_ALL());
            writeField(out, "FORCE_DEGAMMA", word4.FORCE_DEGAMMA());
            writeField(out, "ENDIAN_SWAP", word4.ENDIAN_SWAP());
            writeField(out, "REQUEST_SIZE", word4.REQUEST_SIZE());
            writeField(out, "DST_SEL_X", word4.DST_SEL_X());
            writeField(out, "DST_SEL_Y", word4.DST_SEL_Y());
            writeField(out, "DST_SEL_Z", word4.DST_SEL_Z());
            writeField(out, "DST_SEL_W", word4.DST_SEL_W());
            writeField(out, "BASE_LEVEL", word4.BASE_LEVEL());
         }
         endGroup(out);

         startGroup(out, "SQ_TEX_RESOURCE_WORD0_5");
         {
            auto word5 = tex.regs.word5;
            writeField(out, "LAST_LEVEL", word5.LAST_LEVEL());
            writeField(out, "BASE_ARRAY", word5.BASE_ARRAY());
            writeField(out, "LAST_ARRAY", word5.LAST_ARRAY());
            writeField(out, "YUV_CONV", word5.YUV_CONV());
         }
         endGroup(out);

         startGroup(out, "SQ_TEX_RESOURCE_WORD0_6");
         {
            auto word6 = tex.regs.word6;
            writeField(out, "MPEG_CLAMP", word6.MPEG_CLAMP());
            writeField(out, "MAX_ANISO_RATIO", word6.MAX_ANISO_RATIO());
            writeField(out, "PERF_MODULATION", word6.PERF_MODULATION());
            writeField(out, "INTERLACED", word6.INTERLACED());
            writeField(out, "ADVIS_FAULT_LOD", word6.ADVIS_FAULT_LOD());
            writeField(out, "ADVIS_CLAMP_LOD", word6.ADVIS_CLAMP_LOD());
            writeField(out, "TYPE", word6.TYPE());
         }
         endGroup(out);
      }
      endGroup(out);

      if (tex.surface.image.size()) {
         startGroup(out, "TextureImage");
         writeField(out, "size", tex.surface.image.size());
         endGroup(out);
      }

      if (tex.surface.mipmap.size()) {
         startGroup(out, "TextureMipmap");
         writeField(out, "size", tex.surface.mipmap.size());
         endGroup(out);
      }
   }

   std::cout << out.writer.data();
   return true;
}

static std::string
getFilename(const std::string &path)
{
   auto startBS = path.find_last_of('\\');
   auto startFS = path.find_last_of('/');
   auto start = startBS;

   if (startBS == std::string::npos) {
      start = startFS;
   }

   if (start == std::string::npos) {
      start = 0;
   } else {
      start = start + 1;
   }

   auto filename = path.substr(start);
   return filename;
}

static std::string
getFileBasename(const std::string &filename)
{
   auto start = filename.find_last_of('.');

   if (start == std::string::npos) {
      return filename;
   } else {
      return filename.substr(0, start);
   }
}

static std::string
getExtension(const std::string &filename)
{
   auto start = filename.find_last_of('.');

   if (start == std::string::npos) {
      return {};
   } else {
      return filename.substr(start);
   }
}

static bool
convertTexture(const std::string &path)
{
   OutputState out;
   gfd::GFDFile file;

   try {
      if (!gfd::readFile(file, path)) {
         return false;
      }
   } catch (gfd::GFDReadException ex) {
      gLog->error("Error reading gfd: {}", ex.what());
      return false;
   }

   auto filename = getFilename(path);
   auto basename = getFileBasename(path);
   auto index = 0u;

   for (auto &tex : file.textures) {
      auto format = static_cast<latte::SQ_DATA_FORMAT>(tex.surface.format & 0x3f);
      auto bpp = latte::getDataFormatBitsPerElement(format);
      auto bytesPerElement = bpp / 8;

      // Fill out tiling surface information
      gpu7::tiling::SurfaceInfo surface;
      surface.bpp = bpp;
      surface.tileMode = static_cast<gpu7::tiling::TileMode>(tex.surface.tileMode);
      surface.width = tex.surface.width;
      surface.height = tex.surface.height;
      surface.depth = tex.surface.depth;
      surface.numSamples = 1;
      surface.isDepth = !!(tex.surface.use & cafe::gx2::GX2SurfaceUse::DepthBuffer);
      surface.is3D = (tex.surface.dim == cafe::gx2::GX2SurfaceDim::Texture3D);

      if (format >= latte::SQ_DATA_FORMAT::FMT_BC1 &&
          format <= latte::SQ_DATA_FORMAT::FMT_BC5) {
         surface.width = (surface.width + 3) / 4;
         surface.height = (surface.height + 3) / 4;
      }

      surface.pipeSwizzle = (tex.surface.swizzle >> 8) & 1;
      surface.bankSwizzle = (tex.surface.swizzle >> 9) & 3;

      std::vector<uint8_t> untiled;
      std::vector<uint8_t> imageData;
      std::vector<uint8_t> mipMapData;
      gpu7::tiling::SurfaceMipMapInfo mipMapInfo;
      gpu7::tiling::SurfaceMipMapInfo unpitchedMipMapInfo;

      // Untile image
      untiled.resize(gpu7::tiling::calculateImageSize(surface));
      gpu7::tiling::untileImage(surface, tex.surface.image.data(),
                                untiled.data());

      // Unpitch image
      imageData.resize(gpu7::tiling::calculateUnpitchedImageSize(surface));
      gpu7::tiling::unpitchImage(surface, untiled.data(), imageData.data());

      // Untile mipmaps
      gpu7::tiling::calculateMipMapInfo(surface,
                                        tex.surface.mipLevels,
                                        mipMapInfo);

      untiled.resize(mipMapInfo.size);
      gpu7::tiling::untileMipMaps(surface, mipMapInfo,
                                  tex.surface.mipmap.data(), untiled.data());

      // Unpitch mipmaps
      gpu7::tiling::calculateUnpitchedMipMapInfo(surface,
                                                 tex.surface.mipLevels,
                                                 unpitchedMipMapInfo);
      mipMapData.resize(unpitchedMipMapInfo.size);
      gpu7::tiling::unpitchMipMaps(surface, mipMapInfo, unpitchedMipMapInfo,
                                   untiled.data(), mipMapData.data());

      // Save to DDS file
      std::string outname;
      if (file.textures.size() > 1) {
         outname = fmt::format("{}.gtx.{}.dds", basename, index++);
      } else {
         outname = fmt::format("{}.gtx.dds", basename);
      }

      cafe::gx2::GX2Surface gx2surface;
      cafe::gx2::internal::gfdToGX2Surface(tex.surface, &gx2surface);
      gx2surface.tileMode = cafe::gx2::GX2TileMode::LinearSpecial;
      gx2surface.pitch = gx2surface.width;
      gx2surface.imageSize = static_cast<uint32_t>(imageData.size());
      gx2surface.mipLevels = tex.surface.mipLevels;
      gx2surface.mipmapSize = static_cast<uint32_t>(mipMapData.size());
      cafe::gx2::debug::saveDDS(outname, &gx2surface, imageData.data(), mipMapData.data());
   }

   return true;
}

int main(int argc, char **argv)
{
   int result = -1;
   excmd::parser parser;
   excmd::option_state options;

   // Setup command line options
   parser.global_options()
      .add_option("h,help", excmd::description { "Show the help." });

   parser.add_command("help")
      .add_argument("command", excmd::value<std::string> { });

   parser.add_command("info")
      .add_argument("file in", excmd::value<std::string> { });

   parser.add_command("convert")
      .add_argument("src", excmd::value<std::string> { });

   // Parse command line
   try {
      options = parser.parse(argc, argv);
   } catch (excmd::exception ex) {
      std::cout << "Error parsing command line: " << ex.what() << std::endl;
      std::exit(-1);
   }

   // Print help
   if (argc == 1 || options.has("help")) {
      if (options.has("command")) {
         std::cout << parser.format_help("gfdtool", options.get<std::string>("command")) << std::endl;
      } else {
         std::cout << parser.format_help("gfdtool") << std::endl;
      }

      std::exit(0);
   }

   if (options.has("info")) {
      auto in = options.get<std::string>("file in");
      result = printInfo(in) ? 0 : -1;
   } else if (options.has("convert")) {
      auto src = options.get<std::string>("src");
      result = convertTexture(src) ? 0 : -1;
   }

   return result;
}
