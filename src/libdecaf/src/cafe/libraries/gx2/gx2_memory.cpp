#include "gx2.h"
#include "gx2_aperture.h"
#include "gx2_memory.h"
#include "gx2_state.h"
#include "gx2_internal_cbpool.h"
#include "cafe/libraries/coreinit/coreinit_cache.h"

#include <common/align.h>
#include <common/log.h>
#include <fmt/format.h>

using namespace cafe::coreinit;

namespace cafe::gx2
{

void
GX2Invalidate(GX2InvalidateMode mode,
              virt_ptr<void> buffer,
              uint32_t size)
{
   if (!mode) {
      return;
   }

   auto addr = virt_cast<virt_addr>(buffer);

   if (size != -1) {
      size = align_up(size, 0x100);
   }

   // If the address is in the aperture range, translate it to the
   //  corresponding physical address.  We have to do this synchronously
   //  because by the time the graphics driver receives the packet, the
   //  aperture mapping may no longer be valid.
   if (internal::isApertureAddress(addr)) {
      // Convert from aperture address to physical address
      //  (we assume a single invalidate won't cover multiple apertures)
      virt_addr apertureBase, physBase;
      uint32_t apertureSize;

      if (!internal::lookupAperture(addr, &apertureBase, &apertureSize, &physBase)) {
         gLog->warn("GX2Invalidate on unmapped aperture address 0x{:X}", addr);
         return;
      }

      decaf_assert(addr + size <= apertureBase + apertureSize,
                   fmt::format("GX2Invalidate over multiple apertures (0x{:X} + 0x{:X})", addr, size));

      // Invalidate the entire aperture (since the data layout is different)
      addr = physBase;
      size = apertureSize;
   }

   if (mode & GX2InvalidateMode::CPU) {
      DCFlushRange(virt_cast<void *>(addr), size);
   }

   if (mode != GX2InvalidateMode::CPU) {
      auto cp_coher_cntl = latte::CP_COHER_CNTL::get(0)
         .ENGINE_ME(true);

      if (!addr && size == -1 && (mode & 0xF)) {
         cp_coher_cntl = cp_coher_cntl
            .FULL_CACHE_ENA(true);
      }

      if ((mode & GX2InvalidateMode::Texture) || (mode & GX2InvalidateMode::AttributeBuffer)) {
         cp_coher_cntl = cp_coher_cntl
            .TC_ACTION_ENA(true);
      }

      if (mode & GX2InvalidateMode::UniformBlock) {
         cp_coher_cntl = cp_coher_cntl
            .TC_ACTION_ENA(true)
            .SH_ACTION_ENA(true);
      }

      if (mode & GX2InvalidateMode::Shader) {
         cp_coher_cntl = cp_coher_cntl
            .SH_ACTION_ENA(true);
      }

      if (mode & GX2InvalidateMode::ColorBuffer) {
         cp_coher_cntl = cp_coher_cntl
            .CB0_DEST_BASE_ENA(true)
            .CB1_DEST_BASE_ENA(true)
            .CB2_DEST_BASE_ENA(true)
            .CB3_DEST_BASE_ENA(true)
            .CB4_DEST_BASE_ENA(true)
            .CB5_DEST_BASE_ENA(true)
            .CB6_DEST_BASE_ENA(true)
            .CB7_DEST_BASE_ENA(true)
            .CB_ACTION_ENA(true);
      }

      if (mode & GX2InvalidateMode::DepthBuffer) {
         cp_coher_cntl = cp_coher_cntl
            .DB_DEST_BASE_ENA(true)
            .DB_ACTION_ENA(true);
      }

      if (mode & GX2InvalidateMode::StreamOutBuffer) {
         cp_coher_cntl = cp_coher_cntl
            .SO0_DEST_BASE_ENA(true)
            .SO1_DEST_BASE_ENA(true)
            .SO2_DEST_BASE_ENA(true)
            .SO3_DEST_BASE_ENA(true)
            .SX_ACTION_ENA(true);
      }

      if (mode & GX2InvalidateMode::ExportBuffer) {
         cp_coher_cntl = cp_coher_cntl
            .DEST_BASE_0_ENA(true)
            .TC_ACTION_ENA(true)
            .CB_ACTION_ENA(true)
            .DB_ACTION_ENA(true)
            .SX_ACTION_ENA(true);
      }

      internal::writePM4(latte::pm4::SurfaceSync {
         cp_coher_cntl,
         size >> 8,
         addr >> 8,
         4
      });
   }
}

void
Library::registerMemorySymbols()
{
   RegisterFunctionExport(GX2Invalidate);
}

} // namespace cafe::gx2
