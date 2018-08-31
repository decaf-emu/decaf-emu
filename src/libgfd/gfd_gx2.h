#pragma once
#include <array>
#include <cstdint>
#include <libdecaf/src/cafe/libraries/gx2/gx2_enum.h>
#include <libgpu/latte/latte_registers.h>
#include <string>
#include <vector>

namespace gfd
{

using cafe::gx2::GX2AAMode;
using cafe::gx2::GX2FetchShaderType;
using cafe::gx2::GX2RResourceFlags;
using cafe::gx2::GX2RResourceFlags;
using cafe::gx2::GX2SamplerVarType;
using cafe::gx2::GX2ShaderMode;
using cafe::gx2::GX2ShaderVarType;
using cafe::gx2::GX2SurfaceDim;
using cafe::gx2::GX2SurfaceFormat;
using cafe::gx2::GX2SurfaceUse;
using cafe::gx2::GX2TileMode;

struct GFDSurface
{
   GX2SurfaceDim dim;
   uint32_t width;
   uint32_t height;
   uint32_t depth;
   uint32_t mipLevels;
   GX2SurfaceFormat format;
   GX2AAMode aa;

   union
   {
      GX2SurfaceUse use;
      GX2RResourceFlags resourceFlags;
   };

   std::vector<uint8_t> image;
   std::vector<uint8_t> mipmap;
   GX2TileMode tileMode;
   uint32_t swizzle;
   uint32_t alignment;
   uint32_t pitch;
   std::array<uint32_t, 13> mipLevelOffset;
};

struct GFDTexture
{
   GFDSurface surface;
   uint32_t viewFirstMip;
   uint32_t viewNumMips;
   uint32_t viewFirstSlice;
   uint32_t viewNumSlices;
   uint32_t compMap;

   struct
   {
      latte::SQ_TEX_RESOURCE_WORD0_N word0;
      latte::SQ_TEX_RESOURCE_WORD1_N word1;
      latte::SQ_TEX_RESOURCE_WORD4_N word4;
      latte::SQ_TEX_RESOURCE_WORD5_N word5;
      latte::SQ_TEX_RESOURCE_WORD6_N word6;
   } regs;
};

struct GFDFetchShader
{
   GX2FetchShaderType type;

   struct
   {
      latte::SQ_PGM_RESOURCES_FS sq_pgm_resources_fs;
   } regs;

   uint32_t size;
   uint8_t *data;
   uint32_t attribCount;
   uint32_t numDivisors;
   uint32_t divisors[2];
};

struct GFDUniformVar
{
   std::string name;
   GX2ShaderVarType type;
   uint32_t count;
   uint32_t offset;
   int32_t block;
};

struct GFDUniformInitialValue
{
   std::array<float, 4> value;
   uint32_t offset;
};

struct GFDUniformBlock
{
   std::string name;
   uint32_t offset;
   uint32_t size;
};

struct GFDAttribVar
{
   std::string name;
   GX2ShaderVarType type;
   uint32_t count;
   uint32_t location;
};

struct GFDSamplerVar
{
   std::string name;
   GX2SamplerVarType type;
   uint32_t location;
};

struct GFDLoopVar
{
   uint32_t offset;
   uint32_t value;
};

struct GFDRBuffer
{
   GX2RResourceFlags flags;
   uint32_t elemSize;
   uint32_t elemCount;
   std::vector<uint8_t> buffer;
};

struct GFDVertexShader
{
   struct
   {
      latte::SQ_PGM_RESOURCES_VS sq_pgm_resources_vs;
      latte::VGT_PRIMITIVEID_EN vgt_primitiveid_en;
      latte::SPI_VS_OUT_CONFIG spi_vs_out_config;
      uint32_t num_spi_vs_out_id;
      std::array<latte::SPI_VS_OUT_ID_N, 10> spi_vs_out_id;
      latte::PA_CL_VS_OUT_CNTL pa_cl_vs_out_cntl;
      latte::SQ_VTX_SEMANTIC_CLEAR sq_vtx_semantic_clear;
      uint32_t num_sq_vtx_semantic;
      std::array<latte::SQ_VTX_SEMANTIC_N, 32> sq_vtx_semantic;
      latte::VGT_STRMOUT_BUFFER_EN vgt_strmout_buffer_en;
      latte::VGT_VERTEX_REUSE_BLOCK_CNTL vgt_vertex_reuse_block_cntl;
      latte::VGT_HOS_REUSE_DEPTH vgt_hos_reuse_depth;
   } regs;

