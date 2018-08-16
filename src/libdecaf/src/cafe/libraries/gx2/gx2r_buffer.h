#pragma once
#include "gx2_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

/**
 * \defgroup gx2r_buffer GX2R Buffer
 * \ingroup gx2
 * @{
 */

#pragma pack(push, 1)

struct GX2OutputStream;

struct GX2RBuffer
{
   be2_val<GX2RResourceFlags> flags;
   be2_val<uint32_t> elemSize;
   be2_val<uint32_t> elemCount;
   be2_virt_ptr<void> buffer;
};
CHECK_SIZE(GX2RBuffer, 0x10);
CHECK_OFFSET(GX2RBuffer, 0x00, flags);
CHECK_OFFSET(GX2RBuffer, 0x04, elemSize);
CHECK_OFFSET(GX2RBuffer, 0x08, elemCount);
CHECK_OFFSET(GX2RBuffer, 0x0C, buffer);

#pragma pack(pop)

uint32_t
GX2RGetBufferAlignment(GX2RResourceFlags flags);

uint32_t
GX2RGetBufferAllocationSize(virt_ptr<GX2RBuffer> buffer);

void
GX2RSetBufferName(virt_ptr<GX2RBuffer> buffer,
                  virt_ptr<const char> name);

BOOL
GX2RBufferExists(virt_ptr<GX2RBuffer> buffer);

BOOL
GX2RCreateBuffer(virt_ptr<GX2RBuffer> buffer);

BOOL
GX2RCreateBufferUserMemory(virt_ptr<GX2RBuffer> buffer,
                           virt_ptr<void> memory,
                           uint32_t size);

void
GX2RDestroyBufferEx(virt_ptr<GX2RBuffer> buffer,
                    GX2RResourceFlags flags);

void
GX2RInvalidateBuffer(virt_ptr<GX2RBuffer> buffer,
                     GX2RResourceFlags flags);

virt_ptr<void>
GX2RLockBufferEx(virt_ptr<GX2RBuffer> buffer,
                 GX2RResourceFlags flags);

void
GX2RUnlockBufferEx(virt_ptr<GX2RBuffer> buffer,
                   GX2RResourceFlags flags);

void
GX2RSetStreamOutBuffer(uint32_t index,
                       virt_ptr<GX2OutputStream> stream);

/** @} */

} // namespace cafe::gx2
