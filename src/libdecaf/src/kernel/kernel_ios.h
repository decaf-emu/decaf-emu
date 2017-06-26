#pragma once
#include "kernel_enum.h"

#include <cstdint>
#include <common/structsize.h>
#include <libcpu/address.h>
#include <libcpu/be2_struct.h>

namespace kernel
{

struct IPCBuffer;

/**
 * \defgroup kernel_ios IOS
 * \ingroup kernel
 * @{
 */

#pragma pack(push, 1)

using IOSHandle = int32_t;

struct IOSVec
{
   //! Virtual address of buffer.
   be2_val<cpu::VirtualAddress> vaddr;

   //! Length of buffer.
   be2_val<uint32_t> len;

   //! Physical address of buffer.
   be2_val<cpu::PhysicalAddress> paddr;
};
CHECK_OFFSET(IOSVec, 0x00, vaddr);
CHECK_OFFSET(IOSVec, 0x04, len);
CHECK_OFFSET(IOSVec, 0x08, paddr);
CHECK_SIZE(IOSVec, 0x0C);

#pragma pack(pop)

void
iosDispatchIpcRequest(IPCBuffer *buffer);

void
iosInitDevices();

/** @} */

} // namespace kernel
