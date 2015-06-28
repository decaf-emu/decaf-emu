#include "gx2.h"
#include "gx2_surface.h"

Log::Output &
operator <<(Log::Output &out, GX2SurfaceDim dim)
{
   switch (dim) {
   case GX2SurfaceDim::Texture2D:
      out << "Texture2D";
      break;
   case GX2SurfaceDim::Texture2DMSAA:
      out << "Texture2DMSAA";
      break;
   case GX2SurfaceDim::Texture2DMSAAArray:
      out << "Texture2DMSAAArray";
      break;
   default:
      out << "Unknown (" << static_cast<uint32_t>(dim) << ")";
   }

   return out;
}

Log::Output &
operator <<(Log::Output &out, GX2SurfaceFormat format)
{
   switch (format) {
   case GX2SurfaceFormat::R8G8B8A8:
      out << "R8G8B8A8";
      break;
      break;
   default:
      out << "Unknown (" << static_cast<uint32_t>(format) << ")";
   }

   return out;
}

Log::Output &
operator <<(Log::Output &out, GX2AAMode mode)
{
   switch (mode) {
   case GX2AAMode::Mode1X:
      out << "Mode1X";
      break;
      break;
   default:
      out << "Unknown (" << static_cast<uint32_t>(mode) << ")";
   }

   return out;
}

void
GX2CalcSurfaceSizeAndAlignment(GX2Surface *surface)
{
   surface->alignment = 4;
   surface->imageSize = 4 * surface->width * surface->height;
   surface->pitch = surface->width;

   xLog() << "dim: " << static_cast<GX2SurfaceDim>(surface->dim);
   xLog() << "width: " << surface->width;
   xLog() << "height: " << surface->height;
   xLog() << "depth: " << surface->depth;
   xLog() << "mipLevels: " << surface->mipLevels;
   xLog() << "format: " << static_cast<GX2SurfaceFormat>(surface->format);
   xLog() << "aa: " << static_cast<GX2AAMode>(surface->aa);
   xLog() << "resourceFlags: " << surface->resourceFlags;
   xLog() << "imageSize: " << surface->imageSize;
   xLog() << "image: " << Log::hex(static_cast<uint32_t>(surface->image));
   xLog() << "mipmapSize: " << surface->mipmapSize;
   xLog() << "mipmaps: " << Log::hex(static_cast<uint32_t>(surface->mipmaps));
   xLog() << "tileMode: " << surface->tileMode;
   xLog() << "swizzle: " << surface->swizzle;
   xLog() << "alignment: " << surface->alignment;
   xLog() << "pitch: " << surface->pitch;
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
   RegisterSystemFunction(GX2CalcSurfaceSizeAndAlignment);
   RegisterSystemFunction(GX2CalcDepthBufferHiZInfo);
   RegisterSystemFunction(GX2SetColorBuffer);
   RegisterSystemFunction(GX2SetDepthBuffer);
   RegisterSystemFunction(GX2InitColorBufferRegs);
   RegisterSystemFunction(GX2InitDepthBufferRegs);
}
