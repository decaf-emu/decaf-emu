#include "gx2.h"
#include "gx2_shaders.h"

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
   // TODO: GX2InitFetchShaderEx
}

void
GX2SetFetchShader(GX2FetchShader *shader)
{
   // TODO: GX2SetFetchShader
}

void
GX2SetVertexShader(GX2VertexShader *shader)
{
   // TODO: GX2SetVertexShader
}

void
GX2SetPixelShader(GX2PixelShader *shader)
{
   // TODO: GX2SetPixelShader
}

void
GX2SetGeometryShader(GX2GeometryShader *shader)
{
   // TODO: GX2SetGeometryShader
}

void
GX2SetPixelSampler(GX2PixelSampler *sampler,
                   uint32_t id)
{
   // TODO: GX2SetPixelSampler
}

void
GX2SetVertexUniformReg(uint32_t offset,
                       uint32_t count,
                       void *data)
{
   // TODO: GX2SetVertexUniformReg
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
}

void
GX2::registerShaderFunctions()
{
   RegisterKernelFunction(GX2CalcGeometryShaderInputRingBufferSize);
   RegisterKernelFunction(GX2CalcGeometryShaderOutputRingBufferSize);
   RegisterKernelFunction(GX2CalcFetchShaderSizeEx);
   RegisterKernelFunction(GX2InitFetchShaderEx);
   RegisterKernelFunction(GX2SetFetchShader);
   RegisterKernelFunction(GX2SetVertexShader);
   RegisterKernelFunction(GX2SetPixelShader);
   RegisterKernelFunction(GX2SetGeometryShader);
   RegisterKernelFunction(GX2SetPixelSampler);
   RegisterKernelFunction(GX2SetVertexUniformReg);
   RegisterKernelFunction(GX2SetPixelUniformReg);
   RegisterKernelFunction(GX2SetShaderModeEx);
}
