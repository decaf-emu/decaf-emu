#include "gx2.h"
#include "gx2r_resource.h"
#include "utils/align.h"
#include "utils/wfunc_call.h"

static GX2RAllocFuncPtr
pGX2RMemAlloc;

static GX2RFreeFuncPtr
pGX2RMemFree;

void
GX2RSetAllocator(GX2RAllocFuncPtr allocFn,
                 GX2RFreeFuncPtr freeFn)
{
   pGX2RMemAlloc = allocFn;
   pGX2RMemFree = freeFn;
}

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
GX2RCreateBuffer(GX2RBuffer *buffer)
{
   if (buffer->buffer) {
      gLog->error("GX2RCreateBuffer buffer should be nullptr");
   }

   if (!pGX2RMemAlloc) {
      gLog->error("Attempt to create GX2R buffer without allocator set");
      throw std::logic_error("Attempt to create GX2R buffer without allocator set");
   }

   auto align = GX2RGetBufferAlignment(buffer->flags);
   auto size = GX2RGetBufferAllocationSize(buffer);
   buffer->buffer = pGX2RMemAlloc(buffer->flags, size, align);

   if (!buffer->buffer) {
      return FALSE;
   }

   // TODO: Track GX2R resource
   return TRUE;
}

void
GX2RDestroyBufferEx(GX2RBuffer *buffer, GX2RResourceFlags flags)
{
   if (buffer && buffer->buffer) {
      pGX2RMemFree(flags, buffer->buffer);
   }

   buffer->buffer = nullptr;

   // TODO: Untrack GX2R resource
}

void *
GX2RLockBufferEx(GX2RBuffer *buffer, GX2RResourceFlags flags)
{
   return buffer->buffer;
}

void
GX2RUnlockBufferEx(GX2RBuffer *buffer, GX2RResourceFlags flags)
{
}
