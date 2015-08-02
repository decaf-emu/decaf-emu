#include "gx2.h"
#include "gx2_shaders.h"

uint32_t
GX2CalcGeometryShaderInputRingBufferSize(uint32_t ringItemSize)
{
   // From gx2.rpl but its likely this is custom to our implementation
   return ringItemSize << 12;
}

uint32_t
GX2CalcGeometryShaderOutputRingBufferSize(uint32_t ringItemSize)
{
   // From gx2.rpl but its likely this is custom to our implementation
   return ringItemSize << 14;
}

uint32_t
GX2CalcFetchShaderSizeEx(uint32_t attribs, GX2FetchShaderType::Type fetchShaderType, GX2TessellationMode::Mode tessellationMode)
{
   // This is custom to our implementation.
   return attribs * 16;
}

void
GX2InitFetchShaderEx(GX2FetchShader *fetchShader, void *buffer, uint32_t count, GX2AttribStream *attribs, GX2FetchShaderType::Type type, GX2TessellationMode::Mode tessMode)
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
GX2SetPixelSampler(GX2PixelSampler *sampler, uint32_t id)
{
}

void
GX2SetVertexUniformReg(uint32_t offset, uint32_t count, void *data)
{
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
}
