#include "modules/gx2/gx2.h"
#ifdef GX2_DX12

#include "modules/gx2/gx2_shaders.h"
#include "dx12_fetchshader.h"
#include "utils/byte_swap.h"

uint32_t
GX2CalcGeometryShaderInputRingBufferSize(uint32_t ringItemSize)
{
   // TODO: GX2CalcGeometryShaderInputRingBufferSize
   // Copied from gx2.rpl but its likely this is custom to our implementation
   return ringItemSize << 12;
}

uint32_t
GX2CalcGeometryShaderOutputRingBufferSize(uint32_t ringItemSize)
{
   // TODO: GX2CalcGeometryShaderOutputRingBufferSize
   // Copied from gx2.rpl but its likely this is custom to our implementation
   return ringItemSize << 14;
}

uint32_t
GX2CalcFetchShaderSizeEx(uint32_t attribs,
                         GX2FetchShaderType::Value fetchShaderType,
                         GX2TessellationMode::Value tessellationMode)
{
   // TODO: GX2CalcFetchShaderSizeEx
   // This is definitely custom to our implementation.
   return sizeof(FetchShaderInfo) + (attribs * sizeof(GX2AttribStream));
}

void
GX2InitFetchShaderEx(GX2FetchShader *fetchShader,
                     void *buffer,
                     uint32_t count,
                     GX2AttribStream *attribs,
                     GX2FetchShaderType::Value type,
                     GX2TessellationMode::Value tessMode)
{
   fetchShader->data = buffer;
   fetchShader->size = GX2CalcFetchShaderSizeEx(count, type, tessMode);
   fetchShader->attribCount = count;

   auto dataPtr = (FetchShaderInfo*)buffer;
   dataPtr->type = type;
   dataPtr->tessMode = tessMode;
   memcpy(dataPtr->attribs, attribs, count * sizeof(GX2AttribStream));
}

void
_GX2SetFetchShader(GX2FetchShader *shader)
{
   gDX.state.fetchShader = shader;
}
void GX2SetFetchShader(GX2FetchShader *shader) {
   DX_DLCALL(_GX2SetFetchShader, shader);
}

void
_GX2SetVertexShader(GX2VertexShader *shader)
{
   gDX.state.vertexShader = shader;
}
void GX2SetVertexShader(GX2VertexShader *shader) {
   DX_DLCALL(_GX2SetVertexShader, shader);
}

void
_GX2SetPixelShader(GX2PixelShader *shader)
{
   gDX.state.pixelShader = shader;
}
void GX2SetPixelShader(GX2PixelShader *shader) {
   DX_DLCALL(_GX2SetPixelShader, shader);
}

void
_GX2SetGeometryShader(GX2GeometryShader *shader)
{
   gDX.state.geomShader = shader;
}
void GX2SetGeometryShader(GX2GeometryShader *shader) {
   DX_DLCALL(_GX2SetGeometryShader, shader);
}

void
_GX2SetPixelSampler(GX2PixelSampler *sampler,
                   uint32_t id)
{
   gDX.state.pixelSampler[id] = sampler;
}
void GX2SetPixelSampler(GX2PixelSampler *sampler, uint32_t id) {
   DX_DLCALL(_GX2SetPixelSampler, sampler, id);
}

void
_GX2SetVertexUniformReg(uint32_t offset, float data)
{
   gDX.state.vertUniforms[offset] = data;
}
void
GX2SetVertexUniformReg(uint32_t offset, uint32_t count, void *data) {
   float *floatData = (float*)data;
   for (auto i = 0u; i < count; ++i) {
      DX_DLCALL(_GX2SetVertexUniformReg, offset + i, byte_swap(floatData[i]));
   }
}

void
_GX2SetPixelUniformReg(uint32_t offset, float data)
{
   gDX.state.pixUniforms[offset] = data;
}
void
GX2SetPixelUniformReg(uint32_t offset,
                      uint32_t count,
                      void *data)
{
   float *floatData = (float*)data;
   for (auto i = 0u; i < count; ++i) {
      DX_DLCALL(_GX2SetPixelUniformReg, offset + i, byte_swap(floatData[i]));
   }
}

void
_GX2SetVertexUniformBlock(uint32_t location, uint32_t size, const void *data)
{
   gDX.state.vertUniformBlocks[location].buffer = data;
   gDX.state.vertUniformBlocks[location].size = size;
}

void
GX2SetVertexUniformBlock(uint32_t location, uint32_t size, const void *data)
{
   DX_DLCALL(_GX2SetVertexUniformBlock, location, size, data);
}

void
_GX2SetPixelUniformBlock(uint32_t location, uint32_t size, const void *data)
{
   gDX.state.pixUniformBlocks[location].buffer = data;
   gDX.state.pixUniformBlocks[location].size = size;
}

void
GX2SetPixelUniformBlock(uint32_t location, uint32_t size, const void *data)
{
   DX_DLCALL(_GX2SetPixelUniformBlock, location, size, data);
}

void
_GX2SetShaderModeEx(GX2ShaderMode::Value mode,
   uint32_t unk1, uint32_t unk2, uint32_t unk3,
   uint32_t unk4, uint32_t unk5, uint32_t unk6)
{
   gDX.state.shaderMode = mode;
}

void
GX2SetShaderModeEx(GX2ShaderMode::Value mode,
                   uint32_t unk1, uint32_t unk2, uint32_t unk3,
                   uint32_t unk4, uint32_t unk5, uint32_t unk6)
{
   DX_DLCALL(_GX2SetShaderModeEx, mode, unk1, unk2, unk3, unk4, unk5, unk6);
}

uint32_t
GX2GetPixelShaderGPRs(GX2PixelShader *shader)
{
   return 60;
}

uint32_t
GX2GetPixelShaderStackEntries(GX2PixelShader *shader)
{
   return 60;
}

uint32_t
GX2GetVertexShaderGPRs(GX2VertexShader *shader)
{
   return 60;
}

uint32_t
GX2GetVertexShaderStackEntries(GX2VertexShader *shader)
{
   return 60;
}


#endif