#pragma once
#include "types.h"
#include "gpu/latte_registers.h"
#include "modules/gx2/gx2_enum.h"
#include "utils/be_array.h"
#include "utils/be_val.h"
#include "utils/structsize.h"
#include "utils/virtual_ptr.h"
#include "gx2_sampler.h"

struct GX2FetchShader
{
   be_val<GX2FetchShaderType::Value > type;

   struct
   {
      latte::SQ_PGM_RESOURCES_FS sq_pgm_resources_fs;
   } regs;

   be_val<uint32_t> size;
   be_ptr<void> data;
   be_val<uint32_t> attribCount;
   be_val<uint32_t> numDivisors;
   be_val<uint32_t> divisors[2];
};
CHECK_OFFSET(GX2FetchShader, 0x0, type);
CHECK_OFFSET(GX2FetchShader, 0x4, regs.sq_pgm_resources_fs);
CHECK_OFFSET(GX2FetchShader, 0x8, size);
CHECK_OFFSET(GX2FetchShader, 0xc, data);
CHECK_OFFSET(GX2FetchShader, 0x10, attribCount);
CHECK_OFFSET(GX2FetchShader, 0x14, numDivisors);
CHECK_OFFSET(GX2FetchShader, 0x18, divisors);
CHECK_SIZE(GX2FetchShader, 0x20);

struct GX2UniformVar
{
   be_ptr<const char> name;
   be_val<GX2ShaderVarType::Value> type;
   be_val<uint32_t> count;
   be_val<uint32_t> offset;
   be_val<int32_t> block;
};
CHECK_OFFSET(GX2UniformVar, 0x00, name);
CHECK_OFFSET(GX2UniformVar, 0x04, type);
CHECK_OFFSET(GX2UniformVar, 0x08, count);
CHECK_OFFSET(GX2UniformVar, 0x0C, offset);
CHECK_OFFSET(GX2UniformVar, 0x10, block);
CHECK_SIZE(GX2UniformVar, 0x14);

struct GX2UniformInitialValue
{
   be_val<float> value[4];
   be_val<uint32_t> offset;
};
CHECK_OFFSET(GX2UniformInitialValue, 0x00, value);
CHECK_OFFSET(GX2UniformInitialValue, 0x10, offset);
CHECK_SIZE(GX2UniformInitialValue, 0x14);

struct GX2UniformBlock
{
   be_ptr<const char> name;
   be_val<uint32_t> offset;
   be_val<uint32_t> size;
};
CHECK_OFFSET(GX2UniformBlock, 0x00, name);
CHECK_OFFSET(GX2UniformBlock, 0x04, offset);
CHECK_OFFSET(GX2UniformBlock, 0x08, size);
CHECK_SIZE(GX2UniformBlock, 0x0C);

struct GX2AttribVar
{
   be_ptr<const char> name;
   be_val<GX2ShaderVarType::Value> type;
   be_val<uint32_t> count;
   be_val<uint32_t> location;
};
CHECK_OFFSET(GX2AttribVar, 0x00, name);
CHECK_OFFSET(GX2AttribVar, 0x04, type);
CHECK_OFFSET(GX2AttribVar, 0x08, count);
CHECK_OFFSET(GX2AttribVar, 0x0C, location);
CHECK_SIZE(GX2AttribVar, 0x10);

struct GX2SamplerVar;

struct GX2VertexShader
{
   struct
   {
      be_val<latte::SQ_PGM_RESOURCES_VS> sq_pgm_resources_vs;
      be_val<latte::VGT_PRIMITIVEID_EN> vgt_primitiveid_en;
      be_val<latte::SPI_VS_OUT_CONFIG> spi_vs_out_config;
      be_val<uint32_t> num_spi_vs_out_id;
      be_array<latte::SPI_VS_OUT_ID, 10> spi_vs_out_id;
      be_val<latte::PA_CL_VS_OUT_CNTL> pa_cl_vs_out_cntl;
      be_val<latte::SQ_VTX_SEMANTIC_CLEAR> sq_vtx_semantic_clear;
      be_val<uint32_t> num_sq_vtx_semantic;
      be_array<latte::SQ_VTX_SEMANTIC_N, 32> sq_vtx_semantic;
      be_val<latte::VGT_STRMOUT_BUFFER_EN> vgt_strmout_buffer_en;
      be_val<latte::VGT_VERTEX_REUSE_BLOCK_CNTL> vgt_vertex_reuse_block_cntl;
      be_val<latte::VGT_HOS_REUSE_DEPTH> vgt_hos_reuse_depth;
   } regs;

   be_val<uint32_t> size;
   be_ptr<uint8_t> data;
   be_val<GX2ShaderMode::Value> mode;

