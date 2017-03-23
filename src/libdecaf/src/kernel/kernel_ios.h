#pragma once
#include "kernel_enum.h"

#include <cstdint>
#include <common/be_ptr.h>
#include <common/be_val.h>
#include <common/structsize.h>

namespace kernel
{

struct IPCBuffer;
class IOSDevice;

/**
 * \defgroup kernel_ios IOS
 * \ingroup kernel
 * @{
 */

#pragma pack(push, 1)

using IOSHandle = int32_t;

struct IOSVec
{
   //! Physical address of buffer.
   be_ptr<void> paddr;

   //! Length of buffer.
   be_val<uint32_t> len;

   //! Virtual address of buffer.
   be_ptr<void> vaddr;
};
CHECK_OFFSET(IOSVec, 0x00, paddr);
CHECK_OFFSET(IOSVec, 0x04, len);
CHECK_OFFSET(IOSVec, 0x08, vaddr);
CHECK_SIZE(IOSVec, 0x0C);

#pragma pack(pop)

void
iosDispatchIpcRequest(IPCBuffer *buffer);

IOSDevice *
iosGetDevice(IOSHandle handle);

void
iosInitDevices();

/** @} */

} // namespace kernel
