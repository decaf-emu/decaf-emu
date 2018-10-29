#pragma once
#include "gx2_enum.h"

#include <libcpu/be2_struct.h>
#include <libgpu/latte/latte_registers.h>

namespace cafe::gx2
{

/**
 * \defgroup gx2_surface Surface
 * \ingroup gx2
 * @{
 */

#pragma pack(push, 1)

struct GX2Surface
{
   be2_val<GX2SurfaceDim> dim;
   be2_val<uint32_t> width;
   be2_val<uint32_t> height;
   be2_val<uint32_t> depth;
   be2_val<uint32_t> mipLevels;
   be2_val<GX2SurfaceFormat> format;
   be2_val<GX2AAMode> aa;
   union
   {
      be2_val<GX2SurfaceUse> use;
      be2_val<GX2RResourceFlags> resourceFlags;
   };
   be2_val<uint32_t> imageSize;
   be2_virt_ptr<uint8_t> image;
   be2_val<uint32_t> mipmapSize;
   be2_virt_ptr<uint8_t> mipmaps;
   be2_val<GX2TileMode> tileMode;
   be2_val<uint32_t> swizzle;
   be2_val<uint32_t> alignment;
   be2_val<uint32_t> pitch;
   be2_array<uint32_t, 13> mipLevelOffset;
};
CHECK_OFFSET(GX2Surface, 0x00, dim);
CHECK_OFFSET(GX2Surface, 0x04, width);
CHECK_OFFSET(GX2Surface, 0x08, height);
CHECK_OFFSET(GX2Surface, 0x0C, depth);
CHECK_OFFSET(GX2Surface, 0x10, mipLevels);
CHECK_OFFSET(GX2Surface, 0x14, format);
CHECK_OFFSET(GX2Surface, 0x18, aa);
CHECK_OFFSET(GX2Surface, 0x1C, use);
CHECK_OFFSET(GX2Surface, 0x1C, resourceFlags);
CHECK_OFFSET(GX2Surface, 0x20, imageSize);
CHECK_OFFSET(GX2Surface, 0x24, image);
CHECK_OFFSET(GX2Surface, 0x28, mipmapSize);
CHECK_OFFSET(GX2Surface, 0x2C, mipmaps);
CHECK_OFFSET(GX2Surface, 0x30, tileMode);
CHECK_OFFSET(GX2Surface, 0x34, swizzle);
CHECK_OFFSET(GX2Surface, 0x38, alignment);
CHECK_OFFSET(GX2Surface, 0x3C, pitch);
CHECK_OFFSET(GX2Surface, 0x40, mipLevelOffset);
CHECK_SIZE(GX2Surface, 0x74);

struct GX2DepthBuffer
{
   be2_struct<GX2Surface> surface;
   be2_val<uint32_t> viewMip;
   be2_val<uint32_t> viewFirstSlice;
   be2_val<uint32_t> viewNumSlices;
   be2_virt_ptr<void> hiZPtr;
   be2_val<uint32_t> hiZSize;
   be2_val<float> depthClear;
   be2_val<uint32_t> stencilClear;

   struct
   {
      be2_val<latte::DB_DEPTH_SIZE> db_depth_size;
      be2_val<latte::DB_DEPTH_VIEW> db_depth_view;
      be2_val<latte::DB_DEPTH_INFO> db_depth_info;
      be2_val<latte::DB_HTILE_SURFACE> db_htile_surface;
      be2_val<latte::DB_PREFETCH_LIMIT> db_prefetch_limit;
      be2_val<latte::DB_PRELOAD_CONTROL> db_preload_control;
      be2_val<latte::PA_SU_POLY_OFFSET_DB_FMT_CNTL> pa_poly_offset_cntl;
   } regs;
};
CHECK_OFFSET(GX2DepthBuffer, 0x00, surface);
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
   be2_struct<GX2Surface> surface;
   be2_val<uint32_t> viewMip;
   be2_val<uint32_t> viewFirstSlice;
   be2_val<uint32_t> viewNumSlices;
   be2_virt_ptr<void> aaBuffer;
   be2_val<uint32_t> aaSize;

   struct
   {
      be2_val<latte::CB_COLORN_SIZE> cb_color_size;
      be2_val<latte::CB_COLORN_INFO> cb_color_info;
      be2_val<latte::CB_COLORN_VIEW> cb_color_view;
      be2_val<latte::CB_COLORN_MASK> cb_color_mask;
      be2_val<uint32_t> cmask_offset;
   } regs;
};
CHECK_OFFSET(GX2ColorBuffer, 0x00, surface);
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
GX2CalcSurfaceSizeAndAlignment(virt_ptr<GX2Surface> surface);

void
GX2CalcDepthBufferHiZInfo(virt_ptr<GX2DepthBuffer> depthBuffer,
                          virt_ptr<uint32_t> outSize,
                          virt_ptr<uint32_t> outAlignment);

void
GX2CalcColorBufferAuxInfo(virt_ptr<GX2ColorBuffer> colorBuffer,
                          virt_ptr<uint32_t> outSize,
                          virt_ptr<uint32_t> outAlignment);

void
GX2SetColorBuffer(virt_ptr<GX2ColorBuffer> colorBuffer,
                  GX2RenderTarget target);

void
GX2SetDepthBuffer(virt_ptr<GX2DepthBuffer> depthBuffer);

void
GX2InitColorBufferRegs(virt_ptr<GX2ColorBuffer> colorBuffer);

void
GX2InitDepthBufferRegs(virt_ptr<GX2DepthBuffer> depthBuffer);

void
GX2InitDepthBufferHiZEnable(virt_ptr<GX2DepthBuffer> depthBuffer,
                            BOOL enable);

uint32_t
GX2GetSurfaceSwizzle(virt_ptr<GX2Surface> surface);

uint32_t
GX2GetSurfaceSwizzleOffset(virt_ptr<GX2Surface> surface,
                           uint32_t level);

void
GX2SetSurfaceSwizzle(virt_ptr<GX2Surface> surface,
                     uint32_t swizzle);

uint32_t
GX2GetSurfaceMipPitch(virt_ptr<GX2Surface> surface,
                      uint32_t level);

uint32_t
GX2GetSurfaceMipSliceSize(virt_ptr<GX2Surface> surface,
                          uint32_t level);

void
GX2CopySurface(virt_ptr<GX2Surface> src,
               uint32_t srcLevel,
               uint32_t srcSlice,
               virt_ptr<GX2Surface> dst,
               uint32_t dstLevel,
               uint32_t dstSlice);

void
GX2ExpandDepthBuffer(virt_ptr<GX2DepthBuffer> buffer);

/** @} */

} // namespace cafe::gx2
