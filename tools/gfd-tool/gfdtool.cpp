#include <cassert>
#include <iostream>
#include <excmd.h>
#include <gsl.h>
#include <spdlog/spdlog.h>
#include "common/teenyheap.h"
#include "libcpu/mem.h"
#include "gpu/gfd.h"
#include "gpu/microcode/latte_disassembler.h"
#include "gpu/pm4_buffer.h"
#include "gpu/opengl/glsl2_translate.h"
#include "modules/gx2/gx2_addrlib.h"
#include "modules/gx2/gx2_dds.h"
#include "modules/gx2/gx2_texture.h"
#include "modules/gx2/gx2_shaders.h"
#include "modules/gx2/gx2_enum_string.h"

std::shared_ptr<spdlog::logger>
gLog;

TeenyHeap *
gHeap;

struct Texture
{
   gx2::GX2Texture *header;
   gsl::span<uint8_t> imageData;
   gsl::span<uint8_t> mipmapData;
};

struct VertexShader
{
   gx2::GX2VertexShader *header;
   gsl::span<uint8_t> program;
};

struct PixelShader
{
   gx2::GX2PixelShader *header;
   gsl::span<uint8_t> program;
};

struct GeometryShader
{
   gx2::GX2GeometryShader *header;
   gsl::span<uint8_t> program;
};

struct FetchShader
{
   gx2::GX2FetchShader *header;
   gsl::span<uint8_t> program;
};

struct GfdData
{
   std::map<uint32_t, VertexShader> vertexShaders;
   std::map<uint32_t, PixelShader> pixelShaders;
   std::map<uint32_t, GeometryShader> geometryShaders;
   std::map<uint32_t, FetchShader> fetchShaders;
   std::map<uint32_t, Texture> textures;
};

static void
getGfdData(gfd::File &file, GfdData &data)
{
   for (auto &block : file.blocks) {
      switch (block.header.type) {
      case gfd::BlockType::VertexShaderHeader:
         data.vertexShaders[block.header.index].header = reinterpret_cast<gx2::GX2VertexShader *>(block.data.data());
         break;
      case gfd::BlockType::VertexShaderProgram:
         data.vertexShaders[block.header.index].program = block.data;
         break;

      case gfd::BlockType::PixelShaderHeader:
         data.pixelShaders[block.header.index].header = reinterpret_cast<gx2::GX2PixelShader *>(block.data.data());
         break;
      case gfd::BlockType::PixelShaderProgram:
         data.pixelShaders[block.header.index].program = block.data;
         break;

      case gfd::BlockType::GeometryShaderHeader:
         data.geometryShaders[block.header.index].header = reinterpret_cast<gx2::GX2GeometryShader *>(block.data.data());
         break;
      case gfd::BlockType::GeometryShaderProgram:
         data.geometryShaders[block.header.index].program = block.data;
         break;

      case gfd::BlockType::FetchShaderHeader:
         data.fetchShaders[block.header.index].header = reinterpret_cast<gx2::GX2FetchShader *>(block.data.data());
         break;
      case gfd::BlockType::FetchShaderProgram:
         data.fetchShaders[block.header.index].program = block.data;
         break;

      case gfd::BlockType::TextureHeader:
         data.textures[block.header.index].header = reinterpret_cast<gx2::GX2Texture *>(block.data.data());
      break;
      case gfd::BlockType::TextureImage:
         data.textures[block.header.index].imageData = block.data;
         break;
      case gfd::BlockType::TextureMipmap:
         data.textures[block.header.index].mipmapData = block.data;
         break;
      }
   }
}

struct OutputState
{
   fmt::MemoryWriter writer;
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
   out.writer.write("{}{}\n", out.indent, group);
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
   out.writer.write("{}{:<30} = {}\n", out.indent, field, value);
}

