#include "gx2_shaders.h"

uint32_t
GX2CalcGeometryShaderInputRingBufferSize(uint32_t ringItemSize)
{
   return 0;
}

uint32_t
GX2CalcGeometryShaderOutputRingBufferSize(uint32_t ringItemSize)
{
   return 0;
}

uint32_t
GX2CalcFetchShaderSizeEx(uint32_t attribs,
                         GX2FetchShaderType::Value fetchShaderType,
                         GX2TessellationMode::Value tesellationMode)
{
   return 0;
}

void
GX2InitFetchShaderEx(GX2FetchShader *fetchShader,
                     void *buffer,
                     uint32_t count,
                     GX2AttribStream *attribs,
                     GX2FetchShaderType::Value type,
                     GX2TessellationMode::Value tessMode)
{
}

void
GX2SetFetchShader(GX2FetchShader *shader)
{
}

void
GX2SetVertexShader(GX2VertexShader *shader)
{
/*
if (shader->shaderMode != GEOMETRY_SHADER) {
  SET_CONTEXT_REG 0x216, { EFFECTIVE_ADDRESS(shader->data) >> 8, shader->size >> 3, 0x10, 0x10, shader->_regs[0] } 0x28858:SQ_PGM_START_VS
  SET_CONTEXT_REG 0x2A1, { shader->_regs[1] } 0x28a84:VGT_PRIMITIVEID_EN
  SET_CONTEXT_REG 0x1B1, { shader->_regs[2] } 0x286c4:SPI_VS_OUT_CONFIG
  SET_CONTEXT_REG 0x207, { shader->_regs[14] } 0x2881c:PA_CL_VS_OUT_CNTL
  if (shader->_regs[3] > 0) {
    SET_CONTEXT_REG 0x185, { FOR(i < MIN(shader->_regs[3], 0xA)) -> shader->_regs[4+i] }
  }
  SET_CONTEXT_REG 0x234, { 0x0 }
  if (shader->hasStreamOut) {
    _GX2WriteStreamOutStride(&shader->streamOutVertexStride)
  }
  SET_CONTEXT_REG 0x2C8, { shader._regs[49] }
} else {
  SET_CONTEXT_REG 0x216, { EFFECTIVE_ADDRESS(shader->data) >> 8, shader->size >> 3, 0x10, 0x10, shader->_regs[0] }
  SET_CONTEXT_REG 0x22A, { shader->ringItemsize }
}

SET_CONTEXT_REG 0x238, { shader->_regs[15] }
if (shader->_regs[16] > 0) {
  SET_CONTEXT_REG 0xE0, { FOR(i < MIN(shader->_regs[16], 0x20)) -> shader->_regs[17+i] }
}
SET_CONTEXT_REG 0x316, { shader->_regs[50] }
SET_CONTEXT_REG 0x288, { shader->_regs[51] }

if (shader->_numLoops > 0) {
  _GX2SetVertexLoopVar(shader->_loopVars, shader->_loopVars + (shader->_numLoops << 3));
}
*/
}

void
GX2SetPixelShader(GX2PixelShader *shader)
{
}

void
GX2SetGeometryShader(GX2GeometryShader *shader)
{
}

void
GX2SetPixelSampler(GX2Sampler *sampler, uint32_t id)
{
}

void
GX2SetVertexUniformReg(uint32_t offset, uint32_t count, void *data)
{
}

void
GX2SetPixelUniformReg(uint32_t offset, uint32_t count, void *data)
{
}

void
GX2SetVertexUniformBlock(uint32_t location, uint32_t size, const void *data)
{
}

void
GX2SetPixelUniformBlock(uint32_t location, uint32_t size, const void *data)
{
}

void
GX2SetShaderModeEx(GX2ShaderMode::Value mode,
                   uint32_t unk1, uint32_t unk2, uint32_t unk3,
                   uint32_t unk4, uint32_t unk5, uint32_t unk6)
{
}

uint32_t
GX2GetPixelShaderGPRs(GX2PixelShader *shader)
{
   return 0;
}

uint32_t
GX2GetPixelShaderStackEntries(GX2PixelShader *shader)
{
   return 0;
}

uint32_t
GX2GetVertexShaderGPRs(GX2VertexShader *shader)
{
   return 0;
}

uint32_t
GX2GetVertexShaderStackEntries(GX2VertexShader *shader)
{
   return 0;
}
