#include "gx2.h"
#include "gx2_memory.h"
#include "gx2_shaders.h"
#include "gx2r_buffer.h"
#include "gx2r_memory.h"
#include "gx2r_resource.h"
#include "cafe/libraries/coreinit/coreinit_cache.h"
#include "cafe/libraries/cafe_hle_stub.h"

#include <common/align.h>
#include <common/log.h>

namespace cafe::gx2
{

using namespace coreinit;

uint32_t
GX2RGetBufferAlignment(GX2RResourceFlags flags)
{
   return 256;
}

uint32_t
GX2RGetBufferAllocationSize(virt_ptr<GX2RBuffer> buffer)
{
   return align_up(buffer->elemCount * buffer->elemSize, 64);
}

void
GX2RSetBufferName(virt_ptr<GX2RBuffer> buffer,
                  virt_ptr<const char> name)
{
   decaf_warn_stub();
}

BOOL
GX2RBufferExists(virt_ptr<GX2RBuffer> buffer)
{
   return (buffer && buffer->buffer) ? TRUE : FALSE;
}

BOOL
GX2RCreateBuffer(virt_ptr<GX2RBuffer> buffer)
{
   decaf_check(!buffer->buffer);

   auto align = GX2RGetBufferAlignment(buffer->flags);
   auto size = GX2RGetBufferAllocationSize(buffer);

   buffer->flags &= ~GX2RResourceFlags::Locked;
   buffer->flags |= GX2RResourceFlags::Gx2rAllocated;

   buffer->buffer = gx2::internal::gx2rAlloc(buffer->flags, size, align);

   if (!buffer->buffer) {
      return FALSE;
   }

   // TODO: GX2NotifyMemAlloc(buffer->buffer, size, align);

   // Check if we need to invalidate the buffer
   if ((buffer->flags & GX2RResourceFlags::UsageGpuWrite) ||
       (buffer->flags & GX2RResourceFlags::UsageDmaWrite)) {
      DCInvalidateRange(buffer->buffer, size);
   }

   return TRUE;
}

BOOL
GX2RCreateBufferUserMemory(virt_ptr<GX2RBuffer> buffer,
                           virt_ptr<void> memory,
                           uint32_t size)
{
   decaf_check(buffer);
   decaf_check(memory);
   buffer->buffer = memory;
   buffer->flags &= ~GX2RResourceFlags::Locked;
   buffer->flags &= ~GX2RResourceFlags::Gx2rAllocated;

   // Check if we need to invalidate the buffer
   if ((buffer->flags & GX2RResourceFlags::UsageGpuWrite) ||
       (buffer->flags & GX2RResourceFlags::UsageDmaWrite)) {
      DCInvalidateRange(buffer->buffer, buffer->elemCount * buffer->elemSize);
   }

   // TODO: GX2NotifyMemAlloc(buffer->buffer, size, GX2RGetBufferAlignment(buffer->flags));
   return TRUE;
}

void
GX2RDestroyBufferEx(virt_ptr<GX2RBuffer> buffer,
                    GX2RResourceFlags flags)
{
   if (!buffer || !buffer->buffer) {
      return;
   }

   flags = internal::getOptionFlags(flags);

   if (!GX2RIsUserMemory(buffer->flags) && !(flags & GX2RResourceFlags::DestroyNoFree)) {
      gx2::internal::gx2rFree(flags, buffer->buffer);
      buffer->buffer = nullptr;
   }

   // TODO: GX2NotifyMemFree(buffer->buffer)
}

void
GX2RInvalidateBuffer(virt_ptr<GX2RBuffer> buffer,
                     GX2RResourceFlags flags)
{
   flags = internal::getOptionFlags(flags);
   flags = static_cast<GX2RResourceFlags>(flags | (buffer->flags & ~0xF80000));

   GX2RInvalidateMemory(flags,
                        buffer->buffer,
                        buffer->elemSize * buffer->elemCount);
}

virt_ptr<void>
GX2RLockBufferEx(virt_ptr<GX2RBuffer> buffer,
                 GX2RResourceFlags flags)
{
   flags = buffer->flags | internal::getOptionFlags(flags);

   // Update buffer flags
   buffer->flags |= GX2RResourceFlags::Locked;
   buffer->flags |= flags & GX2RResourceFlags::LockedReadOnly;

   // Check if we need to invalidate the buffer
   if ((flags & GX2RResourceFlags::UsageGpuWrite) ||
       (flags & GX2RResourceFlags::UsageDmaWrite)) {
      if (!(flags & GX2RResourceFlags::DisableCpuInvalidate) &&
          !(flags & GX2RResourceFlags::LockedReadOnly)) {
         DCInvalidateRange(buffer->buffer,
                           buffer->elemCount * buffer->elemSize);
      }
   }

   // Return buffer pointer
   return buffer->buffer;
}

void
GX2RUnlockBufferEx(virt_ptr<GX2RBuffer> buffer,
                   GX2RResourceFlags flags)
{
   flags = internal::getOptionFlags(flags);

   // Invalidate the GPU buffer only if it was not read only locked
   if (!(buffer->flags & GX2RResourceFlags::LockedReadOnly)) {
      GX2RInvalidateBuffer(buffer, flags);
   }

   // Update buffer flags
   buffer->flags &= ~GX2RResourceFlags::Locked;
   buffer->flags &= ~GX2RResourceFlags::LockedReadOnly;
}

void
GX2RSetStreamOutBuffer(uint32_t index,
                       virt_ptr<GX2OutputStream> stream)
{
   GX2SetStreamOutBuffer(index, stream);
}

void
Library::registerGx2rBufferSymbols()
{
   RegisterFunctionExport(GX2RGetBufferAlignment);
   RegisterFunctionExport(GX2RGetBufferAllocationSize);
   RegisterFunctionExport(GX2RSetBufferName);
   RegisterFunctionExport(GX2RBufferExists);
   RegisterFunctionExport(GX2RCreateBuffer);
   RegisterFunctionExport(GX2RCreateBufferUserMemory);
   RegisterFunctionExport(GX2RDestroyBufferEx);
   RegisterFunctionExport(GX2RInvalidateBuffer);
   RegisterFunctionExport(GX2RLockBufferEx);
   RegisterFunctionExport(GX2RUnlockBufferEx);
   RegisterFunctionExport(GX2RSetStreamOutBuffer);
}

} // namespace cafe::gx2