   be_val<uint32_t> uniformBlockCount;
   be_ptr<GX2UniformBlock> uniformBlocks;

   be_val<uint32_t> uniformVarCount;
   be_ptr<GX2UniformVar> uniformVars;

   be_val<uint32_t> initialValueCount;
   be_ptr<GX2UniformInitialValue> initialValues;

   be_val<uint32_t> loopVarCount;
   be_ptr<void> loopVars;

   be_val<uint32_t> samplerVarCount;
   be_ptr<GX2SamplerVar> samplerVars;

   be_val<uint32_t> attribVarCount;
   be_ptr<GX2AttribVar> attribVars;

   be_val<uint32_t> ringItemsize;

   be_val<BOOL> hasStreamOut;
   be_val<uint32_t> streamOutStride[4];

   UNKNOWN(4 * 4);
};
CHECK_OFFSET(GX2VertexShader, 0x0, regs);
CHECK_OFFSET(GX2VertexShader, 0xd0, size);
CHECK_OFFSET(GX2VertexShader, 0xd4, data);
CHECK_OFFSET(GX2VertexShader, 0xd8, mode);
CHECK_OFFSET(GX2VertexShader, 0xdc, uniformBlockCount);
CHECK_OFFSET(GX2VertexShader, 0xe0, uniformBlocks);
CHECK_OFFSET(GX2VertexShader, 0xe4, uniformVarCount);
CHECK_OFFSET(GX2VertexShader, 0xe8, uniformVars);
CHECK_OFFSET(GX2VertexShader, 0xec, initialValueCount);
CHECK_OFFSET(GX2VertexShader, 0xf0, initialValues);
CHECK_OFFSET(GX2VertexShader, 0xf4, loopVarCount);
CHECK_OFFSET(GX2VertexShader, 0xf8, loopVars);
CHECK_OFFSET(GX2VertexShader, 0xfc, samplerVarCount);
CHECK_OFFSET(GX2VertexShader, 0x100, samplerVars);
CHECK_OFFSET(GX2VertexShader, 0x104, attribVarCount);
CHECK_OFFSET(GX2VertexShader, 0x108, attribVars);
CHECK_OFFSET(GX2VertexShader, 0x10c, ringItemsize);
CHECK_OFFSET(GX2VertexShader, 0x110, hasStreamOut);
CHECK_OFFSET(GX2VertexShader, 0x114, streamOutStride);
CHECK_SIZE(GX2VertexShader, 0x134);

struct GX2PixelShader
{
   struct
   {
      latte::SQ_PGM_RESOURCES_PS sq_pgm_resources_ps;
      latte::SQ_PGM_EXPORTS_PS pgm_exports_ps;
      latte::SPI_PS_IN_CONTROL_0 spi_ps_in_control_0;
      latte::SPI_PS_IN_CONTROL_1 spi_ps_in_control_1;
      uint32_t num_spi_ps_input_cntl;
      latte::SPI_PS_INPUT_CNTL_N spi_ps_input_cntls[32];
      latte::CB_SHADER_MASK cb_shader_mask;
      latte::CB_SHADER_CONTROL cb_shader_control;
      latte::DB_SHADER_CONTROL db_shader_control;
      latte::SPI_INPUT_Z spi_input_z;
   } regs;

   be_val<uint32_t> size;
   be_ptr<uint8_t> data;
   be_val<GX2ShaderMode::Value> mode;

   be_val<uint32_t> uniformBlockCount;
   be_ptr<GX2UniformBlock> uniformBlocks;

   be_val<uint32_t> uniformVarCount;
   be_ptr<GX2UniformVar> uniformVars;

   be_val<uint32_t> initialValueCount;
   be_ptr<GX2UniformInitialValue> initialValues;

   be_val<uint32_t> loopVarCount;
   be_ptr<void> loopVars;

   be_val<uint32_t> samplerVarCount;
   be_ptr<GX2SamplerVar> samplerVars;

