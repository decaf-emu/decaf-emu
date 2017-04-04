#include "gx2.h"
#include "gx2_mem.h"
#include "gx2_shaders.h"
#include "gx2r_buffer.h"
#include "gx2r_mem.h"
#include "gx2r_resource.h"
#include "modules/coreinit/coreinit_cache.h"

#include <common/align.h>
#include <common/log.h>

namespace gx2
{

uint32_t
GX2RGetBufferAlignment(GX2RResourceFlags flags)
{
   return 256;
}

uint32_t
GX2RGetBufferAllocationSize(GX2RBuffer *buffer)
{
   return align_up(buffer->elemCount * buffer->elemSize, 64);
}

void
GX2RSetBufferName(GX2RBuffer *buffer,
                  const char *name)
{
   decaf_warn_stub();
}

BOOL
GX2RBufferExists(GX2RBuffer *buffer)
{
   return (buffer && buffer->buffer) ? TRUE : FALSE;
}

BOOL
GX2RCreateBuffer(GX2RBuffer *buffer)
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
      coreinit::DCInvalidateRange(buffer->buffer, size);
   }

   return TRUE;
}

BOOL
GX2RCreateBufferUserMemory(GX2RBuffer *buffer,
                           void *memory,
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
      coreinit::DCInvalidateRange(buffer->buffer, buffer->elemCount * buffer->elemSize);
   }

   // TODO: GX2NotifyMemAlloc(buffer->buffer, size, GX2RGetBufferAlignment(buffer->flags));
   return TRUE;
}

void
GX2RDestroyBufferEx(GX2RBuffer *buffer,
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
GX2RInvalidateBuffer(GX2RBuffer *buffer,
                     GX2RResourceFlags flags)
{
   flags = internal::getOptionFlags(flags);
   flags = static_cast<GX2RResourceFlags>(flags | (buffer->flags & ~0xF80000));

   GX2RInvalidateMemory(flags,
                        buffer->buffer,
                        buffer->elemSize * buffer->elemCount);
}

void *
GX2RLockBufferEx(GX2RBuffer *buffer,
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
         coreinit::DCInvalidateRange(buffer->buffer,
                                     buffer->elemCount * buffer->elemSize);
      }
   }

   // Return buffer pointer
   return buffer->buffer;
}

void
GX2RUnlockBufferEx(GX2RBuffer *buffer,
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
                       GX2OutputStream *stream)
{
   GX2SetStreamOutBuffer(index, stream);
}

} // namespace gx2
