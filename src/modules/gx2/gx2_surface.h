#pragma once
#include "types.h"
#include "gpu/latte_registers.h"
#include "utils/be_val.h"
#include "utils/structsize.h"
#include "utils/virtual_ptr.h"
#include "gx2_enum.h"

#pragma pack(push, 1)

struct GX2Surface
{
   be_val<GX2SurfaceDim::Value> dim;
   be_val<uint32_t> width;
   be_val<uint32_t> height;
   be_val<uint32_t> depth;
   be_val<uint32_t> mipLevels;
   be_val<GX2SurfaceFormat::Value> format;
   be_val<GX2AAMode::Value> aa;
   union // Is this correct?? Union???
   {
      be_val<GX2SurfaceUse::Value> use; // GX2InitTextureRegs
      be_val<uint32_t> resourceFlags; // G2XRCreateSurface
   };
   be_val<uint32_t> imageSize;
   be_ptr<void> image;
   be_val<uint32_t> mipmapSize;
   be_ptr<void> mipmaps;
   be_val<GX2TileMode::Value> tileMode;
   be_val<uint32_t> swizzle;
   be_val<uint32_t> alignment;
   be_val<uint32_t> pitch;
   be_val<uint32_t> mipLevelOffset[13];
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
CHECK_OFFSET(GX2Surface, 0x40, mipLevelOffset);
CHECK_SIZE(GX2Surface, 0x74);

struct GX2DepthBuffer
{
   GX2Surface surface;

   be_val<uint32_t> viewMip;
   be_val<uint32_t> viewFirstSlice;
   be_val<uint32_t> viewNumSlices;
   be_ptr<void> hiZPtr;
   be_val<uint32_t> hiZSize;
   be_val<float> depthClear;
   be_val<uint32_t> stencilClear;
   UNKNOWN(28);
};
CHECK_OFFSET(GX2DepthBuffer, 0x74, viewMip);
CHECK_OFFSET(GX2DepthBuffer, 0x78, viewFirstSlice);
CHECK_OFFSET(GX2DepthBuffer, 0x7C, viewNumSlices);
CHECK_OFFSET(GX2DepthBuffer, 0x80, hiZPtr);
CHECK_OFFSET(GX2DepthBuffer, 0x84, hiZSize);
CHECK_OFFSET(GX2DepthBuffer, 0x88, depthClear);
CHECK_OFFSET(GX2DepthBuffer, 0x8C, stencilClear);
CHECK_SIZE(GX2DepthBuffer, 0xAC);

struct GX2ColorBuffer
{
   GX2Surface surface;

   be_val<uint32_t> viewMip;
   be_val<uint32_t> viewFirstSlice;
   be_val<uint32_t> viewNumSlices;
   be_ptr<void> aaBuffer;
   be_val<uint32_t> aaSize;
   struct
   {
      latte::CB_COLORN_SIZE cb_color_size;
      latte::CB_COLORN_INFO cb_color_info;
      latte::CB_COLORN_VIEW cb_color_view;
      latte::CB_COLORN_MASK cb_color_mask;
      uint32_t cmask_offset;
   } regs;
};
CHECK_OFFSET(GX2ColorBuffer, 0x74, viewMip);
CHECK_OFFSET(GX2ColorBuffer, 0x78, viewFirstSlice);
CHECK_OFFSET(GX2ColorBuffer, 0x7C, viewNumSlices);
CHECK_OFFSET(GX2ColorBuffer, 0x80, aaBuffer);
CHECK_OFFSET(GX2ColorBuffer, 0x84, aaSize);
CHECK_OFFSET(GX2ColorBuffer, 0x88, regs.cb_color_size);
CHECK_OFFSET(GX2ColorBuffer, 0x8C, regs.cb_color_info);
CHECK_OFFSET(GX2ColorBuffer, 0x90, regs.cb_color_view);
CHECK_OFFSET(GX2ColorBuffer, 0x94, regs.cb_color_mask);
CHECK_OFFSET(GX2ColorBuffer, 0x98, regs.cmask_offset);
CHECK_SIZE(GX2ColorBuffer, 0x9C);

#pragma pack(pop)

void
GX2CalcSurfaceSizeAndAlignment(GX2Surface *surface);

void
GX2CalcDepthBufferHiZInfo(GX2DepthBuffer *depthBuffer, be_val<uint32_t> *outSize, be_val<uint32_t> *outAlignment);

void
GX2SetColorBuffer(GX2ColorBuffer *colorBuffer, GX2RenderTarget::Value target);

void
GX2SetDepthBuffer(GX2DepthBuffer *depthBuffer);

void
GX2InitColorBufferRegs(GX2ColorBuffer *colorBuffer);

void
GX2InitDepthBufferRegs(GX2DepthBuffer *depthBuffer);
