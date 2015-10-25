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
                         GX2FetchShaderType::Type fetchShaderType,
                         GX2TessellationMode::Mode tessellationMode)
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
                     GX2FetchShaderType::Type type,
                     GX2TessellationMode::Mode tessMode)
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
_GX2SetVertexUniformReg(uint32_t offset,
                       uint32_t count,
                       void *data)
{
   float *floatData = (float*)data;
   for (auto i = 0u; i < count; ++i) {
      gDX.state.uniforms[offset + i] = byte_swap(floatData[i]);
   }
}
void
GX2SetVertexUniformReg(uint32_t offset, uint32_t count, void *data) {
   DX_DLCALL(_GX2SetVertexUniformReg, offset, count, data);
}


void
GX2SetPixelUniformReg(uint32_t offset,
                      uint32_t count,
                      void *data)
{
   // TODO: GX2SetPixelUniformReg
}

void
GX2SetShaderModeEx(GX2ShaderMode::Mode mode,
                   uint32_t unk1,
                   uint32_t unk2,
                   uint32_t unk3,
                   uint32_t unk4,
                   uint32_t unk5,
                   uint32_t unk6)
{
   // TODO: GX2SetShaderModeEx
   gLog->debug("unimplemented GX2SetShaderModeEx");
}

uint32_t
GX2GetPixelShaderGPRs(GX2PixelShader *shader)
{
   return 8;
}

uint32_t
GX2GetPixelShaderStackEntries(GX2PixelShader *shader)
{
   return 8;
}

uint32_t
GX2GetVertexShaderGPRs(GX2VertexShader *shader)
{
   return 8;
}

uint32_t
GX2GetVertexShaderStackEntries(GX2VertexShader *shader)
{
   return 8;
}


#endif