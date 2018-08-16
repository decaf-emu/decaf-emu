#include "gx2.h"
#include "gx2_addrlib.h"
#include "gx2_aperture.h"
#include "gx2_surface.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/kernel/cafe_kernel_mmu.h"

#include <common/decaf_assert.h>
#include <common/teenyheap.h>
#include <libcpu/be2_struct.h>
#include <mutex>

namespace cafe::gx2
{

class ApertureManager
{
   static const auto MaxApertures = 32u;

   struct ActiveAperture
   {
      virt_ptr<GX2Surface> surface;
      uint32_t level;
      uint32_t slice;
      virt_ptr<uint8_t> address;
      uint32_t size;
   };

public:
   ~ApertureManager()
   {
      if (mHeap) {
         delete mHeap;
         // kernel::freeTilingApertures();
      }
   }

   uint32_t
   allocate(virt_ptr<GX2Surface> surface,
            uint32_t level,
            uint32_t slice,
            GX2EndianSwapMode endian)
   {
      std::unique_lock<std::mutex> lock(mMutex);

      // Find a free slot
      auto id = static_cast<uint32_t>(mActiveApertures.size());

      for (auto i = 0u; i < mActiveApertures.size(); ++i) {
         if (mActiveApertures[i].address == nullptr) {
            id = i;
            break;
         }
      }

      if (id == mActiveApertures.size()) {
         return 0;
      }

      if (!mHeap) {
         initialise();
      }

      // Allocate data
      ADDR_COMPUTE_SURFACE_INFO_OUTPUT output;
      gx2::internal::getSurfaceInfo(surface.getRawPointer(), level, &output);

      auto &aperture = mActiveApertures[id];
      aperture.address = virt_cast<uint8_t *>(cpu::translate(mHeap->alloc(output.sliceSize)));
      aperture.size = output.sliceSize;
      aperture.surface = surface;
      aperture.level = level;
      aperture.slice = slice;

      // Untile existing to memory
      StackObject<GX2Surface> apertureSurface;
      *apertureSurface = *surface;
      apertureSurface->mipmaps = nullptr;
      apertureSurface->image = nullptr;
      apertureSurface->tileMode = GX2TileMode::LinearSpecial;
      apertureSurface->swizzle &= 0xFFFF00FF;
      GX2CalcSurfaceSizeAndAlignment(apertureSurface);

      gx2::internal::copySurface(surface.getRawPointer(), level, slice,
                                 apertureSurface.getRawPointer(), level, slice,
                                 aperture.address.getRawPointer(),
                                 aperture.address.getRawPointer());
      return id + 1;
   }

   virt_ptr<void>
   getAddress(uint32_t id)
   {
      std::unique_lock<std::mutex> lock(mMutex);

      decaf_check(id >= 1 && id <= mActiveApertures.size());
      return mActiveApertures[id - 1].address;
   }

   void
   free(uint32_t id)
   {
      std::unique_lock<std::mutex> lock(mMutex);

      decaf_check(id >= 1 && id <= mActiveApertures.size());
      auto &aperture = mActiveApertures[id - 1];

      // Retile to original memory
      StackObject<GX2Surface> apertureSurface;
      *apertureSurface = *aperture.surface;
      apertureSurface->mipmaps = aperture.address;
      apertureSurface->image = aperture.address;
      apertureSurface->tileMode = GX2TileMode::LinearSpecial;
      apertureSurface->swizzle &= 0xFFFF00FF;
      GX2CalcSurfaceSizeAndAlignment(apertureSurface);

      auto level = aperture.level;
      auto slice = aperture.slice;
      gx2::internal::copySurface(apertureSurface.getRawPointer(),
                                 level, slice,
                                 aperture.surface.getRawPointer(),
                                 level, slice,
                                 aperture.surface->image.getRawPointer(),
                                 aperture.surface->mipmaps.getRawPointer());

      mHeap->free(aperture.address.getRawPointer());
      aperture.address = nullptr;
      aperture.size = 0;
   }

   bool
   lookupAperture(virt_addr address,
                  virt_addr *apertureBase,
                  uint32_t *apertureSize,
                  virt_addr *physBase)
   {
      std::unique_lock<std::mutex> lock(mMutex);

      for (auto &aperture : mActiveApertures) {
         if (!aperture.address) {
            continue;
         }

         auto start = virt_cast<virt_addr>(aperture.address);
         auto end = start + aperture.size;

         if (address >= start && address < end) {
            if (apertureBase) {
               *apertureBase = start;
            }

            if (apertureSize) {
               *apertureSize = aperture.size;
            }

            if (physBase) {
               *physBase = virt_cast<virt_addr>(aperture.surface->image);
            }

            return true;
         }
      }

      return false;
   }

   bool
   isApertureAddress(virt_addr address)
   {
      return (address >= mMemoryBase && address < mMemoryBase + mMemorySize);
   }

private:
   void initialise()
   {
      //auto range = kernel::initialiseTilingApertures();
      mMemoryBase = virt_addr { 0 }; // range.start;
      mMemorySize = 0; // range.size;
      mHeap = new TeenyHeap { virt_cast<void *>(mMemoryBase).getRawPointer(), mMemorySize };
   }

private:
   virt_addr mMemoryBase;
   uint32_t mMemorySize;
   std::mutex mMutex;
   TeenyHeap *mHeap = nullptr;
   std::array<ActiveAperture, MaxApertures> mActiveApertures;
};

static ApertureManager
sApertureManager;

void
GX2AllocateTilingApertureEx(virt_ptr<GX2Surface> surface,
                            uint32_t level,
                            uint32_t depth,
                            GX2EndianSwapMode endian,
                            virt_ptr<GX2ApertureHandle> outHandle,
                            virt_ptr<virt_ptr<void>> outAddress)
{
   auto id = sApertureManager.allocate(surface, level, depth, endian);
   auto addr = virt_ptr<void> { nullptr };

   if (id != 0) {
      addr = sApertureManager.getAddress(id);
   }

   if (outHandle) {
      *outHandle = id;
   }

   if (outAddress) {
      *outAddress = addr;
   }
}

void
GX2FreeTilingAperture(GX2ApertureHandle handle)
{
   sApertureManager.free(handle);
}

namespace internal
{

bool
isApertureAddress(virt_addr address)
{
   return sApertureManager.isApertureAddress(address);
}

bool
lookupAperture(virt_addr address,
               virt_addr *outApertureBase,
               uint32_t *outApertureSize,
               virt_addr *outPhysBase)
{
   return sApertureManager.lookupAperture(address,
                                          outApertureBase,
                                          outApertureSize,
                                          outPhysBase);
}

} // namespace internal

void
Library::registerApertureSymbols()
{
   RegisterFunctionExport(GX2AllocateTilingApertureEx);
   RegisterFunctionExport(GX2FreeTilingAperture);
}

} // namespace cafe::gx2
