#include "gx2_mem.h"
#include "gx2r_buffer.h"
#include "gx2r_mem.h"
#include "gx2r_resource.h"
#include "common/align.h"
#include "common/log.h"

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

BOOL
GX2RBufferExists(GX2RBuffer *buffer)
{
   return buffer->buffer ? TRUE : FALSE;
}

BOOL
GX2RCreateBuffer(GX2RBuffer *buffer)
{
   decaf_check(!buffer->buffer);

   auto align = GX2RGetBufferAlignment(buffer->flags);
   auto size = GX2RGetBufferAllocationSize(buffer);
   buffer->buffer = gx2::internal::gx2rAlloc(buffer->flags, size, align);

   if (!buffer->buffer) {
      return FALSE;
   }

   // TODO: Track GX2R resource
   return TRUE;
}

BOOL
GX2RCreateBufferUserMemory(GX2RBuffer *buffer,
                           void *memory,
                           uint32_t size)
{
   buffer->buffer = memory;
   buffer->flags = GX2RResourceFlags::UserMemory;
   return TRUE;
}

void
GX2RDestroyBufferEx(GX2RBuffer *buffer,
                    GX2RResourceFlags flags)
{
   flags = buffer->flags | flags;

   if (buffer->buffer) {
      gx2::internal::gx2rFree(flags, buffer->buffer);
      buffer->buffer = nullptr;
   }

   // TODO: Untrack GX2R resource
}

void
GX2RInvalidateBuffer(GX2RBuffer *buffer,
                     GX2RResourceFlags flags)
{
   GX2RInvalidateMemory(buffer->flags | flags,
                        buffer->buffer,
                        buffer->elemSize * buffer->elemCount);
}

void *
GX2RLockBufferEx(GX2RBuffer *buffer,
                 GX2RResourceFlags flags)
{
   buffer->flags |= GX2RResourceFlags::Locked;
   return buffer->buffer;
}

void
GX2RUnlockBufferEx(GX2RBuffer *buffer,
                   GX2RResourceFlags flags)
{
   buffer->flags &= ~GX2RResourceFlags::Locked;
}

} // namespace gx2
