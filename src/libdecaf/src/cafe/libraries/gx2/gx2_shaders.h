#pragma once
#include "gx2_enum.h"
#include "gx2_sampler.h"
#include "gx2r_buffer.h"

#include <libcpu/be2_struct.h>
#include <libgpu/latte/latte_registers.h>

namespace cafe::gx2
{

/**
 * \defgroup gx2_shaders Shaders
 * \ingroup gx2
 * @{
 */

#pragma pack(push, 1)

struct GX2FetchShader;

struct GX2UniformVar
{
   be2_virt_ptr<const char> name;
   be2_val<GX2ShaderVarType> type;
   be2_val<uint32_t> count;
   be2_val<uint32_t> offset;
   be2_val<int32_t> block;
};
CHECK_OFFSET(GX2UniformVar, 0x00, name);
CHECK_OFFSET(GX2UniformVar, 0x04, type);
CHECK_OFFSET(GX2UniformVar, 0x08, count);
CHECK_OFFSET(GX2UniformVar, 0x0C, offset);
CHECK_OFFSET(GX2UniformVar, 0x10, block);
CHECK_SIZE(GX2UniformVar, 0x14);

struct GX2UniformInitialValue
{
   be2_array<float, 4> value;
   be2_val<uint32_t> offset;
};
CHECK_OFFSET(GX2UniformInitialValue, 0x00, value);
CHECK_OFFSET(GX2UniformInitialValue, 0x10, offset);
CHECK_SIZE(GX2UniformInitialValue, 0x14);

struct GX2UniformBlock
{
   be2_virt_ptr<const char> name;
   be2_val<uint32_t> offset;
   be2_val<uint32_t> size;
};
CHECK_OFFSET(GX2UniformBlock, 0x00, name);
CHECK_OFFSET(GX2UniformBlock, 0x04, offset);
CHECK_OFFSET(GX2UniformBlock, 0x08, size);
CHECK_SIZE(GX2UniformBlock, 0x0C);

struct GX2AttribVar
{
   be2_virt_ptr<const char> name;
   be2_val<GX2ShaderVarType> type;
   be2_val<uint32_t> count;
   be2_val<uint32_t> location;
};
CHECK_OFFSET(GX2AttribVar, 0x00, name);
CHECK_OFFSET(GX2AttribVar, 0x04, type);
CHECK_OFFSET(GX2AttribVar, 0x08, count);
CHECK_OFFSET(GX2AttribVar, 0x0C, location);
CHECK_SIZE(GX2AttribVar, 0x10);

struct GX2SamplerVar
{
   be2_virt_ptr<const char> name;
   be2_val<GX2SamplerVarType> type;
   be2_val<uint32_t> location;
};
CHECK_OFFSET(GX2SamplerVar, 0x00, name);
CHECK_OFFSET(GX2SamplerVar, 0x04, type);
CHECK_OFFSET(GX2SamplerVar, 0x08, location);
CHECK_SIZE(GX2SamplerVar, 0x0C);

struct GX2LoopVar
{
   be2_val<uint32_t> offset;
   be2_val<uint32_t> value;
};
CHECK_OFFSET(GX2LoopVar, 0x00, offset);
CHECK_OFFSET(GX2LoopVar, 0x04, value);
CHECK_SIZE(GX2LoopVar, 0x08);

struct GX2VertexShader
{
   struct
   {
      be2_val<latte::SQ_PGM_RESOURCES_VS> sq_pgm_resources_vs;
      be2_val<latte::VGT_PRIMITIVEID_EN> vgt_primitiveid_en;
      be2_val<latte::SPI_VS_OUT_CONFIG> spi_vs_out_config;
      be2_val<uint32_t> num_spi_vs_out_id;
      be2_array<latte::SPI_VS_OUT_ID_N, 10> spi_vs_out_id;
      be2_val<latte::PA_CL_VS_OUT_CNTL> pa_cl_vs_out_cntl;
      be2_val<latte::SQ_VTX_SEMANTIC_CLEAR> sq_vtx_semantic_clear;
      be2_val<uint32_t> num_sq_vtx_semantic;
      be2_array<latte::SQ_VTX_SEMANTIC_N, 32> sq_vtx_semantic;
      be2_val<latte::VGT_STRMOUT_BUFFER_EN> vgt_strmout_buffer_en;
      be2_val<latte::VGT_VERTEX_REUSE_BLOCK_CNTL> vgt_vertex_reuse_block_cntl;
      be2_val<latte::VGT_HOS_REUSE_DEPTH> vgt_hos_reuse_depth;
   } regs;