   std::vector<uint8_t> data;
   GX2ShaderMode mode;
   std::vector<GFDUniformBlock> uniformBlocks;
   std::vector<GFDUniformVar> uniformVars;
   std::vector<GFDUniformInitialValue> initialValues;
   std::vector<GFDLoopVar> loopVars;
   std::vector<GFDSamplerVar> samplerVars;
   std::vector<GFDAttribVar> attribVars;
   uint32_t ringItemSize;
   bool hasStreamOut;
   std::array<uint32_t, 4> streamOutStride;
   GFDRBuffer gx2rData;
};

struct GFDPixelShader
{
   struct
   {
      latte::SQ_PGM_RESOURCES_PS sq_pgm_resources_ps;
      latte::SQ_PGM_EXPORTS_PS sq_pgm_exports_ps;
      latte::SPI_PS_IN_CONTROL_0 spi_ps_in_control_0;
      latte::SPI_PS_IN_CONTROL_1 spi_ps_in_control_1;
      uint32_t num_spi_ps_input_cntl;
      std::array<latte::SPI_PS_INPUT_CNTL_N, 32> spi_ps_input_cntls;
      latte::CB_SHADER_MASK cb_shader_mask;
      latte::CB_SHADER_CONTROL cb_shader_control;
      latte::DB_SHADER_CONTROL db_shader_control;
      latte::SPI_INPUT_Z spi_input_z;
   } regs;

   std::vector<uint8_t> data;
   GX2ShaderMode mode;
   std::vector<GFDUniformBlock> uniformBlocks;
   std::vector<GFDUniformVar> uniformVars;
   std::vector<GFDUniformInitialValue> initialValues;
   std::vector<GFDLoopVar> loopVars;
   std::vector<GFDSamplerVar> samplerVars;
   GFDRBuffer gx2rData;
};

struct GFDGeometryShader
{
   struct
   {
      latte::SQ_PGM_RESOURCES_GS sq_pgm_resources_gs;
      latte::VGT_GS_OUT_PRIM_TYPE vgt_gs_out_prim_type;
      latte::VGT_GS_MODE vgt_gs_mode;
      latte::PA_CL_VS_OUT_CNTL pa_cl_vs_out_cntl;
      latte::SQ_PGM_RESOURCES_VS sq_pgm_resources_vs;
      latte::SQ_GS_VERT_ITEMSIZE sq_gs_vert_itemsize;
      latte::SPI_VS_OUT_CONFIG spi_vs_out_config;
      uint32_t num_spi_vs_out_id;
      std::array<latte::SPI_VS_OUT_ID_N, 10> spi_vs_out_id;
      latte::VGT_STRMOUT_BUFFER_EN vgt_strmout_buffer_en;
   } regs;

   std::vector<uint8_t> data;
   std::vector<uint8_t> vertexShaderData;
   GX2ShaderMode mode;
   std::vector<GFDUniformBlock> uniformBlocks;
   std::vector<GFDUniformVar> uniformVars;
   std::vector<GFDUniformInitialValue> initialValues;
   std::vector<GFDLoopVar> loopVars;
   std::vector<GFDSamplerVar> samplerVars;
   uint32_t ringItemSize;
   bool hasStreamOut;
   std::array<uint32_t, 4> streamOutStride;
   GFDRBuffer gx2rData;
   GFDRBuffer gx2rVertexShaderData;
};

} // namespace gfd