   UNKNOWN(4 * 4);
};
CHECK_OFFSET(GX2PixelShader, 0x00, regs.sq_pgm_resources_ps);
CHECK_OFFSET(GX2PixelShader, 0x04, regs.pgm_exports_ps);
CHECK_OFFSET(GX2PixelShader, 0x08, regs.spi_ps_in_control_0);
CHECK_OFFSET(GX2PixelShader, 0x0C, regs.spi_ps_in_control_1);
CHECK_OFFSET(GX2PixelShader, 0x10, regs.num_spi_ps_input_cntl);
CHECK_OFFSET(GX2PixelShader, 0x14, regs.spi_ps_input_cntls);
CHECK_OFFSET(GX2PixelShader, 0x94, regs.cb_shader_mask);
CHECK_OFFSET(GX2PixelShader, 0x98, regs.cb_shader_control);
CHECK_OFFSET(GX2PixelShader, 0x9C, regs.db_shader_control);
CHECK_OFFSET(GX2PixelShader, 0xA0, regs.spi_input_z);
CHECK_OFFSET(GX2PixelShader, 0xA4, size);
CHECK_OFFSET(GX2PixelShader, 0xA8, data);
CHECK_OFFSET(GX2PixelShader, 0xAC, mode);
CHECK_OFFSET(GX2PixelShader, 0xB0, uniformBlockCount);
CHECK_OFFSET(GX2PixelShader, 0xB4, uniformBlocks);
CHECK_OFFSET(GX2PixelShader, 0xB8, uniformVarCount);
CHECK_OFFSET(GX2PixelShader, 0xBC, uniformVars);
CHECK_OFFSET(GX2PixelShader, 0xC0, initialValueCount);
CHECK_OFFSET(GX2PixelShader, 0xC4, initialValues);
CHECK_OFFSET(GX2PixelShader, 0xC8, loopVarCount);
CHECK_OFFSET(GX2PixelShader, 0xCC, loopVars);
CHECK_OFFSET(GX2PixelShader, 0xD0, samplerVarCount);
CHECK_OFFSET(GX2PixelShader, 0xD4, samplerVars);
CHECK_SIZE(GX2PixelShader, 0xe8);

struct GX2GeometryShader;

#pragma pack(push, 1)

struct GX2AttribStream
{
   be_val<uint32_t> location;
   be_val<uint32_t> buffer;
   be_val<uint32_t> offset;
   be_val<GX2AttribFormat::Value> format;
   be_val<GX2AttribIndexType::Value> type;
   be_val<uint32_t> aluDivisor;
   be_val<uint32_t> mask;
   be_val<GX2EndianSwapMode::Value> endianSwap;
};
CHECK_OFFSET(GX2AttribStream, 0x0, location);
CHECK_OFFSET(GX2AttribStream, 0x4, buffer);
CHECK_OFFSET(GX2AttribStream, 0x8, offset);
CHECK_OFFSET(GX2AttribStream, 0xc, format);
CHECK_OFFSET(GX2AttribStream, 0x10, type);
CHECK_OFFSET(GX2AttribStream, 0x14, aluDivisor);
CHECK_OFFSET(GX2AttribStream, 0x18, mask);
CHECK_OFFSET(GX2AttribStream, 0x1c, endianSwap);
CHECK_SIZE(GX2AttribStream, 0x20);

#pragma pack(pop)

uint32_t
GX2CalcGeometryShaderInputRingBufferSize(uint32_t ringItemSize);

uint32_t
GX2CalcGeometryShaderOutputRingBufferSize(uint32_t ringItemSize);

uint32_t
GX2CalcFetchShaderSizeEx(uint32_t attribs,
                         GX2FetchShaderType::Value fetchShaderType,
                         GX2TessellationMode::Value tesellationMode);

void
GX2InitFetchShaderEx(GX2FetchShader *fetchShader,
                     uint8_t *buffer,
                     uint32_t attribCount,
                     GX2AttribStream *attribs,
                     GX2FetchShaderType::Value type,
                     GX2TessellationMode::Value tessMode);

void
GX2SetFetchShader(GX2FetchShader *shader);

void
GX2SetVertexShader(GX2VertexShader *shader);

void
GX2SetPixelShader(GX2PixelShader *shader);

void
GX2SetGeometryShader(GX2GeometryShader *shader);

void
GX2SetVertexSampler(GX2Sampler *sampler, uint32_t id);

void
GX2SetPixelSampler(GX2Sampler *sampler, uint32_t id);

void
GX2SetGeometrySampler(GX2Sampler *sampler, uint32_t id);

void
GX2SetVertexUniformReg(uint32_t offset, uint32_t count, uint32_t *data);

void
GX2SetPixelUniformReg(uint32_t offset, uint32_t count, uint32_t *data);

void
GX2SetVertexUniformBlock(uint32_t location, uint32_t size, const void *data);

void
GX2SetPixelUniformBlock(uint32_t location, uint32_t size, const void *data);

void
GX2SetShaderModeEx(GX2ShaderMode::Value mode,
                   uint32_t unk1, uint32_t unk2, uint32_t unk3,
                   uint32_t unk4, uint32_t unk5, uint32_t unk6);

uint32_t
GX2GetPixelShaderGPRs(GX2PixelShader *shader);

uint32_t
GX2GetPixelShaderStackEntries(GX2PixelShader *shader);

uint32_t
GX2GetVertexShaderGPRs(GX2VertexShader *shader);

uint32_t
GX2GetVertexShaderStackEntries(GX2VertexShader *shader);