   be2_val<uint32_t> size;
   be2_virt_ptr<uint8_t> data;
   be2_val<GX2ShaderMode> mode;

   be2_val<uint32_t> uniformBlockCount;
   be2_virt_ptr<GX2UniformBlock> uniformBlocks;

   be2_val<uint32_t> uniformVarCount;
   be2_virt_ptr<GX2UniformVar> uniformVars;

   be2_val<uint32_t> initialValueCount;
   be2_virt_ptr<GX2UniformInitialValue> initialValues;

   be2_val<uint32_t> loopVarCount;
   be2_virt_ptr<GX2LoopVar> loopVars;

   be2_val<uint32_t> samplerVarCount;
   be2_virt_ptr<GX2SamplerVar> samplerVars;

   be2_val<uint32_t> attribVarCount;
   be2_virt_ptr<GX2AttribVar> attribVars;

   be2_val<uint32_t> ringItemsize;

   be2_val<BOOL> hasStreamOut;
   be2_array<uint32_t, 4> streamOutStride;

   GX2RBuffer gx2rData;
};
CHECK_OFFSET(GX2VertexShader, 0x00, regs.sq_pgm_resources_vs);
CHECK_OFFSET(GX2VertexShader, 0x04, regs.vgt_primitiveid_en);
CHECK_OFFSET(GX2VertexShader, 0x08, regs.spi_vs_out_config);
CHECK_OFFSET(GX2VertexShader, 0x0C, regs.num_spi_vs_out_id);
CHECK_OFFSET(GX2VertexShader, 0x10, regs.spi_vs_out_id);
CHECK_OFFSET(GX2VertexShader, 0x38, regs.pa_cl_vs_out_cntl);
CHECK_OFFSET(GX2VertexShader, 0x3C, regs.sq_vtx_semantic_clear);
CHECK_OFFSET(GX2VertexShader, 0x40, regs.num_sq_vtx_semantic);
CHECK_OFFSET(GX2VertexShader, 0x44, regs.sq_vtx_semantic);
CHECK_OFFSET(GX2VertexShader, 0xC4, regs.vgt_strmout_buffer_en);
CHECK_OFFSET(GX2VertexShader, 0xC8, regs.vgt_vertex_reuse_block_cntl);
CHECK_OFFSET(GX2VertexShader, 0xCC, regs.vgt_hos_reuse_depth);
CHECK_OFFSET(GX2VertexShader, 0xD0, size);
CHECK_OFFSET(GX2VertexShader, 0xD4, data);
CHECK_OFFSET(GX2VertexShader, 0xD8, mode);
CHECK_OFFSET(GX2VertexShader, 0xDC, uniformBlockCount);
CHECK_OFFSET(GX2VertexShader, 0xE0, uniformBlocks);
CHECK_OFFSET(GX2VertexShader, 0xE4, uniformVarCount);
CHECK_OFFSET(GX2VertexShader, 0xE8, uniformVars);
CHECK_OFFSET(GX2VertexShader, 0xEC, initialValueCount);
CHECK_OFFSET(GX2VertexShader, 0xF0, initialValues);
CHECK_OFFSET(GX2VertexShader, 0xF4, loopVarCount);
CHECK_OFFSET(GX2VertexShader, 0xF8, loopVars);
CHECK_OFFSET(GX2VertexShader, 0xFC, samplerVarCount);
CHECK_OFFSET(GX2VertexShader, 0x100, samplerVars);
CHECK_OFFSET(GX2VertexShader, 0x104, attribVarCount);
CHECK_OFFSET(GX2VertexShader, 0x108, attribVars);
CHECK_OFFSET(GX2VertexShader, 0x10C, ringItemsize);
CHECK_OFFSET(GX2VertexShader, 0x110, hasStreamOut);
CHECK_OFFSET(GX2VertexShader, 0x114, streamOutStride);
CHECK_OFFSET(GX2VertexShader, 0x124, gx2rData);
CHECK_SIZE(GX2VertexShader, 0x134);

struct GX2PixelShader
{
   struct
   {
      be2_val<latte::SQ_PGM_RESOURCES_PS> sq_pgm_resources_ps;
      be2_val<latte::SQ_PGM_EXPORTS_PS> sq_pgm_exports_ps;
      be2_val<latte::SPI_PS_IN_CONTROL_0> spi_ps_in_control_0;
      be2_val<latte::SPI_PS_IN_CONTROL_1> spi_ps_in_control_1;
      be2_val<uint32_t> num_spi_ps_input_cntl;
      be2_array<latte::SPI_PS_INPUT_CNTL_N, 32> spi_ps_input_cntls;
      be2_val<latte::CB_SHADER_MASK> cb_shader_mask;
      be2_val<latte::CB_SHADER_CONTROL> cb_shader_control;
      be2_val<latte::DB_SHADER_CONTROL> db_shader_control;
      be2_val<latte::SPI_INPUT_Z> spi_input_z;
   } regs;

