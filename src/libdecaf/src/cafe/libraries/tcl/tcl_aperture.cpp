#include "tcl.h"
#include "tcl_aperture.h"
#include "tcl_driver.h"

#include "cafe/cafe_tinyheap.h"
#include "cafe/libraries/coreinit/coreinit_memory.h"

namespace cafe::tcl
{

using namespace cafe::coreinit;

struct AllocatedAperture
{
   be2_val<phys_addr> surface;
   be2_virt_ptr<void> apertureMemory;
};

struct StaticApertureData
{
   be2_val<virt_addr> baseVirtualAddress;
   be2_val<uint32_t> allocatedMask;

   be2_array<AllocatedAperture, 32> apertures;
   be2_array<uint8_t, TinyHeapHeaderSize + 32 * TinyHeapBlockSize> trackingHeap;
};

static virt_ptr<StaticApertureData>
sApertureData = nullptr;

TCLStatus
TCLAllocTilingAperture(phys_addr addr,
                       uint32_t pitch,
                       uint32_t height,
                       uint32_t bytesPerPixel,
                       uint32_t tileMode,
                       uint32_t endian,
                       virt_ptr<TCLApertureHandle> outHandle,
                       virt_ptr<virt_addr> outAddress)
{
   if (!internal::tclDriverInitialised()) {
      return TCLStatus::NotInitialised;
   }

   auto handle = 32u;
   for (auto i = 0u; i < 32; ++i) {
      if (sApertureData->allocatedMask & (1 << i)) {
         continue;
      }

      handle = i;
      break;
   }

   if (handle >= 32) {
      return TCLStatus::OutOfMemory;
   }

   auto ptr = virt_ptr<void> { nullptr };
   auto size = pitch * height * bytesPerPixel;
   if (TinyHeap_Alloc(virt_cast<TinyHeap *>(virt_addrof(sApertureData->trackingHeap)),
                      size, 256, &ptr) != TinyHeapError::OK) {
      return TCLStatus::OutOfMemory;
   }

   auto address = sApertureData->baseVirtualAddress
                  + static_cast<uint32_t>(virt_cast<virt_addr>(ptr));
   sApertureData->allocatedMask |= 1 << handle;
   sApertureData->apertures[handle].surface = addr;
   sApertureData->apertures[handle].apertureMemory = ptr;

   if (outHandle) {
      *outHandle = handle;
   }

   if (outAddress) {
      *outAddress = address;
   }

   return TCLStatus::OK;
}

TCLStatus
TCLFreeTilingAperture(TCLApertureHandle handle)
{
   if (!internal::tclDriverInitialised()) {
      return TCLStatus::NotInitialised;
   }

   if (handle >= 32) {
      return TCLStatus::InvalidArg;
   }

   if (!(sApertureData->allocatedMask & (1 << handle))) {
      return TCLStatus::InvalidArg;
   }

   TinyHeap_Free(virt_cast<TinyHeap *>(virt_addrof(sApertureData->trackingHeap)),
                 sApertureData->apertures[handle].apertureMemory);

   sApertureData->apertures[handle].apertureMemory = nullptr;
   sApertureData->apertures[handle].surface = phys_addr { 0 };
   sApertureData->allocatedMask &= ~(1 << handle);
   return TCLStatus::OK;
}

namespace internal
{

void
initialiseApertures()
{
   // Create a heap
   TinyHeap_Setup(virt_cast<TinyHeap *>(virt_addrof(sApertureData->trackingHeap)),
                  sApertureData->trackingHeap.size(),
                  nullptr,
                  0x02000000);

   sApertureData->baseVirtualAddress = OSPhysicalToEffectiveUncached(phys_addr { 0xD0000000 });
}

} // namespace internal

void
Library::registerApertureSymbols()
{
   RegisterFunctionExport(TCLAllocTilingAperture);
   RegisterFunctionExport(TCLFreeTilingAperture);

   RegisterDataInternal(sApertureData);
}

} // namespace cafe::tcl
