#if 0
#pragma once
#include "ios/ios_enum.h"

#include <array>
#include <cstdint>
#include <common/structsize.h>
#include <libcpu/be2_struct.h>

namespace ios
{

struct IPCBuffer;

/**
* \defgroup ios_ipc IOS IPC
* \ingroup ios
* @{
*/

#pragma pack(push, 1)

using IOSHandle = int32_t;

/**
 * The actual data which is sent as an IPC request between IOSU (ARM) and
 * PowerPC cores.
 */
struct IPCBuffer
{
   static constexpr auto ArgCount = 5;

   //! IOS command to execute
   be2_val<IOSCommand> command;

   //! IPC command result
   be2_val<IOSError> reply;

   //! IOS Handle
   be2_val<IOSHandle> handle;

   //! Flags, always 0
   be2_val<uint32_t> flags;

   //! CPU the request originated from
   be2_val<IOSCpuID> cpuID;

   //! Process ID the request originated from
   be2_val<uint32_t> processId;

   //! Title ID the request originated from
   be2_val<uint64_t> titleId;
   UNKNOWN(4);

   //! IPC command args
   be2_val<uint32_t> args[ArgCount];

   //! Previous IPC command
   be2_val<IOSCommand> prevCommand;

   //! Previous IPC handle
   be2_val<IOSHandle> prevHandle;

   //! Buffer argument 1
   be2_phys_ptr<void> buffer1;

   //! Buffer argument 2
   be2_phys_ptr<void> buffer2;

   //! Buffer to copy device name to for IOS_Open
   std::array<char, 0x20> nameBuffer;

   UNKNOWN(0x80 - 0x68);
};
CHECK_OFFSET(IPCBuffer, 0x00, command);
CHECK_OFFSET(IPCBuffer, 0x04, reply);
CHECK_OFFSET(IPCBuffer, 0x08, handle);
CHECK_OFFSET(IPCBuffer, 0x0C, flags);
CHECK_OFFSET(IPCBuffer, 0x10, cpuId);
CHECK_OFFSET(IPCBuffer, 0x14, processId);
CHECK_OFFSET(IPCBuffer, 0x18, titleId);
CHECK_OFFSET(IPCBuffer, 0x24, args);
CHECK_OFFSET(IPCBuffer, 0x38, prevCommand);
CHECK_OFFSET(IPCBuffer, 0x3C, prevHandle);
CHECK_OFFSET(IPCBuffer, 0x40, buffer1);
CHECK_OFFSET(IPCBuffer, 0x44, buffer2);
CHECK_OFFSET(IPCBuffer, 0x48, nameBuffer);
CHECK_SIZE(IPCBuffer, 0x80);


/**
 * Structure used for ioctlv arguments.
 */
struct IOSVec
{
   //! Virtual address of buffer.
   be2_val<virt_addr> vaddr;

   //! Length of buffer.
   be2_val<uint32_t> len;

   //! Physical address of buffer.
   be2_val<phys_addr> paddr;
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

} // namespace ios
#endif