#include "modules/gx2/gx2.h"
#ifdef GX2_NULL

#include "modules/gx2/gx2_shaders.h"

uint32_t
GX2CalcGeometryShaderInputRingBufferSize(uint32_t ringItemSize)
{
   return ringItemSize << 12;
}

uint32_t
GX2CalcGeometryShaderOutputRingBufferSize(uint32_t ringItemSize)
{
   return ringItemSize << 14;
}

uint32_t
GX2CalcFetchShaderSizeEx(uint32_t attribs,
                         GX2FetchShaderType::Type fetchShaderType,
                         GX2TessellationMode::Mode tessellationMode)
{
   return attribs * 16;
}

void
GX2InitFetchShaderEx(GX2FetchShader *fetchShader,
                     void *buffer,
                     uint32_t count,
                     GX2AttribStream *attribs,
                     GX2FetchShaderType::Type type,
                     GX2TessellationMode::Mode tessMode)
{
}

void
GX2SetFetchShader(GX2FetchShader *shader)
{
}

void
GX2SetVertexShader(GX2VertexShader *shader)
{
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
GX2SetPixelSampler(GX2PixelSampler *sampler,
                   uint32_t id)
{
}

void
GX2SetVertexUniformReg(uint32_t offset,
                       uint32_t count,
                       void *data)
{
}

void
GX2SetPixelUniformReg(uint32_t offset,
                      uint32_t count,
                      void *data)
{
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