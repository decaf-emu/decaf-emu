#pragma once
#include "common/types.h"
#include "gpu/latte_registers.h"
#include "common/be_array.h"
#include "common/be_val.h"
#include "common/structsize.h"
#include "virtual_ptr.h"
#include "gx2_enum.h"

namespace gx2
{

#pragma pack(push, 1)

struct GX2Surface
{
   GX2Surface()
   {
   }

   be_val<GX2SurfaceDim> dim;
   be_val<uint32_t> width;
   be_val<uint32_t> height;
   be_val<uint32_t> depth;
   be_val<uint32_t> mipLevels;
   be_val<GX2SurfaceFormat> format;
   be_val<GX2AAMode> aa;
   union
   {
      be_val<GX2SurfaceUse> use;
      be_val<GX2RResourceFlags> resourceFlags;
   };
   be_val<uint32_t> imageSize;
   be_ptr<uint8_t> image;
   be_val<uint32_t> mipmapSize;
   be_ptr<uint8_t> mipmaps;
   be_val<GX2TileMode> tileMode;
   be_val<uint32_t> swizzle;
   be_val<uint32_t> alignment;
   be_val<uint32_t> pitch;
   be_array<uint32_t, 13> mipLevelOffset;
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

   struct
   {
      be_val<latte::DB_DEPTH_SIZE> db_depth_size;
      be_val<latte::DB_DEPTH_VIEW> db_depth_view;
      be_val<latte::DB_DEPTH_INFO> db_depth_info;
      be_val<latte::DB_HTILE_SURFACE> db_htile_surface;
      be_val<latte::DB_PREFETCH_LIMIT> db_prefetch_limit;
      be_val<latte::DB_PRELOAD_CONTROL> db_preload_control;
      be_val<latte::PA_SU_POLY_OFFSET_DB_FMT_CNTL> pa_poly_offset_cntl;
   } regs;
};
CHECK_OFFSET(GX2DepthBuffer, 0x74, viewMip);
CHECK_OFFSET(GX2DepthBuffer, 0x78, viewFirstSlice);
CHECK_OFFSET(GX2DepthBuffer, 0x7C, viewNumSlices);
CHECK_OFFSET(GX2DepthBuffer, 0x80, hiZPtr);
CHECK_OFFSET(GX2DepthBuffer, 0x84, hiZSize);
CHECK_OFFSET(GX2DepthBuffer, 0x88, depthClear);
CHECK_OFFSET(GX2DepthBuffer, 0x8C, stencilClear);
CHECK_OFFSET(GX2DepthBuffer, 0x90, regs.db_depth_size);
CHECK_OFFSET(GX2DepthBuffer, 0x94, regs.db_depth_view);
CHECK_OFFSET(GX2DepthBuffer, 0x98, regs.db_depth_info);
CHECK_OFFSET(GX2DepthBuffer, 0x9C, regs.db_htile_surface);
CHECK_OFFSET(GX2DepthBuffer, 0xA0, regs.db_prefetch_limit);
CHECK_OFFSET(GX2DepthBuffer, 0xA4, regs.db_preload_control);
CHECK_OFFSET(GX2DepthBuffer, 0xA8, regs.pa_poly_offset_cntl);
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
      be_val<latte::CB_COLORN_SIZE> cb_color_size;
      be_val<latte::CB_COLORN_INFO> cb_color_info;
      be_val<latte::CB_COLORN_VIEW> cb_color_view;
      be_val<latte::CB_COLORN_MASK> cb_color_mask;
      be_val<uint32_t> cmask_offset;
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
GX2CalcDepthBufferHiZInfo(GX2DepthBuffer *depthBuffer,
                          be_val<uint32_t> *outSize,
                          be_val<uint32_t> *outAlignment);

void
GX2CalcColorBufferAuxInfo(GX2Surface *surface,
                          be_val<uint32_t> *outSize,
                          be_val<uint32_t> *outAlignment);

void
GX2SetColorBuffer(GX2ColorBuffer *colorBuffer,
                  GX2RenderTarget target);

void
GX2SetDepthBuffer(GX2DepthBuffer *depthBuffer);

void
GX2InitColorBufferRegs(GX2ColorBuffer *colorBuffer);

void
GX2InitDepthBufferRegs(GX2DepthBuffer *depthBuffer);

void
GX2InitDepthBufferHiZEnable(GX2DepthBuffer *depthBuffer,
                            BOOL enable);

uint32_t
GX2GetSurfaceSwizzle(GX2Surface *surface);

void
GX2SetSurfaceSwizzle(GX2Surface *surface, uint32_t swizzle);

uint32_t
GX2GetSurfaceMipPitch(GX2Surface *surface, uint32_t level);

void
GX2CopySurface(GX2Surface *src,
               uint32_t srcLevel,
               uint32_t srcSlice,
               GX2Surface *dst,
               uint32_t dstLevel,
               uint32_t dstSlice);

void
GX2ExpandDepthBuffer(GX2DepthBuffer *buffer);

} // namespace gx2