   be2_val<uint32_t> size;
   be2_virt_ptr<uint8_t> data;
   be2_val<GX2ShaderMode> mode;

   be2_val<uint32_t> uniformBlockCount;
   be2_virt_ptr<GX2UniformBlock> uniformBlocks;

   be2_val<uint32_t> uniformVarCount;
   be2_virt_ptr<GX2UniformVar> uniformVars;

   be2_val<uint32_t> initialValueCount;
   be2_virt_ptr<GX2UniformInitialValue> initialValues;

   be2_val<uint32_t> loopVarCount;
   be2_virt_ptr<GX2LoopVar> loopVars;

   be2_val<uint32_t> samplerVarCount;
   be2_virt_ptr<GX2SamplerVar> samplerVars;

   GX2RBuffer gx2rData;
};
CHECK_OFFSET(GX2PixelShader, 0x00, regs.sq_pgm_resources_ps);
CHECK_OFFSET(GX2PixelShader, 0x04, regs.sq_pgm_exports_ps);
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
CHECK_OFFSET(GX2PixelShader, 0xD8, gx2rData);
CHECK_SIZE(GX2PixelShader, 0xe8);

struct GX2GeometryShader
{
   struct
   {
      be2_val<latte::SQ_PGM_RESOURCES_GS> sq_pgm_resources_gs;
      be2_val<latte::VGT_GS_OUT_PRIM_TYPE> vgt_gs_out_prim_type;
      be2_val<latte::VGT_GS_MODE> vgt_gs_mode;
      be2_val<latte::PA_CL_VS_OUT_CNTL> pa_cl_vs_out_cntl;
      be2_val<latte::SQ_PGM_RESOURCES_VS> sq_pgm_resources_vs;
      be2_val<latte::SQ_GS_VERT_ITEMSIZE> sq_gs_vert_itemsize;
      be2_val<latte::SPI_VS_OUT_CONFIG> spi_vs_out_config;
      be2_val<uint32_t> num_spi_vs_out_id;
      be2_array<latte::SPI_VS_OUT_ID_N, 10> spi_vs_out_id;
      be2_val<latte::VGT_STRMOUT_BUFFER_EN> vgt_strmout_buffer_en;
   } regs;

   be2_val<uint32_t> size;
   be2_virt_ptr<uint8_t> data;
   be2_val<uint32_t> vertexShaderSize;
   be2_virt_ptr<uint8_t> vertexShaderData;
   be2_val<GX2ShaderMode> mode;

   be2_val<uint32_t> uniformBlockCount;
   be2_virt_ptr<GX2UniformBlock> uniformBlocks;

   be2_val<uint32_t> uniformVarCount;
   be2_virt_ptr<GX2UniformVar> uniformVars;

   be2_val<uint32_t> initialValueCount;
   be2_virt_ptr<GX2UniformInitialValue> initialValues;

   be2_val<uint32_t> loopVarCount;
   be2_virt_ptr<GX2LoopVar> loopVars;

   be2_val<uint32_t> samplerVarCount;
   be2_virt_ptr<GX2SamplerVar> samplerVars;

