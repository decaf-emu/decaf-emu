#include "modules/gx2/gx2.h"
#ifdef GX2_DX12

#include "modules/gx2/gx2_surface.h"
#include "dx12_state.h"
#include "dx12_colorbuffer.h"

void
_GX2SetColorBuffer(GX2ColorBuffer *colorBuffer,
                  uint32_t renderTarget)
{
   gDX.state.colorBuffer[renderTarget] = colorBuffer;
}

void
GX2SetColorBuffer(GX2ColorBuffer *colorBuffer,
   uint32_t renderTarget)
{
   DX_DLCALL(_GX2SetColorBuffer, colorBuffer, renderTarget);
}

void
_GX2SetDepthBuffer(GX2DepthBuffer *depthBuffer)
{
   gDX.state.depthBuffer = depthBuffer;
}

void
GX2SetDepthBuffer(GX2DepthBuffer *depthBuffer)
{
   DX_DLCALL(_GX2SetDepthBuffer, depthBuffer);
}

void
GX2InitColorBufferRegs(GX2ColorBuffer *colorBuffer)
{
   // TODO: GX2InitColorBufferRegs
}

void
GX2InitDepthBufferRegs(GX2DepthBuffer *depthBuffer)
{
   // TODO: GX2InitDepthBufferRegs
}

#endif
