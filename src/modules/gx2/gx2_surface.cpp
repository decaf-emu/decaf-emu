#include "gx2.h"
#include "gx2_surface.h"

void
GX2CalcSurfaceSizeAndAlignment(GX2Surface *surface)
{
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
   *outSize = depthBuffer->surface.imageSize / (8 * 8);
   *outAlignment = 4;
}

void
GX2SetColorBuffer(GX2ColorBuffer *colorBuffer, uint32_t unk1)
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
