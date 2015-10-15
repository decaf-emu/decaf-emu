#include "gx2.h"
#include "gx2_context.h"
#include "gx2_display.h"
#include "gx2_displaylist.h"
#include "gx2_draw.h"
#include "gx2_renderstate.h"
#include "gx2_shaders.h"
#include "gx2_surface.h"
#include "gx2_temp.h"
#include "gx2_texture.h"
#include "gx2r_resource.h"

GX2::GX2()
{
}

void GX2::initialise()
{
   initialiseVsync();
}

void
GX2::RegisterFunctions()
{
   registerContextFunctions();
   registerDisplayFunctions();
   registerDisplayListFunctions();
   registerDrawFunctions();
   registerRenderStateFunctions();
   registerResourceFunctions();
   registerShaderFunctions();
   registerSurfaceFunctions();
   registerTempFunctions();
   registerTextureFunctions();
   registerVsyncFunctions();
}

void
GX2::registerContextFunctions()
{
   RegisterKernelFunction(GX2Init);
   RegisterKernelFunction(GX2Shutdown);
   RegisterKernelFunction(GX2Flush);
   RegisterKernelFunction(GX2Invalidate);
   RegisterKernelFunction(GX2SetupContextState);
   RegisterKernelFunction(GX2SetupContextStateEx);
   RegisterKernelFunction(GX2GetContextStateDisplayList);
   RegisterKernelFunction(GX2SetContextState);
}

void
GX2::registerDisplayFunctions()
{
   RegisterKernelFunction(GX2SetTVEnable);
   RegisterKernelFunction(GX2SetDRCEnable);
   RegisterKernelFunction(GX2CalcTVSize);
   RegisterKernelFunction(GX2SetTVBuffer);
   RegisterKernelFunction(GX2SetTVScale);
   RegisterKernelFunction(GX2CalcDRCSize);
   RegisterKernelFunction(GX2SetDRCBuffer);
   RegisterKernelFunction(GX2SetDRCScale);
   RegisterKernelFunction(GX2SetSwapInterval);
   RegisterKernelFunction(GX2GetSwapInterval);
   RegisterKernelFunction(GX2GetSystemTVScanMode);
   RegisterKernelFunction(GX2DrawDone);
   RegisterKernelFunction(GX2SwapScanBuffers);
   RegisterKernelFunction(GX2WaitForVsync);
   RegisterKernelFunction(GX2WaitForFlip);
   RegisterKernelFunction(GX2GetSwapStatus);
   RegisterKernelFunction(GX2GetLastFrame);
   RegisterKernelFunction(GX2GetLastFrameGamma);
   RegisterKernelFunction(GX2CopyColorBufferToScanBuffer);
}

void
GX2::registerDisplayListFunctions()
{
   RegisterKernelFunction(GX2BeginDisplayListEx);
   RegisterKernelFunction(GX2BeginDisplayList);
   RegisterKernelFunction(GX2EndDisplayList);
   RegisterKernelFunction(GX2DirectCallDisplayList);
   RegisterKernelFunction(GX2CallDisplayList);
   RegisterKernelFunction(GX2GetDisplayListWriteStatus);
   RegisterKernelFunction(GX2GetCurrentDisplayList);
   RegisterKernelFunction(GX2CopyDisplayList);
}

void
GX2::registerDrawFunctions()
{
   RegisterKernelFunction(GX2ClearBuffersEx);
   RegisterKernelFunction(GX2SetClearDepthStencil);
   RegisterKernelFunction(GX2SetAttribBuffer);
   RegisterKernelFunction(GX2DrawEx);
}

void
GX2::registerRenderStateFunctions()
{
   RegisterKernelFunction(GX2SetDepthStencilControl);
   RegisterKernelFunction(GX2SetStencilMask);
   RegisterKernelFunction(GX2SetPolygonControl);
   RegisterKernelFunction(GX2SetColorControl);
   RegisterKernelFunction(GX2SetBlendControl);
   RegisterKernelFunction(GX2SetBlendConstantColor);
   RegisterKernelFunction(GX2SetAlphaTest);
   RegisterKernelFunction(GX2SetTargetChannelMasks);
   RegisterKernelFunction(GX2SetAlphaToMask);
   RegisterKernelFunction(GX2SetViewport);
   RegisterKernelFunction(GX2SetScissor);
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
   RegisterKernelFunction(GX2GetPixelShaderGPRs);
   RegisterKernelFunction(GX2GetPixelShaderStackEntries);
   RegisterKernelFunction(GX2GetVertexShaderGPRs);
   RegisterKernelFunction(GX2GetVertexShaderStackEntries);
}

void
GX2::registerSurfaceFunctions()
{
   RegisterKernelFunction(GX2CalcSurfaceSizeAndAlignment);
   RegisterKernelFunction(GX2CalcDepthBufferHiZInfo);
   RegisterKernelFunction(GX2SetColorBuffer);
   RegisterKernelFunction(GX2SetDepthBuffer);
   RegisterKernelFunction(GX2InitColorBufferRegs);
   RegisterKernelFunction(GX2InitDepthBufferRegs);
}

void
GX2::registerTempFunctions()
{
   RegisterKernelFunction(GX2TempGetGPUVersion);
}

void
GX2::registerTextureFunctions()
{
   RegisterKernelFunction(GX2InitSampler);
   RegisterKernelFunction(GX2InitTextureRegs);
   RegisterKernelFunction(GX2SetPixelTexture);
}

void
GX2::registerResourceFunctions()
{
   RegisterKernelFunction(GX2RSetAllocator);
}
