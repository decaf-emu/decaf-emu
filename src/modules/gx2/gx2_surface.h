#pragma once
#include "types.h"
#include "utils/be_val.h"
#include "utils/structsize.h"
#include "utils/virtual_ptr.h"
#include "gx2_enum.h"

#pragma pack(push, 1)

// GX2InitTextureRegs
// G2XRCreateSurface
struct GX2Surface
{
   be_val<GX2SurfaceDim::Value> dim; // "GX2_SURFACE_DIM_2D_MSAA or GX2_SURFACE_DIM_2D_MSAA_ARRAY" = 0 or 6?
   be_val<uint32_t> width;
   be_val<uint32_t> height;
   be_val<uint32_t> depth;
   be_val<uint32_t> mipLevels; // GX2CalcSurfaceSizeAndAlignment -> _GX2CalcNumLevels
   be_val<GX2SurfaceFormat::Value> format;
   be_val<GX2AAMode::Value> aa;
   union // Is this correct?? Union???
   {
      be_val<GX2SurfaceUse::Value> use; // GX2InitTextureRegs
      be_val<uint32_t> resourceFlags; // G2XRCreateSurface
   };
   be_val<uint32_t> imageSize;
   be_ptr<void> image;
   be_val<uint32_t> mipmapSize; // sizeof mipPtr
   be_ptr<void> mipmaps;
   be_val<GX2TileMode::Value> tileMode;
   be_val<uint32_t> swizzle; // GX2SetSurfaceSwizzle;
   be_val<uint32_t> alignment;
   be_val<uint32_t> pitch;
   UNKNOWN(0x74 - 0x40);
};
CHECK_OFFSET(GX2Surface, 0x0, dim);
CHECK_OFFSET(GX2Surface, 0x4, width);
CHECK_OFFSET(GX2Surface, 0x8, height);
CHECK_OFFSET(GX2Surface, 0xc, depth);
CHECK_OFFSET(GX2Surface, 0x10, mipLevels);
CHECK_OFFSET(GX2Surface, 0x14, format);
CHECK_OFFSET(GX2Surface, 0x18, aa);
CHECK_OFFSET(GX2Surface, 0x1c, use);
CHECK_OFFSET(GX2Surface, 0x1c, resourceFlags);
CHECK_OFFSET(GX2Surface, 0x20, imageSize);
CHECK_OFFSET(GX2Surface, 0x24, image);
CHECK_OFFSET(GX2Surface, 0x28, mipmapSize);
CHECK_OFFSET(GX2Surface, 0x2c, mipmaps);
CHECK_OFFSET(GX2Surface, 0x30, tileMode);
CHECK_OFFSET(GX2Surface, 0x34, swizzle);
CHECK_OFFSET(GX2Surface, 0x38, alignment);
CHECK_OFFSET(GX2Surface, 0x3C, pitch);
CHECK_SIZE(GX2Surface, 0x74);

struct GX2DepthBuffer
{
   GX2Surface surface;

   be_val<uint32_t> viewMip; // GX2ClearDepthStencilEx
   be_val<uint32_t> viewFirstSlice; // GX2ClearDepthStencilEx
   be_val<uint32_t> viewNumSlices; // GX2ClearDepthStencilEx
   be_ptr<void> hiZPtr; // GX2ExpandDepthBuffer
   be_val<uint32_t> hiZSize;
   UNKNOWN(36);
};
CHECK_OFFSET(GX2DepthBuffer, 0x74, viewMip);
CHECK_OFFSET(GX2DepthBuffer, 0x78, viewFirstSlice);
CHECK_OFFSET(GX2DepthBuffer, 0x7C, viewNumSlices);
CHECK_OFFSET(GX2DepthBuffer, 0x80, hiZPtr);
CHECK_OFFSET(GX2DepthBuffer, 0x84, hiZSize);
CHECK_SIZE(GX2DepthBuffer, 0xAC);

struct GX2ColorBuffer
{
   GX2Surface surface;

   be_val<uint32_t> viewMip; // GX2ClearBuffersEx
   be_val<uint32_t> viewFirstSlice; // GX2ClearBuffersEx
   be_val<uint32_t> viewNumSlices; // GX2ClearBuffersEx
   UNKNOWN(28);
};
CHECK_OFFSET(GX2ColorBuffer, 0x74, viewMip);
CHECK_OFFSET(GX2ColorBuffer, 0x78, viewFirstSlice);
CHECK_OFFSET(GX2ColorBuffer, 0x7C, viewNumSlices);
CHECK_SIZE(GX2ColorBuffer, 0x9C);

#pragma pack(pop)

void
GX2CalcSurfaceSizeAndAlignment(GX2Surface *surface);

void
GX2CalcDepthBufferHiZInfo(GX2DepthBuffer *depthBuffer, be_val<uint32_t> *outSize, be_val<uint32_t> *outAlignment);

void
GX2SetColorBuffer(GX2ColorBuffer *colorBuffer, uint32_t unk1);

void
GX2SetDepthBuffer(GX2DepthBuffer *depthBuffer);

void
GX2InitColorBufferRegs(GX2ColorBuffer *colorBuffer);

void
GX2InitDepthBufferRegs(GX2DepthBuffer *depthBuffer);