static bool
printInfo(const std::string &filename)
{
   gfd::File file;

   if (!file.read(filename)) {
      return false;
   }

   // TODO: Before we can print full shader info we must do pointer fixup in GFD::read
   OutputState out;

   for (auto &block : file.blocks) {
      switch (block.header.type) {
      case gfd::BlockType::VertexShaderHeader:
         startGroup(out, "VertexShaderHeader");
         {
            auto shader = reinterpret_cast<gx2::GX2VertexShader *>(block.data.data());
            writeField(out, "index", block.header.index);
            writeField(out, "size", shader->size);
            writeField(out, "mode", gx2::enumAsString(shader->mode));
            writeField(out, "uniformBlocks", shader->uniformBlockCount);
            writeField(out, "uniformVars", shader->uniformVarCount);
            writeField(out, "initVars", shader->initialValueCount);
            writeField(out, "loopVars", shader->loopVarCount);
            writeField(out, "samplerVars", shader->samplerVarCount);
            writeField(out, "attribVars", shader->attribVarCount);
            writeField(out, "ringItemsize", shader->ringItemsize);
            writeField(out, "hasStreamOut", shader->hasStreamOut);

            startGroup(out, "SQ_PGM_RESOURCES_VS");
            {
               auto sq_pgm_resources_vs = shader->regs.sq_pgm_resources_vs.value();
               writeField(out, "NUM_GPRS", sq_pgm_resources_vs.NUM_GPRS().get());
               writeField(out, "STACK_SIZE", sq_pgm_resources_vs.STACK_SIZE().get());
               writeField(out, "DX10_CLAMP", sq_pgm_resources_vs.DX10_CLAMP().get());
               writeField(out, "PRIME_CACHE_PGM_EN", sq_pgm_resources_vs.PRIME_CACHE_PGM_EN().get());
               writeField(out, "PRIME_CACHE_ON_DRAW", sq_pgm_resources_vs.PRIME_CACHE_ON_DRAW().get());
               writeField(out, "FETCH_CACHE_LINES", sq_pgm_resources_vs.FETCH_CACHE_LINES().get());
               writeField(out, "UNCACHED_FIRST_INST", sq_pgm_resources_vs.UNCACHED_FIRST_INST().get());
               writeField(out, "PRIME_CACHE_ENABLE", sq_pgm_resources_vs.PRIME_CACHE_ENABLE().get());
               writeField(out, "PRIME_CACHE_ON_CONST", sq_pgm_resources_vs.PRIME_CACHE_ON_CONST().get());
            }
            endGroup(out);

            startGroup(out, "VGT_PRIMITIVEID_EN");
            {
               auto vgt_primitiveid_en = shader->regs.vgt_primitiveid_en.value();
               writeField(out, "PRIMITIVEID_EN", vgt_primitiveid_en.PRIMITIVEID_EN().get());
            }
            endGroup(out);

            startGroup(out, "SPI_VS_OUT_CONFIG");
            {
               auto spi_vs_out_config = shader->regs.spi_vs_out_config.value();
               writeField(out, "VS_PER_COMPONENT", spi_vs_out_config.VS_PER_COMPONENT().get());
               writeField(out, "VS_EXPORT_COUNT", spi_vs_out_config.VS_EXPORT_COUNT().get());
               writeField(out, "VS_EXPORTS_FOG", spi_vs_out_config.VS_EXPORTS_FOG().get());
               writeField(out, "VS_OUT_FOG_VEC_ADDR", spi_vs_out_config.VS_OUT_FOG_VEC_ADDR().get());
            }
            endGroup(out);

            auto num_spi_vs_out_id = shader->regs.num_spi_vs_out_id.value();
            writeField(out, "NUM_SPI_VS_OUT_ID", num_spi_vs_out_id);

            auto spi_vs_out_id = shader->regs.spi_vs_out_id.value();

            for (auto i = 0u; i < std::min<size_t>(num_spi_vs_out_id, spi_vs_out_id.size()); ++i) {
               startGroup(out, fmt::format("SPI_VS_OUT_ID[{}]", i));
               {
                  writeField(out, "SEMANTIC_0", spi_vs_out_id[i].SEMANTIC_0().get());
                  writeField(out, "SEMANTIC_1", spi_vs_out_id[i].SEMANTIC_1().get());
                  writeField(out, "SEMANTIC_2", spi_vs_out_id[i].SEMANTIC_2().get());
                  writeField(out, "SEMANTIC_3", spi_vs_out_id[i].SEMANTIC_3().get());
               }
               endGroup(out);
            }

            startGroup(out, "PA_CL_VS_OUT_CNTL");
            {
               auto pa_cl_vs_out_cntl = shader->regs.pa_cl_vs_out_cntl.value();
               writeField(out, "CLIP_DIST_ENA_0", pa_cl_vs_out_cntl.CLIP_DIST_ENA_0().get());
               writeField(out, "CLIP_DIST_ENA_1", pa_cl_vs_out_cntl.CLIP_DIST_ENA_1().get());
               writeField(out, "CLIP_DIST_ENA_2", pa_cl_vs_out_cntl.CLIP_DIST_ENA_2().get());
               writeField(out, "CLIP_DIST_ENA_3", pa_cl_vs_out_cntl.CLIP_DIST_ENA_3().get());
               writeField(out, "CLIP_DIST_ENA_4", pa_cl_vs_out_cntl.CLIP_DIST_ENA_4().get());
               writeField(out, "CLIP_DIST_ENA_5", pa_cl_vs_out_cntl.CLIP_DIST_ENA_5().get());
               writeField(out, "CLIP_DIST_ENA_6", pa_cl_vs_out_cntl.CLIP_DIST_ENA_6().get());
               writeField(out, "CLIP_DIST_ENA_7", pa_cl_vs_out_cntl.CLIP_DIST_ENA_7().get());
               writeField(out, "CULL_DIST_ENA_0", pa_cl_vs_out_cntl.CULL_DIST_ENA_0().get());
               writeField(out, "CULL_DIST_ENA_1", pa_cl_vs_out_cntl.CULL_DIST_ENA_1().get());
               writeField(out, "CULL_DIST_ENA_2", pa_cl_vs_out_cntl.CULL_DIST_ENA_2().get());
               writeField(out, "CULL_DIST_ENA_3", pa_cl_vs_out_cntl.CULL_DIST_ENA_3().get());
               writeField(out, "CULL_DIST_ENA_4", pa_cl_vs_out_cntl.CULL_DIST_ENA_4().get());
               writeField(out, "CULL_DIST_ENA_5", pa_cl_vs_out_cntl.CULL_DIST_ENA_5().get());
               writeField(out, "CULL_DIST_ENA_6", pa_cl_vs_out_cntl.CULL_DIST_ENA_6().get());
               writeField(out, "CULL_DIST_ENA_7", pa_cl_vs_out_cntl.CULL_DIST_ENA_7().get());
               writeField(out, "USE_VTX_POINT_SIZE", pa_cl_vs_out_cntl.USE_VTX_POINT_SIZE().get());
               writeField(out, "USE_VTX_EDGE_FLAG", pa_cl_vs_out_cntl.USE_VTX_EDGE_FLAG().get());
               writeField(out, "USE_VTX_RENDER_TARGET_INDX", pa_cl_vs_out_cntl.USE_VTX_RENDER_TARGET_INDX().get());
               writeField(out, "USE_VTX_VIEWPORT_INDX", pa_cl_vs_out_cntl.USE_VTX_VIEWPORT_INDX().get());
               writeField(out, "USE_VTX_KILL_FLAG", pa_cl_vs_out_cntl.USE_VTX_KILL_FLAG().get());
               writeField(out, "VS_OUT_MISC_VEC_ENA", pa_cl_vs_out_cntl.VS_OUT_MISC_VEC_ENA().get());
               writeField(out, "VS_OUT_CCDIST0_VEC_ENA", pa_cl_vs_out_cntl.VS_OUT_CCDIST0_VEC_ENA().get());
               writeField(out, "VS_OUT_CCDIST1_VEC_ENA", pa_cl_vs_out_cntl.VS_OUT_CCDIST1_VEC_ENA().get());
               writeField(out, "VS_OUT_MISC_SIDE_BUS_ENA", pa_cl_vs_out_cntl.VS_OUT_MISC_SIDE_BUS_ENA().get());
               writeField(out, "USE_VTX_GS_CUT_FLAG", pa_cl_vs_out_cntl.USE_VTX_GS_CUT_FLAG().get());
            }
            endGroup(out);

            startGroup(out, "SQ_VTX_SEMANTIC_CLEAR");
            {
               auto sq_vtx_semantic_clear = shader->regs.sq_vtx_semantic_clear.value();
               writeField(out, "CLEAR", sq_vtx_semantic_clear.CLEAR);
            }
            endGroup(out);

            auto num_sq_vtx_semantic = shader->regs.num_sq_vtx_semantic.value();
            writeField(out, "NUM_SQ_VTX_SEMANTIC", num_sq_vtx_semantic);

            auto sq_vtx_semantic = shader->regs.sq_vtx_semantic.value();
            for (auto i = 0u; i < std::min<size_t>(num_sq_vtx_semantic, sq_vtx_semantic.size()); ++i) {
               startGroup(out, fmt::format("SQ_VTX_SEMANTIC[{}]", i));
               {
                  writeField(out, "SEMANTIC_ID", sq_vtx_semantic[i].SEMANTIC_ID().get());
               }
               endGroup(out);
            }

            startGroup(out, "VGT_STRMOUT_BUFFER_EN");
            {
               auto vgt_strmout_buffer_en = shader->regs.vgt_strmout_buffer_en.value();
               writeField(out, "BUFFER_0_EN", vgt_strmout_buffer_en.BUFFER_0_EN().get());
               writeField(out, "BUFFER_1_EN", vgt_strmout_buffer_en.BUFFER_1_EN().get());
               writeField(out, "BUFFER_2_EN", vgt_strmout_buffer_en.BUFFER_2_EN().get());
               writeField(out, "BUFFER_3_EN", vgt_strmout_buffer_en.BUFFER_3_EN().get());
            }
            endGroup(out);

            startGroup(out, "VGT_VERTEX_REUSE_BLOCK_CNTL");
            {
               auto vgt_vertex_reuse_block_cntl = shader->regs.vgt_vertex_reuse_block_cntl.value();
               writeField(out, "VTX_REUSE_DEPTH", vgt_vertex_reuse_block_cntl.VTX_REUSE_DEPTH().get());
            }
            endGroup(out);

            startGroup(out, "VGT_HOS_REUSE_DEPTH");
            {
               auto vgt_hos_reuse_depth = shader->regs.vgt_hos_reuse_depth.value();
               writeField(out, "REUSE_DEPTH", vgt_hos_reuse_depth.REUSE_DEPTH().get());
            }
            endGroup(out);
         }
         endGroup(out);
         break;
      case gfd::BlockType::PixelShaderHeader:
         startGroup(out, "PixelShaderHeader");
         {
            auto shader = reinterpret_cast<gx2::GX2PixelShader *>(block.data.data());
            writeField(out, "index", block.header.index);
            writeField(out, "size", shader->size);
            writeField(out, "mode", gx2::enumAsString(shader->mode));
            writeField(out, "uniformBlocks", shader->uniformBlockCount);
            writeField(out, "uniformVars", shader->uniformVarCount);
            writeField(out, "initVars", shader->initialValueCount);
            writeField(out, "loopVars", shader->loopVarCount);
            writeField(out, "samplerVars", shader->samplerVarCount);

            startGroup(out, "SQ_PGM_RESOURCES_PS");
            {
               auto sq_pgm_resources_ps = shader->regs.sq_pgm_resources_ps.value();
               writeField(out, "NUM_GPRS", sq_pgm_resources_ps.NUM_GPRS().get());
               writeField(out, "STACK_SIZE", sq_pgm_resources_ps.STACK_SIZE().get());
               writeField(out, "DX10_CLAMP", sq_pgm_resources_ps.DX10_CLAMP().get());
               writeField(out, "PRIME_CACHE_PGM_EN", sq_pgm_resources_ps.PRIME_CACHE_PGM_EN().get());
               writeField(out, "PRIME_CACHE_ON_DRAW", sq_pgm_resources_ps.PRIME_CACHE_ON_DRAW().get());
               writeField(out, "FETCH_CACHE_LINES", sq_pgm_resources_ps.FETCH_CACHE_LINES().get());
               writeField(out, "UNCACHED_FIRST_INST", sq_pgm_resources_ps.UNCACHED_FIRST_INST().get());
               writeField(out, "PRIME_CACHE_ENABLE", sq_pgm_resources_ps.PRIME_CACHE_ENABLE().get());
               writeField(out, "PRIME_CACHE_ON_CONST", sq_pgm_resources_ps.PRIME_CACHE_ON_CONST().get());
               writeField(out, "CLAMP_CONSTS", sq_pgm_resources_ps.CLAMP_CONSTS().get());
            }
            endGroup(out);

            startGroup(out, "SQ_PGM_EXPORTS_PS");
            {
               auto sq_pgm_exports_ps = shader->regs.sq_pgm_exports_ps.value();
               writeField(out, "EXPORT_MODE", sq_pgm_exports_ps.EXPORT_MODE().get());
            }
            endGroup(out);

            startGroup(out, "SPI_PS_IN_CONTROL_0");
            {
               auto spi_ps_in_control_0 = shader->regs.spi_ps_in_control_0.value();
               writeField(out, "NUM_INTERP", spi_ps_in_control_0.NUM_INTERP().get());
               writeField(out, "POSITION_ENA", spi_ps_in_control_0.POSITION_ENA().get());
               writeField(out, "POSITION_CENTROID", spi_ps_in_control_0.POSITION_CENTROID().get());
               writeField(out, "POSITION_ADDR", spi_ps_in_control_0.POSITION_ADDR().get());
               writeField(out, "PARAM_GEN", spi_ps_in_control_0.PARAM_GEN().get());
               writeField(out, "PARAM_GEN_ADDR", spi_ps_in_control_0.PARAM_GEN_ADDR().get());
               writeField(out, "BARYC_SAMPLE_CNTL", spi_ps_in_control_0.BARYC_SAMPLE_CNTL().get());
               writeField(out, "PERSP_GRADIENT_ENA", spi_ps_in_control_0.PERSP_GRADIENT_ENA().get());
               writeField(out, "LINEAR_GRADIENT_ENA", spi_ps_in_control_0.LINEAR_GRADIENT_ENA().get());
               writeField(out, "POSITION_SAMPLE", spi_ps_in_control_0.POSITION_SAMPLE().get());
               writeField(out, "BARYC_AT_SAMPLE_ENA", spi_ps_in_control_0.BARYC_AT_SAMPLE_ENA().get());
            }
            endGroup(out);

            startGroup(out, "SPI_PS_IN_CONTROL_1");
            {
               auto spi_ps_in_control_1 = shader->regs.spi_ps_in_control_1.value();
               writeField(out, "GEN_INDEX_PIX", spi_ps_in_control_1.GEN_INDEX_PIX().get());
               writeField(out, "GEN_INDEX_PIX_ADDR", spi_ps_in_control_1.GEN_INDEX_PIX_ADDR().get());
               writeField(out, "FRONT_FACE_ENA", spi_ps_in_control_1.FRONT_FACE_ENA().get());
               writeField(out, "FRONT_FACE_CHAN", spi_ps_in_control_1.FRONT_FACE_CHAN().get());
               writeField(out, "FRONT_FACE_ALL_BITS", spi_ps_in_control_1.FRONT_FACE_ALL_BITS().get());
               writeField(out, "FRONT_FACE_ADDR", spi_ps_in_control_1.FRONT_FACE_ADDR().get());
               writeField(out, "FOG_ADDR", spi_ps_in_control_1.FOG_ADDR().get());
               writeField(out, "FIXED_PT_POSITION_ENA", spi_ps_in_control_1.FIXED_PT_POSITION_ENA().get());
               writeField(out, "FIXED_PT_POSITION_ADDR", spi_ps_in_control_1.FIXED_PT_POSITION_ADDR().get());
               writeField(out, "POSITION_ULC", spi_ps_in_control_1.POSITION_ULC().get());
            }
            endGroup(out);

            auto num_spi_ps_input_cntl = shader->regs.num_spi_ps_input_cntl.value();
            auto spi_ps_input_cntls = shader->regs.spi_ps_input_cntls.value();
            writeField(out, "NUM_SPI_PS_INPUT_CNTL", num_spi_ps_input_cntl);

            for (auto i = 0u; i < std::min<size_t>(num_spi_ps_input_cntl, spi_ps_input_cntls.size()); ++i) {
               writeField(out, "SEMANTIC", spi_ps_input_cntls[i].SEMANTIC().get());
               writeField(out, "DEFAULT_VAL", spi_ps_input_cntls[i].DEFAULT_VAL().get());
               writeField(out, "FLAT_SHADE", spi_ps_input_cntls[i].FLAT_SHADE().get());
               writeField(out, "SEL_CENTROID", spi_ps_input_cntls[i].SEL_CENTROID().get());
               writeField(out, "SEL_LINEAR", spi_ps_input_cntls[i].SEL_LINEAR().get());
               writeField(out, "CYL_WRAP", spi_ps_input_cntls[i].CYL_WRAP().get());
               writeField(out, "PT_SPRITE_TEX", spi_ps_input_cntls[i].PT_SPRITE_TEX().get());
               writeField(out, "SEL_SAMPLE", spi_ps_input_cntls[i].SEL_SAMPLE().get());
            }

            startGroup(out, "CB_SHADER_MASK");
            {
               auto cb_shader_mask = shader->regs.cb_shader_mask.value();
               writeField(out, "OUTPUT0_ENABLE", cb_shader_mask.OUTPUT0_ENABLE().get());
               writeField(out, "OUTPUT1_ENABLE", cb_shader_mask.OUTPUT1_ENABLE().get());
               writeField(out, "OUTPUT2_ENABLE", cb_shader_mask.OUTPUT2_ENABLE().get());
               writeField(out, "OUTPUT3_ENABLE", cb_shader_mask.OUTPUT3_ENABLE().get());
               writeField(out, "OUTPUT4_ENABLE", cb_shader_mask.OUTPUT4_ENABLE().get());
               writeField(out, "OUTPUT5_ENABLE", cb_shader_mask.OUTPUT5_ENABLE().get());
               writeField(out, "OUTPUT6_ENABLE", cb_shader_mask.OUTPUT6_ENABLE().get());
               writeField(out, "OUTPUT7_ENABLE", cb_shader_mask.OUTPUT7_ENABLE().get());
            }
            endGroup(out);

            startGroup(out, "CB_SHADER_CONTROL");
            {
               auto cb_shader_control = shader->regs.cb_shader_control.value();
               writeField(out, "RT0_ENABLE", cb_shader_control.RT0_ENABLE().get());
               writeField(out, "RT1_ENABLE", cb_shader_control.RT1_ENABLE().get());
               writeField(out, "RT2_ENABLE", cb_shader_control.RT2_ENABLE().get());
               writeField(out, "RT3_ENABLE", cb_shader_control.RT3_ENABLE().get());
               writeField(out, "RT4_ENABLE", cb_shader_control.RT4_ENABLE().get());
               writeField(out, "RT5_ENABLE", cb_shader_control.RT5_ENABLE().get());
               writeField(out, "RT6_ENABLE", cb_shader_control.RT6_ENABLE().get());
               writeField(out, "RT7_ENABLE", cb_shader_control.RT7_ENABLE().get());
            }
            endGroup(out);

            startGroup(out, "DB_SHADER_CONTROL");
            {
               auto db_shader_control = shader->regs.db_shader_control.value();
               writeField(out, "Z_EXPORT_ENABLE", db_shader_control.Z_EXPORT_ENABLE().get());
               writeField(out, "STENCIL_REF_EXPORT_ENABLE", db_shader_control.STENCIL_REF_EXPORT_ENABLE().get());
               writeField(out, "Z_ORDER", db_shader_control.Z_ORDER().get());
               writeField(out, "KILL_ENABLE", db_shader_control.KILL_ENABLE().get());
               writeField(out, "COVERAGE_TO_MASK_ENABLE", db_shader_control.COVERAGE_TO_MASK_ENABLE().get());
               writeField(out, "MASK_EXPORT_ENABLE", db_shader_control.MASK_EXPORT_ENABLE().get());
               writeField(out, "DUAL_EXPORT_ENABLE", db_shader_control.DUAL_EXPORT_ENABLE().get());
               writeField(out, "EXEC_ON_HIER_FAIL", db_shader_control.EXEC_ON_HIER_FAIL().get());
               writeField(out, "EXEC_ON_NOOP", db_shader_control.EXEC_ON_NOOP().get());
               writeField(out, "ALPHA_TO_MASK_DISABLE", db_shader_control.ALPHA_TO_MASK_DISABLE().get());
            }
            endGroup(out);

            startGroup(out, "SPI_INPUT_Z");
            {
               auto spi_input_z = shader->regs.spi_input_z.value();
               writeField(out, "PROVIDE_Z_TO_SPI", spi_input_z.PROVIDE_Z_TO_SPI().get());
            }
            endGroup(out);
         }
         endGroup(out);
         break;
      case gfd::BlockType::GeometryShaderHeader:
         startGroup(out, "GeometryShaderHeader");
         {
            auto shader = reinterpret_cast<gx2::GX2GeometryShader *>(block.data.data());
            writeField(out, "index", block.header.index);
            writeField(out, "size", shader->size);
            writeField(out, "vshSize", shader->vertexShaderSize);
            writeField(out, "mode", gx2::enumAsString(shader->mode));
            writeField(out, "uniformBlocks", shader->uniformBlockCount);
            writeField(out, "uniformVars", shader->uniformVarCount);
            writeField(out, "initVars", shader->initialValueCount);
            writeField(out, "loopVars", shader->loopVarCount);
            writeField(out, "samplerVars", shader->samplerVarCount);
            writeField(out, "ringItemSize", shader->ringItemSize);
            writeField(out, "hasStreamOut", shader->hasStreamOut);

            for (auto i = 0u; i < shader->streamOutStride.size(); ++i) {
               writeField(out, fmt::format("streamOutStride[{}]", i), shader->streamOutStride[i]);
            }

            startGroup(out, "SQ_PGM_RESOURCES_GS");
            {
               auto sq_pgm_resources_gs = shader->regs.sq_pgm_resources_gs.value();
               writeField(out, "NUM_GPRS", sq_pgm_resources_gs.NUM_GPRS().get());
               writeField(out, "STACK_SIZE", sq_pgm_resources_gs.STACK_SIZE().get());
               writeField(out, "DX10_CLAMP", sq_pgm_resources_gs.DX10_CLAMP().get());
               writeField(out, "PRIME_CACHE_PGM_EN", sq_pgm_resources_gs.PRIME_CACHE_PGM_EN().get());
               writeField(out, "PRIME_CACHE_ON_DRAW", sq_pgm_resources_gs.PRIME_CACHE_ON_DRAW().get());
               writeField(out, "FETCH_CACHE_LINES", sq_pgm_resources_gs.FETCH_CACHE_LINES().get());
               writeField(out, "UNCACHED_FIRST_INST", sq_pgm_resources_gs.UNCACHED_FIRST_INST().get());
               writeField(out, "PRIME_CACHE_ENABLE", sq_pgm_resources_gs.PRIME_CACHE_ENABLE().get());
               writeField(out, "PRIME_CACHE_ON_CONST", sq_pgm_resources_gs.PRIME_CACHE_ON_CONST().get());
            }
            endGroup(out);

            startGroup(out, "VGT_GS_OUT_PRIM_TYPE");
            {
               auto vgt_gs_out_prim_type = shader->regs.vgt_gs_out_prim_type.value();
               writeField(out, "PRIM_TYPE", vgt_gs_out_prim_type.PRIM_TYPE().get());
            }
            endGroup(out);

            startGroup(out, "VGT_GS_MODE");
            {
               auto vgt_gs_mode = shader->regs.vgt_gs_mode.value();
               writeField(out, "MODE", vgt_gs_mode.MODE().get());
               writeField(out, "ES_PASSTHRU", vgt_gs_mode.ES_PASSTHRU().get());
               writeField(out, "CUT_MODE", vgt_gs_mode.CUT_MODE().get());
               writeField(out, "MODE_HI", vgt_gs_mode.MODE_HI().get());
               writeField(out, "GS_C_PACK_EN", vgt_gs_mode.GS_C_PACK_EN().get());
               writeField(out, "COMPUTE_MODE", vgt_gs_mode.COMPUTE_MODE().get());
               writeField(out, "FAST_COMPUTE_MODE", vgt_gs_mode.FAST_COMPUTE_MODE().get());
               writeField(out, "ELEMENT_INFO_EN", vgt_gs_mode.ELEMENT_INFO_EN().get());
               writeField(out, "PARTIAL_THD_AT_EOI", vgt_gs_mode.PARTIAL_THD_AT_EOI().get());
            }
            endGroup(out);

            startGroup(out, "PA_CL_VS_OUT_CNTL");
            {
               auto pa_cl_vs_out_cntl = shader->regs.pa_cl_vs_out_cntl.value();
               writeField(out, "CLIP_DIST_ENA_0", pa_cl_vs_out_cntl.CLIP_DIST_ENA_0().get());
               writeField(out, "CLIP_DIST_ENA_1", pa_cl_vs_out_cntl.CLIP_DIST_ENA_1().get());
               writeField(out, "CLIP_DIST_ENA_2", pa_cl_vs_out_cntl.CLIP_DIST_ENA_2().get());
               writeField(out, "CLIP_DIST_ENA_3", pa_cl_vs_out_cntl.CLIP_DIST_ENA_3().get());
               writeField(out, "CLIP_DIST_ENA_4", pa_cl_vs_out_cntl.CLIP_DIST_ENA_4().get());
               writeField(out, "CLIP_DIST_ENA_5", pa_cl_vs_out_cntl.CLIP_DIST_ENA_5().get());
               writeField(out, "CLIP_DIST_ENA_6", pa_cl_vs_out_cntl.CLIP_DIST_ENA_6().get());
               writeField(out, "CLIP_DIST_ENA_7", pa_cl_vs_out_cntl.CLIP_DIST_ENA_7().get());
               writeField(out, "CULL_DIST_ENA_0", pa_cl_vs_out_cntl.CULL_DIST_ENA_0().get());
               writeField(out, "CULL_DIST_ENA_1", pa_cl_vs_out_cntl.CULL_DIST_ENA_1().get());
               writeField(out, "CULL_DIST_ENA_2", pa_cl_vs_out_cntl.CULL_DIST_ENA_2().get());
               writeField(out, "CULL_DIST_ENA_3", pa_cl_vs_out_cntl.CULL_DIST_ENA_3().get());
               writeField(out, "CULL_DIST_ENA_4", pa_cl_vs_out_cntl.CULL_DIST_ENA_4().get());
               writeField(out, "CULL_DIST_ENA_5", pa_cl_vs_out_cntl.CULL_DIST_ENA_5().get());
               writeField(out, "CULL_DIST_ENA_6", pa_cl_vs_out_cntl.CULL_DIST_ENA_6().get());
               writeField(out, "CULL_DIST_ENA_7", pa_cl_vs_out_cntl.CULL_DIST_ENA_7().get());
               writeField(out, "USE_VTX_POINT_SIZE", pa_cl_vs_out_cntl.USE_VTX_POINT_SIZE().get());
               writeField(out, "USE_VTX_EDGE_FLAG", pa_cl_vs_out_cntl.USE_VTX_EDGE_FLAG().get());
               writeField(out, "USE_VTX_RENDER_TARGET_INDX", pa_cl_vs_out_cntl.USE_VTX_RENDER_TARGET_INDX().get());
               writeField(out, "USE_VTX_VIEWPORT_INDX", pa_cl_vs_out_cntl.USE_VTX_VIEWPORT_INDX().get());
               writeField(out, "USE_VTX_KILL_FLAG", pa_cl_vs_out_cntl.USE_VTX_KILL_FLAG().get());
               writeField(out, "VS_OUT_MISC_VEC_ENA", pa_cl_vs_out_cntl.VS_OUT_MISC_VEC_ENA().get());
               writeField(out, "VS_OUT_CCDIST0_VEC_ENA", pa_cl_vs_out_cntl.VS_OUT_CCDIST0_VEC_ENA().get());
               writeField(out, "VS_OUT_CCDIST1_VEC_ENA", pa_cl_vs_out_cntl.VS_OUT_CCDIST1_VEC_ENA().get());
               writeField(out, "VS_OUT_MISC_SIDE_BUS_ENA", pa_cl_vs_out_cntl.VS_OUT_MISC_SIDE_BUS_ENA().get());
               writeField(out, "USE_VTX_GS_CUT_FLAG", pa_cl_vs_out_cntl.USE_VTX_GS_CUT_FLAG().get());
            }
            endGroup(out);

            auto num_spi_vs_out_id = shader->regs.num_spi_vs_out_id.value();
            writeField(out, "NUM_SPI_VS_OUT_ID", num_spi_vs_out_id);

            auto spi_vs_out_id = shader->regs.spi_vs_out_id.value();

            for (auto i = 0u; i < std::min<size_t>(num_spi_vs_out_id, spi_vs_out_id.size()); ++i) {
               startGroup(out, fmt::format("SPI_VS_OUT_ID[{}]", i));
               {
                  writeField(out, "SEMANTIC_0", spi_vs_out_id[i].SEMANTIC_0().get());
                  writeField(out, "SEMANTIC_1", spi_vs_out_id[i].SEMANTIC_1().get());
                  writeField(out, "SEMANTIC_2", spi_vs_out_id[i].SEMANTIC_2().get());
                  writeField(out, "SEMANTIC_3", spi_vs_out_id[i].SEMANTIC_3().get());
               }
               endGroup(out);
            }

            startGroup(out, "SQ_PGM_RESOURCES_VS");
            {
               auto sq_pgm_resources_vs = shader->regs.sq_pgm_resources_vs.value();
               writeField(out, "NUM_GPRS", sq_pgm_resources_vs.NUM_GPRS().get());
               writeField(out, "STACK_SIZE", sq_pgm_resources_vs.STACK_SIZE().get());
               writeField(out, "DX10_CLAMP", sq_pgm_resources_vs.DX10_CLAMP().get());
               writeField(out, "PRIME_CACHE_PGM_EN", sq_pgm_resources_vs.PRIME_CACHE_PGM_EN().get());
               writeField(out, "PRIME_CACHE_ON_DRAW", sq_pgm_resources_vs.PRIME_CACHE_ON_DRAW().get());
               writeField(out, "FETCH_CACHE_LINES", sq_pgm_resources_vs.FETCH_CACHE_LINES().get());
               writeField(out, "UNCACHED_FIRST_INST", sq_pgm_resources_vs.UNCACHED_FIRST_INST().get());
               writeField(out, "PRIME_CACHE_ENABLE", sq_pgm_resources_vs.PRIME_CACHE_ENABLE().get());
               writeField(out, "PRIME_CACHE_ON_CONST", sq_pgm_resources_vs.PRIME_CACHE_ON_CONST().get());
            }
            endGroup(out);

            startGroup(out, "SQ_GS_VERT_ITEMSIZE");
            {
               auto sq_gs_vert_itemsize = shader->regs.sq_gs_vert_itemsize.value();
               writeField(out, "ITEMSIZE", sq_gs_vert_itemsize.ITEMSIZE().get());
            }
            endGroup(out);

            startGroup(out, "SPI_VS_OUT_CONFIG");
            {
               auto spi_vs_out_config = shader->regs.spi_vs_out_config.value();
               writeField(out, "VS_PER_COMPONENT", spi_vs_out_config.VS_PER_COMPONENT().get());
               writeField(out, "VS_EXPORT_COUNT", spi_vs_out_config.VS_EXPORT_COUNT().get());
               writeField(out, "VS_EXPORTS_FOG", spi_vs_out_config.VS_EXPORTS_FOG().get());
               writeField(out, "VS_OUT_FOG_VEC_ADDR", spi_vs_out_config.VS_OUT_FOG_VEC_ADDR().get());
            }
            endGroup(out);

            startGroup(out, "VGT_STRMOUT_BUFFER_EN");
            {
               auto vgt_strmout_buffer_en = shader->regs.vgt_strmout_buffer_en.value();
               writeField(out, "BUFFER_0_EN", vgt_strmout_buffer_en.BUFFER_0_EN().get());
               writeField(out, "BUFFER_1_EN", vgt_strmout_buffer_en.BUFFER_1_EN().get());
               writeField(out, "BUFFER_2_EN", vgt_strmout_buffer_en.BUFFER_2_EN().get());
               writeField(out, "BUFFER_3_EN", vgt_strmout_buffer_en.BUFFER_3_EN().get());
            }
            endGroup(out);
         }
         endGroup(out);
         break;
      case gfd::BlockType::VertexShaderProgram:
         startGroup(out, "VertexShaderProgram");
         {
            writeField(out, "index", block.header.index);
            writeField(out, "size", block.data.size());

            std::string disassembly;
            disassembly = latte::disassemble(block.data);
            out.writer << '\n' << disassembly;

            glsl2::Shader shader;
            shader.type = glsl2::Shader::VertexShader;
            glsl2::translate(shader, block.data);
            out.writer << '\n' << shader.codeBody;
         }
         endGroup(out);
         break;
      case gfd::BlockType::PixelShaderProgram:
         startGroup(out, "PixelShaderProgram");
         {
            writeField(out, "index", block.header.index);
            writeField(out, "size", block.data.size());

            std::string disassembly;
            disassembly = latte::disassemble(block.data);
            out.writer << '\n' << disassembly;

            glsl2::Shader shader;
            shader.type = glsl2::Shader::PixelShader;
            glsl2::translate(shader, block.data);
            out.writer << '\n' << shader.codeBody;
         }
         endGroup(out);
         break;
      case gfd::BlockType::GeometryShaderProgram:
         startGroup(out, "GeometryShaderProgram");
         {
            writeField(out, "index", block.header.index);
            writeField(out, "size", block.data.size());

            std::string disassembly;
            disassembly = latte::disassemble(block.data);
            out.writer << '\n' << disassembly;

            glsl2::Shader shader;
            shader.type = glsl2::Shader::GeometryShader;
            glsl2::translate(shader, block.data);
            out.writer << '\n' << shader.codeBody;
         }
         endGroup(out);
         break;
      case gfd::BlockType::TextureHeader:
      {
         assert(block.data.size() >= sizeof(gx2::GX2Texture));

         startGroup(out, "TextureHeader");
         {
            auto tex = reinterpret_cast<gx2::GX2Texture *>(block.data.data());
            writeField(out, "index", block.header.index);
            writeField(out, "dim", gx2::enumAsString(tex->surface.dim));
            writeField(out, "width", tex->surface.width);
            writeField(out, "height", tex->surface.height);
            writeField(out, "depth", tex->surface.depth);
            writeField(out, "mipLevels", tex->surface.mipLevels);
            writeField(out, "format", gx2::enumAsString(tex->surface.format));
            writeField(out, "aa", gx2::enumAsString(tex->surface.aa));
            writeField(out, "use", gx2::enumAsString(tex->surface.use));
            writeField(out, "imageSize", tex->surface.imageSize);
            writeField(out, "mipmapSize", tex->surface.mipmapSize);
            writeField(out, "tileMode", gx2::enumAsString(tex->surface.tileMode));
            writeField(out, "swizzle", tex->surface.swizzle);
            writeField(out, "alignment", tex->surface.alignment);
            writeField(out, "pitch", tex->surface.pitch);

            for (auto i = 0u; i < tex->surface.mipLevelOffset.size(); ++i) {
               writeField(out, fmt::format("mipLevelOffset[{}]", i), tex->surface.mipLevelOffset[i]);
            }

            writeField(out, "viewFirstMip", tex->viewFirstMip);
            writeField(out, "viewNumMips", tex->viewNumMips);
            writeField(out, "viewFirstSlice", tex->viewFirstSlice);
            writeField(out, "viewNumSlices", tex->viewNumSlices);
            writeField(out, "compMap", tex->compMap);

            startGroup(out, "SQ_TEX_RESOURCE_WORD0_0");
            {
               auto word0 = tex->regs.word0.value();
               writeField(out, "DIM", word0.DIM().get());
               writeField(out, "TILE_MODE", word0.TILE_MODE().get());
               writeField(out, "TILE_TYPE", word0.TILE_TYPE().get());
               writeField(out, "PITCH", word0.PITCH().get());
               writeField(out, "TEX_WIDTH", word0.TEX_WIDTH().get());
            }
            endGroup(out);

            startGroup(out, "SQ_TEX_RESOURCE_WORD0_1");
            {
               auto word1 = tex->regs.word1.value();
               writeField(out, "TEX_HEIGHT", word1.TEX_HEIGHT().get());
               writeField(out, "TEX_DEPTH", word1.TEX_DEPTH().get());
               writeField(out, "DATA_FORMAT", word1.DATA_FORMAT().get());
            }
            endGroup(out);

            startGroup(out, "SQ_TEX_RESOURCE_WORD0_4");
            {
               auto word4 = tex->regs.word4.value();
               writeField(out, "FORMAT_COMP_X", word4.FORMAT_COMP_X().get());
               writeField(out, "FORMAT_COMP_Y", word4.FORMAT_COMP_Y().get());
               writeField(out, "FORMAT_COMP_Z", word4.FORMAT_COMP_Z().get());
               writeField(out, "FORMAT_COMP_W", word4.FORMAT_COMP_W().get());
               writeField(out, "NUM_FORMAT_ALL", word4.NUM_FORMAT_ALL().get());
               writeField(out, "SRF_MODE_ALL", word4.SRF_MODE_ALL().get());
               writeField(out, "FORCE_DEGAMMA", word4.FORCE_DEGAMMA().get());
               writeField(out, "ENDIAN_SWAP", word4.ENDIAN_SWAP().get());
               writeField(out, "REQUEST_SIZE", word4.REQUEST_SIZE().get());
               writeField(out, "DST_SEL_X", word4.DST_SEL_X().get());
               writeField(out, "DST_SEL_Y", word4.DST_SEL_Y().get());
               writeField(out, "DST_SEL_Z", word4.DST_SEL_Z().get());
               writeField(out, "DST_SEL_W", word4.DST_SEL_W().get());
               writeField(out, "BASE_LEVEL", word4.BASE_LEVEL().get());
            }
            endGroup(out);

            startGroup(out, "SQ_TEX_RESOURCE_WORD0_5");
            {
               auto word5 = tex->regs.word5.value();
               writeField(out, "LAST_LEVEL", word5.LAST_LEVEL().get());
               writeField(out, "BASE_ARRAY", word5.BASE_ARRAY().get());
               writeField(out, "LAST_ARRAY", word5.LAST_ARRAY().get());
               writeField(out, "YUV_CONV", word5.YUV_CONV().get());
            }
            endGroup(out);

            startGroup(out, "SQ_TEX_RESOURCE_WORD0_6");
            {
               auto word6 = tex->regs.word6.value();
               writeField(out, "MPEG_CLAMP", word6.MPEG_CLAMP().get());
               writeField(out, "MAX_ANISO_RATIO", word6.MAX_ANISO_RATIO().get());
               writeField(out, "PERF_MODULATION", word6.PERF_MODULATION().get());
               writeField(out, "INTERLACED", word6.INTERLACED().get());
               writeField(out, "ADVIS_FAULT_LOD", word6.ADVIS_FAULT_LOD().get());
               writeField(out, "ADVIS_CLAMP_LOD", word6.ADVIS_CLAMP_LOD().get());
               writeField(out, "TYPE", word6.TYPE().get());
            }
            endGroup(out);
         }
         endGroup(out);
         break;
      }
      case gfd::BlockType::TextureImage:
         startGroup(out, "TextureImage");
         writeField(out, "index", block.header.index);
         writeField(out, "size", block.data.size());
         endGroup(out);
         break;
      case gfd::BlockType::TextureMipmap:
         startGroup(out, "TextureMipmap");
         writeField(out, "index", block.header.index);
         writeField(out, "size", block.data.size());
         endGroup(out);
         break;
      case gfd::BlockType::EndOfFile:
         startGroup(out, "EndOfFile");
         endGroup(out);
         break;
      }

      out.writer << "\n\n";
   }

   std::cout << out.writer.c_str();
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
   gfd::File file;
   GfdData data;
   std::vector<Texture> textures;

   if (!file.read(path)) {
      return false;
   }

   auto filename = getFilename(path);
   auto basename = getFileBasename(path);

   getGfdData(file, data);

   for (auto &pair : data.textures) {
      std::vector<uint8_t> untiledImage, untiledMipmap;
      auto index = pair.first;
      auto &tex = pair.second;

      // Copy surface data to virtual memory
      auto image = reinterpret_cast<uint8_t*>(gHeap->alloc(tex.imageData.size()));
      auto mipmap = reinterpret_cast<uint8_t*>(gHeap->alloc(tex.mipmapData.size()));

      memcpy(image, tex.imageData.data(), tex.imageData.size());
      memcpy(mipmap, tex.mipmapData.data(), tex.mipmapData.size());

      tex.header->surface.image = make_virtual_ptr<uint8_t>(image);
      tex.header->surface.mipmaps = make_virtual_ptr(mipmap);

      // Untile
      gx2::internal::convertTiling(&tex.header->surface, untiledImage, untiledMipmap);

      // Output DDS file
      std::string outname;

      if (data.textures.size()) {
         outname = fmt::format("{}.gtx.{}.dds", basename, index);
      } else {
         outname = fmt::format("{}.gtx.dds", basename);
      }

      tex.header->surface.imageSize = untiledImage.size();
      tex.header->surface.mipmapSize = untiledMipmap.size();
      gx2::debug::saveDDS(outname, &tex.header->surface, untiledImage.data(), untiledMipmap.data());

      gHeap->free(image);
      gHeap->free(mipmap);
   }

   return true;
}

int main(int argc, char **argv)
{
   int result = -1;
   excmd::parser parser;
   excmd::option_state options;

   mem::initialise();
   gHeap = new TeenyHeap(mem::translate(mem::SystemBase), mem::SystemSize);

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

namespace gpu
{
namespace opengl
{
unsigned
MaxUniformBlockSize = 0;
}
}

namespace pm4
{
Buffer *
getBuffer(uint32_t size)
{
   return nullptr;
}

Buffer *
flushBuffer(Buffer *buffer)
{
   return nullptr;
}
} // namespace pm4
