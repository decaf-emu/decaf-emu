#include "../gx2.h"
#ifdef GX2_DX12

#include "../gx2_surface.h"
#include "dx12_state.h"
#include "dx12_colorbuffer.h"

void
GX2CalcSurfaceSizeAndAlignment(GX2Surface *surface)
{
   // TODO: GX2CalcSurfaceSizeAndAlignment
   surface->alignment = 4;
   surface->imageSize = 4 * surface->width * surface->height;
   surface->pitch = surface->width;

   gLog->debug("dim: {}", surface->dim);
   gLog->debug("width: {}", surface->width);
   gLog->debug("height: {}", surface->height);
   gLog->debug("depth: {}", surface->depth);
   gLog->debug("mipLevels: {}", surface->mipLevels);
   gLog->debug("format: {}", surface->format);
   gLog->debug("aa: {}", surface->aa);
   gLog->debug("resourceFlags: {}", surface->resourceFlags);
   gLog->debug("imageSize: {}", surface->imageSize);
   gLog->debug("image: {}", surface->image.getPointer());
   gLog->debug("mipmapSize: {}", surface->mipmapSize);
   gLog->debug("mipmaps: {}", surface->mipmaps.getPointer());
   gLog->debug("tileMode: {}", surface->tileMode);
   gLog->debug("swizzle: {}", surface->swizzle);
   gLog->debug("alignment: {}", surface->alignment);
   gLog->debug("pitch: {}", surface->pitch);
}

void
GX2CalcDepthBufferHiZInfo(GX2DepthBuffer *depthBuffer, be_val<uint32_t> *outSize, be_val<uint32_t> *outAlignment)
{
   // TODO: GX2CalcDepthBufferHiZInfo
   *outSize = depthBuffer->surface.imageSize / (8 * 8);
   *outAlignment = 4;
}

void
GX2SetColorBuffer(GX2ColorBuffer *colorBuffer, uint32_t renderTarget)
{
   gDX.state.colorBuffer[renderTarget] = colorBuffer;
}

void
GX2SetDepthBuffer(GX2DepthBuffer *depthBuffer)
{
   gDX.state.depthBuffer = depthBuffer;
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
