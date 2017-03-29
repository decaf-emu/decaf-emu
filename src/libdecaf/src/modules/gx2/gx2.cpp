#include "gx2.h"
#include "gx2_aperture.h"
#include "gx2_clear.h"
#include "gx2_contextstate.h"
#include "gx2_counter.h"
#include "gx2_debug.h"
#include "gx2_display.h"
#include "gx2_displaylist.h"
#include "gx2_draw.h"
#include "gx2_event.h"
#include "gx2_format.h"
#include "gx2_mem.h"
#include "gx2_query.h"
#include "gx2_registers.h"
#include "gx2_sampler.h"
#include "gx2_shaders.h"
#include "gx2_state.h"
#include "gx2_surface.h"
#include "gx2_swap.h"
#include "gx2_temp.h"
#include "gx2_tessellation.h"
#include "gx2_texture.h"
#include "gx2r_buffer.h"
#include "gx2r_draw.h"
#include "gx2r_displaylist.h"
#include "gx2r_mem.h"
#include "gx2r_resource.h"
#include "gx2r_shaders.h"
#include "gx2r_surface.h"

namespace gx2
{

void
Module::initialise()
{
}

void
Module::RegisterFunctions()
{
   // Aperture
   RegisterKernelFunction(GX2AllocateTilingApertureEx);
   RegisterKernelFunction(GX2FreeTilingAperture);

   // Clear
   RegisterKernelFunction(GX2ClearColor);
   RegisterKernelFunction(GX2ClearDepthStencil);
   RegisterKernelFunction(GX2ClearDepthStencilEx);
   RegisterKernelFunction(GX2ClearBuffers);
   RegisterKernelFunction(GX2ClearBuffersEx);
   RegisterKernelFunction(GX2SetClearDepth);
   RegisterKernelFunction(GX2SetClearStencil);
   RegisterKernelFunction(GX2SetClearDepthStencil);

   // Context State
   RegisterKernelFunction(GX2SetupContextStateEx);
   RegisterKernelFunction(GX2GetContextStateDisplayList);
   RegisterKernelFunction(GX2SetContextState);
   RegisterKernelFunction(GX2SetDefaultState);

   // Counter
   RegisterKernelFunction(GX2InitCounterInfo);
   RegisterKernelFunction(GX2ResetCounterInfo);
   RegisterKernelFunction(GX2GetCounterResult);

   // Debug
   RegisterKernelFunction(GX2DebugTagUserString);
   RegisterKernelFunction(GX2DebugTagUserStringVA);

   // Display
   RegisterKernelFunction(GX2SetTVEnable);
   RegisterKernelFunction(GX2SetDRCEnable);
   RegisterKernelFunction(GX2CalcTVSize);
   RegisterKernelFunction(GX2SetTVBuffer);
   RegisterKernelFunction(GX2SetTVScale);
   RegisterKernelFunction(GX2CalcDRCSize);
   RegisterKernelFunction(GX2SetDRCBuffer);
   RegisterKernelFunction(GX2SetDRCScale);
   RegisterKernelFunction(GX2GetSystemTVScanMode);
   RegisterKernelFunction(GX2GetSystemDRCMode);
   RegisterKernelFunction(GX2GetSystemTVAspectRatio);
   RegisterKernelFunction(GX2IsVideoOutReady);
   RegisterKernelFunction(GX2SetDRCConnectCallback);

   // Display List
   RegisterKernelFunction(GX2BeginDisplayListEx);
   RegisterKernelFunction(GX2BeginDisplayList);
   RegisterKernelFunction(GX2EndDisplayList);
   RegisterKernelFunction(GX2DirectCallDisplayList);
   RegisterKernelFunction(GX2CallDisplayList);
   RegisterKernelFunction(GX2GetDisplayListWriteStatus);
   RegisterKernelFunction(GX2GetCurrentDisplayList);
   RegisterKernelFunction(GX2CopyDisplayList);
   RegisterKernelFunction(GX2PatchDisplayList);

   // Draw
   RegisterKernelFunction(GX2SetAttribBuffer);
   RegisterKernelFunction(GX2DrawEx);
   RegisterKernelFunction(GX2DrawEx2);
   RegisterKernelFunction(GX2DrawIndexedEx);
   RegisterKernelFunction(GX2DrawIndexedEx2);
   RegisterKernelFunction(GX2DrawIndexedImmediateEx);
   RegisterKernelFunction(GX2SetPrimitiveRestartIndex);

   // Event
   RegisterKernelFunction(GX2DrawDone);
   RegisterKernelFunction(GX2WaitForVsync);
   RegisterKernelFunction(GX2WaitForFlip);
   RegisterKernelFunction(GX2SetEventCallback);
   RegisterKernelFunction(GX2GetEventCallback);
   RegisterKernelFunction(GX2GetRetiredTimeStamp);
   RegisterKernelFunction(GX2GetLastSubmittedTimeStamp);
   RegisterKernelFunction(GX2WaitTimeStamp);
   RegisterVsyncFunctions();

   // Format
   RegisterKernelFunction(GX2GetAttribFormatBits);
   RegisterKernelFunction(GX2GetSurfaceFormatBits);
   RegisterKernelFunction(GX2GetSurfaceFormatBitsPerElement);
   RegisterKernelFunction(GX2SurfaceIsCompressed);

   // GX2R Resource
   RegisterGX2RResourceFunctions();

   // GX2R Buffer
   RegisterKernelFunction(GX2RGetBufferAlignment);
   RegisterKernelFunction(GX2RGetBufferAllocationSize);
   RegisterKernelFunction(GX2RBufferExists);
   RegisterKernelFunction(GX2RCreateBuffer);
   RegisterKernelFunction(GX2RCreateBufferUserMemory);
   RegisterKernelFunction(GX2RDestroyBufferEx);
   RegisterKernelFunction(GX2RInvalidateBuffer);
   RegisterKernelFunction(GX2RLockBufferEx);
   RegisterKernelFunction(GX2RUnlockBufferEx);
   RegisterKernelFunction(GX2RSetStreamOutBuffer);

   // GX2R Display List
   RegisterKernelFunction(GX2RBeginDisplayListEx);
   RegisterKernelFunction(GX2REndDisplayList);
   RegisterKernelFunction(GX2RCallDisplayList);
   RegisterKernelFunction(GX2RDirectCallDisplayList);

   // GX2R Draw
   RegisterKernelFunction(GX2RSetAttributeBuffer);
   RegisterKernelFunction(GX2RDrawIndexed);

   // GX2R Shaders
   RegisterKernelFunction(GX2RSetVertexUniformBlock);
   RegisterKernelFunction(GX2RSetPixelUniformBlock);
   RegisterKernelFunction(GX2RSetGeometryUniformBlock);

   // GX2R Surface
   RegisterKernelFunction(GX2RCreateSurface);
   RegisterKernelFunction(GX2RCreateSurfaceUserMemory);
   RegisterKernelFunction(GX2RDestroySurfaceEx);
   RegisterKernelFunction(GX2RLockSurfaceEx);
   RegisterKernelFunction(GX2RUnlockSurfaceEx);
   RegisterKernelFunction(GX2RIsGX2RSurface);
   RegisterKernelFunction(GX2RInvalidateSurface);

   // GX2R Mem
   RegisterKernelFunction(GX2RInvalidateMemory);

   // Mem
   RegisterKernelFunction(GX2Invalidate);

   // Query
   RegisterKernelFunction(GX2QueryBegin);
   RegisterKernelFunction(GX2QueryEnd);
   RegisterKernelFunction(GX2QueryGetOcclusionResult);
   RegisterKernelFunction(GX2QueryBeginConditionalRender);
   RegisterKernelFunction(GX2QueryEndConditionalRender);
   RegisterKernelFunction(GX2SampleTopGPUCycle);
   RegisterKernelFunction(GX2SampleBottomGPUCycle);
   RegisterKernelFunction(GX2GPUTimeToCPUTime);
   RegisterKernelFunction(GX2GetGPUTimeout);
   RegisterKernelFunction(GX2SetGPUTimeout);

   // Register
   RegisterKernelFunction(GX2SetAAMask);
   RegisterKernelFunction(GX2InitAAMaskReg);
   RegisterKernelFunction(GX2GetAAMaskReg);
   RegisterKernelFunction(GX2SetAAMaskReg);
   RegisterKernelFunction(GX2SetAlphaTest);
   RegisterKernelFunction(GX2InitAlphaTestReg);
   RegisterKernelFunction(GX2GetAlphaTestReg);
   RegisterKernelFunction(GX2SetAlphaTestReg);
   RegisterKernelFunction(GX2SetAlphaToMask);
   RegisterKernelFunction(GX2InitAlphaToMaskReg);
   RegisterKernelFunction(GX2GetAlphaToMaskReg);
   RegisterKernelFunction(GX2SetAlphaToMaskReg);
   RegisterKernelFunction(GX2SetBlendConstantColor);
   RegisterKernelFunction(GX2InitBlendConstantColorReg);
   RegisterKernelFunction(GX2GetBlendConstantColorReg);
   RegisterKernelFunction(GX2SetBlendConstantColorReg);
   RegisterKernelFunction(GX2SetBlendControl);
   RegisterKernelFunction(GX2InitBlendControlReg);
   RegisterKernelFunction(GX2GetBlendControlReg);
   RegisterKernelFunction(GX2SetBlendControlReg);
   RegisterKernelFunction(GX2SetColorControl);
   RegisterKernelFunction(GX2InitColorControlReg);
   RegisterKernelFunction(GX2GetColorControlReg);
   RegisterKernelFunction(GX2SetColorControlReg);
   RegisterKernelFunction(GX2SetDepthOnlyControl);
   RegisterKernelFunction(GX2SetDepthStencilControl);
   RegisterKernelFunction(GX2InitDepthStencilControlReg);
   RegisterKernelFunction(GX2GetDepthStencilControlReg);
   RegisterKernelFunction(GX2SetDepthStencilControlReg);
   RegisterKernelFunction(GX2SetStencilMask);
   RegisterKernelFunction(GX2InitStencilMaskReg);
   RegisterKernelFunction(GX2GetStencilMaskReg);
   RegisterKernelFunction(GX2SetStencilMaskReg);
   RegisterKernelFunction(GX2SetLineWidth);
   RegisterKernelFunction(GX2InitLineWidthReg);
   RegisterKernelFunction(GX2GetLineWidthReg);
   RegisterKernelFunction(GX2SetLineWidthReg);
   RegisterKernelFunction(GX2SetPointSize);
   RegisterKernelFunction(GX2InitPointSizeReg);
   RegisterKernelFunction(GX2GetPointSizeReg);
   RegisterKernelFunction(GX2SetPointSizeReg);
   RegisterKernelFunction(GX2SetPointLimits);
   RegisterKernelFunction(GX2InitPointLimitsReg);
   RegisterKernelFunction(GX2GetPointLimitsReg);
   RegisterKernelFunction(GX2SetPointLimitsReg);
   RegisterKernelFunction(GX2SetCullOnlyControl);
   RegisterKernelFunction(GX2SetPolygonControl);
   RegisterKernelFunction(GX2InitPolygonControlReg);
   RegisterKernelFunction(GX2SetPolygonControlReg);
   RegisterKernelFunction(GX2SetPolygonOffset);
   RegisterKernelFunction(GX2InitPolygonOffsetReg);
   RegisterKernelFunction(GX2GetPolygonOffsetReg);
   RegisterKernelFunction(GX2SetPolygonOffsetReg);
   RegisterKernelFunction(GX2SetScissor);
   RegisterKernelFunction(GX2InitScissorReg);
   RegisterKernelFunction(GX2GetScissorReg);
   RegisterKernelFunction(GX2SetScissorReg);
   RegisterKernelFunction(GX2SetTargetChannelMasks);
   RegisterKernelFunction(GX2InitTargetChannelMasksReg);
   RegisterKernelFunction(GX2GetTargetChannelMasksReg);
   RegisterKernelFunction(GX2SetTargetChannelMasksReg);
   RegisterKernelFunction(GX2SetViewport);
   RegisterKernelFunction(GX2InitViewportReg);
   RegisterKernelFunction(GX2GetViewportReg);
   RegisterKernelFunction(GX2SetViewportReg);
   RegisterKernelFunction(GX2SetRasterizerClipControl);
   RegisterKernelFunction(GX2SetRasterizerClipControlEx);
   RegisterKernelFunction(GX2SetRasterizerClipControlHalfZ);

   // Sampler
   RegisterKernelFunction(GX2InitSampler);
   RegisterKernelFunction(GX2InitSamplerBorderType);
   RegisterKernelFunction(GX2InitSamplerClamping);
   RegisterKernelFunction(GX2InitSamplerDepthCompare);
   RegisterKernelFunction(GX2InitSamplerFilterAdjust);
   RegisterKernelFunction(GX2InitSamplerLOD);
   RegisterKernelFunction(GX2InitSamplerLODAdjust);
   RegisterKernelFunction(GX2InitSamplerRoundingMode);
   RegisterKernelFunction(GX2InitSamplerXYFilter);
   RegisterKernelFunction(GX2InitSamplerZMFilter);
   RegisterKernelFunction(GX2SetPixelSamplerBorderColor);
   RegisterKernelFunction(GX2SetVertexSamplerBorderColor);
   RegisterKernelFunction(GX2SetGeometrySamplerBorderColor);

   // Shader
   RegisterKernelFunction(GX2CalcGeometryShaderInputRingBufferSize);
   RegisterKernelFunction(GX2CalcGeometryShaderOutputRingBufferSize);
   RegisterKernelFunction(GX2CalcFetchShaderSizeEx);
   RegisterKernelFunction(GX2InitFetchShaderEx);
   RegisterKernelFunction(GX2SetFetchShader);
   RegisterKernelFunction(GX2SetVertexShader);
   RegisterKernelFunction(GX2SetPixelShader);
   RegisterKernelFunction(GX2SetGeometryShader);
   RegisterKernelFunction(GX2SetVertexSampler);
   RegisterKernelFunction(GX2SetPixelSampler);
   RegisterKernelFunction(GX2SetGeometrySampler);
   RegisterKernelFunction(GX2SetVertexUniformReg);
   RegisterKernelFunction(GX2SetPixelUniformReg);
   RegisterKernelFunction(GX2SetVertexUniformBlock);
   RegisterKernelFunction(GX2SetPixelUniformBlock);
   RegisterKernelFunction(GX2SetGeometryUniformBlock);
   RegisterKernelFunction(GX2SetShaderModeEx);
   RegisterKernelFunction(GX2SetStreamOutBuffer);
   RegisterKernelFunction(GX2SetStreamOutEnable);
   RegisterKernelFunction(GX2SetStreamOutContext);
   RegisterKernelFunction(GX2SaveStreamOutContext);
   RegisterKernelFunction(GX2SetGeometryShaderInputRingBuffer);
   RegisterKernelFunction(GX2SetGeometryShaderOutputRingBuffer);
   RegisterKernelFunction(GX2GetPixelShaderGPRs);
   RegisterKernelFunction(GX2GetPixelShaderStackEntries);
   RegisterKernelFunction(GX2GetVertexShaderGPRs);
   RegisterKernelFunction(GX2GetVertexShaderStackEntries);
   RegisterKernelFunction(GX2GetGeometryShaderGPRs);
   RegisterKernelFunction(GX2GetGeometryShaderStackEntries);

   // State
   RegisterKernelFunction(GX2Init);
   RegisterKernelFunction(GX2Shutdown);
   RegisterKernelFunction(GX2Flush);

   // Surface
   RegisterKernelFunction(GX2CalcSurfaceSizeAndAlignment);
   RegisterKernelFunction(GX2CalcDepthBufferHiZInfo);
   RegisterKernelFunction(GX2CalcColorBufferAuxInfo);
   RegisterKernelFunction(GX2SetColorBuffer);
   RegisterKernelFunction(GX2SetDepthBuffer);
   RegisterKernelFunction(GX2InitColorBufferRegs);
   RegisterKernelFunction(GX2InitDepthBufferRegs);
   RegisterKernelFunction(GX2InitDepthBufferHiZEnable);
   RegisterKernelFunction(GX2GetSurfaceSwizzle);
   RegisterKernelFunction(GX2GetSurfaceSwizzleOffset);
   RegisterKernelFunction(GX2SetSurfaceSwizzle);
   RegisterKernelFunction(GX2GetSurfaceMipPitch);
   RegisterKernelFunction(GX2GetSurfaceMipSliceSize);
   RegisterKernelFunction(GX2CopySurface);
   RegisterKernelFunction(GX2ExpandDepthBuffer);

   // Swap
   RegisterKernelFunction(GX2CopyColorBufferToScanBuffer);
   RegisterKernelFunction(GX2SwapScanBuffers);
   RegisterKernelFunction(GX2GetLastFrame);
   RegisterKernelFunction(GX2GetLastFrameGamma);
   RegisterKernelFunction(GX2GetSwapStatus);
   RegisterKernelFunction(GX2GetSwapInterval);
   RegisterKernelFunction(GX2SetSwapInterval);

   // Temp
   RegisterKernelFunction(GX2TempGetGPUVersion);

   // Tessellation
   RegisterKernelFunction(GX2SetTessellation);
   RegisterKernelFunction(GX2SetMinTessellationLevel);
   RegisterKernelFunction(GX2SetMaxTessellationLevel);

   // Texture
   RegisterKernelFunction(GX2InitTextureRegs);
   RegisterKernelFunction(GX2SetPixelTexture);
   RegisterKernelFunction(GX2SetVertexTexture);
   RegisterKernelFunction(GX2SetGeometryTexture);
}

} // namespace gx2
