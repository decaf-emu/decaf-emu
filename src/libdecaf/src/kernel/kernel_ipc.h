#pragma once
#include "kernel_enum.h"
#include "kernel_ios.h"

#include <common/be_val.h>
#include <common/be_ptr.h>
#include <common/structsize.h>

namespace kernel
{

/**
 * \defgroup kernel_ipc IPC
 * \ingroup kernel
 * @{
 */

#pragma pack(push, 1)

/**
 * The actual data which is sent as an IPC request between IOSU (ARM) and
 * PowerPC cores.
 */
struct IPCBuffer
{
   static constexpr auto ArgCount = 5;

   //! IOS command to execute
   be_val<IOSCommand> command;

   //! IPC command result
   be_val<IOSError> reply;

   //! IOS Handle
   be_val<IOSHandle> handle;

   //! Flags, always 0
   be_val<uint32_t> flags;

   //! CPU the request originated from
   be_val<IOSCpuId> cpuId;

   //! Process ID the request originated from
   be_val<uint32_t> processId;

   //! Title ID the request originated from
   be_val<uint64_t> titleId;
   UNKNOWN(4);

   //! IPC command args
   be_val<uint32_t> args[ArgCount];

   //! Previous IPC command
   be_val<IOSCommand> prevCommand;

   //! Previous IPC handle
   be_val<IOSHandle> prevHandle;

   //! Buffer argument 1
   be_ptr<void> buffer1;

   //! Buffer argument 2
   be_ptr<void> buffer2;

   //! Buffer to copy device name to for IOS_Open
   char nameBuffer[0x20];

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

#pragma pack(pop)

void
ipcStart();

void
ipcShutdown();

void
ipcDriverKernelSubmitRequest(IPCBuffer *buffer);

void
ipcDriverKernelHandleInterrupt();

/** @} */

} // namespace kernel
