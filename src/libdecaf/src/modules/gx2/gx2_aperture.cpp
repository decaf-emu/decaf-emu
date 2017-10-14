#include "kernel/kernel_memory.h"
#include "gx2_addrlib.h"
#include "gx2_aperture.h"
#include "gx2_surface.h"

#include <common/decaf_assert.h>
#include <common/teenyheap.h>
#include <libcpu/be2_struct.h>
#include <mutex>

namespace gx2
{

class ApertureManager
{
   static const auto MaxApertures = 32u;

   struct ActiveAperture
   {
      GX2Surface *surface;
      uint32_t level;
      uint32_t slice;
      uint8_t *address;
      uint32_t size;
   };

public:
   ~ApertureManager()
   {
      if (mHeap) {
         delete mHeap;
         kernel::freeTilingApertures();
      }
   }

   uint32_t
   allocate(GX2Surface *surface,
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
      gx2::internal::getSurfaceInfo(surface, level, &output);

      auto &aperture = mActiveApertures[id];
      aperture.address = reinterpret_cast<uint8_t *>(mHeap->alloc(output.sliceSize));
      aperture.size = output.sliceSize;
      aperture.surface = surface;
      aperture.level = level;
      aperture.slice = slice;

      // Untile existing to memory
      auto apertureSurface = GX2Surface { *surface };
      apertureSurface.mipmaps = nullptr;
      apertureSurface.image = nullptr;
      apertureSurface.tileMode = GX2TileMode::LinearSpecial;
      apertureSurface.swizzle &= 0xFFFF00FF;
      GX2CalcSurfaceSizeAndAlignment(&apertureSurface);

      gx2::internal::copySurface(surface, level, slice, &apertureSurface, level, slice, aperture.address, aperture.address);
      return id + 1;
   }

   void *
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
      auto apertureSurface = GX2Surface { *aperture.surface };
      apertureSurface.mipmaps = aperture.address;
      apertureSurface.image = aperture.address;
      apertureSurface.tileMode = GX2TileMode::LinearSpecial;
      apertureSurface.swizzle &= 0xFFFF00FF;
      GX2CalcSurfaceSizeAndAlignment(&apertureSurface);

      auto level = aperture.level;
      auto slice = aperture.slice;
      gx2::internal::copySurface(&apertureSurface, level, slice, aperture.surface, level, slice, aperture.surface->image, aperture.surface->mipmaps);

      mHeap->free(aperture.address);
      aperture.address = nullptr;
      aperture.size = 0;
   }

   bool
   lookupAperture(uint32_t address,
                  uint32_t *apertureBase,
                  uint32_t *apertureSize,
                  uint32_t *physBase)
   {
      std::unique_lock<std::mutex> lock(mMutex);

      for (auto &aperture : mActiveApertures) {
         if (!aperture.address) {
            continue;
         }

         auto start = mem::untranslate(aperture.address);
         auto end = start + aperture.size;

         if (address >= start && address < end) {
            if (apertureBase) {
               *apertureBase = start;
            }

            if (apertureSize) {
               *apertureSize = aperture.size;
            }

            if (physBase) {
               *physBase = aperture.surface->image.getAddress();
            }

            return true;
         }
      }

      return false;
   }

   bool
   isApertureAddress(cpu::VirtualAddress address)
   {
      return (address >= mMemoryBase && address < mMemoryBase + mMemorySize);
   }

private:
   void initialise()
   {
      auto range = kernel::initialiseTilingApertures();
      mMemoryBase = range.start;
      mMemorySize = range.size;
      mHeap = new TeenyHeap { virt_cast<void *>(mMemoryBase).getRawPointer(), mMemorySize };
   }

private:
   cpu::VirtualAddress mMemoryBase;
   uint32_t mMemorySize;
   std::mutex mMutex;
   TeenyHeap *mHeap = nullptr;
   std::array<ActiveAperture, MaxApertures> mActiveApertures;
};

static ApertureManager
sApertureManager;

void
GX2AllocateTilingApertureEx(GX2Surface *surface,
                            uint32_t level,
                            uint32_t depth,
                            GX2EndianSwapMode endian,
                            be_val<GX2ApertureHandle> *handle,
                            be_ptr<void> *address)
{
   auto id = sApertureManager.allocate(surface, level, depth, endian);
   auto addr = static_cast<void *>(nullptr);

   if (id != 0) {
      addr = sApertureManager.getAddress(id);
   }

   if (handle) {
      *handle = id;
   }

   if (address) {
      *address = addr;
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
isApertureAddress(uint32_t address)
{
   return sApertureManager.isApertureAddress(cpu::VirtualAddress { address });
}

bool
lookupAperture(uint32_t address,
               uint32_t *apertureBase,
               uint32_t *apertureSize,
               uint32_t *physBase)
{
   return sApertureManager.lookupAperture(address,
                                          apertureBase,
                                          apertureSize,
                                          physBase);
}

} // namespace internal

} // namespace gx2
