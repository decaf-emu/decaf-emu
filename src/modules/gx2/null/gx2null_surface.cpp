#include "../gx2.h"
#ifdef GX2_NULL

#include "../gx2_surface.h"

void
GX2CalcSurfaceSizeAndAlignment(GX2Surface *surface)
{
   surface->alignment = 4;
   surface->imageSize = 4 * surface->width * surface->height;
   surface->pitch = surface->width;
}

void
GX2CalcDepthBufferHiZInfo(GX2DepthBuffer *depthBuffer, be_val<uint32_t> *outSize, be_val<uint32_t> *outAlignment)
{
   *outSize = depthBuffer->surface.imageSize / (8 * 8);
   *outAlignment = 4;
}

void
GX2SetColorBuffer(GX2ColorBuffer *colorBuffer, uint32_t renderTarget)
{
}

void
GX2SetDepthBuffer(GX2DepthBuffer *depthBuffer)
{
}

void
GX2InitColorBufferRegs(GX2ColorBuffer *colorBuffer)
{
}

void
GX2InitDepthBufferRegs(GX2DepthBuffer *depthBuffer)
{
}

#endif