   be2_val<uint32_t> ringItemSize;
   be2_val<BOOL> hasStreamOut;
   be2_array<uint32_t, 4> streamOutStride;

   GX2RBuffer gx2rData;
   GX2RBuffer gx2rVertexShaderData;
};
CHECK_OFFSET(GX2GeometryShader, 0x00, regs.sq_pgm_resources_gs);
CHECK_OFFSET(GX2GeometryShader, 0x04, regs.vgt_gs_out_prim_type);
CHECK_OFFSET(GX2GeometryShader, 0x08, regs.vgt_gs_mode);
CHECK_OFFSET(GX2GeometryShader, 0x0C, regs.pa_cl_vs_out_cntl);
CHECK_OFFSET(GX2GeometryShader, 0x10, regs.sq_pgm_resources_vs);
CHECK_OFFSET(GX2GeometryShader, 0x14, regs.sq_gs_vert_itemsize);
CHECK_OFFSET(GX2GeometryShader, 0x18, regs.spi_vs_out_config);
CHECK_OFFSET(GX2GeometryShader, 0x1C, regs.num_spi_vs_out_id);
CHECK_OFFSET(GX2GeometryShader, 0x20, regs.spi_vs_out_id);
CHECK_OFFSET(GX2GeometryShader, 0x48, regs.vgt_strmout_buffer_en);
CHECK_OFFSET(GX2GeometryShader, 0x4C, size);
CHECK_OFFSET(GX2GeometryShader, 0x50, data);
CHECK_OFFSET(GX2GeometryShader, 0x54, vertexShaderSize);
CHECK_OFFSET(GX2GeometryShader, 0x58, vertexShaderData);
CHECK_OFFSET(GX2GeometryShader, 0x5C, mode);
CHECK_OFFSET(GX2GeometryShader, 0x60, uniformBlockCount);
CHECK_OFFSET(GX2GeometryShader, 0x64, uniformBlocks);
CHECK_OFFSET(GX2GeometryShader, 0x68, uniformVarCount);
CHECK_OFFSET(GX2GeometryShader, 0x6C, uniformVars);
CHECK_OFFSET(GX2GeometryShader, 0x70, initialValueCount);
CHECK_OFFSET(GX2GeometryShader, 0x74, initialValues);
CHECK_OFFSET(GX2GeometryShader, 0x78, loopVarCount);
CHECK_OFFSET(GX2GeometryShader, 0x7C, loopVars);
CHECK_OFFSET(GX2GeometryShader, 0x80, samplerVarCount);
CHECK_OFFSET(GX2GeometryShader, 0x84, samplerVars);
CHECK_OFFSET(GX2GeometryShader, 0x88, ringItemSize);
CHECK_OFFSET(GX2GeometryShader, 0x8C, hasStreamOut);
CHECK_OFFSET(GX2GeometryShader, 0x90, streamOutStride);
CHECK_OFFSET(GX2GeometryShader, 0xA0, gx2rData);
CHECK_OFFSET(GX2GeometryShader, 0xB0, gx2rVertexShaderData);
CHECK_SIZE(GX2GeometryShader, 0xC0);

struct GX2AttribStream
{
   be2_val<uint32_t> location;
   be2_val<uint32_t> buffer;
   be2_val<uint32_t> offset;
   be2_val<GX2AttribFormat> format;
   be2_val<GX2AttribIndexType> type;
   be2_val<uint32_t> aluDivisor;
   be2_val<uint32_t> mask;
   be2_val<GX2EndianSwapMode> endianSwap;
};
CHECK_OFFSET(GX2AttribStream, 0x0, location);
CHECK_OFFSET(GX2AttribStream, 0x4, buffer);
CHECK_OFFSET(GX2AttribStream, 0x8, offset);
CHECK_OFFSET(GX2AttribStream, 0xC, format);
CHECK_OFFSET(GX2AttribStream, 0x10, type);
CHECK_OFFSET(GX2AttribStream, 0x14, aluDivisor);
CHECK_OFFSET(GX2AttribStream, 0x18, mask);
CHECK_OFFSET(GX2AttribStream, 0x1C, endianSwap);
CHECK_SIZE(GX2AttribStream, 0x20);

struct GX2StreamContext
{
   be2_val<uint32_t> currentOffset;
   // Total size unknown (but <= 256)
};
CHECK_OFFSET(GX2StreamContext, 0x00, currentOffset);

struct GX2OutputStream
{
   be2_val<uint32_t> size;
   be2_virt_ptr<uint8_t> buffer;
   be2_val<uint32_t> stride;
   GX2RBuffer gx2rData;
   be2_virt_ptr<GX2StreamContext> context;
};
CHECK_OFFSET(GX2OutputStream, 0x00, size);
CHECK_OFFSET(GX2OutputStream, 0x04, buffer);
CHECK_OFFSET(GX2OutputStream, 0x08, stride);
CHECK_OFFSET(GX2OutputStream, 0x0C, gx2rData);
CHECK_OFFSET(GX2OutputStream, 0x1C, context);
CHECK_SIZE(GX2OutputStream, 0x20);

#pragma pack(pop)

uint32_t
GX2CalcGeometryShaderInputRingBufferSize(uint32_t ringItemSize);

uint32_t
GX2CalcGeometryShaderOutputRingBufferSize(uint32_t ringItemSize);

void
GX2SetFetchShader(virt_ptr<GX2FetchShader> shader);

void
GX2SetVertexShader(virt_ptr<GX2VertexShader> shader);

void
GX2SetPixelShader(virt_ptr<GX2PixelShader> shader);

void
GX2SetGeometryShader(virt_ptr<GX2GeometryShader> shader);

void
GX2SetVertexSampler(virt_ptr<GX2Sampler> sampler,
                    uint32_t id);

void
GX2SetPixelSampler(virt_ptr<GX2Sampler> sampler,
                   uint32_t id);

void
GX2SetGeometrySampler(virt_ptr<GX2Sampler> sampler,
                      uint32_t id);

void
GX2SetVertexUniformReg(uint32_t offset,
                       uint32_t count,
                       virt_ptr<uint32_t> data);

void
GX2SetPixelUniformReg(uint32_t offset,
                      uint32_t count,
                      virt_ptr<uint32_t> data);

void
GX2SetVertexUniformBlock(uint32_t location,
                         uint32_t size,
                         virt_ptr<const void> data);

void
GX2SetPixelUniformBlock(uint32_t location,
                        uint32_t size,
                        virt_ptr<const void> data);

void
GX2SetGeometryUniformBlock(uint32_t location,
                           uint32_t size,
                           virt_ptr<const void> data);

void
GX2SetShaderModeEx(GX2ShaderMode mode,
                   uint32_t numVsGpr, uint32_t numVsStackEntries,
                   uint32_t numGsGpr, uint32_t numGsStackEntries,
                   uint32_t numPsGpr, uint32_t numPsStackEntries);

void
GX2SetStreamOutBuffer(uint32_t index,
                      virt_ptr<GX2OutputStream> stream);

void
GX2SetStreamOutEnable(BOOL enable);

void
GX2SetStreamOutContext(uint32_t index,
                       virt_ptr<GX2OutputStream> stream,
                       GX2StreamOutContextMode mode);

void
GX2SaveStreamOutContext(uint32_t index,
                        virt_ptr<GX2OutputStream> stream);

void
GX2SetGeometryShaderInputRingBuffer(virt_ptr<void> buffer,
                                    uint32_t size);

void
GX2SetGeometryShaderOutputRingBuffer(virt_ptr<void> buffer,
                                     uint32_t size);

uint32_t
GX2GetPixelShaderGPRs(virt_ptr<GX2PixelShader> shader);

uint32_t
GX2GetPixelShaderStackEntries(virt_ptr<GX2PixelShader> shader);

uint32_t
GX2GetVertexShaderGPRs(virt_ptr<GX2VertexShader> shader);

uint32_t
GX2GetVertexShaderStackEntries(virt_ptr<GX2VertexShader> shader);

uint32_t
GX2GetGeometryShaderGPRs(virt_ptr<GX2GeometryShader> shader);

uint32_t
GX2GetGeometryShaderStackEntries(virt_ptr<GX2GeometryShader> shader);

/** @} */

} // namespace cafe::gx2
